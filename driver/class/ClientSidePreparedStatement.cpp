/************************************************************************************
   Copyright (C) 2022, 2023 MariaDB Corporation AB

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/


#include "ClientSidePreparedStatement.h"
#include "Results.h"
#include "ServerSidePreparedStatement.h"
#include "ResultSetMetaData.h"
#include "Protocol.h"
#include "interface/ResultSet.h"

namespace mariadb
{
  /**
    * Private constructor for the clone.
  */
  ClientSidePreparedStatement::ClientSidePreparedStatement(Protocol* _connection,
    int32_t resultSetScrollType,
    bool _noBackslashEscapes)
    : PreparedStatement(_connection, resultSetScrollType)
  {
  }
  /**
    * Constructor.
    *
    * @param connection connection
    * @param sql sql query
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.SQL_CURSOR_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
    *     or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @param autoGeneratedKeys a flag indicating whether auto-generated keys should be returned; one
    *     of <code>Statement.RETURN_GENERATED_KEYS</code> or <code>Statement.NO_GENERATED_KEYS</code>
    * @throws SQLException exception
    */
  ClientSidePreparedStatement::ClientSidePreparedStatement(Protocol* _connection, const SQLString& _sql,
    int32_t resultSetScrollType, bool _noBackslashEscapes)
    : PreparedStatement(_connection, _sql, resultSetScrollType),
      noBackslashEscapes(_noBackslashEscapes)
  {
    prepareResult.reset(ClientPrepareResult::rewritableParts(_sql, noBackslashEscapes));
    // Prepare results owns query string, PS objects only have pointer. Thus it needs to be updated.
    sql= &prepareResult->getSql();
  }

  ClientSidePreparedStatement::~ClientSidePreparedStatement()
  {
    // This is important - to read and release results before statement object itself is destroyed.
    if (results) {
      // in case of multistatment and some of queries fail - we need catch and eat it
      try {
        // If RS object can exist w/out stmt object, then we cannot just skip resultset. as we need to 
        // leave current result alive, and skip all pending. Need 
        results.reset();
      }
      catch (...)
      {}
    }
  }

  /**
    * Clone statement.
    *
    * @param connection connection
    * @return Clone statement.
    * @throws CloneNotSupportedException if any error occur.
    */
  ClientSidePreparedStatement* ClientSidePreparedStatement::clone(Protocol* _connection)
  {
    ClientSidePreparedStatement* clone= new ClientSidePreparedStatement(_connection, resultSetScrollType, noBackslashEscapes);
    clone->sql= sql;
    clone->prepareResult.reset(new ClientPrepareResult(*prepareResult));
    clone->metadata.reset(new ResultSetMetaData(*metadata));
    return clone;
  }

  // This has to be changed to use Protocol service
  bool ClientSidePreparedStatement::executeInternal(int32_t fetchSize)
  {
    validateParamset(prepareResult->getParamCount());

    executeQueryPrologue(false);
    results.reset(
      new Results(
        this,
        fetchSize,
        false,
        1,
        false,
        resultSetScrollType,
        *sql,
        param));

    SQLString sql;
    // Rough allocation
    sql.reserve(prepareResult->getSql().length() + (queryTimeout > 0 ? 42/* need const for this */ : 0) +
      prepareResult->getParamCount()*8/*and for this*/);
    addQueryTimeout(sql, queryTimeout);
    prepareResult->assembleQuery(sql, param, longData);

    try {
      //std::lock_guard<std::mutex> localScopeLock(guard->getLock());
      guard->executeQuery(results.get(), sql);
      results->commandEnd();

      return results->getResultSet() != nullptr;
    }
    catch (SQLException &e) {
      // Do we reallly need it if something goes wrong?
      results->commandEnd();
      throw e;
    }
    return false;
  }


  PrepareResult* ClientSidePreparedStatement::getPrepareResult()
  {
    return dynamic_cast<PrepareResult*>(prepareResult.get());
  }


  uint32_t ClientSidePreparedStatement::fieldCount() const
  {
    return mysql_field_count(guard->getCHandle());// guard->fieldCount(nullptr);
  }

  /**  */
  mariadb::Longs& ClientSidePreparedStatement::executeBatch()
  {
    checkClose();
    if (batchArraySize == 0) {
      return batchRes.wrap(nullptr, 0);
    }

    try {
      executeBatchInternal(batchArraySize);
      results->commandEnd();
      return batchRes.wrap(results->getCmdInformation()->getUpdateCounts());

    }
    catch (int32_t rc) {
      clearBatch();
      throw rc;
    }
    clearBatch();
  }

  /**
    * Non JDBC : Permit to retrieve server update counts when using option rewriteBatchedStatements.
    *
    * @return an array of update counts containing one element for each command in the batch. The
    *     elements of the array are ordered according to the order in which commands were added to
    *     the batch.
    */
  mariadb::Longs& ClientSidePreparedStatement::getServerUpdateCounts()
  {
    if (results && results->getCmdInformation()) {
      return batchRes.wrap(results->getCmdInformation()->getServerUpdateCounts());
    }
    return batchRes.wrap(nullptr, 0);
  }

  /**
    * Choose better way to execute queries according to query and options.
    *
    * @param size parameters number
    * @throws SQLException if any error occur
    */
  void ClientSidePreparedStatement::executeBatchInternal(uint32_t size)
  {
    executeQueryPrologue(true);
    results.reset(
      new Results(
        this,
        0,
        true,
        size,
        false,
        resultSetScrollType,
        emptyStr,
        nullptr));

    std::size_t nextIndex= 0;

    while (nextIndex < size) {
      SQLString sql("");
      nextIndex= prepareResult->assembleBatchQuery(sql, param, size, nextIndex);
      // Or should it still go after the query?
      results->setRewritten(prepareResult->isQueryMultiValuesRewritable());
      guard->realQuery(sql);
      guard->getResult(results.get());
    }
    return;
  }

  /**
    * Retrieves a <code>ResultSetMetaData</code> object that contains information about the columns
    * of the <code>ResultSet</code> object that will be returned when this <code>PreparedStatement
    * </code> object is executed. <br>
    * Because a <code>PreparedStatement</code> object is precompiled, it is possible to know about
    * the <code>ResultSet</code> object that it will return without having to execute it.
    * Consequently, it is possible to invoke the method <code>getMetaData</code> on a <code>
    * PreparedStatement</code> object rather than waiting to execute it and then invoking the <code>
    * ResultSet.getMetaData</code> method on the <code>ResultSet</code> object that is returned.
    *
    * @return the description of a <code>ResultSet</code> object's columns or <code>null</code> if
    *     the driver cannot return a <code>ResultSetMetaData</code> object
    * @throws SQLException if a database access error occurs or this method is called on a closed
    *     <code>PreparedStatement</code>
    */
  ResultSetMetaData* ClientSidePreparedStatement::getMetaData()
  {
    checkClose();
    ResultSet* rs= results->getResultSet();
    if (rs != nullptr) {
      return rs->getMetaData();
    }
    if (!metadata) {
      loadParametersData();
    }
    return metadata.get();
  }


  /**
    * Retrieves the number, types and properties of this <code>PreparedStatement</code> object's
    * parameters.
    *
    * @return a <code>ParameterMetaData</code> object that contains information about the number,
    *     types and properties for each parameter marker of this <code>PreparedStatement</code>
    *     object
    * @throws SQLException if a database access error occurs or this method is called on a closed
    *     <code>PreparedStatement</code>
    * @see ParameterMetaData
    * @since 1.4
    */
  /*ParameterMetaData* ClientSidePreparedStatement::getParameterMetaData()
  {
    checkClose();
    if (!parameterMetaData) {
      loadParametersData();
    }
    return parameterMetaData.get();
  }*/

  void ClientSidePreparedStatement::loadParametersData()
  {
    try {
      ServerSidePreparedStatement ssps(
        guard,
        *sql,
        ResultSet::TYPE_SCROLL_INSENSITIVE);
      metadata.reset(ssps.getMetaData());
      //parameterMetaData.reset(ssps.getParameterMetaData());
    }
    catch (int32_t) {
      //parameterMetaData.reset(new SimpleParameterMetaData(static_cast<uint32_t>(prepareResult->getParamCount())));
    }
  }


  void ClientSidePreparedStatement::executeQueryPrologue(bool isBatch)
  {
    checkClose();
  }


  void ClientSidePreparedStatement::close()
  {
    markClosed();
  }


  uint32_t ClientSidePreparedStatement::getParameterCount()
  {
    return static_cast<uint32_t>(prepareResult->getParamCount());
  }


  const char* ClientSidePreparedStatement::getError()
  {
    return mysql_error(guard->getCHandle());
  }

  uint32_t ClientSidePreparedStatement::getErrno()
  {
    return mysql_errno(guard->getCHandle());
  }

  const char* ClientSidePreparedStatement::getSqlState()
  {
    return mysql_sqlstate(guard->getCHandle());
  }

  bool ClientSidePreparedStatement::bind(MYSQL_BIND* param)
  {
    this->param= param;
    return true;
  }

  bool ClientSidePreparedStatement::sendLongData(uint32_t paramNum, const char* data, std::size_t length)
  {
    std::string& buf= longData[paramNum];

    if (buf.capacity() < buf.length() + length + 1) {
      buf.reserve(buf.length() + 10 * length);
    }
    buf.append(data, length);
    return true;
  }

  bool ClientSidePreparedStatement::hasMoreResults()
  {
    return results && results->hasMoreResults(guard);
  }


  /*SQLString ClientSidePreparedStatement::toString()
  {
    SQLString sb("sql : '"+sql +"'");
    sb.append(", parameters : [");
    for (const auto& cit: parameters ) {
      if (!cit) {
        sb.append("NULL");
      }
      else {
        sb.append(cit->toString());
      }
      if (cit != parameters.back()) {
        sb.append(",");
      }
    }
    sb.append("]");
    return sb;
  }*/

}
