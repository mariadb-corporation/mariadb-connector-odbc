/************************************************************************************
   Copyright (C) 2022,2023 MariaDB Corporation AB

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


#include <vector>
#include <array>
#include <sstream>
#include <algorithm>

#include "ResultSetBin.h"
#include "Results.h"

#include "ColumnDefinition.h"
#include "Row.h"
#include "BinRow.h"
#include "TextRow.h"
#include "ServerPrepareResult.h"
#include "Exception.h"
#include "PreparedStatement.h"

#ifdef max
# undef max
#endif // max

namespace mariadb
{
  /**
    * Create Streaming resultSet.
    *
    * @param results results
    * @param protocol current protocol
    * @param spr ServerPrepareResult
    * @param callableResult is it from a callableStatement ?
    * @param eofDeprecated is EOF deprecated
    */
  ResultSetBin::ResultSetBin(Results* results,
                             ServerPrepareResult* spr)
    : ResultSet(results->getFetchSize()),
      columnsInformation(spr->getColumns()),
      statement(results->getStatement()),
      isClosedFlag(false),
      noBackslashEscapes(false),
      columnInformationLength(static_cast<int32_t>(mysql_stmt_field_count(spr->getStatementId()))),
      dataSize(0),
      resultSetScrollType(results->getResultSetScrollType()),
      rowPointer(-1),
      callableResult(callableResult),
      eofDeprecated(eofDeprecated),
      isEof(false),
      capiStmtHandle(spr->getStatementId()),
      forceAlias(false),
      lastRowPointer(-1),
      resultBind(nullptr)
  {
    if (fetchSize == 0 || callableResult) {
      data.reserve(10);//= new char[10]; // This has to be array of arrays. Need to decide what to use for its representation
      if (mysql_stmt_store_result(capiStmtHandle)) {
        throw 1;
      }
      dataSize= static_cast<std::size_t>(mysql_stmt_num_rows(capiStmtHandle));
      streaming= false;
      resetVariables();
      row.reset(new BinRow(columnsInformation, columnInformationLength, capiStmtHandle));
    }
    else {

      //protocol->setActiveStreamingResult(statement->getInternalResults());
      //protocol->removeHasMoreResults();

      data.reserve(std::max(10, fetchSize)); // Same
      row.reset(new BinRow(columnsInformation, columnInformationLength, capiStmtHandle));
      nextStreamingValue();
      streaming= true;
    }
  }


  ResultSetBin::~ResultSetBin()
  {
    if (!isFullyLoaded()) {
      //close();
      fetchAllResults();
    }
    checkOut();
  }

  /**
    * Indicate if result-set is still streaming results from server.
    *
    * @return true if streaming is finished
    */
  bool ResultSetBin::isFullyLoaded() const {
    // result-set is fully loaded when reaching EOF packet.
    return isEof;
  }


  void ResultSetBin::fetchAllResults()
  {
    dataSize= 0;
    while (readNextValue()) {
    }
    ++dataFetchTime;
  }

  const char * ResultSetBin::getErrMessage()
  {
    return mysql_stmt_error(capiStmtHandle);
  }


  const char * ResultSetBin::getSqlState()
  {
    return mysql_stmt_error(capiStmtHandle);
  }

  uint32_t ResultSetBin::getErrNo()
  {
    return mysql_stmt_errno(capiStmtHandle);
  }

  uint32_t ResultSetBin::warningCount()
  {
    return mysql_stmt_warning_count(capiStmtHandle);
  }

  /**
    * When protocol has a current Streaming result (this) fetch all to permit another query is
    * executing. The lock should be acquired before calling this method
    *
    * @throws SQLException if any error occur
    */
  void ResultSetBin::fetchRemaining() {
    if (!isEof) {
      try {
        lastRowPointer = -1;
        if (!isEof && dataSize > 0 && fetchSize == 1) {
          // We need to grow the array till current size. Its main purpose is to create room for newly fetched
          // fetched row, so it grows till dataSize + 1. But we need to space for already fetched(from server)
          // row. Thus fooling growDataArray by decrementing dataSize
          --dataSize;
          growDataArray();
          // Since index of the last row is smaller from dataSize by 1, we have correct index
          row->cacheCurrentRow(data[dataSize], columnsInformation.size());
          rowPointer= 0;
          resetRow();
          ++dataSize;
        }
        /*if (mysql_stmt_store_result(capiStmtHandle) != 0) {
          throwStmtError(capiStmtHandle);
        }*/
        while (!isEof) {
          addStreamingValue(true);
        }
      }
      catch (SQLException & queryException) {
        throw queryException;
      }
      catch (std::exception & ioe) {
        handleIoException(ioe);
      }
      dataFetchTime++;
    }
  }

  void ResultSetBin::handleIoException(std::exception& ioe) const
  {
    throw 1; /*SQLException(
        "Server has closed the connection. \n"
        "Please check net_read_timeout/net_write_timeout/wait_timeout server variables. "
        "If result set contain huge amount of data, Server expects client to"
        " read off the result set relatively fast. "
        "In this case, please consider increasing net_read_timeout session variable"
        " / processing your result set faster (check Streaming result sets documentation for more information)",
        "HY000", &ioe);*/
  }

  /**
    * This permit to replace current stream results by next ones.
    *
    * @throws IOException if socket exception occur
    * @throws SQLException if server return an unexpected error
    */
  void ResultSetBin::nextStreamingValue() {
    lastRowPointer= -1;

    if (resultSetScrollType == SQL_CURSOR_FORWARD_ONLY) {
      dataSize= 0;
    }

    addStreamingValue(fetchSize > 1);
  }


  /**
    * Read next value.
    *
    * @return true if have a new value
    * @throws IOException exception
    * @throws SQLException exception
    */
  bool ResultSetBin::readNextValue(bool cacheLocally)
  {
    switch (row->fetchNext()) {
    case 1: {
      SQLString err("Internal error: most probably fetch on not yet executed statment handle. ");
      unsigned int nativeErrno = getErrNo();
      err.append(getErrMessage());
      throw SQLException(err, "HY000", nativeErrno );
    }
    case MYSQL_DATA_TRUNCATED: {
      /*protocol->removeActiveStreamingResult();
      protocol->removeHasMoreResults();*/
      //protocol->setHasWarnings(true);
      break;

      /*resetVariables();
      throw *ExceptionFactory::INSTANCE.create(
        getErrMessage(),
        getSqlState(),
        getErrNo(),
        nullptr,
        false);*/
    }

    case MYSQL_NO_DATA: {
      uint32_t serverStatus;
      uint32_t warnings;

      if (!eofDeprecated) {
        warnings= warningCount();
        mariadb_get_infov(capiStmtHandle->mysql, MARIADB_CONNECTION_SERVER_STATUS, (void*)&serverStatus);

        // CallableResult has been read from intermediate EOF server_status
        // and is mandatory because :
        //
        // - Call query will have an callable resultSet for OUT parameters
        //   this resultSet must be identified and not listed in JDBC statement.getResultSet()
        //
        // - after a callable resultSet, a OK packet is send,
        //   but mysql before 5.7.4 doesn't send MORE_RESULTS_EXISTS flag
        if (callableResult) {
          serverStatus|= SERVER_MORE_RESULTS_EXIST;
        }
      }
      else {
        // OK_Packet with a 0xFE header
        // protocol->readOkPacket()?
        mariadb_get_infov(capiStmtHandle->mysql, MARIADB_CONNECTION_SERVER_STATUS, (void*)&serverStatus);
        warnings= warningCount();;
        callableResult= (serverStatus & SERVER_PS_OUT_PARAMS) != 0;
      }

      if ((serverStatus & SERVER_MORE_RESULTS_EXIST) == 0) {
        //protocol->removeActiveStreamingResult();
      }
      resetVariables();
      return false;
    }
    }

    if (cacheLocally) {
      if (dataSize + 1 >= data.size()) {
        growDataArray();
      }
      row->cacheCurrentRow(data[dataSize], columnsInformation.size());
    }
    ++dataSize;
    return true;
  }

  /**
    * Get current row's raw bytes.
    *
    * @return row's raw bytes
    */
  std::vector<mariadb::bytes>& ResultSetBin::getCurrentRowData() {
    return data[rowPointer];
  }

  /**
    * Update row's raw bytes. in case of row update, refresh the data. (format must correspond to
    * current resultset binary/text row encryption)
    *
    * @param rawData new row's raw data.
    */
  void ResultSetBin::updateRowData(std::vector<mariadb::bytes>& rawData)
  {
    data[rowPointer]= rawData;
    row->resetRow(data[rowPointer]);
  }

  /**
    * Delete current data. Position cursor to the previous row->
    *
    * @throws SQLException if previous() fail.
    */
  void ResultSetBin::deleteCurrentRowData() {

    data.erase(data.begin()+lastRowPointer);
    dataSize--;
    lastRowPointer= -1;
    previous();
  }

  void ResultSetBin::addRowData(std::vector<mariadb::bytes>& rawData) {
    if (dataSize +1 >= data.size()) {
      growDataArray();
    }
    data[dataSize]= rawData;
    rowPointer= static_cast<int32_t>(dataSize);
    ++dataSize;
  }

  /*int32_t ResultSetBin::skipLengthEncodedValue(std::string& buf, int32_t pos) {
    int32_t type= buf[pos++] &0xff;
    switch (type) {
    case 251:
      return pos;
    case 252:
      return pos +2 +(0xffff &(((buf[pos] &0xff)+((buf[pos +1] &0xff)<<8))));
    case 253:
      return pos
        +3
        +(0xffffff
          &((buf[pos] &0xff)
            +((buf[pos +1] &0xff)<<8)
            +((buf[pos +2] &0xff)<<16)));
    case 254:
      return (int32_t)
        (pos
          +8
          +((buf[pos] &0xff)
            +((int64_t)(buf[pos +1] &0xff)<<8)
            +((int64_t)(buf[pos +2] &0xff)<<16)
            +((int64_t)(buf[pos +3] &0xff)<<24)
            +((int64_t)(buf[pos +4] &0xff)<<32)
            +((int64_t)(buf[pos +5] &0xff)<<40)
            +((int64_t)(buf[pos +6] &0xff)<<48)
            +((int64_t)(buf[pos +7] &0xff)<<56)));
    default:
      return pos +type;
    }
  }*/

  /** Grow data array. */
  void ResultSetBin::growDataArray() {
    std::size_t curSize = data.size();

    if (data.capacity() < curSize + 1) {
      std::size_t newCapacity = static_cast<std::size_t>(curSize + (curSize >> 1));

      if (newCapacity > MAX_ARRAY_SIZE) {
        newCapacity= static_cast<std::size_t>(MAX_ARRAY_SIZE);
      }

      data.reserve(newCapacity);
    }
    for (std::size_t i = curSize; i < dataSize + 1; ++i) {
      data.push_back({});
    }
    data[dataSize].reserve(columnsInformation.size());
  }

  /**
    * Connection.abort() has been called, abort result-set.
    *
    * @throws SQLException exception
    */
  void ResultSetBin::abort() {
    isClosedFlag= true;
    resetVariables();

    for (auto& row : data) {
      row.clear();
    }

    if (statement != nullptr) {
      //statement->checkCloseOnCompletion(this);
      statement= nullptr;
    }
  }

  /** Close resultSet. */
  void ResultSetBin::close() {
    isClosedFlag= true;
    if (!isEof) {
      try {
        while (!isEof) {
          dataSize= 0; // to avoid storing data
          readNextValue();
        }
      }
      catch (SQLException& queryException) {
        throw queryException;
      }
      catch (std::runtime_error& ioe) {
        resetVariables();
        handleIoException(ioe);
      }
    }

    checkOut();
    resetVariables();

    data.clear();

    if (statement != nullptr) {
      //statement->checkCloseOnCompletion(this);
      statement= nullptr;
    }
  }


  void ResultSetBin::resetVariables() {
    isEof= true;
  }


  bool ResultSetBin::fetchNext()
  {
    ++rowPointer;
    if (data.size() > 0) {
      row->resetRow(data[rowPointer]);
    }
    else {
      if (row->fetchNext() == MYSQL_NO_DATA) {
        return false;
      }
    }
    lastRowPointer= rowPointer;
    return true;
  }

  bool ResultSetBin::next()
  {
    if (isClosedFlag) {
      throw SQLException("Operation not permit on a closed resultSet", "HY000");
    }
    if (rowPointer < static_cast<int32_t>(dataSize) - 1) {
      ++rowPointer;
      return true;
    }
    else {
      if (streaming && !isEof) {
        try {
          if (!isEof) {
            nextStreamingValue();
          }
        }
        catch (std::exception& ioe) {
          handleIoException(ioe);
        }

        if (resultSetScrollType == SQL_CURSOR_FORWARD_ONLY) {

          rowPointer= 0;
          return dataSize > 0;
        }
        else {
          rowPointer++;
          return dataSize > static_cast<std::size_t>(rowPointer);
        }
      }

      rowPointer= static_cast<int32_t>(dataSize);
      return false;
    }
  }

  // It has to be const, because it's called by getters, and properties it changes are mutable
  void ResultSetBin::resetRow() const
  {
    if (rowPointer > -1 && data.size() > static_cast<std::size_t>(rowPointer)) {
      row->resetRow(const_cast<std::vector<mariadb::bytes> &>(data[rowPointer]));
    }
    else {
      if (rowPointer != lastRowPointer + 1) {
        row->installCursorAtPosition(rowPointer);
      }
      if (!streaming) {
        row->fetchNext();
      }
    }
    lastRowPointer= rowPointer;
  }


  void ResultSetBin::checkObjectRange(int32_t position) const {
    if (rowPointer < 0) {
      throw SQLException("Current position is before the first row", "22023");
    }

    if (static_cast<uint32_t>(rowPointer) >= dataSize) {
      throw SQLException("Current position is after the last row", "22023");
    }

    if (position <= 0 || position > columnInformationLength) {
      throw SQLException("No such column: " + std::to_string(position), "22023");
    }

    if (lastRowPointer != rowPointer) {
      resetRow();
    }
    row->setPosition(position - 1);
  }


  bool ResultSetBin::isBeforeFirst() const {
    checkClose();
    return (dataFetchTime >0) ? rowPointer == -1 && dataSize > 0 : rowPointer == -1;
  }

  bool ResultSetBin::isAfterLast() {
    checkClose();
    if (rowPointer < 0 || static_cast<std::size_t>(rowPointer) < dataSize) {
      // has remaining results
      return false;
    }
    else {
      
      if (streaming && !isEof)
      {
      // has to read more result to know if it's finished or not
      // (next packet may be new data or an EOF packet indicating that there is no more data)
        try {
          // this time, fetch is added even for streaming forward type only to keep current pointer
          // row.
          if (!isEof) {
            addStreamingValue();
          }
        }
        catch (std::exception& ioe) {
          handleIoException(ioe);
        }
        return dataSize == rowPointer;
      }
      // has read all data and pointer is after last result
      // so result would have to always to be true,
      // but when result contain no row at all jdbc say that must return false
      return dataSize >0 ||dataFetchTime >1;
    }
  }

  bool ResultSetBin::isFirst() const {
    checkClose();
    return /*dataFetchTime == 1 && */rowPointer == 0 && dataSize > 0;
  }

  bool ResultSetBin::isLast() {
    checkClose();
    if (static_cast<std::size_t>(rowPointer + 1) < dataSize) {
      return false;
    }
    else if (isEof) {
      return rowPointer == dataSize - 1 && dataSize > 0;
    }
    else {
      // when streaming and not having read all results,
      // must read next packet to know if next packet is an EOF packet or some additional data
      try {
        if (!isEof) {
          addStreamingValue();
        }
      }
      catch (std::exception& ioe) {
        handleIoException(ioe);
      }

      if (isEof) {

        return rowPointer == dataSize - 1 && dataSize > 0;
      }
      return false;
    }
  }

  void ResultSetBin::beforeFirst() {
    checkClose();

    if (streaming &&resultSetScrollType == SQL_CURSOR_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type SQL_CURSOR_FORWARD_ONLY");
    }
    rowPointer= -1;
  }

  void ResultSetBin::afterLast() {
    checkClose();
    if (!isEof) {
      //ResultSet objects only have lock if streaming
      fetchRemaining();
    }
    rowPointer= static_cast<int32_t>(dataSize);
  }

  bool ResultSetBin::first() {
    checkClose();

    if (streaming && resultSetScrollType == SQL_CURSOR_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type SQL_CURSOR_FORWARD_ONLY");
    }

    rowPointer= 0;
    return dataSize > 0;
  }

  bool ResultSetBin::last() {
    checkClose();
    if (!isEof) {
      //ResultSet objects only have lock if streaming
      fetchRemaining();
    }
    rowPointer= static_cast<int32_t>(dataSize) - 1;
    return dataSize > 0;
  }

  int64_t ResultSetBin::getRow() {
    checkClose();
    if (streaming && resultSetScrollType == SQL_CURSOR_FORWARD_ONLY) {
      return 0;
    }
    return rowPointer + 1;
  }

  bool ResultSetBin::absolute(int64_t rowPos) {
    checkClose();

    if (streaming && resultSetScrollType == SQL_CURSOR_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type SQL_CURSOR_FORWARD_ONLY");
    }

    if (rowPos >= 0 && static_cast<uint32_t>(rowPos) <= dataSize) {
      rowPointer= static_cast<int32_t>(rowPos - 1);
      return true;
    }
    if (!isEof) {
      //ResultSet objects only have lock if streaming
      fetchRemaining();
    }
    if (rowPos >= 0) {

      if (static_cast<uint32_t>(rowPos) <= dataSize) {
        rowPointer= static_cast<int32_t>(rowPos - 1);
        return true;
      }
      rowPointer= static_cast<int32_t>(dataSize);
      return false;
    }
    else {

      // Need to cast, or otherwise the result would be size_t -> always not negative
      if (static_cast<int64_t>(dataSize) + rowPos >= 0) {
        rowPointer= static_cast<int32_t>(dataSize + rowPos);
        return true;
      }
      rowPointer= -1;
      return false;
    }
  }


  bool ResultSetBin::relative(int64_t rows) {
    checkClose();
    if (streaming && resultSetScrollType == SQL_CURSOR_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type SQL_CURSOR_FORWARD_ONLY");
    }
    int32_t newPos= static_cast<int32_t>(rowPointer + rows);
    if (newPos <=-1) {
      rowPointer= -1;
      return false;
    }
    else if (static_cast<uint32_t>(newPos) >= dataSize) {
      rowPointer= static_cast<int32_t>(dataSize);
      return false;
    }
    else {
      rowPointer= newPos;
      return true;
    }
  }

  bool ResultSetBin::previous() {
    checkClose();
    if (streaming && resultSetScrollType == SQL_CURSOR_FORWARD_ONLY) {
      throw SQLException("Invalid operation for result set type SQL_CURSOR_FORWARD_ONLY");
    }
    if (rowPointer > -1) {
      --rowPointer;
      return rowPointer != -1;
    }
    return false;
  }


  int32_t ResultSetBin::getFetchSize() const {
    return this->fetchSize;
  }

  void ResultSetBin::setFetchSize(int32_t fetchSize) {
    if (streaming && fetchSize == 0) {
      try {
        // fetch all results
        while (!isEof) {
          addStreamingValue();
        }
      }
      catch (std::exception& ioe) {
        handleIoException(ioe);
      }
      streaming= dataFetchTime == 1;
    }
    this->fetchSize= fetchSize;
  }

  int32_t ResultSetBin::getType()  const {
    return resultSetScrollType;
  }

  void ResultSetBin::checkClose() const {
    if (isClosedFlag) {
      throw SQLException("Operation not permit on a closed resultSet", "HY000");
    }
  }

  bool ResultSetBin::isCallableResult() const {
    return callableResult;
  }

  bool ResultSetBin::isClosed() const {
    return isClosedFlag;
  }

  /** {inheritDoc}. */
  bool ResultSetBin::wasNull() const {
    return row->wasNull();
  }

  bool ResultSetBin::isNull(int32_t columnIndex) const
  {
    checkObjectRange(columnIndex);
    return row->lastValueWasNull();
  }

  /** {inheritDoc}. */
  SQLString ResultSetBin::getString(int32_t columnIndex) const
  {
    checkObjectRange(columnIndex);

    return std::move(row->getInternalString(&columnsInformation[columnIndex - 1]));
  }


  SQLString ResultSetBin::zeroFillingIfNeeded(const SQLString& value, ColumnDefinition* columnInformation)
  {
    if (columnInformation->isZeroFill()) {
      SQLString zeroAppendStr;
      int64_t zeroToAdd= columnInformation->getDisplaySize() - value.size();
      while ((zeroToAdd--) > 0) {
        zeroAppendStr.append("0");
      }
      return zeroAppendStr.append(value);
    }
    return value;
  }

  /** {inheritDoc}. */
  std::istream* ResultSetBin::getBinaryStream(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    if (row->lastValueWasNull()) {
      return nullptr;
    }
    blobBuffer[columnIndex].reset(new memBuf(row->fieldBuf.arr + row->pos, row->fieldBuf.arr + row->pos + row->getLengthMaxFieldSize()));
    return new std::istream(blobBuffer[columnIndex].get());
  }

  /** {inheritDoc}. */
  int32_t ResultSetBin::getInt(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalInt(&columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  int64_t ResultSetBin::getLong(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalLong(&columnsInformation[columnIndex -1]);
  }


  uint64_t ResultSetBin::getUInt64(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return static_cast<uint64_t>(row->getInternalULong(&columnsInformation[columnIndex -1]));
  }


  uint32_t ResultSetBin::getUInt(int32_t columnIndex) const {
    checkObjectRange(columnIndex);

    const ColumnDefinition* columnInfo= &columnsInformation[columnIndex - 1];
    int64_t value= row->getInternalLong(columnInfo);

    row->rangeCheck("uint32_t", 0, UINT32_MAX, value, columnInfo);

    return static_cast<uint32_t>(value);
  }

  /** {inheritDoc}. */
  float ResultSetBin::getFloat(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalFloat(&columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  long double ResultSetBin::getDouble(int32_t columnIndex) const {
    checkObjectRange(columnIndex);
    return row->getInternalDouble(&columnsInformation[columnIndex -1]);
  }

  /** {inheritDoc}. */
  ResultSetMetaData* ResultSetBin::getMetaData() const {
    return new ResultSetMetaData(columnsInformation, forceAlias);
  }

  ///** {inheritDoc}. */
  //Blob* ResultSetBin::getBlob(int32_t columnIndex) const {
  //  return getBinaryStream(columnIndex);
  //}

  /** {inheritDoc}. */
  bool ResultSetBin::getBoolean(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalBoolean(&columnsInformation[static_cast<std::size_t>(index) -1]);
  }

  /** {inheritDoc}. */
  int8_t ResultSetBin::getByte(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalByte(&columnsInformation[static_cast<std::size_t>(index) - 1]);
  }

  /** {inheritDoc}. */
  short ResultSetBin::getShort(int32_t index) const {
    checkObjectRange(index);
    return row->getInternalShort(&columnsInformation[static_cast<std::size_t>(index) - 1]);
  }

 
  std::size_t ResultSetBin::rowsCount() const
  {
    return dataSize;
  }

  /** Force metadata getTableName to return table alias, not original table name. */
  void ResultSetBin::setForceTableAlias() {
    this->forceAlias= true;
  }

  void ResultSetBin::rangeCheck(const SQLString& className, int64_t minValue, int64_t maxValue, int64_t value, ColumnDefinition* columnInfo) {
    if (value < minValue || value > maxValue) {
      throw SQLException(
        "Out of range value for column '"
        +columnInfo->getName()
        +"' : value "
        + std::to_string(value)
        +" is not in "
        + className
        +" range",
        "22003",
        1264);
    }
  }

  int32_t ResultSetBin::getRowPointer() {
    return rowPointer;
  }

  void ResultSetBin::setRowPointer(int32_t pointer) {
    rowPointer= pointer;
  }

  void ResultSetBin::checkOut()
  {
    if (statement != nullptr && statement->getInternalResults()) {
      statement->getInternalResults()->checkOut(this);
    }
  }


  std::size_t ResultSetBin::getDataSize() {
    return dataSize;
  }


  bool ResultSetBin::isBinaryEncoded() {
    return row->isBinaryEncoded();
  }


  void ResultSetBin::realClose(bool noLock)
  {
    isClosedFlag = true;
    if (!isEof) {
      if (!noLock) {
      }
      try {
        while (!isEof) {
          dataSize = 0; // to avoid storing data
          readNextValue();
        }
      }
      catch (SQLException & queryException) {
        if (!noLock) {
        }
        resetVariables();
        throw queryException;
      }
      if (!noLock) {
      }
    }

    checkOut();
    resetVariables();

    data.clear();

    if (statement != nullptr) {
      statement= nullptr;
    }
  }
  void ResultSetBin::bind(MYSQL_BIND* bind)
  {
    //mysql_stmt_bind_result(capiStmtHandle, bind);
    resultBind.reset(new MYSQL_BIND[columnInformationLength]());
    std::memcpy(resultBind.get(), bind, columnInformationLength*sizeof(MYSQL_BIND));
  }

  bool ResultSetBin::get(MYSQL_BIND* bind, uint32_t colIdx0based, uint64_t offset)
  {
    checkObjectRange(colIdx0based + 1);
    return mysql_stmt_fetch_column(capiStmtHandle, bind, colIdx0based, static_cast<unsigned long>(offset)) != 0;
  }

  bool ResultSetBin::get()
  {
    bool truncations= false;
    if (resultBind) {
      if (lastRowPointer != rowPointer) {
        resetRow();
      }
      for (int32_t i = 0; i < columnInformationLength; ++i) {
        MYSQL_BIND* bind= resultBind.get() + i;
        if (bind->error == nullptr) {
          bind->error= &bind->error_value;
        }
        get(bind, i, 0);
        if (*bind->error) {
          truncations= true;
        }
      }
    }
    return truncations;
  }

} // namespace mariadb
