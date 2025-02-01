/************************************************************************************
   Copyright (C) 2013,2025 MariaDB Corporation plc
   
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
#ifndef _ma_connection_h_
#define _ma_connection_h_


/* sql_mode's identifiers */
enum enum_madb_sql_mode {MADB_NO_BACKSLASH_ESCAPES};

struct st_madb_isolation {
  long SqlIsolation;
  const char *StrIsolation;
  const char* TrackStr; /* String coming with session tracking */
};


struct MADB_Dbc
{
  MADB_Error Error;
  std::mutex ListsCs;      /*       for operations with lists */
  MADB_Env::ListIterator ListItem;
  Client_Charset Charset= {0,nullptr};
  Unique::Protocol guard;
  MYSQL*    mariadb=     nullptr;         /* handle to a mariadb connection */
  MADB_Env* Environment= nullptr;         /* global environment */
  MADB_Dsn* Dsn=         nullptr;

  Client_Charset* ConnOrSrcCharset= nullptr; /* "Source" here stands for which charset Windows DM was using as source, when converted to unicode.
                                  We have to use same charset to recode from unicode to get same string as application sent it.
                                  For Unicode application that is the same as "Charset", or in case of ANSI on Windows - defaulst system
                                  codepage */
  MADB_List* Stmts=  nullptr;
  MADB_List* Descrs= nullptr;
  /* Attributes */
  char*      CatalogName= nullptr; /* Schema name set via SQLSetConnectAttr - it can be set before connection, thus we need it to have here */
  HWND       QuietMode=   nullptr;

  SQLPOINTER EnlistInDtc= nullptr;
  SQLULEN    AsyncEnable= 0;
  SQLULEN    OdbcCursors= 0;
  unsigned long Options=  0;
  SQLUINTEGER AutoIpd=    0;
  SQLUINTEGER AutoCommit= 4;
  SQLUINTEGER ConnectionDead= 0;
  SQLUINTEGER ReadTimeout=  0;
  SQLUINTEGER WriteTimeout= 0;
  SQLUINTEGER PacketSize= 0;
  SQLINTEGER  AccessMode= 0;
  SQLINTEGER  IsolationLevel= 0;     /* tx_isolation */
  SQLUINTEGER Trace=        0;
  SQLUINTEGER MetadataId=   0;
  SQLINTEGER  TxnIsolation= 0; /* Sames as catalog name - we need it here */
  SQLINTEGER  CursorCount=  0;
  uint32_t    LoginTimeout= 0; /* The attribute is SQLUINTEGER, that is unsigned long, that technically can be 8bytes
                                (not sure how does other DM define it) But C/C option is unsigned int */
  char ServerCapabilities= '\0';
  char lcTableNamesMode2= '\xff'; /* -1 means we don't know if lower_case_table_names=2, ie that info has never been requested  yet */

  bool IsAnsi=  false;
  bool IsMySQL= false;
  bool ExecDirectOnServer=  false;
  bool PrepareOnClient=     false;

  MADB_Dbc(MADB_Env* Env);
  SQLRETURN EndTran(SQLSMALLINT CompletionType);
  SQLRETURN SetAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength, bool isWChar);
  SQLRETURN GetAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength, SQLINTEGER *StringLengthPtr, bool isWChar);
  SQLRETURN CoreConnect(MYSQL* mariadb, MADB_Dsn *Dsn, MADB_Error* _Error, unsigned long clientFlags= 0);
  SQLRETURN ConnectDB(MADB_Dsn *Dsn);
  SQLRETURN GetFunctions(SQLUSMALLINT FunctionId, SQLUSMALLINT *SupportedPtr);
  SQLRETURN GetInfo(SQLUSMALLINT InfoType, SQLPOINTER InfoValuePtr, SQLSMALLINT BufferLength, SQLSMALLINT *StringLengthPtr, bool isWChar);
  SQLRETURN DriverConnect(SQLHWND WindowHandle, SQLCHAR *InConnectionString, SQLULEN StringLength1, SQLCHAR *OutConnectionString,
                          SQLULEN BufferLength, SQLSMALLINT *StringLength2Ptr, SQLUSMALLINT DriverCompletion);
  
  bool CheckConnection();
private:
  SQLRETURN GetCurrentDB(SQLPOINTER CurrentDB, SQLINTEGER CurrentDBLength, SQLSMALLINT* StringLengthPtr, bool isWChar);
  const char* getDefaultSchema(MADB_Dsn* Dsn);
  void setDefaultAttributeValues(MADB_Dsn* Dsn);
};

SQLRETURN MADB_SQLDisconnect(SQLHDBC ConnectionHandle);
SQLRETURN MADB_DbcFree(MADB_Dbc *Connection);
MADB_Dbc * MADB_DbcInit(MADB_Env *Env);
bool MADB_SqlMode(MADB_Dbc *Connection, enum enum_madb_sql_mode SqlMode);
/* Has platform versions */
char* MADB_GetDefaultPluginsDir(char* Buffer, size_t Size);


#define MADB_SUPPORTED_CONVERSIONS  SQL_CVT_BIGINT | SQL_CVT_BIT | SQL_CVT_CHAR | SQL_CVT_DATE |\
                                    SQL_CVT_DECIMAL | SQL_CVT_DOUBLE | SQL_CVT_FLOAT |\
                                    SQL_CVT_INTEGER | SQL_CVT_LONGVARCHAR | SQL_CVT_NUMERIC |\
                                    SQL_CVT_REAL | SQL_CVT_SMALLINT | SQL_CVT_TIME | SQL_CVT_TIMESTAMP |\
                                    SQL_CVT_TINYINT | SQL_CVT_VARCHAR | SQL_CVT_WCHAR | \
                                    SQL_CVT_WLONGVARCHAR | SQL_CVT_WVARCHAR

#endif /* _ma_connection_h */
