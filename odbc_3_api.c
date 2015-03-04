/************************************************************************************
   Copyright (C) 2013,2105 MariaDB Corporation AB
   
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
#undef MA_ODBC_DEBUG_ALL

#include <ma_odbc.h>

/* {{{ SQLAllocHandle */
SQLRETURN SQL_API SQLAllocHandle(SQLSMALLINT HandleType,
    SQLHANDLE InputHandle,
    SQLHANDLE *OutputHandlePtr)
{
  SQLRETURN ret= SQL_ERROR;

  MDBUG_ENTER("SQLAllocHandle");
  MDBUG_DUMP(HandleType, d);
  MDBUG_DUMP(InputHandle, 0x);
  MDBUG_DUMP(OutputHandlePtr, 0x);
  
  switch(HandleType) {
    case SQL_HANDLE_DBC:
      EnterCriticalSection(&((MADB_Env *)InputHandle)->cs);
      if (*OutputHandlePtr = (SQLHANDLE)MADB_DbcInit(InputHandle))
        ret= SQL_SUCCESS;
      LeaveCriticalSection(&((MADB_Env *)InputHandle)->cs);
      break;
    case SQL_HANDLE_DESC:
      EnterCriticalSection(&((MADB_Dbc *)InputHandle)->cs);
      if (*OutputHandlePtr = (SQLHANDLE)MADB_DescInit((MADB_Dbc *)InputHandle, MADB_DESC_UNKNOWN, TRUE))
        ret= SQL_SUCCESS;
      LeaveCriticalSection(&((MADB_Dbc *)InputHandle)->cs);
      break;
    case SQL_HANDLE_ENV:
      if ((*OutputHandlePtr = (SQLHANDLE)MADB_EnvInit()))
        ret= SQL_SUCCESS;
      break;
    case SQL_HANDLE_STMT:
      {
        MADB_Dbc *Connection= (MADB_Dbc *)InputHandle;
       
        if (!CheckConnection(Connection))
        {
          MADB_SetError(&Connection->Error, MADB_ERR_08003, NULL, 0);
          break;
        }

        ret= MADB_StmtInit(Connection, OutputHandlePtr);
      }
      break;
    default:
      /* todo: set error message */
      break;
  }
  MDBUG_DUMP(ret,d);
  MDBUG_RETURN(ret);
}
/* }}} */

/* {{{ SQLAllocConnect */
SQLRETURN SQL_API SQLAllocConnect(SQLHANDLE InputHandle,
                                  SQLHANDLE *OutputHandlePtr)
{
  return SQLAllocHandle(SQL_HANDLE_DBC, InputHandle, OutputHandlePtr);
}
/* }}} */

/* {{{ SQLAllocStmt */
SQLRETURN SQL_API SQLAllocStmt(SQLHANDLE InputHandle,
                               SQLHANDLE *OutputHandlePtr)
{
  return SQLAllocHandle(SQL_HANDLE_STMT, InputHandle, OutputHandlePtr);
}
/* }}} */

/* {{{ SQLAllocEnv */
SQLRETURN SQL_API SQLAllocEnv(SQLHANDLE *OutputHandlePtr)
{
  return SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, OutputHandlePtr);
}
/* }}} */

/* {{{ SQLBindCol */
SQLRETURN SQL_API SQLBindCol(SQLHSTMT StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLSMALLINT TargetType,
    SQLPOINTER TargetValuePtr,
    SQLLEN BufferLength,
    SQLLEN *StrLen_or_Ind)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  MADB_CHECK_STMT_HANDLE(Stmt,stmt);  

  MDBUG_C_ENTER(Stmt->Connection, "SQLBindCol");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  MDBUG_C_DUMP(Stmt->Connection, TargetType, d);
  MDBUG_C_DUMP(Stmt->Connection, BufferLength, d);
  MDBUG_C_DUMP(Stmt->Connection, StrLen_or_Ind, 0x);

  ret= Stmt->Methods->BindColumn(Stmt, ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_Ind);

  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLBindParameter */
SQLRETURN SQL_API SQLBindParameter(SQLHSTMT StatementHandle,
    SQLUSMALLINT ParameterNumber,
    SQLSMALLINT InputOutputType,
    SQLSMALLINT ValueType,
    SQLSMALLINT ParameterType,
    SQLULEN ColumnSize,
    SQLSMALLINT DecimalDigits,
    SQLPOINTER ParameterValuePtr,
    SQLLEN BufferLength,
    SQLLEN *StrLen_or_IndPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLBindParameter");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ParameterNumber, u);
  MDBUG_C_DUMP(Stmt->Connection, InputOutputType, d);
  MDBUG_C_DUMP(Stmt->Connection, ValueType, d);
  MDBUG_C_DUMP(Stmt->Connection, ParameterType, d);
  MDBUG_C_DUMP(Stmt->Connection, ColumnSize, u);
  MDBUG_C_DUMP(Stmt->Connection, DecimalDigits, d);
  MDBUG_C_DUMP(Stmt->Connection, ParameterValuePtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, BufferLength, d);
  MDBUG_C_DUMP(Stmt->Connection, StrLen_or_IndPtr, 0x);
      
  MADB_CHECK_STMT_HANDLE(Stmt,stmt);
  ret= Stmt->Methods->BindParam(Stmt, ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize, DecimalDigits,
                                  ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */
/* {{{ SQLBrowseConnect */
SQLRETURN SQL_API SQLBrowseConnect(SQLHDBC ConnectionHandle,
    SQLCHAR *InConnectionString,
    SQLSMALLINT StringLength1,
    SQLCHAR *OutConnectionString,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLength2Ptr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;
  MDBUG_C_ENTER(Dbc, "SQLBrowseConnect");
  MADB_SetError(&Dbc->Error, MADB_ERR_IM001, NULL, 0);
  ret= Dbc->Error.ReturnValue;
  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */

/* {{{ SQLBrowseConnectW */
SQLRETURN SQL_API SQLBrowseConnectW(SQLHDBC ConnectionHandle,
    SQLWCHAR *InConnectionString,
    SQLSMALLINT StringLength1,
    SQLWCHAR *OutConnectionString,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLength2Ptr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;
  MDBUG_C_ENTER(Dbc, SQLBrowseConnectW);
  MADB_SetError(&Dbc->Error, MADB_ERR_IM001, NULL, 0);
  ret= Dbc->Error.ReturnValue;
  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */

/* {{{ SQLBulkOperations */
SQLRETURN SQL_API SQLBulkOperations(SQLHSTMT StatementHandle,
    SQLSMALLINT Operation)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  Stmt->FetchType= MADB_FETCH_TYPE_BULK;
  MDBUG_C_ENTER(Stmt->Connection, "SQLBulkOperations");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, Operation, d);
  ret= Stmt->Methods->BulkOperations(Stmt, Operation);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLCancel */
SQLRETURN SQL_API SQLCancel(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret= SQL_ERROR;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLCancel");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);

  if (TryEnterCriticalSection(&Stmt->Connection->cs))
  {
    LeaveCriticalSection(&Stmt->Connection->cs);
    ret= Stmt->Methods->StmtFree(Stmt, SQL_CLOSE);
    MDBUG_C_DUMP(Stmt->Connection, ret, d);
    MDBUG_C_RETURN(Stmt->Connection, ret);
  } else
  {
    MYSQL *MariaDb, *Kill=Stmt->Connection->mariadb;
    
    char StmtStr[30];

    if (!(MariaDb= mysql_init(NULL)))
      return SQL_ERROR;
    if (!(mysql_real_connect(MariaDb, Kill->host, Kill->user, Kill->passwd,
                             "", Kill->port, Kill->unix_socket, 0)))
    {
      mysql_close(MariaDb);
      goto end;
    }
    
    my_snprintf(StmtStr, 30, "KILL QUERY %ld", mysql_thread_id(Kill));
    if (mysql_query(MariaDb, StmtStr))
    {
      mysql_close(MariaDb);
      goto end;
    }
    mysql_close(MariaDb);
    ret= SQL_SUCCESS;
  }
end:
  LeaveCriticalSection(&Stmt->Connection->cs);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLCancelHandle */
/* ODBC version 3.8 */
SQLRETURN SQL_API SQLCancelHandle(SQLSMALLINT HandleType,
    SQLHANDLE Handle)
{
  if (Handle)
    return SQL_INVALID_HANDLE;

  switch(HandleType) {
  case SQL_HANDLE_DBC:
    {
      MADB_Stmt Stmt;
      Stmt.Connection= (MADB_Dbc *)HandleType;
      return SQLCancel((SQLHSTMT)&Stmt);
    }
    break;
  case SQL_HANDLE_STMT:
    return SQLCancel((SQLHSTMT)Handle);
    break;
  }
  return SQL_INVALID_HANDLE;
}
/* }}} */

/* {{{ SQLCloseCursor */
SQLRETURN SQL_API SQLCloseCursor(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLCloseCursor");
  MDBUG_C_DUMP(Stmt->Connection, StatementHandle, 0x);

  if (!Stmt->stmt || 
     (!mysql_stmt_field_count(Stmt->stmt) && 
       Stmt->Connection->Environment->OdbcVersion >= SQL_OV_ODBC3))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_24000, NULL, 0);
    ret= Stmt->Error.ReturnValue;
  }
  else
    ret= SQLFreeStmt(StatementHandle, SQL_CLOSE);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLColAttribute */
SQLRETURN SQL_API SQLColAttribute (SQLHSTMT StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier,
    SQLPOINTER CharacterAttributePtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLLEN *NumericAttributePtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLColAttribute");
  MDBUG_C_DUMP(Stmt->Connection, StatementHandle, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  MDBUG_C_DUMP(Stmt->Connection, FieldIdentifier, u);
  MDBUG_C_DUMP(Stmt->Connection, CharacterAttributePtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, BufferLength, d);
  MDBUG_C_DUMP(Stmt->Connection, StringLengthPtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, NumericAttributePtr, 0x);

  ret= Stmt->Methods->ColAttribute(Stmt, ColumnNumber, FieldIdentifier, CharacterAttributePtr,
                                     BufferLength, StringLengthPtr, NumericAttributePtr, FALSE);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLColAttributeW */
SQLRETURN SQL_API SQLColAttributeW (SQLHSTMT StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier,
    SQLPOINTER CharacterAttributePtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLLEN *NumericAttributePtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Stmt->Connection, "SQLColAttributeW");
  MDBUG_C_DUMP(Stmt->Connection, StatementHandle, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  MDBUG_C_DUMP(Stmt->Connection, FieldIdentifier, u);
  MDBUG_C_DUMP(Stmt->Connection, CharacterAttributePtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, BufferLength, d);
  MDBUG_C_DUMP(Stmt->Connection, StringLengthPtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, NumericAttributePtr, 0x);

  
  ret= Stmt->Methods->ColAttribute(Stmt, ColumnNumber, FieldIdentifier, CharacterAttributePtr,
                                     BufferLength, StringLengthPtr, NumericAttributePtr, TRUE);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

SQLUSMALLINT MapColAttributeDescType(SQLUSMALLINT FieldIdentifier)
{
  /* we need to map the old field identifiers, see bug ODBC-8 */
  switch (FieldIdentifier)
  {
  case SQL_COLUMN_SCALE:
    return SQL_DESC_SCALE;
  case SQL_COLUMN_PRECISION:
    return SQL_DESC_PRECISION;
  case SQL_COLUMN_NULLABLE:
    return SQL_DESC_NULLABLE;
  case SQL_COLUMN_LENGTH:
    return SQL_DESC_OCTET_LENGTH;
  case SQL_COLUMN_NAME:
    return SQL_DESC_NAME;
  default:
    return FieldIdentifier;
  }
}

SQLRETURN SQL_API SQLColAttributes(SQLHSTMT hstmt, 
	SQLUSMALLINT icol,
	SQLUSMALLINT fDescType,
	SQLPOINTER rgbDesc,
	SQLSMALLINT cbDescMax,
	SQLSMALLINT * pcbDesc,
	SQLLEN * pfDesc)
{
  return SQLColAttribute(hstmt, icol, MapColAttributeDescType(fDescType), rgbDesc, cbDescMax, pcbDesc, pfDesc);
}

SQLRETURN SQL_API SQLColAttributesW(SQLHSTMT hstmt, 
	SQLUSMALLINT icol,
	SQLUSMALLINT fDescType,
	SQLPOINTER rgbDesc,
	SQLSMALLINT cbDescMax,
	SQLSMALLINT * pcbDesc,
	SQLLEN * pfDesc)
{
  return SQLColAttributeW(hstmt, icol, MapColAttributeDescType(fDescType), rgbDesc, cbDescMax, pcbDesc, pfDesc);
}


/* {{{ SQLColumnPrivileges */
SQLRETURN SQL_API SQLColumnPrivileges(SQLHSTMT StatementHandle,
    SQLCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLCHAR *ColumnName,
    SQLSMALLINT NameLength4)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Stmt->Connection, "SQLColumnPrivileges");
  ret= Stmt->Methods->ColumnPrivileges(Stmt, (char *)CatalogName, NameLength1, (char *)SchemaName, NameLength2,
                                         (char *)TableName, NameLength3, (char *)ColumnName, NameLength4);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
 }
/* }}} */

/* {{{ SQLColumnPrivilegesW */
SQLRETURN SQL_API SQLColumnPrivilegesW(SQLHSTMT StatementHandle,
    SQLWCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLWCHAR *ColumnName,
    SQLSMALLINT NameLength4)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLINTEGER CpLength1, CpLength2, CpLength3, CpLength4;
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpTable= NULL,
       *CpColumn= NULL;
  SQLRETURN ret;

  if (!StatementHandle)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumnPrivilegesW");

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->CodePage, NULL);
  CpColumn= MADB_ConvertFromWChar(ColumnName, NameLength4, &CpLength4, Stmt->Connection->CodePage, NULL);

  ret= Stmt->Methods->ColumnPrivileges(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
                                       CpTable, (SQLSMALLINT)CpLength3, CpColumn, (SQLSMALLINT)CpLength4);
    
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  MADB_FREE(CpColumn);

  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */


/* {{{ SQLColumns */
SQLRETURN SQL_API SQLColumns(SQLHSTMT StatementHandle,
    SQLCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLCHAR *ColumnName,
    SQLSMALLINT NameLength4)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Stmt->Connection, "SQLColumns");

  ret= Stmt->Methods->Columns(Stmt, (char *)CatalogName,NameLength1, (char *)SchemaName, NameLength2,
                                (char *)TableName, NameLength3, (char *)ColumnName, NameLength4);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLColumnsW */
SQLRETURN SQL_API SQLColumnsW(SQLHSTMT StatementHandle,
    SQLWCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLWCHAR *ColumnName,
    SQLSMALLINT NameLength4)
{
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpTable= NULL,
       *CpColumn= NULL;
  SQLINTEGER CpLength1, CpLength2, CpLength3, CpLength4;
  SQLRETURN ret;
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumns");

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->CodePage,NULL);
  CpColumn= MADB_ConvertFromWChar(ColumnName, NameLength4, &CpLength4, Stmt->Connection->CodePage, NULL);

  ret= Stmt->Methods->Columns(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
                              CpTable, (SQLSMALLINT)CpLength3, CpColumn, (SQLSMALLINT)CpLength4);
    
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  MADB_FREE(CpColumn);

  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */


/* {{{ SQLCompleteAsync */
/* ODBC 3.8 */
SQLRETURN SQL_API SQLCompleteAsync(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    RETCODE *AsyncRetCodePtr)
{
  SQLRETURN ret= SQL_ERROR;
  return ret;
}

/* }}} */

/* {{{ SQLConnect */
SQLRETURN SQLConnectCommon(SQLHDBC ConnectionHandle,
    SQLCHAR *ServerName,
    SQLSMALLINT NameLength1,
    SQLCHAR *UserName,
    SQLSMALLINT NameLength2,
    SQLCHAR *Authentication,
    SQLSMALLINT NameLength3)
{
  MADB_Dbc *Connection= (MADB_Dbc *)ConnectionHandle;
  MADB_Dsn *Dsn;
  SQLRETURN ret;
  my_bool DsnFound;

  if (!Connection)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Connection, "SQLConnect");
  MDBUG_C_DUMP(Connection, Connection, 0x);
  MDBUG_C_DUMP(Connection, ServerName, s);
  MDBUG_C_DUMP(Connection, NameLength1, d);
  MDBUG_C_DUMP(Connection,  UserName, s);
  MDBUG_C_DUMP(Connection, NameLength2, d);
  MDBUG_C_DUMP(Connection, Authentication, s);
  MDBUG_C_DUMP(Connection, NameLength3, d);

  
  if (CheckConnection(Connection))
  {
    MADB_SetError(&Connection->Error, MADB_ERR_08002, NULL, 0);
    return SQL_ERROR;
  }

  MADB_CLEAR_ERROR(&Connection->Error);

  if (!(Dsn= MADB_DSN_Init()))
  {
    MADB_SetError(&Connection->Error, MADB_ERR_HY001, NULL, 0);
    return SQL_ERROR; 
  }

  if (ServerName && !ServerName[0])
  {
    MADB_SetError(&Connection->Error, MADB_ERR_HY000, "Invalid DSN", 0);
    return Connection->Error.ReturnValue;
  }

  MADB_DSN_SET_STR(Dsn, DSNName, (char *)ServerName, NameLength1);
  DsnFound= MADB_ReadDSN(Dsn, NULL, TRUE);

  MADB_DSN_SET_STR(Dsn, UserName, (char *)UserName, NameLength2);
  MADB_DSN_SET_STR(Dsn, Password, (char *)Authentication, NameLength3);

  ret= Connection->Methods->ConnectDB(Connection, Dsn);

  if (SQL_SUCCEEDED(ret))
  {
    Connection->Dsn= Dsn;
  }
  MDBUG_C_DUMP(Connection, ret, d);
  MDBUG_C_RETURN(Connection, ret);
}
/* }}} */

SQLRETURN SQL_API SQLConnect(SQLHDBC ConnectionHandle,
    SQLCHAR *ServerName,
    SQLSMALLINT NameLength1,
    SQLCHAR *UserName,
    SQLSMALLINT NameLength2,
    SQLCHAR *Authentication,
    SQLSMALLINT NameLength3)
{
  return SQLConnectCommon(ConnectionHandle, ServerName, NameLength1,
	                      UserName, NameLength2, Authentication, NameLength3);
}


/* {{{ SQLConnectW */
SQLRETURN SQL_API SQLConnectW(SQLHDBC ConnectionHandle,
    SQLWCHAR *ServerName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *UserName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *Authentication,
    SQLSMALLINT NameLength3)
{
  char *MBServerName= NULL, *MBUserName= NULL, *MBAuthentication= NULL;
  SQLRETURN ret;
  
  if (!ConnectionHandle)
    return SQL_INVALID_HANDLE;

   /* Convert parameters to Cp */
  if (ServerName)
    MBServerName= MADB_ConvertFromWChar(ServerName, NameLength1, 0, CP_UTF8, NULL);
  if (UserName)
    MBUserName= MADB_ConvertFromWChar(UserName, NameLength2, 0, CP_UTF8, NULL);
  if (Authentication)
    MBAuthentication= MADB_ConvertFromWChar(Authentication, NameLength3, 0, CP_UTF8, NULL);
  
  ret= SQLConnectCommon(ConnectionHandle, (SQLCHAR *)MBServerName, SQL_NTS, (SQLCHAR *)MBUserName, SQL_NTS, 
                   (SQLCHAR *)MBAuthentication, SQL_NTS);
  MADB_FREE(MBServerName);
  MADB_FREE(MBUserName);
  MADB_FREE(MBAuthentication);
  return ret;
}
/* }}} */



/* {{{ SQLCopyDesc */
SQLRETURN SQL_API SQLCopyDesc(SQLHDESC SourceDescHandle,
    SQLHDESC TargetDescHandle)
{
  return MADB_DescCopyDesc((MADB_Desc *)SourceDescHandle, (MADB_Desc *)TargetDescHandle);
}
/* }}} */
/* {{{ SQLDataSources */
SQLRETURN SQL_API SQLDataSources(SQLHENV EnvironmentHandle,
    SQLUSMALLINT Direction,
    SQLCHAR *ServerName,
    SQLSMALLINT BufferLength1,
    SQLSMALLINT *NameLength1Ptr,
    SQLCHAR *Description,
    SQLSMALLINT BufferLength2,
    SQLSMALLINT *NameLength2Ptr)
{
  SQLRETURN ret= SQL_ERROR;

  return ret;
}
/* }}} */

/* {{{ SQLDataSourcesW */
SQLRETURN SQL_API SQLDataSourcesW(SQLHENV EnvironmentHandle,
    SQLUSMALLINT Direction,
    SQLWCHAR *ServerName,
    SQLSMALLINT BufferLength1,
    SQLSMALLINT *NameLength1Ptr,
    SQLWCHAR *Description,
    SQLSMALLINT BufferLength2,
    SQLSMALLINT *NameLength2Ptr)
{
  SQLRETURN ret= SQL_ERROR;

  return ret;
}
/* }}} */


/* {{{ SQLDescribeCol */
SQLRETURN SQL_API SQLDescribeCol(SQLHSTMT StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLCHAR *ColumnName,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *NameLengthPtr,
    SQLSMALLINT *DataTypePtr,
    SQLULEN *ColumnSizePtr,
    SQLSMALLINT *DecimalDigitsPtr,
    SQLSMALLINT *NullablePtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLDescribeCol");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  
  ret= Stmt->Methods->DescribeCol(Stmt, ColumnNumber, (void *)ColumnName, BufferLength,
                                    NameLengthPtr, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, 
                                    NullablePtr, FALSE);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLDescribeColW */
SQLRETURN SQL_API SQLDescribeColW(SQLHSTMT StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLWCHAR *ColumnName,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *NameLengthPtr,
    SQLSMALLINT *DataTypePtr,
    SQLULEN *ColumnSizePtr,
    SQLSMALLINT *DecimalDigitsPtr,
    SQLSMALLINT *NullablePtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLDescribeColW");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  
  ret= Stmt->Methods->DescribeCol(Stmt, ColumnNumber, (void *)ColumnName, BufferLength,
                                    NameLengthPtr, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, 
                                    NullablePtr, TRUE);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */


/* {{{ SQLDescribeParam */
SQLRETURN SQL_API SQLDescribeParam(SQLHSTMT StatementHandle,
    SQLUSMALLINT ParameterNumber,
    SQLSMALLINT *DataTypePtr,
    SQLULEN *ParameterSizePtr,
    SQLSMALLINT *DecimalDigitsPtr,
    SQLSMALLINT *NullablePtr)
{
  /* MariaDB doesn't support metadata for parameters,
     so we return default values */
  if (DataTypePtr)
    *DataTypePtr= SQL_VARCHAR;
  if (ParameterSizePtr)
    *ParameterSizePtr= 1024 * 1024 * 24;
  if (NullablePtr)
    *NullablePtr= SQL_NULLABLE_UNKNOWN;
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ SQLDisconnect */
SQLRETURN SQL_API SQLDisconnect(SQLHDBC ConnectionHandle)
{
  SQLRETURN ret= SQL_ERROR;
  MADB_Dbc *Connection = (MADB_Dbc *)ConnectionHandle;
  LIST *Element, *NextElement;

  if (!Connection)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Connection, "SQLDisconnect");
  MDBUG_C_DUMP(Connection, ConnectionHandle, 0x);

  /* Close all statements */
  for (Element= Connection->Stmts; Element; Element= NextElement)
  {
    NextElement= Element->next;
    SQLFreeStmt((SQLHSTMT)Element->data, SQL_DROP);
  }

  /* Close all explicitly aloocated descriptors */
  for (Element= Connection->Descrs; Element; Element= NextElement)
  {
    NextElement= Element->next;
    SQLFreeHandle(SQL_HANDLE_DESC, (SQLHANDLE)Element->data);
  }

  if (Connection->mariadb)
  {
    mysql_close(Connection->mariadb);
    Connection->mariadb= NULL;
    ret= SQL_SUCCESS;
  }  else
  {
    MADB_SetError(&Connection->Error, MADB_ERR_08003, NULL, 0);
    ret= Connection->Error.ReturnValue;
  }
  MDBUG_C_DUMP(Connection, ret, d);
  MDBUG_C_RETURN(Connection, ret);
}
/* }}} */

/* {{{ SQLDriverConnect */
SQLRETURN SQL_API SQLDriverConnect(SQLHDBC ConnectionHandle,
    SQLHWND WindowHandle,
    SQLCHAR *InConnectionString,
    SQLSMALLINT StringLength1,
    SQLCHAR *OutConnectionString,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLength2Ptr,
    SQLUSMALLINT DriverCompletion)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;
  if (!Dbc)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Dbc, "SQLDriverConnect");
  MDBUG_C_DUMP(Dbc, Dbc, 0x);
  MDBUG_C_DUMP(Dbc, InConnectionString, s);
  MDBUG_C_DUMP(Dbc, StringLength1, d);
  MDBUG_C_DUMP(Dbc, OutConnectionString, 0x);
  MDBUG_C_DUMP(Dbc, BufferLength, d);
  MDBUG_C_DUMP(Dbc, StringLength2Ptr, 0x);
  MDBUG_C_DUMP(Dbc, DriverCompletion, d);
  ret= Dbc->Methods->DriverConnect(Dbc, WindowHandle, InConnectionString, StringLength1, OutConnectionString,
                                     BufferLength, StringLength2Ptr, DriverCompletion);
  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */

/* {{{ SQLDriverConnectW */
SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC ConnectionHandle,
    SQLHWND WindowHandle,
    SQLWCHAR *InConnectionString,
    SQLSMALLINT StringLength1,
    SQLWCHAR *OutConnectionString,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLength2Ptr,
    SQLUSMALLINT DriverCompletion)
{
  SQLRETURN   ret=          SQL_ERROR;
  SQLSMALLINT Length=       0;
  char        *InConnStrA=  NULL;
  SQLINTEGER  StrLength=    0;
  char        *OutConnStrA= NULL;
  MADB_Dbc    *Dbc=         (MADB_Dbc *)ConnectionHandle;
   
  if (!ConnectionHandle)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Dbc, "SQLDriverConnectW");

  MADB_CLEAR_ERROR(&Dbc->Error);

  InConnStrA= MADB_ConvertFromWChar(InConnectionString, StringLength1, &StrLength, CP_UTF8, NULL);
  MDBUG_C_DUMP(Dbc, Dbc, 0x);
  MDBUG_C_DUMP(Dbc, InConnStrA, s);
  MDBUG_C_DUMP(Dbc, StringLength1, d);
  MDBUG_C_DUMP(Dbc, OutConnectionString, 0x);
  MDBUG_C_DUMP(Dbc, BufferLength, d);
  MDBUG_C_DUMP(Dbc, StringLength2Ptr, 0x);
  MDBUG_C_DUMP(Dbc, DriverCompletion, d);

  /* Allocate buffer for Asc OutConnectionString */
  if (OutConnectionString && BufferLength)
  {
    Length= BufferLength;
    OutConnStrA= (char *)MADB_CALLOC(Length);
  }

  ret= Dbc->Methods->DriverConnect(Dbc, WindowHandle, (SQLCHAR *)InConnStrA, (SQLSMALLINT)StrLength, (SQLCHAR *)OutConnStrA,
                                     Length, StringLength2Ptr, DriverCompletion); 
  MDBUG_C_DUMP(Dbc, ret, d);
  if (!SQL_SUCCEEDED(ret))
    goto end;

  if (DriverCompletion == SQL_DRIVER_NOPROMPT &&
      OutConnectionString && BufferLength)
  {
    Length= (SQLSMALLINT)MADB_SetString(CP_UTF8, OutConnectionString, BufferLength,
                                      InConnStrA, StrLength, &((MADB_Dbc *)ConnectionHandle)->Error);
    if (BufferLength < Length)
      MADB_SetError(&Dbc->Error, MADB_ERR_01004, NULL, 0);
  }
  if (DriverCompletion == SQL_DRIVER_COMPLETE ||
      DriverCompletion == SQL_DRIVER_COMPLETE_REQUIRED)
  {
    Length= (SQLSMALLINT)MADB_SetString(CP_UTF8, OutConnectionString, BufferLength,
                                      OutConnStrA, SQL_NTS, &((MADB_Dbc *)ConnectionHandle)->Error);
  }
  if (StringLength2Ptr)
    *StringLength2Ptr= Length;
  
end:
  MADB_FREE(OutConnStrA);
  MADB_FREE(InConnStrA);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */


/* {{{ SQLDrivers */
SQLRETURN SQL_API SQLDrivers(SQLHENV EnvironmentHandle,
    SQLUSMALLINT Direction,
    SQLCHAR *DriverDescription,
    SQLSMALLINT BufferLength1,
    SQLSMALLINT *DescriptionLengthPtr,
    SQLCHAR *DriverAttributes,
    SQLSMALLINT BufferLength2,
    SQLSMALLINT *AttributesLengthPtr)
{
  SQLRETURN ret= SQL_ERROR;

  return ret;
}
/* }}} */

/* {{{ SQLDriversW */
SQLRETURN SQL_API SQLDriversW(SQLHENV EnvironmentHandle,
    SQLUSMALLINT Direction,
    SQLWCHAR *DriverDescription,
    SQLSMALLINT BufferLength1,
    SQLSMALLINT *DescriptionLengthPtr,
    SQLWCHAR *DriverAttributes,
    SQLSMALLINT BufferLength2,
    SQLSMALLINT *AttributesLengthPtr)
{
  SQLRETURN ret= SQL_ERROR;

  return ret;
}
/* }}} */


/* {{{ SQLEndTran */
SQLRETURN SQL_API SQLEndTran(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT CompletionType)
{
  SQLRETURN ret= SQL_SUCCESS;
  switch (HandleType) {
  case SQL_HANDLE_ENV:
    {
      MADB_Env *Env= (MADB_Env *)Handle;
      LIST *List= Env->Dbcs;

      MADB_CLEAR_ERROR(&Env->Error);
      for (List= Env->Dbcs; List; List= List->next)
        ((MADB_Dbc *)List->data)->Methods->EndTran((MADB_Dbc *)List->data, CompletionType);
    }
    break;
  case SQL_HANDLE_DBC:
    {
      MADB_Dbc *Dbc= (MADB_Dbc *)Handle;
      if (!Dbc->mariadb)
        MADB_SetError(&Dbc->Error, MADB_ERR_08002, NULL, 0);
      else
        Dbc->Methods->EndTran(Dbc, CompletionType);
      ret= Dbc->Error.ReturnValue;
    }
    break;
  default:
    /* todo: Do we need to set an error ?! */
    break;
  }

  return ret;
}
/* }}} */

/* {{{ SQLError */
SQLRETURN SQL_API SQLError(SQLHENV Env, SQLHDBC Dbc, SQLHSTMT Stmt, 
                           SQLCHAR *Sqlstate, SQLINTEGER *NativeError, 
                           SQLCHAR *Message, SQLSMALLINT MessageMax,
                           SQLSMALLINT *MessageLen)
{
  SQLSMALLINT HandleType= 0;
  SQLHANDLE Handle= NULL;

  if (Env)
  {
    Handle= Env;
    HandleType= SQL_HANDLE_ENV;
  } 
  else if (Dbc)
  {
    Handle= Dbc;
    HandleType= SQL_HANDLE_DBC;
  }
  else
  {
    Handle= Stmt;
    HandleType= SQL_HANDLE_STMT;
  }

  return SQLGetDiagRec(HandleType, Handle, 1, Sqlstate, NativeError, Message, MessageMax, MessageLen);
}
/* }}} */

/*{{{ SQLErrorW */
SQLRETURN SQL_API
SQLErrorW(SQLHENV Env, SQLHDBC Dbc, SQLHSTMT Stmt, SQLWCHAR *Sqlstate,
          SQLINTEGER *NativeError, SQLWCHAR *Message, SQLSMALLINT MessageMax,
          SQLSMALLINT *MessageLen)

{
    SQLSMALLINT HandleType= 0;
  SQLHANDLE Handle= NULL;

  if (Env)
  {
    Handle= Env;
    HandleType= SQL_HANDLE_ENV;
  } 
  else if (Dbc)
  {
    Handle= Dbc;
    HandleType= SQL_HANDLE_DBC;
  }
  else
  {
    Handle= Stmt;
    HandleType= SQL_HANDLE_STMT;
  }

  return SQLGetDiagRecW(HandleType, Handle, 1, Sqlstate, NativeError, Message, MessageMax, MessageLen);
}
/* }}} */


/* {{{ SQLTransact */
SQLRETURN SQL_API SQLTransact(SQLHENV Env, SQLHDBC Dbc, SQLUSMALLINT CompletionType)
{
  if (Env != SQL_NULL_HENV)
    return SQLEndTran(SQL_HANDLE_ENV, Env, CompletionType);
  else if (Dbc != SQL_NULL_HDBC)
    return SQLEndTran(SQL_HANDLE_DBC, Dbc, CompletionType);
  else
    return SQL_INVALID_HANDLE;
}
/* }}} */

/* {{{ SQLExecDirect */
SQLRETURN SQL_API SQLExecDirect(SQLHSTMT StatementHandle,
    SQLCHAR *StatementText,
    SQLINTEGER TextLength)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Stmt->Connection, "SQLExecDirect");
  MDBUG_C_DUMP(Stmt->Connection, StatementText, s);

  if (!Stmt)
    ret= SQL_INVALID_HANDLE;
  else
    ret= Stmt->Methods->ExecDirect(Stmt, (char *)StatementText, TextLength);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLExecDirectW */
SQLRETURN SQL_API SQLExecDirectW(SQLHSTMT StatementHandle,
    SQLWCHAR *StatementText,
    SQLINTEGER TextLength)
{
  char *CpStmt;
  SQLINTEGER StmtLength;
  SQLRETURN ret;
  BOOL ConversionError;

  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLExecDirectW");

  CpStmt= MADB_ConvertFromWChar(StatementText, TextLength, &StmtLength, Stmt->Connection->CodePage, &ConversionError);
  MDBUG_C_DUMP(Stmt->Connection, CpStmt, s);
  if (ConversionError)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_22018, NULL, 0);
    ret= Stmt->Error.ReturnValue;
  }
  else
    ret= Stmt->Methods->ExecDirect(Stmt, CpStmt, StmtLength);
  MADB_FREE(CpStmt);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLExecute */
SQLRETURN SQL_API SQLExecute(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt = (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLExecute");

  ret= Stmt->Methods->Execute(Stmt);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLExtendedFetch */
SQLRETURN SQL_API SQLExtendedFetch(SQLHSTMT StatementHandle,
    SQLUSMALLINT FetchOrientation,
    SQLLEN FetchOffset,
    SQLULEN *RowCountPtr,
    SQLUSMALLINT *RowStatusArray)
{
  SQLRETURN ret;
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  SQLUINTEGER *SaveRowsProcessedPtr= Stmt->Ird->Header.RowsProcessedPtr;
  SQLUSMALLINT *SaveArrayStatusPtr= Stmt->Ird->Header.ArrayStatusPtr;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLExtendedFetch");
  MDBUG_C_DUMP(Stmt->Connection, FetchOrientation, u);
  MDBUG_C_DUMP(Stmt->Connection, FetchOffset, d);
  MDBUG_C_DUMP(Stmt->Connection, RowCountPtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, RowStatusArray, 0x);
   
  Stmt->Ird->Header.RowsProcessedPtr= RowCountPtr;
  Stmt->Ird->Header.ArrayStatusPtr= RowStatusArray;
  ret= SQLFetchScroll((SQLHSTMT)Stmt, FetchOrientation, FetchOffset);

  if (RowStatusArray && SaveArrayStatusPtr)
  {
    SQLUINTEGER i;
    for (i=0; i < Stmt->Ard->Header.ArraySize; i++)
      SaveArrayStatusPtr[i]= RowStatusArray[i];
  }

  Stmt->Ird->Header.RowsProcessedPtr= SaveRowsProcessedPtr;
  Stmt->Ird->Header.ArrayStatusPtr= SaveArrayStatusPtr;

  if (ret == SQL_NO_DATA)
  {
    if (RowCountPtr)
      *RowCountPtr= 0;
  }
  if (ret == SQL_ERROR)
    if (strcmp(Stmt->Error.SqlState, "22002") == 0)
      ret= SQL_SUCCESS_WITH_INFO;
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLFetch */
SQLRETURN SQL_API SQLFetch(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  Stmt->FetchType= MADB_FETCH_TYPE_FETCH;
  MDBUG_C_ENTER(Stmt->Connection, "SQLFetch");
  ret= Stmt->Methods->Fetch(Stmt, FALSE);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLFetchScroll */
SQLRETURN SQL_API SQLFetchScroll(SQLHSTMT StatementHandle,
    SQLSMALLINT FetchOrientation,
    SQLLEN FetchOffset)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if(!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLFetchScroll");
  MDBUG_C_DUMP(Stmt->Connection, FetchOrientation, d);

  ret= Stmt->Methods->FetchScroll(Stmt, FetchOrientation, FetchOffset);

  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLFreeHandle */
SQLRETURN SQL_API SQLFreeHandle(SQLSMALLINT HandleType,
    SQLHANDLE Handle)
{
  SQLRETURN ret= SQL_ERROR;

  if (!Handle)
    return SQL_INVALID_HANDLE;

  MDBUG_ENTER("SQLFreeHandle");

  MDBUG_DUMP(HandleType, d);
  MDBUG_DUMP(Handle, 0x);

  switch (HandleType) {
    case SQL_HANDLE_ENV:
      ret= MADB_EnvFree((MADB_Env *)Handle);
      break;
    case SQL_HANDLE_DBC:
      if (Handle && ((MADB_Dbc *)Handle)->Environment)
      {
        MADB_Env *Environment = ((MADB_Dbc *)Handle)->Environment;
        EnterCriticalSection(&Environment->cs);
        ret= MADB_DbcFree((MADB_Dbc *)Handle);
        LeaveCriticalSection(&Environment->cs);
      } else
        ret= SQL_ERROR; 
      break;
    case SQL_HANDLE_DESC:
      {
        MADB_Desc *Desc= (MADB_Desc *)Handle;

        /* Error if the descriptor does not belong to application(was automatically alliocated by the driver)
           Basically DM is supposed to take care of this. Keeping in mind direct linking */
        if (!Desc->AppType)
        {
          MADB_SetError(&Desc->Error, MADB_ERR_HY017, NULL, 0);
          return Desc->Error.ReturnValue;
        }

        return MADB_DescFree(Desc, FALSE);
      }
    case SQL_HANDLE_STMT:
      {
        MADB_Stmt *Stmt= (MADB_Stmt *)Handle;
        return Stmt->Methods->StmtFree(Stmt, SQL_DROP);
      }
  }
  MDBUG_DUMP(ret, d);
  MDBUG_RETURN(ret);
}
/* }}} */

/* {{{ SQLFreeEnv */
SQLRETURN SQL_API SQLFreeEnv(SQLHANDLE henv)
{
  return SQLFreeHandle(SQL_HANDLE_ENV, henv);
}
/* }}} */

/* {{{ SQLFreeConnect */
SQLRETURN SQL_API SQLFreeConnect(SQLHANDLE hdbc)
{
  return SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
}
/* }}} */

/* {{{ SQLFreeStmt */
SQLRETURN SQL_API SQLFreeStmt(SQLHSTMT StatementHandle,
           SQLUSMALLINT Option)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  return Stmt->Methods->StmtFree(Stmt, Option);
}
/* }}} */

/* {{{ SQLForeignKeys */
SQLRETURN SQL_API SQLForeignKeys(SQLHSTMT StatementHandle,
    SQLCHAR *PKCatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *PKSchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *PKTableName,
    SQLSMALLINT NameLength3,
    SQLCHAR *FKCatalogName,
    SQLSMALLINT NameLength4,
    SQLCHAR *FKSchemaName,
    SQLSMALLINT NameLength5,
    SQLCHAR *FKTableName,
    SQLSMALLINT NameLength6)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;

  if(!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLForeignKeys");

  ret= Stmt->Methods->ForeignKeys(Stmt, (char *)PKCatalogName, NameLength1, (char *)PKSchemaName, NameLength2,
                                    (char *)PKTableName, NameLength3, (char *)FKCatalogName, NameLength4,
                                    (char *)FKSchemaName, NameLength4, (char *)FKTableName, NameLength6);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLForeignKeysW */
SQLRETURN SQL_API SQLForeignKeysW(SQLHSTMT StatementHandle,
    SQLWCHAR *PKCatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *PKSchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *PKTableName,
    SQLSMALLINT NameLength3,
    SQLWCHAR *FKCatalogName,
    SQLSMALLINT NameLength4,
    SQLWCHAR *FKSchemaName,
    SQLSMALLINT NameLength5,
    SQLWCHAR *FKTableName,
    SQLSMALLINT NameLength6)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  char *CpPkCatalog= NULL,
       *CpPkSchema= NULL,
       *CpPkTable= NULL,
       *CpFkCatalog= NULL,
       *CpFkSchema= NULL,
       *CpFkTable= NULL;
  SQLINTEGER CpLength1, CpLength2, CpLength3,
             CpLength4, CpLength5, CpLength6;
  SQLRETURN ret;

  MDBUG_C_ENTER(Stmt->Connection, "SQLForeignKeysW");

  CpPkCatalog= MADB_ConvertFromWChar(PKCatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpPkSchema= MADB_ConvertFromWChar(PKSchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpPkTable= MADB_ConvertFromWChar(PKTableName, NameLength3, &CpLength3, Stmt->Connection->CodePage, NULL);
  CpFkCatalog= MADB_ConvertFromWChar(FKCatalogName, NameLength4, &CpLength4, Stmt->Connection->CodePage, NULL);
  CpFkSchema= MADB_ConvertFromWChar(FKSchemaName, NameLength5, &CpLength5, Stmt->Connection->CodePage, NULL);
  CpFkTable= MADB_ConvertFromWChar(FKTableName, NameLength6, &CpLength6, Stmt->Connection->CodePage, NULL);

  ret= Stmt->Methods->ForeignKeys(Stmt, CpPkCatalog, (SQLSMALLINT)CpLength1, CpPkSchema, (SQLSMALLINT)CpLength2,
                                  CpPkTable, (SQLSMALLINT)CpLength3, CpFkCatalog, (SQLSMALLINT)CpLength4, 
                                  CpFkSchema, (SQLSMALLINT)CpLength5, CpFkTable, (SQLSMALLINT)CpLength6);
  MADB_FREE(CpPkCatalog);
  MADB_FREE(CpPkSchema);
  MADB_FREE(CpPkTable);
  MADB_FREE(CpFkCatalog);
  MADB_FREE(CpFkSchema);
  MADB_FREE(CpFkTable);

  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */


/* {{{ SQLGetConnectAttr */
SQLRETURN SQL_API SQLGetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;
  if (!Dbc)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Dbc, "SQLGetConnectAttr");
  MDBUG_C_DUMP(Dbc, Attribute, d);
  MDBUG_C_DUMP(Dbc, ValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, BufferLength, d);
  MDBUG_C_DUMP(Dbc, StringLengthPtr, 0x);

  ret= Dbc->Methods->GetAttr(Dbc, Attribute, ValuePtr, BufferLength, StringLengthPtr, FALSE);

  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */

/* {{{ SQLGetConnectAttrW */
SQLRETURN SQL_API SQLGetConnectAttrW(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;

  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Dbc, "SQLGetConnectAttr");
  MDBUG_C_DUMP(Dbc, Attribute, d);
  MDBUG_C_DUMP(Dbc, ValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, BufferLength, d);
  MDBUG_C_DUMP(Dbc, StringLengthPtr, 0x);

  ret= Dbc->Methods->GetAttr(Dbc, Attribute, ValuePtr, BufferLength, StringLengthPtr, TRUE);

  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */


/* {{{ SQLGetConnectOption */
SQLRETURN SQL_API SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER ValuePtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  
  if (!Dbc)
    return SQL_INVALID_HANDLE;
  return SQLGetConnectAttr(ConnectionHandle, Option, ValuePtr,
                           Option == SQL_ATTR_CURRENT_CATALOG ? SQL_MAX_OPTION_STRING_LENGTH : 0, NULL);
}
/* }}} */

/* {{{ SQLGetConnectOptionW */
SQLRETURN SQL_API SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER ValuePtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  if (!Dbc)
    return SQL_INVALID_HANDLE;
  return SQLGetConnectAttrW(ConnectionHandle, Option, ValuePtr,
                           Option == SQL_ATTR_CURRENT_CATALOG ? SQL_MAX_OPTION_STRING_LENGTH : 0, NULL);
}
/* }}} */


/* {{{ SQLGetCursorName */
SQLRETURN SQL_API SQLGetCursorName(
     SQLHSTMT        StatementHandle,
     SQLCHAR *       CursorName,
     SQLSMALLINT     BufferLength,
     SQLSMALLINT *   NameLengthPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->GetCursorName(Stmt, CursorName, BufferLength, NameLengthPtr, FALSE);
}
/* }}} */

/* {{{ SQLGetCursorNameW */
SQLRETURN SQL_API SQLGetCursorNameW(
     SQLHSTMT        StatementHandle,
     SQLWCHAR *      CursorName,
     SQLSMALLINT     BufferLength,
     SQLSMALLINT *   NameLengthPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->GetCursorName(Stmt, CursorName, BufferLength, NameLengthPtr, TRUE);
}
/* }}} */


/* {{{ SQLGetData */
SQLRETURN SQL_API SQLGetData(SQLHSTMT StatementHandle,
    SQLUSMALLINT Col_or_Param_Num,
    SQLSMALLINT TargetType,
    SQLPOINTER TargetValuePtr,
    SQLLEN BufferLength,
    SQLLEN *StrLen_or_IndPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->GetData(StatementHandle, Col_or_Param_Num, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
}
/* }}} */

/* {{{ SQLGetDescField */
SQLRETURN SQL_API SQLGetDescField(SQLHDESC DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  if (!DescriptorHandle)
    return SQL_INVALID_HANDLE;
  return MADB_DescGetField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, StringLengthPtr, FALSE); 
}
/* }}} */

/* {{{ SQLGetDescFieldW */
SQLRETURN SQL_API SQLGetDescFieldW(SQLHDESC DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  return MADB_DescGetField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, StringLengthPtr, TRUE); 
}
/* }}} */


/* {{{ SQLGetDescRec */
SQLRETURN SQL_API SQLGetDescRec(SQLHDESC DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLCHAR *Name,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLSMALLINT *TypePtr,
    SQLSMALLINT *SubTypePtr,
    SQLLEN *LengthPtr,
    SQLSMALLINT *PrecisionPtr,
    SQLSMALLINT *ScalePtr,
    SQLSMALLINT *NullablePtr)
{
  MADB_Desc *Desc= (MADB_Desc *)DescriptorHandle;
  if (!Desc)
    return SQL_INVALID_HANDLE;
  return MADB_DescGetRec(Desc, RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr,
                         LengthPtr, PrecisionPtr, ScalePtr, NullablePtr, FALSE);
}
/* }}} */

/* {{{ SQLGetDescRecW */
SQLRETURN SQL_API SQLGetDescRecW(SQLHDESC DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLWCHAR *Name,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLSMALLINT *TypePtr,
    SQLSMALLINT *SubTypePtr,
    SQLLEN *LengthPtr,
    SQLSMALLINT *PrecisionPtr,
    SQLSMALLINT *ScalePtr,
    SQLSMALLINT *NullablePtr)
{
 MADB_Desc *Desc= (MADB_Desc *)DescriptorHandle;
  if (!Desc)
    return SQL_INVALID_HANDLE;
  return MADB_DescGetRec(Desc, RecNumber, (SQLCHAR *)Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr,
                         LengthPtr, PrecisionPtr, ScalePtr, NullablePtr, TRUE);
}
/* }}} */


/* {{{ SQLGetDiagField */
SQLRETURN SQL_API SQLGetDiagField(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT DiagIdentifier,
    SQLPOINTER DiagInfoPtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr)
{
  if (!Handle)
    return SQL_INVALID_HANDLE;
  return MADB_GetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfoPtr, BufferLength, StringLengthPtr, FALSE);
}
/* }}} */

/* {{{ SQLGetDiagFieldW */
SQLRETURN SQL_API SQLGetDiagFieldW(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT DiagIdentifier,
    SQLPOINTER DiagInfoPtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr)
{
  if (!Handle)
    return SQL_INVALID_HANDLE;
  return MADB_GetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfoPtr, BufferLength, StringLengthPtr, TRUE);
}
/* }}} */


/* {{{ SQLGetDiagRec */
SQLRETURN SQL_API SQLGetDiagRec(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLCHAR *SQLState,
    SQLINTEGER *NativeErrorPtr,
    SQLCHAR *MessageText,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *TextLengthPtr)
{
  if (!Handle)
    return SQL_INVALID_HANDLE;

  if (RecNumber < 1 || BufferLength < 0)
    return SQL_ERROR;

  /* Maria ODBC driver doesn't support error lists, so only the first record can be retrieved */
  if (RecNumber != 1)
    return SQL_NO_DATA_FOUND;
  
  switch (HandleType) {
    case SQL_HANDLE_DBC:
      {
        MADB_Dbc *Dbc= (MADB_Dbc *)Handle;
        return MADB_GetDiagRec(&Dbc->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                              (void *) MessageText, BufferLength, TextLengthPtr, FALSE,
+                              Dbc->Environment->OdbcVersion);
      }
      break;
    case SQL_HANDLE_STMT:
      {
        MADB_Stmt *Stmt= (MADB_Stmt *)Handle;
        return MADB_GetDiagRec(&Stmt->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, FALSE,
                               Stmt->Connection->Environment->OdbcVersion);
      }
      break;
    case SQL_HANDLE_DESC:
      {
        MADB_Desc *Desc= (MADB_Desc *)Handle;
        return MADB_GetDiagRec(&Desc->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, FALSE,
                               SQL_OV_ODBC3);
      }
      break;
    case SQL_HANDLE_ENV:
      {
        MADB_Env *Env= (MADB_Env *)Handle;
        return MADB_GetDiagRec(&Env->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, FALSE,
                               Env->OdbcVersion);
      }
    default:
      return SQL_ERROR;  
      break;
  }  
}
/* }}} */

/* {{{ SQLGetDiagRecW */
SQLRETURN SQL_API SQLGetDiagRecW(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLWCHAR *SQLState,
    SQLINTEGER *NativeErrorPtr,
    SQLWCHAR *MessageText,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *TextLengthPtr)
{
  if (!Handle)
    return SQL_INVALID_HANDLE;

  /* Maria ODBC driver doesn't support error lists, so only the first record can be retrieved */
  if (RecNumber != 1)
    return SQL_NO_DATA_FOUND;
  
  switch (HandleType) {
    case SQL_HANDLE_DBC:
      {
        MADB_Dbc *Dbc= (MADB_Dbc *)Handle;
        return MADB_GetDiagRec(&Dbc->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, TRUE,
                               Dbc->Environment->OdbcVersion);
      }
      break;
    case SQL_HANDLE_STMT:
      {
        MADB_Stmt *Stmt= (MADB_Stmt *)Handle;
        return MADB_GetDiagRec(&Stmt->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, TRUE,
                               Stmt->Connection->Environment->OdbcVersion);
      }
      break;
    case SQL_HANDLE_DESC:
      {
        MADB_Desc *Desc= (MADB_Desc *)Handle;
        return MADB_GetDiagRec(&Desc->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, TRUE,
                               SQL_OV_ODBC3);
      }
      break;
    case SQL_HANDLE_ENV:
      {
        MADB_Env *Env= (MADB_Env *)Handle;
        return MADB_GetDiagRec(&Env->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, TRUE,
                               Env->OdbcVersion);
      }
    default:
      return SQL_ERROR;  
      break;
  }
}
/* }}} */


/* {{{ SQLGetEnvAttr */
SQLRETURN SQL_API SQLGetEnvAttr(SQLHENV EnvironmentHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  MADB_Env *Env= (MADB_Env *)EnvironmentHandle;
  SQLRETURN ret;

  MDBUG_ENTER("SQLGetEnvAttr");
  MDBUG_DUMP(Attribute, d);
  MDBUG_DUMP(ValuePtr, 0x);
  MDBUG_DUMP(BufferLength, d);
  MDBUG_DUMP(StringLengthPtr, 0x);

  if (!Env)
    ret= SQL_INVALID_HANDLE;
  else
    ret= MADB_EnvGetAttr(Env, Attribute, ValuePtr, BufferLength, StringLengthPtr);

  MDBUG_DUMP(ret, d);
  MDBUG_RETURN(ret);
}
/* }}} */

/* {{{ SQLGetFunctions */
SQLRETURN SQL_API SQLGetFunctions(SQLHDBC ConnectionHandle,
    SQLUSMALLINT FunctionId,
    SQLUSMALLINT *SupportedPtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;

  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Dbc, "SQLGetFunctions");
  MDBUG_C_DUMP(Dbc, FunctionId, d);
  MDBUG_C_DUMP(Dbc, SupportedPtr, 0x);
  ret= Dbc->Methods->GetFunctions(Dbc, FunctionId, SupportedPtr);
  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc,ret);
}
/* }}} */

/* {{{ SQLGetInfo */
SQLRETURN SQL_API SQLGetInfo(SQLHDBC ConnectionHandle,
    SQLUSMALLINT InfoType,
    SQLPOINTER InfoValuePtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;
  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Dbc, "SQLGetInfo");
  MDBUG_C_DUMP(Dbc, InfoType, d);
  ret= Dbc->Methods->GetInfo(Dbc, InfoType, InfoValuePtr, BufferLength, StringLengthPtr, FALSE);
  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */

/* {{{ SQLGetInfoW */
SQLRETURN SQL_API SQLGetInfoW(SQLHDBC ConnectionHandle,
    SQLUSMALLINT InfoType,
    SQLPOINTER InfoValuePtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;
  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Dbc, "SQLGetInfo");
  MDBUG_C_DUMP(Dbc, InfoType, d);
  MDBUG_C_DUMP(Dbc, InfoValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, StringLengthPtr, 0x);
  ret= Dbc->Methods->GetInfo(Dbc, InfoType, InfoValuePtr, BufferLength, StringLengthPtr, TRUE);
  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */

/* {{{ SQLGetStmtAttr */
SQLRETURN SQL_API SQLGetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->GetAttr(Stmt, Attribute, ValuePtr, BufferLength, StringLengthPtr);
}
/* }}} */

/* {{{ SQLGetStmtAttrW */
SQLRETURN SQL_API SQLGetStmtAttrW(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->GetAttr(Stmt, Attribute, ValuePtr, BufferLength, StringLengthPtr);
}
/* }}} */


/* {{{ SQLGetStmtOption */
SQLRETURN  SQL_API SQLGetStmtOption(SQLHSTMT StatementHandle,
                                    SQLUSMALLINT Option, SQLPOINTER Value)
{
  return SQLGetStmtAttr(StatementHandle, Option, Value, SQL_NTS, (SQLINTEGER *)NULL);
}

/* }}} */

/* {{{ SQLGetTypeInfo */
SQLRETURN SQL_API SQLGetTypeInfo(SQLHSTMT StatementHandle,
    SQLSMALLINT DataType)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  return MADB_GetTypeInfo(Stmt, DataType);
}
/* }}} */

/* {{{ SQLGetTypeInfoW */
SQLRETURN SQL_API SQLGetTypeInfoW(SQLHSTMT StatementHandle,
    SQLSMALLINT DataType)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  

  return MADB_GetTypeInfo(Stmt, DataType);}
/* }}} */


/* {{{ SQLMoreResults */
SQLRETURN SQL_API SQLMoreResults(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  return MADB_StmtMoreResults(Stmt);
}
/* }}} */

/* {{{ SQLNativeSql */
SQLRETURN SQL_API SQLNativeSql(SQLHDBC ConnectionHandle,
    SQLCHAR *InStatementText,
    SQLINTEGER TextLength1,
    SQLCHAR *OutStatementText,
    SQLINTEGER BufferLength,
    SQLINTEGER *TextLength2Ptr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLINTEGER Length;
  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Dbc->Error);
  if (!TextLength2Ptr && (!OutStatementText || !BufferLength))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_01004, NULL, 0);
    return Dbc->Error.ReturnValue;
  }
  Length= MADB_SetString(0, OutStatementText, BufferLength, (char *)InStatementText, TextLength1, &Dbc->Error);
  if (TextLength2Ptr)
    *TextLength2Ptr= Length;
  return Dbc->Error.ReturnValue;
}
/* }}} */

/* {{{ SQLNativeSqlW */
SQLRETURN SQL_API SQLNativeSqlW(SQLHDBC ConnectionHandle,
    SQLWCHAR *InStatementText,
    SQLINTEGER TextLength1,
    SQLWCHAR *OutStatementText,
    SQLINTEGER BufferLength,
    SQLINTEGER *TextLength2Ptr)
{
  MADB_Dbc *Conn= (MADB_Dbc *)ConnectionHandle;
  SQLINTEGER Length= (TextLength1 == SQL_NTS) ? wcslen(InStatementText) : TextLength1;
  if (TextLength2Ptr)
    *TextLength2Ptr= Length;

  if(OutStatementText && BufferLength < Length)
    MADB_SetError(&Conn->Error, MADB_ERR_01004, NULL, 0);

  if(OutStatementText && BufferLength < Length)
    MADB_SetError(&Conn->Error, MADB_ERR_01004, NULL, 0);
  Length= MIN(Length, BufferLength - 1);

  if (OutStatementText && BufferLength)
  {
    memcpy(OutStatementText, InStatementText, Length * sizeof(SQLWCHAR));
    OutStatementText[Length]= 0;
  }
  return Conn->Error.ReturnValue;
}
/* }}} */


/* {{{ SQLNumParams */
SQLRETURN SQL_API SQLNumParams(SQLHSTMT StatementHandle,
    SQLSMALLINT *ParameterCountPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  MADB_CHECK_STMT_HANDLE(Stmt, stmt);
  return Stmt->Methods->ParamCount(Stmt, ParameterCountPtr);
}
/* }}} */

/* {{{ SQLNumResultCols */
SQLRETURN SQL_API SQLNumResultCols(SQLHSTMT StatementHandle,
    SQLSMALLINT *ColumnCountPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  return Stmt->Methods->ColumnCount(Stmt, ColumnCountPtr);
}
/* }}} */

/* {{{ SQLParamData */
SQLRETURN SQL_API SQLParamData(SQLHSTMT StatementHandle,
    SQLPOINTER *ValuePtrPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->ParamData(Stmt, ValuePtrPtr);
}
/* }}} */

/* {{{ SQLPrepare */
SQLRETURN SQL_API SQLPrepare(SQLHSTMT StatementHandle,
    SQLCHAR *StatementText,
    SQLINTEGER TextLength)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Stmt->Connection, "SQLPrepare");

  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, StatementText, s);
  MDBUG_C_DUMP(Stmt->Connection, TextLength, d);
  ret= Stmt->Methods->Prepare(Stmt, (char *)StatementText, TextLength);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLPrepareW */
SQLRETURN SQL_API SQLPrepareW(SQLHSTMT StatementHandle,
    SQLWCHAR *StatementText,
    SQLINTEGER TextLength)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  char *StmtStr;
  SQLINTEGER StmtLength;
  SQLRETURN ret;
  BOOL ConversionError;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLPrepareW");

  StmtStr= MADB_ConvertFromWChar(StatementText, TextLength, &StmtLength, Stmt->Connection->CodePage, &ConversionError);

  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, StmtStr, s);
  MDBUG_C_DUMP(Stmt->Connection, TextLength, d);

  if (ConversionError)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_22018, NULL, 0);
    ret= Stmt->Error.ReturnValue;
  }
  else
    ret= Stmt->Methods->Prepare(Stmt, StmtStr, StmtLength);
  MADB_FREE(StmtStr);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */


/* {{{ SQLPrimaryKeys */
SQLRETURN SQL_API SQLPrimaryKeys(SQLHSTMT StatementHandle,
    SQLCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *TableName,
    SQLSMALLINT NameLength3)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Stmt->Connection, "SQLPrimaryKeys");
  MDBUG_C_DUMP(Stmt->Connection, StatementHandle, 0x);
  MDBUG_C_DUMP(Stmt->Connection, CatalogName, s);
  MDBUG_C_DUMP(Stmt->Connection, NameLength1, d);
  MDBUG_C_DUMP(Stmt->Connection, SchemaName, s);
  MDBUG_C_DUMP(Stmt->Connection, NameLength2, d);
  MDBUG_C_DUMP(Stmt->Connection, TableName, s);
  MDBUG_C_DUMP(Stmt->Connection, NameLength3, d);

  if (!Stmt)
    ret= SQL_INVALID_HANDLE;
  else
    ret= Stmt->Methods->PrimaryKeys(Stmt, (char *)CatalogName, NameLength1, (char *)SchemaName, NameLength2,
                                    (char *)TableName, NameLength3);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLPrimaryKeysW */
SQLRETURN SQL_API SQLPrimaryKeysW(SQLHSTMT StatementHandle,
    SQLWCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *TableName,
    SQLSMALLINT NameLength3)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpTable= NULL;
  SQLINTEGER CpLength1, CpLength2, CpLength3;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->CodePage, NULL);

  MDBUG_C_ENTER(Stmt->Connection, "SQLPrimaryKeysW");
  MDBUG_C_DUMP(Stmt->Connection, StatementHandle, 0x);
  MDBUG_C_DUMP(Stmt->Connection, CpCatalog, s);
  MDBUG_C_DUMP(Stmt->Connection, CpLength1, d);
  MDBUG_C_DUMP(Stmt->Connection, CpSchema, s);
  MDBUG_C_DUMP(Stmt->Connection, CpLength2, d);
  MDBUG_C_DUMP(Stmt->Connection, CpTable, s);
  MDBUG_C_DUMP(Stmt->Connection, CpLength3, d);
  
  ret= Stmt->Methods->PrimaryKeys(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
                                  CpTable, (SQLSMALLINT)CpLength3);
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);

  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */


/* {{{ SQLProcedureColumns */
SQLRETURN SQL_API SQLProcedureColumns(SQLHSTMT StatementHandle,
    SQLCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *ProcName,
    SQLSMALLINT NameLength3,
    SQLCHAR *ColumnName,
    SQLSMALLINT NameLength4)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->ProcedureColumns(Stmt, (char *)CatalogName,NameLength1, (char *)SchemaName, NameLength2,
                                         (char *)ProcName, NameLength3, (char *)ColumnName, NameLength4);

}
/* }}} */

/* {{{ SQLProcedureColumnsW */
SQLRETURN SQL_API SQLProcedureColumnsW(SQLHSTMT StatementHandle,
    SQLWCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *ProcName,
    SQLSMALLINT NameLength3,
    SQLWCHAR *ColumnName,
    SQLSMALLINT NameLength4)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpProc= NULL,
       *CpColumn= NULL;
  SQLINTEGER CpLength1, CpLength2, CpLength3, CpLength4;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpProc= MADB_ConvertFromWChar(ProcName, NameLength3, &CpLength3, Stmt->Connection->CodePage, NULL);
  CpColumn= MADB_ConvertFromWChar(ColumnName, NameLength4, &CpLength4, Stmt->Connection->CodePage, NULL);

  ret= Stmt->Methods->ProcedureColumns(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
                                       CpProc, (SQLSMALLINT)CpLength3, CpColumn, (SQLSMALLINT)CpLength4);
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpProc);
  MADB_FREE(CpColumn);

  return ret;
}
/* }}} */


/* {{{ SQLProcedures */
SQLRETURN SQL_API SQLProcedures(SQLHSTMT StatementHandle,
    SQLCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *ProcName,
    SQLSMALLINT NameLength3)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  return Stmt->Methods->Procedures(Stmt, (char *)CatalogName, NameLength1, (char *)SchemaName, 
                                   NameLength2, (char *)ProcName, NameLength3);
}
/* }}} */

/* {{{ SQLProceduresW */
SQLRETURN SQL_API SQLProceduresW(SQLHSTMT StatementHandle,
    SQLWCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *ProcName,
    SQLSMALLINT NameLength3)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpProc= NULL;
  SQLINTEGER CpLength1, CpLength2, CpLength3;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpProc= MADB_ConvertFromWChar(ProcName, NameLength3, &CpLength3, Stmt->Connection->CodePage, NULL);
  
  ret= Stmt->Methods->Procedures(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
                                 CpProc, (SQLSMALLINT)CpLength3);
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpProc);
  return ret;
}
/* }}} */


/* {{{ SQLPutData */
SQLRETURN SQL_API SQLPutData(SQLHSTMT StatementHandle,
    SQLPOINTER DataPtr,
    SQLLEN StrLen_or_Ind)
{
  MADB_Stmt *Stmt=(MADB_Stmt *)StatementHandle;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLPutData");
  MDBUG_C_DUMP(Stmt->Connection, DataPtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, StrLen_or_Ind, d);

  ret= Stmt->Methods->PutData(Stmt, DataPtr, StrLen_or_Ind);

  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLRowCount */
SQLRETURN SQL_API SQLRowCount(SQLHSTMT StatementHandle,
    SQLLEN *RowCountPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  MADB_CHECK_STMT_HANDLE(Stmt, stmt);
  return Stmt->Methods->RowCount(Stmt, RowCountPtr);
}
/* }}} */

/* {{{ SQLSetConnectAttr */
SQLRETURN SQL_API SQLSetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;

  if (!Dbc)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Dbc, "SQLSetConnectAttr");
  MDBUG_C_DUMP(Dbc, Attribute, d);
  MDBUG_C_DUMP(Dbc, ValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, StringLength, d);

  ret= Dbc->Methods->SetAttr(Dbc, Attribute, ValuePtr, StringLength, FALSE);

  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */

/* {{{ SQLSetConnectAttrW */
SQLRETURN SQL_API SQLSetConnectAttrW(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;
  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Dbc, "SetConnectAttrW");
  MDBUG_C_DUMP(Dbc, Dbc, 0x);
  MDBUG_C_DUMP(Dbc, Attribute, d);
  MDBUG_C_DUMP(Dbc, ValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, StringLength, d);
  ret= Dbc->Methods->SetAttr(Dbc, Attribute, ValuePtr, StringLength, TRUE);
  MDBUG_C_DUMP(Dbc, ret, d);
  MDBUG_C_RETURN(Dbc, ret);
}
/* }}} */



/* {{{ SQLSetConnectOption */
SQLRETURN SQL_API
SQLSetConnectOption(SQLHDBC Hdbc, SQLUSMALLINT Option, SQLULEN Param)
{
  SQLINTEGER StringLength= 0;
  SQLRETURN ret;

  if (!Hdbc)
    return SQL_INVALID_HANDLE;

  /* todo: do we have more string options ? */
  if (Option == SQL_ATTR_CURRENT_CATALOG)
    StringLength= SQL_NTS;
  ret= SQLSetConnectAttr(Hdbc, Option, (SQLPOINTER)Param, StringLength);
  return ret;
}
/* }}} */

/* {{{ SQLSetConnectOptionW */
SQLRETURN SQL_API
SQLSetConnectOptionW(SQLHDBC Hdbc, SQLUSMALLINT Option, SQLULEN Param)
{
  SQLINTEGER StringLength= 0;
  SQLRETURN ret;

  if (!Hdbc)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER((MADB_Dbc *)Hdbc, "SetSetConnectOptionW");
  MDBUG_C_DUMP((MADB_Dbc *)Hdbc, Option, d);
  MDBUG_C_DUMP((MADB_Dbc *)Hdbc, Param, u);
  /* todo: do we have more string options ? */
  if (Option == SQL_ATTR_CURRENT_CATALOG)
    StringLength= SQL_NTS;
  ret= SQLSetConnectAttrW(Hdbc, Option, (SQLPOINTER)Param, StringLength);
  MDBUG_C_DUMP((MADB_Dbc *)Hdbc, ret, d);
  MDBUG_C_RETURN((MADB_Dbc *)Hdbc, ret);
}
/* }}} */


/* {{{ SQLSetCursorName */
SQLRETURN SQL_API SQLSetCursorName(SQLHSTMT StatementHandle,
    SQLCHAR *CursorName,
    SQLSMALLINT NameLength)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->SetCursorName(Stmt, (char *)CursorName, NameLength);
}
/* }}} */

/* {{{ SQLSetCursorNameW */
SQLRETURN SQL_API SQLSetCursorNameW(SQLHSTMT StatementHandle,
    SQLWCHAR *CursorName,
    SQLSMALLINT NameLength)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  char *CpName= NULL;
  SQLINTEGER Length;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  CpName= MADB_ConvertFromWChar(CursorName, NameLength, &Length, Stmt->Connection->CodePage, NULL);
  return Stmt->Methods->SetCursorName(Stmt, (char *)CpName, Length);
}
/* }}} */


/* {{{ SQLSetDescField */
SQLRETURN SQL_API SQLSetDescField(SQLHDESC DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength)
{
  MADB_Desc *Desc= (MADB_Desc *)DescriptorHandle;
  if (!Desc)
    return SQL_INVALID_HANDLE;
  return MADB_DescSetField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, FALSE); 
}
/* }}} */

/* {{{ SQLSetDescFieldW */
SQLRETURN SQL_API SQLSetDescFieldW(SQLHDESC DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength)
{
  MADB_Desc *Desc= (MADB_Desc *)DescriptorHandle;
  if (!Desc)
    return SQL_INVALID_HANDLE;
  return MADB_DescSetField(DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, TRUE); 
}
/* }}} */


/* {{{ SQLSetDescRec */
SQLRETURN SQL_API SQLSetDescRec(SQLHDESC DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT Type,
    SQLSMALLINT SubType,
    SQLLEN Length,
    SQLSMALLINT Precision,
    SQLSMALLINT Scale,
    SQLPOINTER DataPtr,
    SQLLEN *StringLengthPtr,
    SQLLEN *IndicatorPtr)
{
  MADB_Desc *Desc= (MADB_Desc *)DescriptorHandle;
  MADB_NOT_IMPLEMENTED(Desc);
}
/* }}} */


/* {{{ SQLSetDescRecW */
SQLRETURN SQL_API SQLSetDescRecW(SQLHDESC DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT Type,
    SQLSMALLINT SubType,
    SQLLEN Length,
    SQLSMALLINT Precision,
    SQLSMALLINT Scale,
    SQLPOINTER DataPtr,
    SQLLEN *StringLengthPtr,
    SQLLEN *IndicatorPtr)
{
  MADB_Desc *Desc= (MADB_Desc *)DescriptorHandle;
  MADB_NOT_IMPLEMENTED(Desc);
}
/* }}} */


/* {{{ SQLSetEnvAttr */
SQLRETURN SQL_API SQLSetEnvAttr(SQLHENV EnvironmentHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  MADB_Env *Env= (MADB_Env *)EnvironmentHandle;
  SQLRETURN ret;
  MDBUG_ENTER("SQLSetEnvAttr");
  MDBUG_DUMP(Attribute, d);
  MDBUG_DUMP(ValuePtr, 0x);
  if (!Env)
    ret= SQL_INVALID_HANDLE;
  else
    ret= MADB_EnvSetAttr(Env, Attribute, ValuePtr, StringLength);
  MDBUG_DUMP(ret, d);
  MDBUG_RETURN(ret);
}
/* }}} */


/* {{{ SQLSetPos */
SQLRETURN SQL_API SQLSetPos(SQLHSTMT StatementHandle,
    SQLSETPOSIROW RowNumber,
    SQLUSMALLINT Operation,
    SQLUSMALLINT LockType)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(Stmt->Connection, "SQLSetPos");
  MDBUG_C_DUMP(Stmt->Connection, RowNumber, d);
  MDBUG_C_DUMP(Stmt->Connection, Operation, u);
  MDBUG_C_DUMP(Stmt->Connection, LockType, d);

  Stmt->FetchType= MADB_FETCH_TYPE_SETPOS;
  ret= Stmt->Methods->SetPos(Stmt, RowNumber, Operation, LockType, 0);

  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLSetParam */
SQLRETURN SQL_API SQLSetParam(SQLHSTMT stmt,
	                            SQLUSMALLINT par,
	                            SQLSMALLINT type,
	                            SQLSMALLINT sqltype,
	                            SQLULEN coldef,
	                            SQLSMALLINT scale,
	                            SQLPOINTER val,
	                            SQLLEN *nval)
{
  if (!stmt)
    return SQL_INVALID_HANDLE;
  return SQLBindParameter(stmt, par, SQL_PARAM_INPUT_OUTPUT, type, sqltype, coldef,
                       scale, val, SQL_SETPARAM_VALUE_MAX, nval);
}

/* }}} */

/* {{{ SQLSetStmtAttr */
SQLRETURN SQL_API SQLSetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Stmt->Connection, "SQLSetStmtAttr");
  MDBUG_C_DUMP(Stmt->Connection, Attribute, d);
  MDBUG_C_DUMP(Stmt->Connection, ValuePtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, StringLength, d);

  ret= Stmt->Methods->SetAttr(Stmt, Attribute, ValuePtr, StringLength);

  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  MDBUG_C_RETURN(Stmt->Connection, ret);
}
/* }}} */

/* {{{ SQLSetStmtAttrW */
SQLRETURN SQL_API SQLSetStmtAttrW(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  return SQLSetStmtAttr(StatementHandle, Attribute, ValuePtr, StringLength); 
}
/* }}} */


/* {{{ SQLSetStmtOption */
SQLRETURN  SQL_API SQLSetStmtOption(SQLHSTMT StatementHandle,
                                    SQLUSMALLINT Option, SQLULEN Value)
{
  return SQLSetStmtAttr(StatementHandle, Option, (SQLPOINTER)Value, SQL_NTS);
}
/* }}} */

/* {{{ SQLSpecialColumns */
SQLRETURN SQL_API SQLSpecialColumns(SQLHSTMT StatementHandle,
    SQLUSMALLINT IdentifierType,
    SQLCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLUSMALLINT Scope,
    SQLUSMALLINT Nullable)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  return Stmt->Methods->SpecialColumns(Stmt,IdentifierType, (char *)CatalogName, NameLength1, 
                                       (char *)SchemaName, NameLength2,
                                       (char *)TableName, NameLength3, Scope, Nullable);
}
/* }}} */

/* {{{ SQLSpecialColumnsW */
SQLRETURN SQL_API SQLSpecialColumnsW(SQLHSTMT StatementHandle,
    SQLUSMALLINT IdentifierType,
    SQLWCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLUSMALLINT Scope,
    SQLUSMALLINT Nullable)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpTable= NULL;
  SQLINTEGER CpLength1, CpLength2, CpLength3;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->CodePage, NULL);

  ret= Stmt->Methods->SpecialColumns(Stmt,IdentifierType, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, 
                                     (SQLSMALLINT)CpLength2, CpTable, (SQLSMALLINT)CpLength3, Scope, Nullable);
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  return ret;
}
/* }}} */


/* {{{ SQLStatistics */
SQLRETURN SQL_API SQLStatistics(SQLHSTMT StatementHandle,
    SQLCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLUSMALLINT Unique,
    SQLUSMALLINT Reserved)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  return Stmt->Methods->Statistics(Stmt, (char *)CatalogName, NameLength1, (char *)SchemaName, NameLength2,
                                   (char *)TableName, NameLength3, Unique, Reserved);
}
/* }}} */

/* {{{ SQLStatistics */
SQLRETURN SQL_API SQLStatisticsW(SQLHSTMT StatementHandle,
    SQLWCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLUSMALLINT Unique,
    SQLUSMALLINT Reserved)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpTable= NULL;
  SQLINTEGER CpLength1, CpLength2, CpLength3;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->CodePage, NULL);

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  ret= Stmt->Methods->Statistics(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2, 
                                 CpTable, (SQLSMALLINT)CpLength3, Unique, Reserved);
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  return ret;
}
/* }}} */


/* {{{ SQLTablePrivileges */
SQLRETURN SQL_API SQLTablePrivileges(SQLHSTMT StatementHandle,
    SQLCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *TableName,
    SQLSMALLINT NameLength3)
{
    MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  return Stmt->Methods->TablePrivileges(Stmt, (char *)CatalogName, NameLength1, (char *)SchemaName, NameLength2,
                                        (char *)TableName, NameLength3);
  }
/* }}} */

/* {{{ SQLTablePrivilegesW */
SQLRETURN SQL_API SQLTablePrivilegesW(SQLHSTMT StatementHandle,
    SQLWCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *TableName,
    SQLSMALLINT NameLength3)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpTable= NULL;
  SQLINTEGER CpLength1, CpLength2, CpLength3;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->CodePage, NULL);

  ret= Stmt->Methods->TablePrivileges(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
                                      CpTable, (SQLSMALLINT)CpLength3);

  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  return ret;
}
/* }}} */


/* {{{ SQLTables */
SQLRETURN SQL_API SQLTables(SQLHSTMT StatementHandle,
    SQLCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLCHAR *TableType,
    SQLSMALLINT NameLength4)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->Tables(Stmt, (char *)CatalogName, NameLength1, (char *)SchemaName, NameLength2, 
                               (char *)TableName,NameLength3, (char *)TableType, NameLength4);
}
/* }}} */

/* {{{ SQLTablesW */
SQLRETURN SQL_API SQLTablesW(SQLHSTMT StatementHandle,
    SQLWCHAR *CatalogName,
    SQLSMALLINT NameLength1,
    SQLWCHAR *SchemaName,
    SQLSMALLINT NameLength2,
    SQLWCHAR *TableName,
    SQLSMALLINT NameLength3,
    SQLWCHAR *TableType,
    SQLSMALLINT NameLength4)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpTable= NULL,
       *CpType= NULL;
  SQLINTEGER CpLength1, CpLength2, CpLength3, CpLength4;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->CodePage, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->CodePage, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->CodePage, NULL);
  CpType= MADB_ConvertFromWChar(TableType, NameLength4, &CpLength4, Stmt->Connection->CodePage, NULL);

  ret= Stmt->Methods->Tables(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2, 
                               CpTable, (SQLSMALLINT)CpLength3, CpType, (SQLSMALLINT)CpLength4);
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  MADB_FREE(CpType);
  return ret;
}
/* }}} */


SQLRETURN SQL_API SQLSetScrollOptions(SQLHSTMT hstmt,
    SQLUSMALLINT Concurrency,
	SQLLEN crowKeySet,
	SQLUSMALLINT crowRowSet)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)hstmt;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return MADB_DescSetField(Stmt->Ard, 0, SQL_DESC_ARRAY_SIZE, (SQLPOINTER)(SQLUINTEGER)crowKeySet, SQL_IS_USMALLINT, 0);
}

SQLRETURN SQL_API SQLParamOptions(
    SQLHSTMT hstmt,
    SQLULEN  crow,
    SQLULEN  *pirow)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)hstmt;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return MADB_DescSetField(Stmt->Apd, 0, SQL_DESC_ARRAY_SIZE, (SQLPOINTER)(SQLUINTEGER)crow, SQL_IS_USMALLINT, 0);
}
