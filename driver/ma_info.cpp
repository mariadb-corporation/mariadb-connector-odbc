/************************************************************************************
   Copyright (C) 2013, 2017 MariaDB Corporation AB
   
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
#include <ma_odbc.h>

MADB_TypeInfo TypeInfoV3[]=
{
  {"BIT",SQL_BIT,1,"","","NULL",1,1,3,0,0,0,"BIT",0,0,0,0,10, SQL_BIT},
  {"BOOL",SQL_BIT,1,"","","NULL",1,1,3,0,0,0,"BOOL",0,0,0,0,10, SQL_BIT},
  {"TINYINT",SQL_TINYINT,3,"","","NULL",1,0,3,SQL_FALSE,0,1,"TINYINT",0,0,0,0,10, SQL_TINYINT},
  {"TINYINT UNSIGNED",SQL_TINYINT,3,"","","NULL",1,0,3,SQL_TRUE,0,1,"TINYINT UNSIGNED",0,0,0,0,10, SQL_TINYINT},
  {"BIGINT",SQL_BIGINT,19,"","","NULL",1,0,3,SQL_FALSE,0,1,"BIGINT",0,0,0,0,10, SQL_BIGINT},
  {"BIGINT UNSIGNED",SQL_BIGINT,20,"","","NULL",1,0,3,1,0,1,"BIGINT UNSIGNED",0,0,0,0,10, SQL_BIGINT},
  {"LONG VARBINARY",SQL_LONGVARBINARY,16777215,"''","''","NULL",1,1,3,0,0,0,"LONG VARBINARY",0,0,0,0,10, SQL_LONGVARBINARY},
  {"MEDIUMBLOB",SQL_LONGVARBINARY,16777215,"''","''","NULL",1,1,3,0,0,0,"MEDIUMBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"LONGBLOB",SQL_LONGVARBINARY,2147483647,"''","''","NULL",1,1,3,0,0,0,"LONGBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"BLOB",SQL_LONGVARBINARY,65535,"''","''","NULL",1,1,3,0,0,0,"BLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"TINYBLOB",SQL_LONGVARBINARY,255,"''","''","NULL",1,1,3,0,0,0,"TINYBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"VARBINARY",SQL_VARBINARY,255,"''","''","'length'",1,1,3,0,0,0,"VARBINARY",0,0,0,0,10, SQL_VARBINARY},
  {"BINARY",SQL_BINARY,255,"''","''","'length'",1,1,3,0,0,0,"BINARY",0,0,0,0,10, SQL_BINARY},
  {"LONG VARCHAR",SQL_LONGVARCHAR,16777215,"''","''","NULL",1,0,3,0,0,0,"LONG VARCHAR",0,0,0,0,10, SQL_LONGVARCHAR},
  {"MEDIUMTEXT",SQL_LONGVARCHAR,16777215,"''","''","NULL",1,0,3,0,0,0,"MEDIUMTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"LONGTEXT",SQL_LONGVARCHAR,2147483647,"''","''","NULL",1,0,3,0,0,0,"LONGTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"TEXT",SQL_LONGVARCHAR,65535,"''","''","NULL",1,0,3,0,0,0,"TEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"TINYTEXT",SQL_LONGVARCHAR,255,"''","''","NULL",1,0,3,0,0,0,"TINYTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"CHAR",SQL_CHAR,255,"''","''","'length'",1,0,3,0,0,0,"CHAR",0,0,0,0,10, SQL_CHAR},
  {"NUMERIC",SQL_NUMERIC,65,"","","'precision,scale'",1,0,3,0,0,1,"NUMERIC", -308,308,0,0,10, SQL_NUMERIC}, /* Todo: ?? */
  {"DECIMAL",SQL_DECIMAL,65,"","","'precision,scale'",1,0,3,0,0,1,"DECIMAL",-308,308,0,0,10, SQL_DECIMAL},
  {"INTEGER",SQL_INTEGER,10,"","","NULL",1,0,3,SQL_FALSE,0,1,"INTEGER",0,0,0,0,10,SQL_INTEGER},
  {"INTEGER UNSIGNED",SQL_INTEGER,10,"","","NULL",1,0,3,1,0,1,"INTEGER UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"INT",SQL_INTEGER,10,"","","NULL",1,0,3,SQL_FALSE,0,1,"INT",0,0,0,0,10, SQL_INTEGER},
  {"INT UNSIGNED",SQL_INTEGER,10,"","","NULL",1,0,3,1,0,1,"INT UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"MEDIUMINT",SQL_INTEGER,7,"","","NULL",1,0,3,SQL_FALSE,0,1,"MEDIUMINT",0,0,0,0,10},
  {"MEDIUMINT UNSIGNED",SQL_INTEGER,8,"","","NULL",1,0,3,1,0,1,"MEDIUMINT UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"SMALLINT",SQL_SMALLINT,5,"","","NULL",1,0,3,SQL_FALSE,0,1,"SMALLINT",0,0,0,0,10, SQL_SMALLINT},
  {"SMALLINT UNSIGNED",SQL_SMALLINT,5,"","","NULL",1,0,3,1,0,1,"SMALLINT UNSIGNED",0,0,0,0,10, SQL_SMALLINT},
  {"FLOAT",SQL_FLOAT,10,"","","'precision,scale'",1,0,3,0,0,1,"FLOAT",-38,38,0,0,10, SQL_FLOAT},
  {"DOUBLE",SQL_DOUBLE,17,"","","'precision,scale'",1,0,3,0,0,1,"DOUBLE",-308,308,0,0,10, SQL_DOUBLE},
  {"DOUBLE PRECISION",SQL_DOUBLE,17,"","","'precision,scale'",1,0,3,0,0,1,"DOUBLE PRECISION",-308,308,0,0,10, SQL_DOUBLE},
  {"REAL",SQL_DOUBLE,17,"","","'precision,scale'",1,0,3,0,0,1,"REAL",-308,308,0,0,10, SQL_DOUBLE},
  {"VARCHAR",SQL_VARCHAR,255,"''","''","'length'",1,0,3,0,0,0,"VARCHAR",0,0,0,0,10, SQL_VARCHAR},
  {"ENUM",SQL_VARCHAR,65535,"''","''","NULL",1,0,3,0,0,0,"ENUM",0,0,0,0,10, SQL_VARCHAR},
  {"SET",SQL_VARCHAR,64,"''","''","NULL",1,0,3,0,0,0,"SET",0,0,0,0,10, SQL_VARCHAR},
  {"DATE",SQL_TYPE_DATE,10,"''","''","NULL",1,0,3,0,0,0,"DATE",0,0,0,0,10, SQL_DATETIME},
  {"TIME",SQL_TYPE_TIME,8,"''","''","NULL",1,0,3,0,0,0,"TIME",0,0,0,0,10, SQL_DATETIME},
  {"DATETIME",SQL_TYPE_TIMESTAMP,16,"''","''","NULL",1,0,3,0,0,0,"DATETIME",0,0,0,0,10, SQL_DATETIME},
  {"TIMESTAMP",SQL_TYPE_TIMESTAMP,16,"''","''","'scale'",1,0,3,0,0,0,"TIMESTAMP",0,0,0,0,10, SQL_DATETIME},
  {"CHAR",SQL_WCHAR,255,"''","''","'length'",1,0,3,0,0,0,"CHAR",0,0,0,0,10, SQL_WCHAR},
  {"VARCHAR",SQL_WVARCHAR,255,"''","''","'length'",1,0,3,0,0,0,"VARCHAR",0,0,0,0,10, SQL_WVARCHAR},
  {"LONG VARCHAR",SQL_WLONGVARCHAR,16777215,"''","''","NULL",1,0,3,0,0,0,"LONG VARCHAR",0,0,0,0,10, SQL_LONGVARCHAR},
  {NULL,0,0,NULL,NULL,NULL,0,0,0,0,0,0,NULL,0,0,0,0,0}
};

MADB_TypeInfo TypeInfoV2[]=
{
  {"BIT",SQL_BIT,1,"","","NULL",1,1,3,0,0,0,"BIT",0,0,0,0,10, SQL_BIT},
  {"BOOL",SQL_BIT,1,"","","NULL",1,1,3,0,0,0,"BOOL",0,0,0,0,10, SQL_BIT},
  {"TINYINT",SQL_TINYINT,3,"","","NULL",1,0,3,SQL_FALSE,0,1,"TINYINT",0,0,0,0,10, SQL_TINYINT},
  {"TINYINT UNSIGNED",SQL_TINYINT,3,"","","NULL",1,0,3,SQL_FALSE,0,1,"TINYINT UNSIGNED",0,0,0,0,10, SQL_TINYINT},
  {"BIGINT",SQL_BIGINT,19,"","","NULL",1,0,3,SQL_FALSE,0,1,"BIGINT",0,0,0,0,10, SQL_BIGINT},
  {"BIGINT UNSIGNED",SQL_BIGINT,20,"","","NULL",1,0,3,SQL_TRUE,0,1,"BIGINT UNSIGNED",0,0,0,0,10, SQL_BIGINT},
  {"LONG VARBINARY",SQL_LONGVARBINARY,16777215,"''","''","NULL",1,1,3,0,0,0,"LONG VARBINARY",0,0,0,0,10, SQL_LONGVARBINARY},
  {"MEDIUMBLOB",SQL_LONGVARBINARY,16777215,"''","''","NULL",1,1,3,0,0,0,"MEDIUMBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"LONGBLOB",SQL_LONGVARBINARY,2147483647,"''","''","NULL",1,1,3,0,0,0,"LONGBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"BLOB",SQL_LONGVARBINARY,65535,"''","''","NULL",1,1,3,0,0,0,"BLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"TINYBLOB",SQL_LONGVARBINARY,255,"''","''","NULL",1,1,3,0,0,0,"TINYBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"VARBINARY",SQL_VARBINARY,255,"''","''","'length'",1,1,3,0,0,0,"VARBINARY",0,0,0,0,10, SQL_VARBINARY},
  {"BINARY",SQL_BINARY,255,"''","''","'length'",1,1,3,0,0,0,"BINARY",0,0,0,0,10, SQL_BINARY},
  {"LONG VARCHAR",SQL_LONGVARCHAR,16777215,"''","''","NULL",1,0,3,0,0,0,"LONG VARCHAR",0,0,0,0,10, SQL_LONGVARCHAR},
  {"MEDIUMTEXT",SQL_LONGVARCHAR,16777215,"''","''","NULL",1,0,3,0,0,0,"MEDIUMTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"LONGTEXT",SQL_LONGVARCHAR,2147483647,"''","''","NULL",1,0,3,0,0,0,"LONGTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"TEXT",SQL_LONGVARCHAR,65535,"''","''","NULL",1,0,3,0,0,0,"TEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"TINYTEXT",SQL_LONGVARCHAR,255,"''","''","NULL",1,0,3,0,0,0,"TINYTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"CHAR",SQL_CHAR,255,"''","''","'length'",1,0,3,0,0,0,"CHAR",0,0,0,0,10, SQL_CHAR},
  {"NUMERIC",SQL_NUMERIC,65,"","","'precision,scale'",1,0,3,0,0,1,"NUMERIC", -308,308,0,0,10, SQL_NUMERIC}, /* Todo: ?? */
  {"DECIMAL",SQL_DECIMAL,65,"","","'precision,scale'",1,0,3,0,0,1,"DECIMAL",-308,308,0,0,10, SQL_DECIMAL},
  {"INTEGER",SQL_INTEGER,10,"","","NULL",1,0,3,SQL_FALSE,0,1,"INTEGER",0,0,0,0,10,SQL_INTEGER},
  {"INTEGER UNSIGNED",SQL_INTEGER,10,"","","NULL",1,0,3,1,0,1,"INTEGER UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"INT",SQL_INTEGER,10,"","","NULL",1,0,3,SQL_FALSE,0,1,"INT",0,0,0,0,10, SQL_INTEGER},
  {"INT UNSIGNED",SQL_INTEGER,10,"","","NULL",1,0,3,1,0,1,"INT UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"MEDIUMINT",SQL_INTEGER,7,"","","NULL",1,0,3,SQL_FALSE,0,1,"MEDIUMINT",0,0,0,0,10},
  {"MEDIUMINT UNSIGNED",SQL_INTEGER,8,"","","NULL",1,0,3,1,0,1,"MEDIUMINT UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"SMALLINT",SQL_SMALLINT,5,"","","NULL",1,0,3,SQL_FALSE,0,1,"SMALLINT",0,0,0,0,10, SQL_SMALLINT},
  {"SMALLINT UNSIGNED",SQL_SMALLINT,5,"","","NULL",1,0,3,1,0,1,"SMALLINT UNSIGNED",0,0,0,0,10, SQL_SMALLINT},
  {"FLOAT",SQL_FLOAT,10,"","","'precision,scale'",1,0,3,0,0,1,"FLOAT",-38,38,0,0,10, SQL_FLOAT},
  {"DOUBLE",SQL_DOUBLE,17,"","","'precision,scale'",1,0,3,0,0,1,"DOUBLE",-308,308,0,0,10, SQL_DOUBLE},
  {"DOUBLE PRECISION",SQL_DOUBLE,17,"","","'precision,scale'",1,0,3,0,0,1,"DOUBLE PRECISION",-308,308,0,0,10, SQL_DOUBLE},
  {"REAL",SQL_DOUBLE,17,"","","'precision,scale'",1,0,3,0,0,1,"REAL",-308,308,0,0,10, SQL_DOUBLE},
  {"VARCHAR",SQL_VARCHAR,255,"''","''","'length'",1,0,3,0,0,0,"VARCHAR",0,0,0,0,10, SQL_VARCHAR},
  {"ENUM",SQL_VARCHAR,65535,"''","''","NULL",1,0,3,0,0,0,"ENUM",0,0,0,0,10, SQL_VARCHAR},
  {"SET",SQL_VARCHAR,64,"''","''","NULL",1,0,3,0,0,0,"SET",0,0,0,0,10, SQL_VARCHAR},
  {"DATE",SQL_DATE,10,"''","''","NULL",1,0,3,0,0,0,"DATE",0,0,0,0,10, SQL_DATETIME},
  {"TIME",SQL_TIME,18,"''","''","NULL",1,0,3,0,0,0,"TIME",0,0,0,0,10, SQL_DATETIME},
  {"DATETIME",SQL_TIMESTAMP,27,"''","''","NULL",1,0,3,0,0,0,"DATETIME",0,0,0,0,10, SQL_DATETIME},
  {"TIMESTAMP",SQL_TIMESTAMP,27,"''","''","'scale'",1,0,3,0,0,0,"TIMESTAMP",0,0,0,0,10, SQL_DATETIME},
  {"CHAR",SQL_WCHAR,255,"''","''","'length'",1,0,3,0,0,0,"CHAR",0,0,0,0,10, SQL_WCHAR},
  {"VARCHAR",SQL_WVARCHAR,255,"''","''","'length'",1,0,3,0,0,0,"VARCHAR",0,0,0,0,10, SQL_WVARCHAR},
  {"LONG VARCHAR",SQL_WLONGVARCHAR,16777215,"''","''","NULL",1,0,3,0,0,0,"LONG VARCHAR",0,0,0,0,10, SQL_WLONGVARCHAR},
  {NULL,0,0,NULL,NULL,NULL,0,0,0,0,0,0,NULL,0,0,0,0,0}
};

static MADB_ShortTypeInfo gtiDefType[19]= {{0, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_INTEGER, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
                                 /*7*/     {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0},
                                 /*11*/    {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {0, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0},
                                 /*16*/    {SQL_SMALLINT, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0}, {SQL_INTEGER, 0, 0, 0}, {SQL_SMALLINT, 0, 0, 0} };
/* {{{ MADB_GetTypeInfo */
SQLRETURN MADB_GetTypeInfo(SQLHSTMT StatementHandle,
                           SQLSMALLINT DataType)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  my_bool   isFirst= TRUE;
  char      StmtStr[5120];
  char      *p= StmtStr;
  int       i;
  MADB_TypeInfo *TypeInfo= TypeInfoV3;

  if (Stmt->Connection->Environment->OdbcVersion == SQL_OV_ODBC2)
  {
    TypeInfo= TypeInfoV2;
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

  StmtStr[0]= 0;
  for (i=0;TypeInfo[i].TypeName; i++)
  {
    if (DataType == SQL_ALL_TYPES ||
       TypeInfo[i].DataType == DataType)
    {
      if(isFirst)
      {
        isFirst= FALSE;
        p+= _snprintf(p, 5120 - strlen(StmtStr),
          "SELECT '%s' AS TYPE_NAME, %d AS DATA_TYPE, %lu AS COLUMN_SIZE, '%s' AS LITERAL_PREFIX, "
          "'%s' AS LITERAL_SUFFIX, %s AS CREATE_PARAMS, %d AS NULLABLE, %d AS CASE_SENSITIVE, "
          "%d AS SEARCHABLE, %d AS UNSIGNED_ATTRIBUTE, %d AS FIXED_PREC_SCALE, %d AS AUTO_UNIQUE_VALUE, "
          "'%s' AS LOCAL_TYPE_NAME, %d AS MINIMUM_SCALE, %d AS MAXIMUM_SCALE, "
          "%d AS SQL_DATA_TYPE, "
          "%d AS SQL_DATETIME_SUB, %d AS NUM_PREC_RADIX, NULL AS INTERVAL_PRECISION ",
         TypeInfo[i].TypeName,TypeInfo[i].DataType,TypeInfo[i].ColumnSize,TypeInfo[i].LiteralPrefix,
         TypeInfo[i].LiteralSuffix,TypeInfo[i].CreateParams,TypeInfo[i].Nullable,TypeInfo[i].CaseSensitive,
         TypeInfo[i].Searchable,TypeInfo[i].Unsigned,TypeInfo[i].FixedPrecScale,TypeInfo[i].AutoUniqueValue,
         TypeInfo[i].LocalTypeName,TypeInfo[i].MinimumScale,TypeInfo[i].MaximumScale,
         TypeInfo[i].SqlDataType,
         TypeInfo[i].SqlDateTimeSub,TypeInfo[i].NumPrecRadix);
      }
      else
          p+= _snprintf(p, 5120 - strlen(StmtStr),
          "UNION SELECT '%s', %d, %lu , '%s', "
          "'%s', %s, %d, %d, "
          "%d, %d, %d, %d, "
          "'%s', %d, %d, "
          "%d, "
          "%d, %d, NULL ",
         TypeInfo[i].TypeName,TypeInfo[i].DataType,TypeInfo[i].ColumnSize,TypeInfo[i].LiteralPrefix,
         TypeInfo[i].LiteralSuffix,TypeInfo[i].CreateParams,TypeInfo[i].Nullable,TypeInfo[i].CaseSensitive,
         TypeInfo[i].Searchable,TypeInfo[i].Unsigned,TypeInfo[i].FixedPrecScale,TypeInfo[i].AutoUniqueValue,
         TypeInfo[i].LocalTypeName,TypeInfo[i].MinimumScale,TypeInfo[i].MaximumScale,
         TypeInfo[i].SqlDataType,
         TypeInfo[i].SqlDateTimeSub,TypeInfo[i].NumPrecRadix);
    }
  }
  ret= Stmt->Methods->ExecDirect(Stmt, StmtStr, SQL_NTS);
  if (SQL_SUCCEEDED(ret))
  {
    MADB_FixColumnDataTypes(Stmt, gtiDefType);
  }
  return ret;
}
/* }}} */
