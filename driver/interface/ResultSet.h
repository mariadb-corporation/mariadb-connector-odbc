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


#ifndef _ResultSet_H_
#define _ResultSet_H_

#include <exception>
#include <vector>

#include "mysql.h"

#include "CArray.h"
#include "Row.h"
#include "ColumnDefinition.h"
#include "pimpls.h"

namespace mariadb
{

extern const MYSQL_FIELD FIELDBIGINT;
extern const MYSQL_FIELD FIELDSTRING;
extern const MYSQL_FIELD FIELDSHORT;
extern const MYSQL_FIELD FIELDINT;

class ResultSet
{
  void operator=(ResultSet&) = delete;

protected:
  static uint64_t MAX_ARRAY_SIZE;

  static int32_t TINYINT1_IS_BIT; /*1*/
  static int32_t YEAR_IS_DATE_TYPE; /*2*/
  Protocol *protocol= nullptr;
  int32_t dataFetchTime= 0;
  bool streaming= false;
  int32_t fetchSize= 0;
  mutable Unique::Row row;
  bool isEof= false;
  

  ResultSet(Protocol* guard, int32_t _fetchSize) :
    protocol(guard),
    fetchSize(_fetchSize) 
  {}
public:

  enum {
    TYPE_FORWARD_ONLY = 1003,
    TYPE_SCROLL_INSENSITIVE,
    TYPE_SCROLL_SENSITIVE
  };

  static ResultSet* create(
    Results*,
    Protocol* _protocol,
    ServerPrepareResult* pr);

  static ResultSet* create(
    Results*,
    Protocol* _protocol,
    MYSQL* capiConnHandle);

  static ResultSet* create(
    const MYSQL_FIELD* columnInformation,
    std::vector<std::vector<bytes_view>>& resultSet,
    Protocol* _protocol,
    int32_t resultSetScrollType);

  static ResultSet* create(
    std::vector<ColumnDefinition>& columnInformation,
    const std::vector<std::vector<bytes_view>>& resultSet,
    Protocol* _protocol,
    int32_t resultSetScrollType);

  static ResultSet* createGeneratedData(std::vector<int64_t>& data, bool findColumnReturnsOne);
  static ResultSet* createEmptyResultSet();

  /**
  * Create a result set from given data. Useful for creating "fake" resultSets for
  * DatabaseMetaData, (one example is MariaDbDatabaseMetaData.getTypeInfo())
  *
  * @param columnNames - string array of column names
  * @param columnTypes - column types
  * @param data - each element of this array represents a complete row in the ResultSet. Each value
  *     is given in its string representation, as in MariaDB text protocol, except boolean (BIT(1))
  *     values that are represented as "1" or "0" strings
  * @param protocol protocol
  * @return resultset
  */

  static ResultSet* createResultSet(const std::vector<SQLString>& columnNames, const std::vector<const MYSQL_FIELD*>& columnTypes,
    const std::vector<std::vector<bytes_view>>& data);

  virtual ~ResultSet();

  virtual void close()=0;
  virtual bool next()= 0;

  virtual bool isFullyLoaded() const=0;
  virtual void fetchRemaining()=0;
  virtual ResultSetMetaData* getMetaData() const=0;
  virtual std::size_t rowsCount() const=0;

  virtual bool isLast()=0;
  virtual bool isAfterLast()=0;

  virtual void beforeFirst()=0;
  virtual void afterLast()=0;
  virtual bool first()=0;
  virtual bool last()=0;
  virtual int64_t getRow()=0;
  virtual bool absolute(int64_t row)=0;
  virtual bool relative(int64_t rows)=0;
  virtual bool previous()=0;
 
protected:
  virtual std::vector<bytes_view>& getCurrentRowData()=0;
  virtual void updateRowData(std::vector<bytes_view>& rawData)=0;
  virtual void deleteCurrentRowData()=0;
  virtual void addRowData(std::vector<bytes_view>& rawData)=0;
  void addStreamingValue(bool cacheLocally= false);
  virtual bool readNextValue(bool cacheLocally= false)= 0;

public:
  virtual void abort()=0;
  virtual bool isCallableResult() const=0;
//  virtual PreparedStatement* getStatement()=0;
  virtual void setForceTableAlias()=0;
  virtual int32_t getRowPointer()=0;
  
  virtual void bind(MYSQL_BIND* result)= 0;
  /* Fetches single column value into BIND structure. Returns true on error */
  virtual bool get(MYSQL_BIND* result, uint32_t column0basedIdx, uint64_t offset)= 0;
  /* Fills all bound buffers */
  virtual bool get()= 0;
protected:
  virtual void setRowPointer(int32_t pointer)=0;
  
public:
  /* Some classes(Results) may hold pointer to this object - it may be required in case of RS streaming to
     to fetch remaining rows in order to unblock connection for new queries, or to close RS, if next RS is requested or statements is destructed.
     After releasing the RS by API methods, it's owned by application. If app destructs the RS, this method is called by destructor, and
     implementation should do the job on checking out of the object, so it can't be attempted to use any more */
  virtual void checkOut()= 0;
  virtual std::size_t getDataSize()=0;
  virtual bool isBinaryEncoded()=0;
  virtual void realClose(bool noLock=true)=0;
};

} // namespace mariadb
#endif
