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
#ifndef _ma_connection_h_
#define _ma_connection_h_

#define MADB_CONN_OPT_NOT_SUPPORTED 0
#define MADB_CONN_OPT_BEFORE        1
#define MADB_CONN_OPT_AFTER         2
#define MADB_CONN_OPT_BOTH          3

struct st_ma_connection_methods;

struct st_madb_isolation {
  long SqlIsolation;
  char *StrIsolation;
};

struct st_ma_connection_methods
{
  SQLRETURN (*SetAttr)(MADB_Dbc *Dbc, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength, my_bool isWChar);
  SQLRETURN (*GetAttr)(MADB_Dbc *Dbc, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength, SQLINTEGER *StringLengthPtr, my_bool isWChar);
  SQLRETURN (*ConnectDB)(MADB_Dbc *Connection, MADB_Dsn *Dsn);
  SQLRETURN (*EndTran)(MADB_Dbc *Dbc, SQLSMALLINT CompletionType);
  SQLRETURN (*GetFunctions)(MADB_Dbc *Dbc, SQLUSMALLINT FunctionId, SQLUSMALLINT *SupportedPtr);
  SQLRETURN(*GetInfo)(MADB_Dbc *Dnc, SQLUSMALLINT InfoType, SQLPOINTER InfoValuePtr,
                      SQLSMALLINT BufferLength, SQLSMALLINT *StringLengthPtr, my_bool isWChar);
  SQLRETURN (*DriverConnect)(MADB_Dbc *Dbc, SQLHWND WindowHandle, SQLCHAR *InConnectionString,
                             SQLSMALLINT StringLength1, SQLCHAR *OutConnectionString,
                             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength2Ptr,
                             SQLUSMALLINT DriverCompletion);
};

#ifdef WIN32
/* Show configuration dialog for SQLDriverConnect */
typedef BOOL (__stdcall *PromptDSN)(HWND hwnd, MADB_Dsn *Dsn);
#endif

my_bool CheckConnection(MADB_Dbc *Dbc);

SQLRETURN MADB_DbcFree(MADB_Dbc *Connection);
MADB_Dbc * MADB_DbcInit(SQLHANDLE Env);
SQLRETURN MADB_Dbc_GetCurrentDB(MADB_Dbc *Connection, SQLPOINTER CurrentDB, SQLINTEGER CurrentDBLength, 
                                SQLSMALLINT *StringLengthPtr, my_bool isWChar);

#define MADB_SUPPORTED_CONVERSIONS  SQL_CVT_BIGINT | SQL_CVT_BIT | SQL_CVT_CHAR | SQL_CVT_DATE |\
                                    SQL_CVT_DECIMAL | SQL_CVT_DOUBLE | SQL_CVT_FLOAT |\
                                    SQL_CVT_INTEGER | SQL_CVT_LONGVARCHAR | SQL_CVT_NUMERIC |\
                                    SQL_CVT_REAL | SQL_CVT_SMALLINT | SQL_CVT_TIME | SQL_CVT_TIMESTAMP |\
                                    SQL_CVT_TINYINT | SQL_CVT_VARCHAR | SQL_CVT_WCHAR | \
                                    SQL_CVT_WLONGVARCHAR | SQL_CVT_WVARCHAR
/**************** Helper macros ****************/
/* check if the connection is established */
#define MADB_Dbc_ACTIVE(a) \
  ((a)->mariadb && (a)->mariadb->net.vio)

#define MADB_Dbc_DSN(a) \
(a) && (a)->Dsn  

#define MADB_Dbc_AUTOCOMMIT(a) \
  ((a)->mariadb && ((a)->mariadb->server_status && SERVER_STATUS_AUTOCOMMIT))

#define MADB_Dbc_TRANSACTION(a) \
  ((a)->mariadb && ((a)->mariadb->server_capabilities && CLIENT_TRANSACTIONS))

#endif /* _ma_connection_h */
