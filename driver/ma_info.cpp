/************************************************************************************
   Copyright (C) 2013, 2023 MariaDB Corporation AB
   
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
#include "ma_odbc.h"
#include "class/ResultSetMetaData.h"
#include "interface/ResultSet.h"
#include "interface/PreparedStatement.h"

#include "template/CArray.h"

#define BV(_STRCONST) mariadb::bytes_view(_STRCONST, sizeof(_STRCONST))
#define XBV(_STRCONST) BV(#_STRCONST)

static mariadb::bytes_view zero("0", 2), one("1", 2), three("3", 2), Null, ten("10", 3), empty("",1),
  sqlwchar("-8", 3), sqlwvarchar("-9", 3), sqlwlongvarchar("-10", 4), sqlbit("-7", 3), sqltinyint("-6", 3),
  sqlbigint("-5", 3), sqllongvarbinary("-4", 3), sqlvarbinary("-3", 3), sqlbinary("-2", 3), sqllongvarchar("-1", 3); // They defined as (-8) etc, and that's not parsed well as int

std::vector <std::vector<mariadb::bytes_view>> TypeInfoV3=
{
  {bytes_view(XBV(BIT), 4), sqlbit, one, empty, empty, Null, one, one, three, zero, zero, zero, zero, zero, zero, zero, zero,
    ten, sqlbit},
  {XBV(BOOL), sqlbit,one,empty,empty,Null,one,one,three,zero,zero,zero,XBV(BOOL),zero,zero,zero,zero,ten, sqlbit},
  {XBV(TINYINT),sqltinyint,three,empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(TINYINT),zero,zero,zero,zero,
    ten, sqltinyint},
  {XBV(TINYINT UNSIGNED),sqltinyint,three,empty,empty,Null,one,zero,three,BV(XSTR(SQL_TRUE)),zero,one,XBV(TINYINT UNSIGNED),zero,
    zero,zero,zero,ten, sqltinyint},
  {XBV(BIGINT),sqlbigint,XBV(19),empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(BIGINT),zero,zero,zero,zero,
    ten, sqlbigint},
  {XBV(BIGINT UNSIGNED),sqlbigint,XBV(20),empty,empty,Null,one,zero,three,one,zero,one,XBV(BIGINT UNSIGNED),zero,zero,zero,zero,
    ten, sqlbigint},
  {XBV(LONG VARBINARY),sqllongvarbinary,XBV(16777215),empty,empty,Null,one,one,three,zero,zero,zero, XBV(LONG VARBINARY),zero,
    zero,zero,zero,ten, sqllongvarbinary},
  {XBV(MEDIUMBLOB),sqllongvarbinary,XBV(16777215),empty,empty,Null,one,one,three,zero,zero,zero,XBV(MEDIUMBLOB),zero,zero,zero,
    zero,ten, sqllongvarbinary},
  {XBV(LONGBLOB),sqllongvarbinary,XBV(2147483647),empty,empty,Null,one,one,three,zero,zero,zero,XBV(LONGBLOB),zero,zero,zero,zero,
    ten, sqllongvarbinary},
  {XBV(BLOB),sqllongvarbinary,XBV(65535),empty,empty,Null,one,one,three,zero,zero,zero,XBV(BLOB),zero,zero,zero,zero,ten,
    sqllongvarbinary},
  {XBV(TINYBLOB),sqllongvarbinary,XBV(255),empty,empty,Null,one,one,three,zero,zero,zero,XBV(TINYBLOB),zero,zero,zero,zero,ten,
    sqllongvarbinary},
  {XBV(VARBINARY),sqlvarbinary,XBV(255),empty,empty,XBV(length),one,one,three,zero,zero,zero,XBV(VARBINARY),zero,zero,zero,zero,
    ten, sqlvarbinary},
  {XBV(BINARY),sqlbinary,XBV(255),empty,empty,XBV(length),one,one,three,zero,zero,zero,XBV(BINARY),zero,zero,zero,zero,ten,
    sqlbinary},
  {XBV(LONG VARCHAR),sqllongvarchar,XBV(16777215),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(LONG VARCHAR),zero,zero,zero,
    zero,ten, sqllongvarchar},
  {XBV(MEDIUMTEXT),sqllongvarchar,XBV(16777215),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(MEDIUMTEXT),zero,zero,zero,zero,
    ten, sqllongvarchar},
  {XBV(LONGTEXT),sqllongvarchar,XBV(2147483647),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(LONGTEXT),zero,zero,zero,zero,
    ten, sqllongvarchar},
  {XBV(TEXT),sqllongvarchar,XBV(65535),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(TEXT),zero,zero,zero,zero,ten,
    sqllongvarchar},
  {XBV(TINYTEXT),sqllongvarchar,XBV(255),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(TINYTEXT),zero,zero,zero,zero,ten,
    sqllongvarchar},
  {XBV(CHAR),BV(XSTR(SQL_CHAR)),XBV(255),empty,empty,XBV(length),one,zero,three,zero,zero,zero,XBV(CHAR),zero,zero,zero,
    zero,ten, BV(XSTR(SQL_CHAR))},
  {XBV(NUMERIC),BV(XSTR(SQL_NUMERIC)),BV(XSTR(MADB_DECIMAL_MAX_PRECISION)),empty,empty,BV("precision,scale"),one,zero,three,zero,
    zero,one,XBV(NUMERIC),XBV(-308), XBV(308),zero,zero,ten, BV(XSTR(SQL_NUMERIC))},
  {XBV(DECIMAL),BV(XSTR(SQL_DECIMAL)),BV(XSTR(MADB_DECIMAL_MAX_PRECISION)),empty,empty,BV("precision,scale"),one,zero,three,zero,
    zero,one,XBV(DECIMAL),XBV(-308), XBV(308),zero,zero,ten, BV(XSTR(SQL_DECIMAL))},
  {XBV(INTEGER),BV(XSTR(SQL_INTEGER)),ten,empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(INTEGER),zero,zero,zero,
    zero,ten,BV(XSTR(SQL_INTEGER))},
  {XBV(INTEGER UNSIGNED),BV(XSTR(SQL_INTEGER)),ten,empty,empty,Null,one,zero,three,one,zero,one,XBV(INTEGER UNSIGNED),zero,zero,zero,
    zero,ten, BV(XSTR(SQL_INTEGER))},
  {XBV(INT),BV(XSTR(SQL_INTEGER)),ten,empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(INT),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_INTEGER))},
  {XBV(INT UNSIGNED),BV(XSTR(SQL_INTEGER)),ten,empty,empty,Null,one,zero,three,one,zero,one,XBV(INT UNSIGNED),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_INTEGER))},
  {XBV(MEDIUMINT),BV(XSTR(SQL_INTEGER)),XBV(7),empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(MEDIUMINT),zero,zero,
    zero,zero,ten,BV(XSTR(SQL_INTEGER))},
  {XBV(MEDIUMINT UNSIGNED),BV(XSTR(SQL_INTEGER)),XBV(8),empty,empty,Null,one,zero,three,one,zero,one,XBV(MEDIUMINT UNSIGNED),zero,zero,
    zero,zero,ten, BV(XSTR(SQL_INTEGER))},
  {XBV(SMALLINT),BV(XSTR(SQL_SMALLINT)),XBV(5),empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(SMALLINT),zero,zero,zero,
    zero,ten, BV(XSTR(SQL_SMALLINT))},
  {XBV(SMALLINT UNSIGNED),BV(XSTR(SQL_SMALLINT)),XBV(5),empty,empty,Null,one,zero,three,one,zero,one,XBV(SMALLINT UNSIGNED),zero,zero,
    zero,zero,ten, BV(XSTR(SQL_SMALLINT))},
  {XBV(FLOAT),BV(XSTR(SQL_FLOAT)),ten,empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,XBV(FLOAT),XBV(-38),XBV(38),zero,
    zero,ten, BV(XSTR(SQL_FLOAT))},
  {XBV(DOUBLE),BV(XSTR(SQL_DOUBLE)),XBV(17),empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,XBV(DOUBLE),XBV(-308),
    XBV(308),zero,zero,ten, BV(XSTR(SQL_DOUBLE))},
  {XBV(DOUBLE PRECISION),BV(XSTR(SQL_DOUBLE)),XBV(17),empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,
    XBV(DOUBLE PRECISION),XBV(-308),XBV(308),zero,zero,ten, BV(XSTR(SQL_DOUBLE))},
  {XBV(REAL),BV(XSTR(SQL_DOUBLE)),XBV(17),empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,XBV(REAL),XBV(-308),XBV(308),
    zero,zero,ten, BV(XSTR(SQL_DOUBLE))},
  {XBV(VARCHAR),BV(XSTR(SQL_VARCHAR)),XBV(255),empty,empty,XBV(length),one,zero,three,zero,zero,zero,XBV(VARCHAR),zero,zero,zero,
    zero,ten, BV(XSTR(SQL_VARCHAR))},
  {XBV(ENUM),BV(XSTR(SQL_VARCHAR)),XBV(65535),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(ENUM),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_VARCHAR))},
  {XBV(SET),BV(XSTR(SQL_VARCHAR)),XBV(64),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(SET),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_VARCHAR))},
  {XBV(DATE),BV(XSTR(SQL_TYPE_DATE)),ten,empty,empty,Null,one,zero,three,zero,zero,zero,XBV(DATE),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_DATETIME))},
  {XBV(TIME),BV(XSTR(SQL_TYPE_TIME)),XBV(8),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(TIME),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_DATETIME))},
  {XBV(DATETIME),BV(XSTR(SQL_TYPE_TIMESTAMP)),XBV(16),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(DATETIME),zero,zero,zero,zero,
    ten, BV(XSTR(SQL_DATETIME))},
  {XBV(TIMESTAMP),BV(XSTR(SQL_TYPE_TIMESTAMP)),XBV(16),empty,empty,XBV(scale),one,zero,three,zero,zero,zero,XBV(TIMESTAMP),zero,zero,
    zero,zero,ten, BV(XSTR(SQL_DATETIME))},
  {XBV(CHAR),sqlwchar,XBV(255),empty,empty,XBV(length),one,zero,three,zero,zero,zero,XBV(CHAR),zero,zero,zero,zero,ten,
    sqlwchar},
  {XBV(VARCHAR),sqlwvarchar,XBV(255),empty,empty,XBV(length),one,zero,three,zero,zero,zero,XBV(VARCHAR),zero,zero,zero,zero,
    ten, sqlwvarchar},
  {XBV(LONG VARCHAR),sqlwlongvarchar,XBV(16777215),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(LONG VARCHAR),zero,zero,
    zero,zero,ten, sqlwlongvarchar}
};

std::vector<std::vector<mariadb::bytes_view>> TypeInfoV2=
{
  {bytes_view(XBV(BIT), 4), sqlbit, one, empty, empty, Null, one, one, three, zero, zero, zero, zero, zero, zero,
    zero, zero, ten, sqlbit},
  {XBV(BIT),sqlbit,one,empty,empty,Null,one,one,three,zero,zero,zero,XBV(BIT),zero,zero,zero,zero,ten, sqlbit},
  {XBV(BOOL),sqlbit,one,empty,empty,Null,one,one,three,zero,zero,zero,XBV(BOOL),zero,zero,zero,zero,ten, sqlbit},
  {XBV(TINYINT),sqltinyint,three,empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(TINYINT),zero,zero,zero,zero,ten,
    sqltinyint},
  {XBV(TINYINT UNSIGNED),sqltinyint,three,empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(TINYINT UNSIGNED),zero,
    zero,zero,zero,ten, sqltinyint},
  {XBV(BIGINT),sqlbigint,XBV(19),empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(BIGINT),zero,zero,zero,zero,ten,
  sqlbigint},
  {XBV(BIGINT UNSIGNED),sqlbigint,XBV(20),empty,empty,Null,one,zero,three,BV(XSTR(SQL_TRUE)),zero,one,XBV(BIGINT UNSIGNED),zero,zero
    ,zero,zero,ten, sqlbigint},
  {XBV(LONG VARBINARY),sqllongvarbinary,XBV(16777215),empty,empty,Null,one,one,three,zero,zero,zero,XBV(LONG VARBINARY),zero,
    zero,zero,zero,ten, sqllongvarbinary},
  {XBV(MEDIUMBLOB),sqllongvarbinary,XBV(16777215),empty,empty,Null,one,one,three,zero,zero,zero,XBV(MEDIUMBLOB),zero,zero,zero,
    zero,ten, sqllongvarbinary},
  {XBV(LONGBLOB),sqllongvarbinary,XBV(2147483647),empty,empty,Null,one,one,three,zero,zero,zero,XBV(LONGBLOB),zero,zero,zero,
    zero,ten, sqllongvarbinary},
  {XBV(BLOB),sqllongvarbinary,XBV(65535),empty,empty,Null,one,one,three,zero,zero,zero,XBV(BLOB),zero,zero,zero,zero,ten,
    sqllongvarbinary},
  {XBV(TINYBLOB),sqllongvarbinary,XBV(255),empty,empty,Null,one,one,three,zero,zero,zero,XBV(TINYBLOB),zero,zero,zero,zero,ten,
    sqllongvarbinary},
  {XBV(VARBINARY),sqlvarbinary,XBV(255),empty,empty,XBV(length),one,one,three,zero,zero,zero,XBV(VARBINARY),zero,zero,zero,zero,
    ten, sqlvarbinary},
  {XBV(BINARY),sqlbinary,XBV(255),empty,empty,XBV(length),one,one,three,zero,zero,zero,XBV(BINARY),zero,zero,zero,zero,ten,
    sqlbinary},
  {XBV(LONG VARCHAR),sqllongvarchar,XBV(16777215),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(LONG VARCHAR),zero,zero,zero,
    zero,ten, sqllongvarchar},
  {XBV(MEDIUMTEXT),sqllongvarchar,XBV(16777215),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(MEDIUMTEXT),zero,zero,zero,zero,
    ten, sqllongvarchar},
  {XBV(LONGTEXT),sqllongvarchar,XBV(2147483647),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(LONGTEXT),zero,zero,zero,zero,
    ten, sqllongvarchar},
  {XBV(TEXT),sqllongvarchar,XBV(65535),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(TEXT),zero,zero,zero,zero,ten,
    sqllongvarchar},
  {XBV(TINYTEXT),sqllongvarchar,XBV(255),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(TINYTEXT),zero,zero,zero,zero,ten,
    sqllongvarchar},
  {XBV(CHAR),BV(XSTR(SQL_CHAR)),XBV(255),empty,empty,XBV(length),one,zero,three,zero,zero,zero,XBV(CHAR),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_CHAR))},
  {XBV(NUMERIC),BV(XSTR(SQL_NUMERIC)),BV(XSTR(MADB_DECIMAL_MAX_PRECISION)),empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,
    XBV(NUMERIC),XBV(-308),XBV(308),zero, zero,ten, BV(XSTR(SQL_NUMERIC))},
  {XBV(DECIMAL),BV(XSTR(SQL_DECIMAL)),BV(XSTR(MADB_DECIMAL_MAX_PRECISION)),empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,
    XBV(DECIMAL),XBV(-308),XBV(308),zero, zero,ten, BV(XSTR(SQL_DECIMAL))},
  {XBV(INTEGER),BV(XSTR(SQL_INTEGER)),ten,empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(INTEGER),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_INTEGER))},
  {XBV(INTEGER UNSIGNED),BV(XSTR(SQL_INTEGER)),ten,empty,empty,Null,one,zero,three,one,zero,one,XBV(INTEGER UNSIGNED),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_INTEGER))},
  {XBV(INT),BV(XSTR(SQL_INTEGER)),ten,empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(INT),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_INTEGER))},
  {XBV(INT UNSIGNED),BV(XSTR(SQL_INTEGER)),ten,empty,empty,Null,one,zero,three,one,zero,one,XBV(INT UNSIGNED),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_INTEGER))},
  {XBV(MEDIUMINT),BV(XSTR(SQL_INTEGER)),XBV(7),empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(MEDIUMINT),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_INTEGER))},
  {XBV(MEDIUMINT UNSIGNED),BV(XSTR(SQL_INTEGER)),XBV(8),empty,empty,Null,one,zero,three,one,zero,one,XBV(MEDIUMINT UNSIGNED),zero,zero,zero,zero,
    ten, BV(XSTR(SQL_INTEGER))},
  {XBV(SMALLINT),BV(XSTR(SQL_SMALLINT)),XBV(5),empty,empty,Null,one,zero,three,BV(XSTR(SQL_FALSE)),zero,one,XBV(SMALLINT),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_SMALLINT))},
  {XBV(SMALLINT UNSIGNED),BV(XSTR(SQL_SMALLINT)),XBV(5),empty,empty,Null,one,zero,three,one,zero,one,XBV(SMALLINT UNSIGNED),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_SMALLINT))},
  {XBV(FLOAT),BV(XSTR(SQL_FLOAT)),ten,empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,XBV(FLOAT),XBV(-38),XBV(38),zero,zero,ten,
    BV(XSTR(SQL_FLOAT))},
  {XBV(DOUBLE),BV(XSTR(SQL_DOUBLE)),XBV(17),empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,XBV(DOUBLE),XBV(-308),XBV(308),zero,zero,
    ten, BV(XSTR(SQL_DOUBLE))},
  {XBV(DOUBLE PRECISION),BV(XSTR(SQL_DOUBLE)),XBV(17),empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,XBV(DOUBLE PRECISION),XBV(-308),
    XBV(308),zero,zero,ten, BV(XSTR(SQL_DOUBLE))},
  {XBV(REAL),BV(XSTR(SQL_DOUBLE)),XBV(17),empty,empty,BV("precision,scale"),one,zero,three,zero,zero,one,XBV(REAL),XBV(-308),XBV(308),zero,zero,ten,
    BV(XSTR(SQL_DOUBLE))},
  {XBV(VARCHAR),BV(XSTR(SQL_VARCHAR)),XBV(255),empty,empty,XBV(length),one,zero,three,zero,zero,zero,XBV(VARCHAR),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_VARCHAR))},
  {XBV(ENUM),BV(XSTR(SQL_VARCHAR)),XBV(65535),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(ENUM),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_VARCHAR))},
  {XBV(SET),BV(XSTR(SQL_VARCHAR)),XBV(64),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(SET),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_VARCHAR))},
  {XBV(DATE),BV(XSTR(SQL_DATE)),ten,empty,empty,Null,one,zero,three,zero,zero,zero,XBV(DATE),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_DATETIME))},
  {XBV(TIME),BV(XSTR(SQL_TIME)),XBV(18),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(TIME),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_DATETIME))},
  {XBV(DATETIME),BV(XSTR(SQL_TIMESTAMP)),XBV(27),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(DATETIME),zero,zero,zero,zero,ten,
    BV(XSTR(SQL_DATETIME))},
  {XBV(TIMESTAMP),BV(XSTR(SQL_TIMESTAMP)),XBV(27),empty,empty,XBV(scale),one,zero,three,zero,zero,zero,XBV(TIMESTAMP),zero,zero,zero,zero,
    ten, BV(XSTR(SQL_DATETIME))},
  {XBV(CHAR),sqlwchar,XBV(255),empty,empty,XBV(length),one,zero,three,zero,zero,zero,XBV(CHAR),zero,zero,zero,zero,ten, sqlwchar},
  {XBV(VARCHAR),sqlwvarchar,XBV(255),empty,empty,XBV(length),one,zero,three,zero,zero,zero,XBV(VARCHAR),zero,zero,zero,zero,ten, sqlwvarchar},
  {XBV(LONG VARCHAR),sqlwlongvarchar,XBV(16777215),empty,empty,Null,one,zero,three,zero,zero,zero,XBV(LONG VARCHAR),zero,zero,zero,zero,
    ten, sqlwlongvarchar}
};

static std::vector<SQLString> TypeInfoColumnName{"TYPE_NAME", "DATA_TYPE", "COLUMN_SIZE", "LITERAL_PREFIX",
          "LITERAL_SUFFIX", "CREATE_PARAMS", "NULLABLE", "CASE_SENSITIVE", "SEARCHABLE", "UNSIGNED_ATTRIBUTE",
          "FIXED_PREC_SCALE", "AUTO_UNIQUE_VALUE", "LOCAL_TYPE_NAME", "MINIMUM_SCALE", "MAXIMUM_SCALE", "SQL_DATA_TYPE",
          "SQL_DATETIME_SUB", "NUM_PREC_RADIX", "INTERVAL_PRECISION"};

#define MA_CONST_LEN(LITERALSTRCONST) sizeof(#LITERALSTRCONST) - 1
static MADB_ShortTypeInfo gtiDefType[19]= {};
static std::vector<const MYSQL_FIELD*> TypeInfoColumnType= {
  &FIELDSTRING, &FIELDSHORT, &FIELDINT, &FIELDSTRING, &FIELDSTRING, &FIELDSTRING, &FIELDSHORT, &FIELDSHORT, &FIELDSHORT, &FIELDSHORT,
  &FIELDSHORT, &FIELDSHORT, &FIELDSTRING, &FIELDSHORT, &FIELDSHORT, &FIELDSHORT, &FIELDSHORT, &FIELDINT, &FIELDSHORT };

/* {{{ MADB_GetTypeInfo */
SQLRETURN MADB_GetTypeInfo(SQLHSTMT StatementHandle,
                           SQLSMALLINT DataType)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  std::vector<std::vector<bytes_view>> *TypeInfo= &TypeInfoV3;

  if (Stmt->Connection->Environment->OdbcVersion == SQL_OV_ODBC2)
  {
    TypeInfo= &TypeInfoV2;
    /* We need to map time types */
    switch(DataType) {
      case SQL_TYPE_TIMESTAMP:
        DataType=SQL_TIMESTAMP;
        break;
      case SQL_TYPE_DATE:
        DataType= SQL_DATE;
        break;
      case SQL_TYPE_TIME:
        DataType= SQL_TIME;
        break;
      default:
      break;
    }
  }

  std::vector<std::vector<mariadb::bytes_view>> row;

  Stmt->stmt.reset();
  if (DataType == SQL_ALL_TYPES)
  {
    Stmt->rs.reset(ResultSet::createResultSet(TypeInfoColumnName, TypeInfoColumnType, *TypeInfo));
  }
  else
  {
    std::string dataTypeAsStr(std::to_string(DataType));
    for (const auto& cit : *TypeInfo)
    {
      if (dataTypeAsStr.compare(cit[1].arr) == 0)
      {
        row.push_back(cit);
      }
    }
    Stmt->rs.reset(ResultSet::createResultSet(TypeInfoColumnName, TypeInfoColumnType, row));
  }

  Stmt->State= MADB_SS_EXECUTED;
  Stmt->AfterExecute();

  //MADB_FixColumnDataTypes(Stmt, gtiDefType);
  return SQL_SUCCESS;
}
/* }}} */
