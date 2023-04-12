/************************************************************************************
   Copyright (C) 2022 MariaDB Corporation AB

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


#ifndef _PREPARESTATEMENT_H_
#define _PREPARESTATEMENT_H_

//#include "ColumnType.h"
//#include "ParameterHolder.h"
#include "PrepareResult.h"
#include "ResultSet.h"
#include "Results.h"
#include "ResultSetMetaData.h"

struct MADB_Stmt;

namespace odbc
{
namespace mariadb
{

SQLString& addQueryTimeout(SQLString& sql, int32_t queryTimeout);

class PreparedStatement
{
protected:
  MYSQL* connection;

  SQLString sql;
  std::size_t parameterCount= 0; // Do we need it here if we can get it pro prepare result class
  bool hasLongData= false;
  bool useFractionalSeconds= true;
  int32_t fetchSize= 0;
  int32_t resultSetScrollType= 0;
  bool closed= false;
  odbc::Longs batchRes;
  Unique::ResultSetMetaData metadata;
  Unique::Results results;
  MYSQL_BIND* param= nullptr;
  uint32_t batchArraySize= 0;
  bool continueBatchOnError= false;
  uint32_t queryTimeout= 0;

  PreparedStatement(
    MYSQL* maHandle,
    int32_t resultSetScrollType);

public:

  enum {
    EXECUTE_FAILED = -3,
    SUCCESS_NO_INFO = -2
  };

  virtual ~PreparedStatement(){}

protected:
  virtual bool executeInternal(int32_t fetchSize)=0;
  virtual PrepareResult* getPrepareResult()=0;
  virtual void executeBatchInternal(uint32_t queryParameterSize)=0;
  // For internal use - app should use metadata object
  virtual uint32_t fieldCount() const=0;
;
  //void initParamset(std::size_t paramCount);
  /**
   * Check if statement is closed, and throw exception if so.
   *
   * @throws SQLException if statement close
   */
  void checkClose();
  void markClosed();
  void validateParamset(std::size_t paramCount);

public:
  int64_t getServerThreadId();
  void    clearBatch();
  int64_t    executeUpdate();
  bool       execute();
  ResultSet* executeQuery();
  ResultSet* getResultSet();
  int64_t    getUpdateCount();

  const odbc::Longs&  executeBatch();
  ResultSetMetaData*  getEarlyMetaData();
  std::size_t         getParamCount();

  void setBatchSize(int32_t batchSize);
  bool getMoreResults();

  virtual bool        hasMoreResults()=0;
  virtual void        moveToNextResult()=0;
  virtual const char* getError()=0;
  virtual uint32_t    getErrno()=0;
  virtual const char* getSqlState()= 0;;

  virtual bool bind(MYSQL_BIND* param)=0;
  virtual bool sendLongData(uint32_t paramNum, const char* data, std::size_t length)=0;
  virtual bool isServerSide() const=0;
  virtual enum enum_field_types getPreferredParamType(enum enum_field_types appType) const=0;
  Results* getInternalResults() { return results.get(); }

  //void validateParamset(std::size_t paramCount);
  //operator MADB_Stmt* () { return stmt; }
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
  */
  /* virtual ParameterMetaData* getParameterMetaData()=0;
  virtual void setParameter(int32_t parameterIndex, ParameterHolder* holder)=0;

  void setNull(int32_t parameterIndex, int32_t sqlType);
  void setNull(int32_t parameterIndex, const ColumnType& mariadbType);
  void setNull(int32_t parameterIndex, int32_t sqlType, const SQLString& typeName);
  void setBlob(int32_t parameterIndex, std::istream* inputStream, const int64_t length);
  void setBlob(int32_t parameterIndex, std::istream* inputStream);

  void setBoolean(int32_t parameterIndex,bool value);
  void setByte(int32_t parameterIndex, int8_t byte);
  void setShort(int32_t parameterIndex, int16_t value);
  void setString(int32_t parameterIndex, const SQLString& str);
  void setBytes(int32_t parameterIndex, odbc::bytes* bytes);
  void setInt(int32_t column, int32_t value);
  void setLong(int32_t parameterIndex, int64_t value);
  void setInt64(int32_t parameterIndex, int64_t value) { setLong(parameterIndex, value); }
  void setUInt64(int32_t parameterIndex, uint64_t value);
  void setUInt(int32_t parameterIndex, uint32_t value);
  void setFloat(int32_t parameterIndex, float value);
  void setDouble(int32_t parameterIndex, double value);
  void setDateTime(int32_t parameterIndex, const SQLString& dt);
  void setBigInt(int32_t column, const SQLString& value); 

  void clearParameters();
  void addBatch(); */
  };

}
}
#endif
