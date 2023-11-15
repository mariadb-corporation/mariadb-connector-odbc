/************************************************************************************
   Copyright (C) 2020 MariaDB Corporation AB

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


#ifndef _RESULTSETCBIN_H_
#define _RESULTSETCBIN_H_

#include <exception>
#include <vector>
#include <map>
#include "ma_odbc.h"

#include "ColumnDefinition.h"
#include "ResultSet.h"


namespace mariadb
{
class ServerPrepareResult;
struct memBuf;

class ResultSetBin : public ResultSet
{
  const std::vector<ColumnDefinition>& columnsInformation;
  int32_t columnInformationLength;
  bool noBackslashEscapes;
  // we don't create buffers for all columns without call. Thus has to be mutable while getters are const
  mutable std::map<int32_t, std::unique_ptr<memBuf>> blobBuffer;

  bool callableResult;
  PreparedStatement* statement;
  MYSQL_STMT* capiStmtHandle;
  std::unique_ptr<MYSQL_BIND[]> resultBind;

  std::vector<std::vector<mariadb::bytes_view>> data;
  std::size_t dataSize; //Should go after data

  int32_t resultSetScrollType;
  int32_t rowPointer;

//  std::unique_ptr<ColumnNameMap> columnNameMap;

  mutable int32_t lastRowPointer; /*-1*/
  bool isClosedFlag;
  bool forceAlias;

public:

  ResultSetBin(
    Results* results,
    Protocol* guard,
    ServerPrepareResult* pr);

  ~ResultSetBin();

  bool isFullyLoaded() const;

private:
  void fetchAllResults();

  const char* getErrMessage();
  const char* getSqlState();
  uint32_t getErrNo();
  uint32_t warningCount();
public:
  void fetchRemaining();

private:
  void handleIoException(std::exception& ioe) const;
  void nextStreamingValue();
  bool readNextValue(bool cacheLocally= false);

protected:
  std::vector<mariadb::bytes_view>& getCurrentRowData();
  void updateRowData(std::vector<mariadb::bytes_view>& rawData);
  void deleteCurrentRowData();
  void addRowData(std::vector<mariadb::bytes_view>& rawData);

private:
  void growDataArray();

public:
  void abort();
  void close();

private:
  void resetVariables();
public:
  bool fetchNext();
  bool next();
private:
  void resetRow() const;
  void checkObjectRange(int32_t position) const;
public:
  //SQLWarning* getWarnings();
  //void clearWarnings();
  bool isBeforeFirst() const;
  bool isAfterLast();
  bool isFirst() const;
  bool isLast();
  void beforeFirst();
  void afterLast();
  bool first();
  bool last();
  int64_t getRow();
  bool absolute(int64_t row);
  bool relative(int64_t rows);
  bool previous();
  int32_t getFetchSize() const;
  void setFetchSize(int32_t fetchSize);
  int32_t getType() const;
  int32_t getConcurrency() const;
private:
  void checkClose() const;
public:
  bool isCallableResult() const;
  bool isClosed() const;
  bool wasNull() const;

  bool isNull(int32_t columnIndex) const;
  SQLString getString(int32_t columnIndex) const;
private:
  SQLString zeroFillingIfNeeded(const SQLString& value, ColumnDefinition* columnInformation);
public:
  std::istream* getBinaryStream(int32_t columnIndex) const;
  int32_t getInt(int32_t columnIndex) const;
  int64_t getLong(int32_t columnIndex) const;
  uint64_t getUInt64(int32_t columnIndex) const;
  uint32_t getUInt(int32_t columnIndex) const;
  float getFloat(int32_t columnIndex) const;
  long double getDouble(int32_t columnIndex) const;
  bool getBoolean(int32_t index) const;
  int8_t getByte(int32_t index) const;
  int16_t getShort(int32_t index) const;

  ResultSetMetaData* getMetaData() const;
  /*Blob* getBlob(int32_t columnIndex) const;*/

  std::size_t rowsCount() const;

  void setForceTableAlias();
private:
  void rangeCheck(const SQLString& className,int64_t minValue,int64_t maxValue,int64_t value, ColumnDefinition* columnInfo);
public:
  int32_t getRowPointer();
protected:
  void setRowPointer(int32_t pointer);
  void checkOut();
public:
  std::size_t getDataSize();
  bool isBinaryEncoded();
  void realClose(bool noLock= true);
  void bind(MYSQL_BIND* bind);
  bool get(MYSQL_BIND* bind, uint32_t colIdx0based, uint64_t offset);
  bool get();
};

}
#endif
