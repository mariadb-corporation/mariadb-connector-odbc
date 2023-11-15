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

#include "ResultSet.h"
#include "ServerPrepareResult.h"
#include "ResultSetBin.h"
#include "ResultSetText.h"

namespace mariadb
{
  const MYSQL_FIELD FIELDBIGINT{0,0,0,0,0,0,0, 21, 0,0,0,0,0,0,0,0,0,0,0, MYSQL_TYPE_LONGLONG, 0};
  const MYSQL_FIELD FIELDSTRING{0,0,0,0,0,0,0, 255,0,0,0,0,0,0,0,0,0,0,0, MYSQL_TYPE_STRING, 0};
  const MYSQL_FIELD FIELDSHORT {0,0,0,0,0,0,0, 6,  0,0,0,0,0,0,0,0,0,0,0, MYSQL_TYPE_SHORT, 0};
  const MYSQL_FIELD FIELDINT   {0,0,0,0,0,0,0, 11, 0,0,0,0,0,0,0,0,0,0,0, MYSQL_TYPE_LONG, 0};

  static std::vector<ColumnDefinition> INSERT_ID_COLUMNS{{"insert_id", &FIELDBIGINT}};

  int32_t ResultSet::TINYINT1_IS_BIT= 1;
  int32_t ResultSet::YEAR_IS_DATE_TYPE= 2;

  uint64_t ResultSet::MAX_ARRAY_SIZE= INT32_MAX - 8;


  ResultSet* ResultSet::create(Results* results,
                               Protocol* _protocol,
                               ServerPrepareResult * spr/*, bool callableResult*/)
  {
    return new ResultSetBin(results, _protocol, spr/*, callableResult*/);
  }


  ResultSet* ResultSet::create(Results* results,
                               Protocol* _protocol,
                               MYSQL* connection)
  {
    return new ResultSetText(results, _protocol, connection);
  }

  /**
    * Create filled result-set.
    *
    * @param columnInformation column information
    * @param resultSet result-set data
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.SQL_CURSOR_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    */
  ResultSet* ResultSet::create(
    const MYSQL_FIELD* columnInformation,
    std::vector<std::vector<mariadb::bytes_view>>& resultSet,
    Protocol* _protocol,
    int32_t resultSetScrollType)
  {
    return new ResultSetText(columnInformation, resultSet, _protocol, resultSetScrollType);
  }

  /**
    * Create filled result-set.
    *
    * @param columnInformation column information
    * @param resultSet result-set data
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.SQL_CURSOR_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    */
  ResultSet* ResultSet::create(
    std::vector<ColumnDefinition>& columnInformation,
    const std::vector<std::vector<mariadb::bytes_view>>& resultSet,
    Protocol* _protocol,
    int32_t resultSetScrollType)
  {
    return new ResultSetText(columnInformation, resultSet, _protocol, resultSetScrollType);
  }

  /**
    * Create a result set from given data. Useful for creating "fake" resultsets for
    * DatabaseMetaData, (one example is MariaDbDatabaseMetaData.getTypeInfo())
    *
    * @param data - each element of this array represents a complete row in the ResultSet. Each value
    *     is given in its string representation, as in MariaDB text protocol, except boolean (BIT(1))
    *     values that are represented as "1" or "0" strings
    * @param protocol protocol
    * @param findColumnReturnsOne - special parameter, used only in generated key result sets
    * @return resultset
    */
  ResultSet* ResultSet::createGeneratedData(std::vector<int64_t>& data, bool findColumnReturnsOne)
  {
    /*std::vector<ColumnDefinition> columns{ColumnDefinition::create("insert_id", &bigint)};*/
    std::vector<std::vector<mariadb::bytes_view>> rows;
    std::string idAsStr;


    for (int64_t rowData : data) {
      std::vector<mariadb::bytes_view> row;
      if (rowData != 0) {
        idAsStr= std::to_string(rowData);
        row.emplace_back(idAsStr);
        rows.push_back(row);
      }
    }
    if (findColumnReturnsOne) {
      return create({INSERT_ID_COLUMNS[0].getColumnRawData()}, rows, nullptr, TYPE_SCROLL_SENSITIVE);
      /*int32_t ResultSet::findColumn(const SQLString& name) {
        return 1;
      }*/
    }

    return new ResultSetText(INSERT_ID_COLUMNS, rows, nullptr, TYPE_SCROLL_SENSITIVE);
  }

  ResultSet* ResultSet::createEmptyResultSet() {
    static std::vector<std::vector<mariadb::bytes_view>> emptyRs;

    return create({INSERT_ID_COLUMNS[0].getColumnRawData()}, emptyRs, nullptr, TYPE_SCROLL_SENSITIVE);
  }


  ResultSet * ResultSet::createResultSet(const std::vector<SQLString>& columnNames,
    const std::vector<const MYSQL_FIELD*>& columnTypes,
    const std::vector<std::vector<mariadb::bytes_view>>& data)
  {
    std::size_t columnNameLength= columnNames.size();

    std::vector<ColumnDefinition> columns;
    columns.reserve(columnTypes.size());

    for (std::size_t i= 0; i < columnNameLength; i++) {
      columns.emplace_back(columnNames[i], columnTypes[i]);
    }

    return create(columns, data, nullptr, TYPE_SCROLL_SENSITIVE);
  }


  ResultSet::~ResultSet()
  {
  }

  /**
    * This permit to add next streaming values to existing resultSet.
    *
    * @throws IOException if socket exception occur
    * @throws SQLException if server return an unexpected error
    */
  void ResultSet::addStreamingValue(bool cacheLocally) {

    int32_t fetchSizeTmp = fetchSize;
    while (fetchSizeTmp > 0 && readNextValue(cacheLocally)) {
      --fetchSizeTmp;
    }
    ++dataFetchTime;
  }

} // namespace mariadb