/************************************************************************************
   Copyright (C) 2013 SkySQL AB
   
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
  {"BIT",SQL_BIT,1,"","","",1,1,3,0,0,0,"BIT",0,0,0,0,10, SQL_BIT},
  {"BOOL",SQL_BIT,1,"","","",1,1,3,0,0,0,"BOOL",0,0,0,0,10, SQL_BIT},
  {"TINYINT",SQL_TINYINT,3,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"TINYINT",0,0,0,0,10, SQL_TINYINT},
  {"TINYINT UNSIGNED",SQL_TINYINT,3,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"TINYINT UNSIGNED",0,0,0,0,10, SQL_TINYINT},
  {"BIGINT",SQL_BIGINT,19,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"BIGINT",0,0,0,0,10, SQL_BIGINT},
  {"BIGINT UNSIGNED",SQL_BIGINT,20,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"BIGINT UNSIGNED",0,0,0,0,10, SQL_BIGINT},
  {"LONG VARBINARY",SQL_LONGVARBINARY,16777215,"'","'","",1,1,3,0,0,0,"LONG VARBINARY",0,0,0,0,10, SQL_LONGVARBINARY},
  {"MEDIUMBLOB",SQL_LONGVARBINARY,16777215,"'","'","",1,1,3,0,0,0,"MEDIUMBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"LONGBLOB",SQL_LONGVARBINARY,2147483647,"'","'","",1,1,3,0,0,0,"LONGBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"BLOB",SQL_LONGVARBINARY,65535,"'","'","",1,1,3,0,0,0,"BLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"TINYBLOB",SQL_LONGVARBINARY,255,"'","'","",1,1,3,0,0,0,"TINYBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"VARBINARY",SQL_VARBINARY,255,"'","'","(M)",1,1,3,0,0,0,"VARBINARY",0,0,0,0,10, SQL_VARBINARY},
  {"BINARY",SQL_BINARY,255,"'","'","(M)",1,1,3,0,0,0,"BINARY",0,0,0,0,10, SQL_BINARY},
  {"LONG VARCHAR",SQL_LONGVARCHAR,16777215,"'","'","",1,0,3,0,0,0,"LONG VARCHAR",0,0,0,0,10, SQL_LONGVARCHAR},
  {"MEDIUMTEXT",SQL_LONGVARCHAR,16777215,"'","'","",1,0,3,0,0,0,"MEDIUMTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"LONGTEXT",SQL_LONGVARCHAR,2147483647,"'","'","",1,0,3,0,0,0,"LONGTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"TEXT",SQL_LONGVARCHAR,65535,"'","'","",1,0,3,0,0,0,"TEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"TINYTEXT",SQL_LONGVARCHAR,255,"'","'","",1,0,3,0,0,0,"TINYTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"CHAR",SQL_CHAR,255,"'","'","(M)",1,0,3,0,0,0,"CHAR",0,0,0,0,10, SQL_CHAR},
  {"NUMERIC",SQL_NUMERIC,65,"","","[(M,D])] [ZEROFILL]",1,0,3,0,0,1,"NUMERIC", -308,308,0,0,10, SQL_NUMERIC}, /* Todo: ?? */
  {"DECIMAL",SQL_DECIMAL,65,"","","[(M,D])] [ZEROFILL]",1,0,3,0,0,1,"DECIMAL",-308,308,0,0,10, SQL_DECIMAL},
  {"INTEGER",SQL_INTEGER,10,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"INTEGER",0,0,0,0,10,SQL_INTEGER},
  {"INTEGER UNSIGNED",SQL_INTEGER,10,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"INTEGER UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"INT",SQL_INTEGER,10,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"INT",0,0,0,0,10, SQL_INTEGER},
  {"INT UNSIGNED",SQL_INTEGER,10,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"INT UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"MEDIUMINT",SQL_INTEGER,7,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"MEDIUMINT",0,0,0,0,10},
  {"MEDIUMINT UNSIGNED",SQL_INTEGER,8,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"MEDIUMINT UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"SMALLINT",SQL_SMALLINT,5,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"SMALLINT",0,0,0,0,10, SQL_SMALLINT},
  {"SMALLINT UNSIGNED",SQL_SMALLINT,5,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"SMALLINT UNSIGNED",0,0,0,0,10, SQL_SMALLINT},
  {"FLOAT",SQL_FLOAT,10,"","","[(M|D)] [ZEROFILL]",1,0,3,0,0,1,"FLOAT",-38,38,0,0,10, SQL_FLOAT},
  {"DOUBLE",SQL_DOUBLE,17,"","","[(M|D)] [ZEROFILL]",1,0,3,0,0,1,"DOUBLE",-308,308,0,0,10, SQL_DOUBLE},
  {"DOUBLE PRECISION",SQL_DOUBLE,17,"","","[(M,D)] [ZEROFILL]",1,0,3,0,0,1,"DOUBLE PRECISION",-308,308,0,0,10, SQL_DOUBLE},
  {"REAL",SQL_DOUBLE,17,"","","[(M,D)] [ZEROFILL]",1,0,3,0,0,1,"REAL",-308,308,0,0,10, SQL_DOUBLE},
  {"VARCHAR",SQL_VARCHAR,255,"'","'","(M)",1,0,3,0,0,0,"VARCHAR",0,0,0,0,10, SQL_VARCHAR},
  {"ENUM",SQL_VARCHAR,65535,"'","'","",1,0,3,0,0,0,"ENUM",0,0,0,0,10, SQL_VARCHAR},
  {"SET",SQL_VARCHAR,64,"'","'","",1,0,3,0,0,0,"SET",0,0,0,0,10, SQL_VARCHAR},
  {"DATE",SQL_TYPE_DATE,10,"'","'","",1,0,3,0,0,0,"DATE",0,0,0,0,10, SQL_DATETIME},
  {"TIME",SQL_TYPE_TIME,8,"'","'","[(M)]",1,0,3,0,0,0,"TIME",0,0,0,0,10, SQL_DATETIME},
  {"DATETIME",SQL_TYPE_TIMESTAMP,16,"'","'","[(M)]",1,0,3,0,0,0,"DATETIME",0,0,0,0,10, SQL_DATETIME},
  {"TIMESTAMP",SQL_TYPE_TIMESTAMP,16,"'","'","[(M)]",1,0,3,0,0,0,"TIMESTAMP",0,0,0,0,10, SQL_DATETIME},
  {NULL,0,0,NULL,NULL,NULL,0,0,0,0,0,0,NULL,0,0,0,0,0}
};

MADB_TypeInfo TypeInfoV2[]=
{
  {"BIT",SQL_BIT,1,"","","",1,1,3,0,0,0,"BIT",0,0,0,0,10, SQL_BIT},
  {"BOOL",SQL_BIT,1,"","","",1,1,3,0,0,0,"BOOL",0,0,0,0,10, SQL_BIT},
  {"TINYINT",SQL_TINYINT,3,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"TINYINT",0,0,0,0,10, SQL_TINYINT},
  {"TINYINT UNSIGNED",SQL_TINYINT,3,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"TINYINT UNSIGNED",0,0,0,0,10, SQL_TINYINT},
  {"BIGINT",SQL_BIGINT,19,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"BIGINT",0,0,0,0,10, SQL_BIGINT},
  {"BIGINT UNSIGNED",SQL_BIGINT,20,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"BIGINT UNSIGNED",0,0,0,0,10, SQL_BIGINT},
  {"LONG VARBINARY",SQL_LONGVARBINARY,16777215,"'","'","",1,1,3,0,0,0,"LONG VARBINARY",0,0,0,0,10, SQL_LONGVARBINARY},
  {"MEDIUMBLOB",SQL_LONGVARBINARY,16777215,"'","'","",1,1,3,0,0,0,"MEDIUMBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"LONGBLOB",SQL_LONGVARBINARY,2147483647,"'","'","",1,1,3,0,0,0,"LONGBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"BLOB",SQL_LONGVARBINARY,65535,"'","'","",1,1,3,0,0,0,"BLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"TINYBLOB",SQL_LONGVARBINARY,255,"'","'","",1,1,3,0,0,0,"TINYBLOB",0,0,0,0,10, SQL_LONGVARBINARY},
  {"VARBINARY",SQL_VARBINARY,255,"'","'","(M)",1,1,3,0,0,0,"VARBINARY",0,0,0,0,10, SQL_VARBINARY},
  {"BINARY",SQL_BINARY,255,"'","'","(M)",1,1,3,0,0,0,"BINARY",0,0,0,0,10, SQL_BINARY},
  {"LONG VARCHAR",SQL_LONGVARCHAR,16777215,"'","'","",1,0,3,0,0,0,"LONG VARCHAR",0,0,0,0,10, SQL_LONGVARCHAR},
  {"MEDIUMTEXT",SQL_LONGVARCHAR,16777215,"'","'","",1,0,3,0,0,0,"MEDIUMTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"LONGTEXT",SQL_LONGVARCHAR,2147483647,"'","'","",1,0,3,0,0,0,"LONGTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"TEXT",SQL_LONGVARCHAR,65535,"'","'","",1,0,3,0,0,0,"TEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"TINYTEXT",SQL_LONGVARCHAR,255,"'","'","",1,0,3,0,0,0,"TINYTEXT",0,0,0,0,10, SQL_LONGVARCHAR},
  {"CHAR",SQL_CHAR,255,"'","'","(M)",1,0,3,0,0,0,"CHAR",0,0,0,0,10, SQL_CHAR},
  {"NUMERIC",SQL_NUMERIC,65,"","","[(M,D])] [ZEROFILL]",1,0,3,0,0,1,"NUMERIC", -308,308,0,0,10, SQL_NUMERIC}, /* Todo: ?? */
  {"DECIMAL",SQL_DECIMAL,65,"","","[(M,D])] [ZEROFILL]",1,0,3,0,0,1,"DECIMAL",-308,308,0,0,10, SQL_DECIMAL},
  {"INTEGER",SQL_INTEGER,10,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"INTEGER",0,0,0,0,10,SQL_INTEGER},
  {"INTEGER UNSIGNED",SQL_INTEGER,10,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"INTEGER UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"INT",SQL_INTEGER,10,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"INT",0,0,0,0,10, SQL_INTEGER},
  {"INT UNSIGNED",SQL_INTEGER,10,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"INT UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"MEDIUMINT",SQL_INTEGER,7,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"MEDIUMINT",0,0,0,0,10},
  {"MEDIUMINT UNSIGNED",SQL_INTEGER,8,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"MEDIUMINT UNSIGNED",0,0,0,0,10, SQL_INTEGER},
  {"SMALLINT",SQL_SMALLINT,5,"","","[(M)] [UNSIGNED] [ZEROFILL]",1,0,3,1,0,1,"SMALLINT",0,0,0,0,10, SQL_SMALLINT},
  {"SMALLINT UNSIGNED",SQL_SMALLINT,5,"","","[(M)] [ZEROFILL]",1,0,3,1,0,1,"SMALLINT UNSIGNED",0,0,0,0,10, SQL_SMALLINT},
  {"FLOAT",SQL_FLOAT,10,"","","[(M|D)] [ZEROFILL]",1,0,3,0,0,1,"FLOAT",-38,38,0,0,10, SQL_FLOAT},
  {"DOUBLE",SQL_DOUBLE,17,"","","[(M|D)] [ZEROFILL]",1,0,3,0,0,1,"DOUBLE",-308,308,0,0,10, SQL_DOUBLE},
  {"DOUBLE PRECISION",SQL_DOUBLE,17,"","","[(M,D)] [ZEROFILL]",1,0,3,0,0,1,"DOUBLE PRECISION",-308,308,0,0,10, SQL_DOUBLE},
  {"REAL",SQL_DOUBLE,17,"","","[(M,D)] [ZEROFILL]",1,0,3,0,0,1,"REAL",-308,308,0,0,10, SQL_DOUBLE},
  {"VARCHAR",SQL_VARCHAR,255,"'","'","(M)",1,0,3,0,0,0,"VARCHAR",0,0,0,0,10, SQL_VARCHAR},
  {"ENUM",SQL_VARCHAR,65535,"'","'","",1,0,3,0,0,0,"ENUM",0,0,0,0,10, SQL_VARCHAR},
  {"SET",SQL_VARCHAR,64,"'","'","",1,0,3,0,0,0,"SET",0,0,0,0,10, SQL_VARCHAR},
  {"DATE",SQL_DATE,10,"'","'","",1,0,3,0,0,0,"DATE",0,0,0,0,10, SQL_DATETIME},
  {"TIME",SQL_TIME,18,"'","'","[(M)]",1,0,3,0,0,0,"TIME",0,0,0,0,10, SQL_DATETIME},
  {"DATETIME",SQL_TIMESTAMP,27,"'","'","[(M)]",1,0,3,0,0,0,"DATETIME",0,0,0,0,10, SQL_DATETIME},
  {"TIMESTAMP",SQL_TIMESTAMP,27,"'","'","[(M)]",1,0,3,0,0,0,"TIMESTAMP",0,0,0,0,10, SQL_DATETIME},
  {NULL,0,0,NULL,NULL,NULL,0,0,0,0,0,0,NULL,0,0,0,0,0}
};

/* {{{ MADB_GetTypeInfo */
SQLRETURN MADB_GetTypeInfo(SQLHSTMT StatementHandle,
                           SQLSMALLINT DataType)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  my_bool isFirst= TRUE;
  char StmtStr[5120];
  char *p= StmtStr;
  int i;
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
        p+= my_snprintf(p, 5120 - strlen(StmtStr),
          "SELECT \"%s\" AS TYPE_NAME, %d AS DATA_TYPE, %lu AS COLUMN_SIZE, \"%s\" AS LITERAL_PREFIX, "
          "\"%s\" AS LITERAL_SUFFIX, \"%s\" AS CREATE_PARAMS, %d AS NULLABLE, %d AS CASE_SENSITIVE, "
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
          p+= my_snprintf(p, 5120 - strlen(StmtStr),
          "UNION SELECT \"%s\", %d, %lu , \"%s\", "
          "\"%s\", \"%s\", %d, %d, "
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
  ret= Stmt->Methods->Prepare(Stmt, StmtStr, SQL_NTS);
  if (SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);
  return ret;
}
/* }}} */

SQLRETURN MADB_Info_Get(SQLHDBC ConnectionHandle,
    SQLUSMALLINT InfoType,
    SQLPOINTER InfoValuePtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr)
{
  MADB_Dbc *Connection= (MADB_Dbc *)ConnectionHandle;

  switch (InfoType) {
    /* Section 1: Driver Information */  
    case SQL_ACTIVE_ENVIRONMENTS:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, 0);
      break;
#if (ODBCVER>=0x0380)
    case SQL_ASYNC_DBC_FUNCTIONS:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_ASYNC_DBC_NOT_CAPABLE);  
      break;
    case SQL_ASYNC_MODE:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_AM_NONE);
      break;
    case SQL_ASYNC_NOTIFICATION:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_ASYNC_NOTIFICATION_NOT_CAPABLE);
      break;
#endif
    case SQL_BATCH_ROW_COUNT:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_BRC_EXPLICIT);
      break;
    case SQL_BATCH_SUPPORT:
      {  
        SQLUINTEGER BitMask= SQL_BS_SELECT_EXPLICIT | SQL_BS_ROW_COUNT_EXPLICIT |
          SQL_BS_SELECT_PROC | SQL_BS_ROW_COUNT_PROC;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }  
      break;
    case SQL_DATA_SOURCE_NAME:
      /* FIXME */
      break;
#ifdef SQL_DRIVER_AWARE_POOLING_SUPPORTED
    case SQL_DRIVER_AWARE_POOLING_SUPPORTED:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_DRIVER_AWARE_POOLING_NOT_CAPABLE);
      break;
#endif
    case SQL_DRIVER_HDBC:
      /* FIXME */
      break;
    case SQL_DRIVER_HDESC:
      /* FIXME */
      break;
    case SQL_DRIVER_HENV:
      /* FIXME */
      break;
    case SQL_DRIVER_HLIB:
      /* FIXME */
      break;
    case SQL_DRIVER_HSTMT:
      /* FIXME */
      break;
    case SQL_DRIVER_NAME:
#ifdef _WIN32    
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"maodbca.dll"); 
#endif  
      break;
    case SQL_DRIVER_ODBC_VER:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"03.00"); 
      break;
    case SQL_DRIVER_VER:
      MADB_SET_STRVAL(InfoValuePtr, BufferLength, MARIADB_ODBC_VERSION);    
      break;
    case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
      {  
        SQLUINTEGER BitMask= SQL_CA1_BULK_ADD | SQL_CA1_ABSOLUTE | SQL_CA1_LOCK_NO_CHANGE |
                             SQL_CA1_NEXT | SQL_CA1_POSITIONED_DELETE | SQL_CA1_POSITIONED_UPDATE |
                             SQL_CA1_POS_DELETE | SQL_CA1_POS_POSITION | SQL_CA1_POS_REFRESH | 
                             SQL_CA1_POS_UPDATE | SQL_CA1_RELATIVE ;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      } 
      break;
    case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
      {  
        SQLUINTEGER BitMask= SQL_CA2_CRC_EXACT | SQL_CA2_MAX_ROWS_DELETE | SQL_CA2_MAX_ROWS_INSERT |
                             SQL_CA2_MAX_ROWS_SELECT | SQL_CA2_MAX_ROWS_UPDATE | SQL_CA2_SENSITIVITY_ADDITIONS |
                             SQL_CA2_SENSITIVITY_DELETIONS | SQL_CA2_SENSITIVITY_UPDATES | SQL_CA2_SIMULATE_TRY_UNIQUE;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }
      break;
    case SQL_FILE_USAGE:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, SQL_FILE_NOT_SUPPORTED);
      break;
    case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
       {  
        SQLUINTEGER BitMask= SQL_CA1_NEXT;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      };
    case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
       {  
        SQLUINTEGER BitMask= SQL_CA2_MAX_ROWS_SELECT | SQL_CA2_MAX_ROWS_INSERT |
                             SQL_CA2_MAX_ROWS_DELETE | SQL_CA2_MAX_ROWS_UPDATE;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }
    case SQL_GETDATA_EXTENSIONS:
      {
        SQLUINTEGER BitMask= SQL_GD_ANY_COLUMN |
          SQL_GD_ANY_ORDER  |
          SQL_GD_BLOCK      |
          SQL_GD_BOUND;
        /* FIXME:            | SQL_GD_OUTPUT_PARAMS ?? */
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }
      break;
    case SQL_INFO_SCHEMA_VIEWS:
      {
        /* FIXME: Check Server Versions: pre 5.0, 5.1 - 5.5 */
        SQLUINTEGER BitMask= SQL_ISV_CHARACTER_SETS | 
          SQL_ISV_COLLATIONS |
          SQL_ISV_COLUMN_PRIVILEGES |
          SQL_ISV_COLUMNS |
          SQL_ISV_KEY_COLUMN_USAGE |
          SQL_ISV_REFERENTIAL_CONSTRAINTS |
          SQL_ISV_SCHEMATA |
          SQL_ISV_TABLE_CONSTRAINTS |
          SQL_ISV_TABLE_PRIVILEGES | 
          SQL_ISV_TABLES |
          SQL_ISV_VIEWS;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }
      break;
    case SQL_KEYSET_CURSOR_ATTRIBUTES1:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"N");
      break;
    case SQL_KEYSET_CURSOR_ATTRIBUTES2:
      /* FIXME */
      break;
    case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, 0);
      break;
    case SQL_MAX_CONCURRENT_ACTIVITIES:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, 0);
      break;
    case SQL_MAX_DRIVER_CONNECTIONS:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, 0);
      break;
    case SQL_ODBC_INTERFACE_CONFORMANCE:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_OIC_CORE);
      break;
    case SQL_STANDARD_CLI_CONFORMANCE:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_SCC_ISO92_CLI);
      break;
    case SQL_ODBC_VER:
      /* FIXME: Set to 3.8 when connection pooling and async was implemented */
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"03.50");
      break;
    case SQL_PARAM_ARRAY_ROW_COUNTS:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_PARC_NO_BATCH);
      break;
    case SQL_PARAM_ARRAY_SELECTS:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_PAS_NO_BATCH);
      break;
    case SQL_ROW_UPDATES:
      break;
    case SQL_SEARCH_PATTERN_ESCAPE:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"\\");
      break;
    case SQL_SERVER_NAME:
      MADB_SET_STRVAL(InfoValuePtr, BufferLength, Connection->mariadb->host_info);
      break;
    case SQL_STATIC_CURSOR_ATTRIBUTES1:
      /* FIXME */
      break;
    case SQL_STATIC_CURSOR_ATTRIBUTES2:
      /* FIXME */
      break;

      /* Section 2: Datasource Information */
    case SQL_ACCESSIBLE_PROCEDURES:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"Y");
      break;
    case SQL_ACCESSIBLE_TABLES:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"Y");
      break;
    case SQL_BOOKMARK_PERSISTENCE:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, 0);
      break;
    case SQL_CATALOG_TERM:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"database");
      break;
    case SQL_COLLATION_SEQ:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"UTF8");
      break;
    case SQL_CONCAT_NULL_BEHAVIOR:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, SQL_CB_NULL);
      break;
    case SQL_CURSOR_COMMIT_BEHAVIOR:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, SQL_CB_PRESERVE);
      break;
    case SQL_CURSOR_ROLLBACK_BEHAVIOR:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, SQL_CB_PRESERVE);
      break;
    case SQL_CURSOR_SENSITIVITY:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_UNSPECIFIED);
      break;
    case SQL_DATA_SOURCE_READ_ONLY:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"N");
      break;
    case SQL_DEFAULT_TXN_ISOLATION:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, SQL_TXN_REPEATABLE_READ);
      break;
    case SQL_DESCRIBE_PARAMETER:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"N");
      break;
    case SQL_MULTIPLE_ACTIVE_TXN:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"Y");
      break;
    case SQL_MULT_RESULT_SETS:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"Y");
      break;
    case SQL_NEED_LONG_DATA_LEN:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"N");
      break;
    case SQL_NULL_COLLATION:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, SQL_NC_LOW);
      break;
    case SQL_PROCEDURE_TERM:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"stored procedure");
      break;
    case SQL_SCHEMA_TERM:
      /* MariaDB uses catalog instead of schema */
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"");
      break;
    case SQL_SCROLL_OPTIONS:
      {
        SQLUINTEGER BitMask= SQL_SO_FORWARD_ONLY | SQL_SO_DYNAMIC | SQL_SO_STATIC;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }
      break;
    case SQL_TABLE_TERM:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"table");
      break;
    case SQL_TXN_CAPABLE:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, SQL_TC_DDL_COMMIT);
      break;
    case SQL_TXN_ISOLATION_OPTION:
      {
        SQLUINTEGER BitMask= SQL_TXN_READ_COMMITTED | 
          SQL_TXN_READ_UNCOMMITTED |
          SQL_TXN_REPEATABLE_READ |
          SQL_TXN_SERIALIZABLE;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }
      break;
    case SQL_USER_NAME:
      /* FIXME: Connection->mariadb->user or DSN->UserName ?? */
      break;
      /* Section 3:  Supported SQL */
    case SQL_AGGREGATE_FUNCTIONS:
      {
        SQLUINTEGER BitMask= SQL_AF_ALL | 
          SQL_AF_AVG | 
          SQL_AF_COUNT |
          SQL_AF_DISTINCT |
          SQL_AF_MAX |
          SQL_AF_MIN |
          SQL_AF_SUM;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }
      break;
    case SQL_ALTER_DOMAIN:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, 0);
      break;
      /*  case SQL_ALTER_SCHEMA:
          break; */
    case SQL_ALTER_TABLE:
      {
        SQLUINTEGER BitMask= SQL_AT_ADD_COLUMN_COLLATION |
          SQL_AT_ADD_COLUMN_DEFAULT |
          SQL_AT_ADD_COLUMN_SINGLE |
          SQL_AT_SET_COLUMN_DEFAULT;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }
      break;
    case SQL_DATETIME_LITERALS:
      {
        SQLUINTEGER BitMask= SQL_DL_SQL92_DATE |
          SQL_DL_SQL92_TIME |
          SQL_DL_SQL92_TIMESTAMP;
        MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUINTEGER, BitMask);
      }
      break;
    case SQL_CATALOG_LOCATION:
      MADB_SET_INTVAL(InfoValuePtr, BufferLength, SQLUSMALLINT, SQL_CL_START);
      break;
    case SQL_CATALOG_NAME:
      /* FIXME */
      break;
    case SQL_CATALOG_NAME_SEPARATOR:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L".");
      break;
    case SQL_CATALOG_USAGE:
      break;
    case SQL_COLUMN_ALIAS:
      MADB_SET_WSTRVAL(InfoValuePtr, BufferLength, L"Y");
      break;
    case SQL_CORRELATION_NAME:
      break;
    case SQL_CREATE_ASSERTION:
      break;
    case SQL_CREATE_CHARACTER_SET:
      break;
    case SQL_CREATE_COLLATION:
      break;
    case SQL_CREATE_DOMAIN:
      break;
    case SQL_CREATE_SCHEMA:
      break;
    case SQL_CREATE_TABLE:
      break;
    case SQL_CREATE_TRANSLATION:
      break;
    case SQL_DDL_INDEX:
      break;
    case SQL_DROP_ASSERTION:
      break;
    case SQL_DROP_CHARACTER_SET:
      break;
    case SQL_DROP_COLLATION:
      break;
    case SQL_DROP_DOMAIN:
      break;
    case SQL_DROP_SCHEMA:
      break;
    case SQL_DROP_TABLE:
      break;
    case SQL_DROP_TRANSLATION:
      break;
    case SQL_DROP_VIEW:
      break;
    case SQL_EXPRESSIONS_IN_ORDERBY:
      break;
    case SQL_GROUP_BY:
      break;
    case SQL_IDENTIFIER_CASE:
      break;
    case SQL_IDENTIFIER_QUOTE_CHAR:
      break;
    case SQL_INDEX_KEYWORDS:
      break;
    case SQL_INSERT_STATEMENT:
      break;
    case SQL_INTEGRITY:
      break;
    case SQL_KEYWORDS:
      break;
    case SQL_LIKE_ESCAPE_CLAUSE:
      break;
    case SQL_NON_NULLABLE_COLUMNS:
      break;
    case SQL_OJ_CAPABILITIES:
      break;
    case SQL_ORDER_BY_COLUMNS_IN_SELECT:
      break;
    case SQL_OUTER_JOINS:
      break;
    case SQL_PROCEDURES:
      break;
    case SQL_QUOTED_IDENTIFIER_CASE:
      break;
    case SQL_SCHEMA_USAGE:
      break;
    case SQL_SPECIAL_CHARACTERS:
      break;
    case SQL_SQL_CONFORMANCE:
      break;
    case SQL_SUBQUERIES:
      break;
    case SQL_UNION:
      break;
      /* Section 4: SQL Limits */  
    case SQL_MAX_BINARY_LITERAL_LEN:
      break;
    case SQL_MAX_CATALOG_NAME_LEN:
      break;
    case SQL_MAX_CHAR_LITERAL_LEN:
      break;
    case SQL_MAX_COLUMNS_IN_GROUP_BY:
      break;
    case SQL_MAX_COLUMNS_IN_INDEX:
      break;
    case SQL_MAX_COLUMNS_IN_ORDER_BY:
      break;
    case SQL_MAX_COLUMNS_IN_SELECT:
      break;
    case SQL_MAX_COLUMNS_IN_TABLE:
      break;
    case SQL_MAX_COLUMN_NAME_LEN:
      break;
    case SQL_MAX_CURSOR_NAME_LEN:
      break;
    case SQL_MAX_IDENTIFIER_LEN:
      break;
    case SQL_MAX_INDEX_SIZE:
      break;
    case SQL_MAX_PROCEDURE_NAME_LEN:
      break;
    case SQL_MAX_ROW_SIZE:
      break;
    case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
      break;
    case SQL_MAX_SCHEMA_NAME_LEN:
      break;
    case SQL_MAX_STATEMENT_LEN:
      break;
    case SQL_MAX_TABLES_IN_SELECT:
      break;
    case SQL_MAX_TABLE_NAME_LEN:
      break;
    case SQL_MAX_USER_NAME_LEN:
      break;
      /* Section 5: Scalar Information */
    case SQL_CONVERT_FUNCTIONS:
      break;
    case SQL_NUMERIC_FUNCTIONS:
      break;
    case SQL_STRING_FUNCTIONS:
      break;
    case SQL_SYSTEM_FUNCTIONS:
      break;
    case SQL_TIMEDATE_ADD_INTERVALS:
      break;
    case SQL_TIMEDATE_DIFF_INTERVALS:
      break;
    case SQL_TIMEDATE_FUNCTIONS:
      break;
      /* Section 6: Conversion Information */
    case SQL_CONVERT_BIGINT:
      break;
    case SQL_CONVERT_BINARY:
      break;
    case SQL_CONVERT_BIT:
      break;
    case SQL_CONVERT_CHAR:
      break;
    case SQL_CONVERT_DATE:
      break;
    case SQL_CONVERT_DECIMAL:
      break;
    case SQL_CONVERT_DOUBLE:
      break;
    case SQL_CONVERT_FLOAT:
      break;
    case SQL_CONVERT_INTEGER:
      break;
    case SQL_CONVERT_INTERVAL_DAY_TIME:
      break;
    case SQL_CONVERT_INTERVAL_YEAR_MONTH:
      break;
    case SQL_CONVERT_LONGVARBINARY:
      break;
    case SQL_CONVERT_LONGVARCHAR:
      break;
    case SQL_CONVERT_NUMERIC:
      break;
    case SQL_CONVERT_REAL:
      break;
    case SQL_CONVERT_SMALLINT:
      break;
    case SQL_CONVERT_TIME:
      break;
    case SQL_CONVERT_TIMESTAMP:
      break;
    case SQL_CONVERT_TINYINT:
      break;
    case SQL_CONVERT_VARBINARY:
      break;
    case SQL_CONVERT_VARCHAR:
      break;
    default:
      return SQL_ERROR;  
  }
  return SQL_SUCCESS;
}
