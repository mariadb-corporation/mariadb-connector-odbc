/************************************************************************************
   Copyright (C) 2013,2016 MariaDB Corporation AB
   
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

extern Client_Charset utf8;

/* {{{ SQLAllocHandle */
SQLRETURN MA_SQLAllocHandle(SQLSMALLINT HandleType,
    SQLHANDLE InputHandle,
    SQLHANDLE *OutputHandlePtr)
{
  SQLRETURN ret= SQL_ERROR;

  switch(HandleType) {
    case SQL_HANDLE_DBC:
      EnterCriticalSection(&((MADB_Env *)InputHandle)->cs);
      MADB_CLEAR_ERROR(&((MADB_Env *)InputHandle)->Error);
      if (*OutputHandlePtr = (SQLHANDLE)MADB_DbcInit((MADB_Env *)InputHandle))
        ret= SQL_SUCCESS;
      LeaveCriticalSection(&((MADB_Env *)InputHandle)->cs);
      break;
    case SQL_HANDLE_DESC:
      EnterCriticalSection(&((MADB_Dbc *)InputHandle)->cs);
      MADB_CLEAR_ERROR(&((MADB_Dbc *)InputHandle)->Error);
      if (*OutputHandlePtr = (SQLHANDLE)MADB_DescInit((MADB_Dbc *)InputHandle, MADB_DESC_UNKNOWN, TRUE))
        ret= SQL_SUCCESS;
      LeaveCriticalSection(&((MADB_Dbc *)InputHandle)->cs);
      break;
    case SQL_HANDLE_ENV:
      if ((*OutputHandlePtr = (SQLHANDLE)MADB_EnvInit()))
      {
        ret= SQL_SUCCESS;
      }
      break;
    case SQL_HANDLE_STMT:
      {
        MADB_Dbc *Connection= (MADB_Dbc *)InputHandle;
        MDBUG_C_ENTER(InputHandle, "MA_SQLAllocHandle(Stmt)");
        MDBUG_C_DUMP(InputHandle, InputHandle, 0x);
        MDBUG_C_DUMP(InputHandle, OutputHandlePtr, 0x);

        MADB_CLEAR_ERROR(&Connection->Error);
       
        if (!CheckConnection(Connection))
        {
          MADB_SetError(&Connection->Error, MADB_ERR_08003, NULL, 0);
          break;
        }

        ret= MADB_StmtInit(Connection, OutputHandlePtr);
        MDBUG_C_DUMP(InputHandle, *OutputHandlePtr, 0x);
        MDBUG_C_RETURN(InputHandle,ret, &Connection->Error);
      }
      break;
    default:
      /* todo: set error message */
      break;
  }
  return ret;
}
/* }}} */

/* {{{ SQLAllocHandle */
SQLRETURN SQL_API SQLAllocHandle(SQLSMALLINT HandleType,
    SQLHANDLE InputHandle,
    SQLHANDLE *OutputHandlePtr)
{
  SQLRETURN ret;
  MDBUG_ENTER("SQLAllocHandle");
  MDBUG_DUMP(HandleType, d);
  MDBUG_DUMP(InputHandle, 0x);
  MDBUG_DUMP(OutputHandlePtr, 0x);
  
  ret= MA_SQLAllocHandle(HandleType, InputHandle, OutputHandlePtr);

  MDBUG_DUMP(ret,d);
  MDBUG_RETURN(ret);
}
/* }}} */

/* {{{ SQLAllocConnect */
SQLRETURN MA_SQLAllocConnect(SQLHANDLE InputHandle,
                             SQLHANDLE *OutputHandlePtr)
{
  return MA_SQLAllocHandle(SQL_HANDLE_DBC, InputHandle, OutputHandlePtr);
}
SQLRETURN SQL_API SQLAllocConnect(SQLHANDLE InputHandle,
                                  SQLHANDLE *OutputHandlePtr)
{
  return MA_SQLAllocConnect(InputHandle, OutputHandlePtr);
}
/* }}} */

/* {{{ SQLAllocStmt */
SQLRETURN SQL_API SQLAllocStmt(SQLHANDLE InputHandle,
                               SQLHANDLE *OutputHandlePtr)
{
  MDBUG_C_ENTER(InputHandle, "SQLAllocStmt");
  MDBUG_C_DUMP(InputHandle, InputHandle, 0x);
  MDBUG_C_DUMP(InputHandle, OutputHandlePtr, 0x);

  return MA_SQLAllocHandle(SQL_HANDLE_STMT, InputHandle, OutputHandlePtr);
}
/* }}} */

/* {{{ SQLAllocEnv */
SQLRETURN SQL_API SQLAllocEnv(SQLHANDLE *OutputHandlePtr)
{
  return MA_SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, OutputHandlePtr);
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
  MADB_CLEAR_ERROR(&Stmt->Error);
  MADB_CHECK_STMT_HANDLE(Stmt,stmt);  

  MDBUG_C_ENTER(Stmt->Connection, "SQLBindCol");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  MDBUG_C_DUMP(Stmt->Connection, TargetType, d);
  MDBUG_C_DUMP(Stmt->Connection, BufferLength, d);
  MDBUG_C_DUMP(Stmt->Connection, StrLen_or_Ind, 0x);

  ret= Stmt->Methods->BindColumn(Stmt, ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_Ind);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLBindParameter */
SQLRETURN MA_SQLBindParameter(SQLHSTMT StatementHandle,
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

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&((MADB_Stmt*)StatementHandle)->Error);
  return MA_SQLBindParameter(StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType,
                             ColumnSize, DecimalDigits, ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
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

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
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

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
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
  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLBulkOperations");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, Operation, d);

  ret= Stmt->Methods->BulkOperations(Stmt, Operation);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLCancel */
SQLRETURN MA_SQLCancel(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret= SQL_ERROR;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLCancel");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);

  if (TryEnterCriticalSection(&Stmt->Connection->cs))
  {
    LeaveCriticalSection(&Stmt->Connection->cs);
    ret= Stmt->Methods->StmtFree(Stmt, SQL_CLOSE);

    MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
  } else
  {
    MYSQL *MariaDb, *Kill=Stmt->Connection->mariadb;
    
    char StmtStr[30];

    if (!(MariaDb= mysql_init(NULL)))
    {
      ret= SQL_ERROR;
      goto end;
    }
    if (!(mysql_real_connect(MariaDb, Kill->host, Kill->user, Kill->passwd,
                             "", Kill->port, Kill->unix_socket, 0)))
    {
      mysql_close(MariaDb);
      goto end;
    }
    
    _snprintf(StmtStr, 30, "KILL QUERY %ld", mysql_thread_id(Kill));
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

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}

SQLRETURN SQL_API SQLCancel(SQLHSTMT StatementHandle)
{
  return MA_SQLCancel(StatementHandle);
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
      Stmt.Connection= (MADB_Dbc *)Handle;
      return MA_SQLCancel((SQLHSTMT)&Stmt);
    }
    break;
  case SQL_HANDLE_STMT:
    return MA_SQLCancel((SQLHSTMT)Handle);
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

  MADB_CLEAR_ERROR(&Stmt->Error);

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
    ret= MA_SQLFreeStmt(StatementHandle, SQL_CLOSE);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLColAttribute */
SQLRETURN SQL_API SQLColAttribute (SQLHSTMT StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier,
    SQLPOINTER CharacterAttributePtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr,
#ifdef SQLCOLATTRIB_SQLPOINTER
    SQLPOINTER NumericAttributePtr
#else
    SQLLEN *NumericAttributePtr
#endif
    )
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

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

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLColAttributeW */
SQLRETURN SQL_API SQLColAttributeW (SQLHSTMT StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier,
    SQLPOINTER CharacterAttributePtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr,
#ifdef SQLCOLATTRIB_SQLPOINTER
    SQLPOINTER NumericAttributePtr
#else
    SQLLEN *NumericAttributePtr
#endif
    )
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret;
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

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

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLColAttributes */
SQLRETURN SQL_API SQLColAttributes(SQLHSTMT hstmt, 
	SQLUSMALLINT icol,
	SQLUSMALLINT fDescType,
	SQLPOINTER rgbDesc,
	SQLSMALLINT cbDescMax,
	SQLSMALLINT * pcbDesc,
	SQLLEN * pfDesc)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)hstmt;
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

  return Stmt->Methods->ColAttribute(Stmt, icol, MapColAttributeDescType(fDescType), rgbDesc,
                                     cbDescMax, pcbDesc, pfDesc, FALSE);
}
/* }}} */

/* {{{ SQLColAttributesW */
SQLRETURN SQL_API SQLColAttributesW(SQLHSTMT hstmt, 
	SQLUSMALLINT icol,
	SQLUSMALLINT fDescType,
	SQLPOINTER rgbDesc,
	SQLSMALLINT cbDescMax,
	SQLSMALLINT * pcbDesc,
	SQLLEN * pfDesc)
{
  /* TODO: use internal function, not api */
  return SQLColAttributeW(hstmt, icol, MapColAttributeDescType(fDescType), rgbDesc, cbDescMax, pcbDesc, pfDesc);
}
/* }}} */

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

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumnPrivileges");
  ret= Stmt->Methods->ColumnPrivileges(Stmt, (char *)CatalogName, NameLength1, (char *)SchemaName, NameLength2,
                                         (char *)TableName, NameLength3, (char *)ColumnName, NameLength4);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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
  SQLULEN CpLength1, CpLength2, CpLength3, CpLength4;
  char *CpCatalog= NULL,
       *CpSchema= NULL,
       *CpTable= NULL,
       *CpColumn= NULL;
  SQLRETURN ret;

  if (!StatementHandle)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumnPrivilegesW");

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpColumn= MADB_ConvertFromWChar(ColumnName, NameLength4, &CpLength4, Stmt->Connection->ConnOrSrcCharset, NULL);

  ret= Stmt->Methods->ColumnPrivileges(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
                                       CpTable, (SQLSMALLINT)CpLength3, CpColumn, (SQLSMALLINT)CpLength4);
    
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  MADB_FREE(CpColumn);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumns");

  ret= Stmt->Methods->Columns(Stmt, (char *)CatalogName,NameLength1, (char *)SchemaName, NameLength2,
                                (char *)TableName, NameLength3, (char *)ColumnName, NameLength4);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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
  SQLULEN CpLength1, CpLength2, CpLength3, CpLength4;
  SQLRETURN ret;
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumns");

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpSchema=  MADB_ConvertFromWChar(SchemaName,  NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpTable=   MADB_ConvertFromWChar(TableName,   NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpColumn=  MADB_ConvertFromWChar(ColumnName,  NameLength4, &CpLength4, Stmt->Connection->ConnOrSrcCharset, NULL);

  ret= Stmt->Methods->Columns(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
                              CpTable, (SQLSMALLINT)CpLength3, CpColumn, (SQLSMALLINT)CpLength4);
    
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  MADB_FREE(CpColumn);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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

/* {{{ SQLConnectCommon */
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

  MADB_CLEAR_ERROR(&Connection->Error);

  MDBUG_C_ENTER(Connection, "SQLConnect");
  MDBUG_C_DUMP(Connection, Connection, 0x);
  MDBUG_C_DUMP(Connection, ServerName, s);
  MDBUG_C_DUMP(Connection, NameLength1, d);
  MDBUG_C_DUMP(Connection, UserName, s);
  MDBUG_C_DUMP(Connection, NameLength2, d);
  MDBUG_C_DUMP(Connection, Authentication, s);
  MDBUG_C_DUMP(Connection, NameLength3, d);

  if (CheckConnection(Connection))
  {
    MADB_SetError(&Connection->Error, MADB_ERR_08002, NULL, 0);
    return SQL_ERROR;
  }

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
    MADB_DSN_Free(Connection->Dsn);
    Connection->Dsn= Dsn;
  }
  else
  {
    MADB_DSN_Free(Dsn);
  }

  MDBUG_C_RETURN(Connection, ret, &Connection->Error);
}
/* }}} */

/* {{{ SQLConnect */
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
/* }}} */

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
  MADB_Dbc *Dbc= (MADB_Dbc*)ConnectionHandle;
  
  if (!Dbc)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Dbc->Error);

   /* Convert parameters to Cp */
  if (ServerName)
    MBServerName= MADB_ConvertFromWChar(ServerName, NameLength1, 0, Dbc->IsAnsi ? Dbc->ConnOrSrcCharset : &utf8, NULL);
  if (UserName)
    MBUserName= MADB_ConvertFromWChar(UserName, NameLength2, 0, Dbc->IsAnsi ? Dbc->ConnOrSrcCharset : &utf8, NULL);
  if (Authentication)
    MBAuthentication= MADB_ConvertFromWChar(Authentication, NameLength3, 0, Dbc->IsAnsi ? Dbc->ConnOrSrcCharset : &utf8, NULL);
  
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
  /*TODO: clear error */
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

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLDescribeCol");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  
  ret= Stmt->Methods->DescribeCol(Stmt, ColumnNumber, (void *)ColumnName, BufferLength,
                                    NameLengthPtr, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, 
                                    NullablePtr, FALSE);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLDescribeColW");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  
  ret= Stmt->Methods->DescribeCol(Stmt, ColumnNumber, (void *)ColumnName, BufferLength,
                                    NameLengthPtr, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, 
                                    NullablePtr, TRUE);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

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
  MADB_List *Element, *NextElement;

  if (!Connection)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Connection->Error);

  MDBUG_C_ENTER(Connection, "SQLDisconnect");
  MDBUG_C_DUMP(Connection, ConnectionHandle, 0x);

  /* Close all statements */
  for (Element= Connection->Stmts; Element; Element= NextElement)
  {
    NextElement= Element->next;
    MA_SQLFreeStmt((SQLHSTMT)Element->data, SQL_DROP);
  }

  /* Close all explicitly allocated descriptors */
  for (Element= Connection->Descrs; Element; Element= NextElement)
  {
    NextElement= Element->next;
    MADB_DescFree((MADB_Desc*)Element->data, FALSE);
  }

  if (Connection->mariadb)
  {
    mysql_close(Connection->mariadb);
    Connection->mariadb= NULL;
    ret= SQL_SUCCESS;
  }
  else
  {
    MADB_SetError(&Connection->Error, MADB_ERR_08003, NULL, 0);
    ret= Connection->Error.ReturnValue;
  }
  Connection->ConnOrSrcCharset= NULL;

  MDBUG_C_RETURN(Connection, ret, &Connection->Error);
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

  MADB_CLEAR_ERROR(&Dbc->Error);

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

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ SQLDriverConnectW */
SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC      ConnectionHandle,
                                    SQLHWND      WindowHandle,
                                    SQLWCHAR    *InConnectionString,
                                    SQLSMALLINT  StringLength1,
                                    SQLWCHAR    *OutConnectionString,
                                    SQLSMALLINT  BufferLength,
                                    SQLSMALLINT *StringLength2Ptr,
                                    SQLUSMALLINT DriverCompletion)
{
  SQLRETURN   ret=          SQL_ERROR;
  SQLSMALLINT Length=       0;
  char        *InConnStrA=  NULL;
  SQLULEN     InStrAOctLen= 0;
  char        *OutConnStrA= NULL;
  MADB_Dbc    *Dbc=         (MADB_Dbc *)ConnectionHandle;
   
  if (!ConnectionHandle)
    return SQL_INVALID_HANDLE;

  MDBUG_C_ENTER(Dbc, "SQLDriverConnectW");

  MADB_CLEAR_ERROR(&Dbc->Error);

  InConnStrA= MADB_ConvertFromWChar(InConnectionString, StringLength1, &InStrAOctLen, Dbc->IsAnsi ? Dbc->ConnOrSrcCharset : &utf8, NULL);
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
    Length= BufferLength*4 /*Max bytes per utf8 character */;
    OutConnStrA= (char *)MADB_CALLOC(Length);

    if (OutConnStrA == NULL)
    {
      ret= MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
      goto end;
    }
  }

  ret= Dbc->Methods->DriverConnect(Dbc, WindowHandle, (SQLCHAR *)InConnStrA, (SQLSMALLINT)InStrAOctLen, (SQLCHAR *)OutConnStrA,
                                     Length, StringLength2Ptr, DriverCompletion); 
  MDBUG_C_DUMP(Dbc, ret, d);
  if (!SQL_SUCCEEDED(ret))
    goto end;

  if (OutConnectionString)
  {
    Length= (SQLSMALLINT)MADB_SetString(&utf8, OutConnectionString, BufferLength,
                                        OutConnStrA, SQL_NTS, &((MADB_Dbc *)ConnectionHandle)->Error);
    if (StringLength2Ptr)
      *StringLength2Ptr= Length;
  }
  
end:
  MADB_FREE(OutConnStrA);
  MADB_FREE(InConnStrA);
  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
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

/* {{{ MA_SQLEndTran */
SQLRETURN MA_SQLEndTran(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT CompletionType)
{
  SQLRETURN ret= SQL_SUCCESS;
  switch (HandleType) {
  case SQL_HANDLE_ENV:
    {
      MADB_Env *Env= (MADB_Env *)Handle;
      MADB_List *List= Env->Dbcs;

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

SQLRETURN SQL_API SQLEndTran(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT CompletionType)
{
  MADB_CHECK_HANDLE_CLEAR_ERROR(HandleType, Handle);
  return MA_SQLEndTran(HandleType, Handle, CompletionType);
}
/* }}} */

/* {{{ SQLError */
SQLRETURN SQL_API SQLError(SQLHENV Env, SQLHDBC Dbc, SQLHSTMT Stmt, 
                           SQLCHAR *Sqlstate, SQLINTEGER *NativeError, 
                           SQLCHAR *Message, SQLSMALLINT MessageMax,
                           SQLSMALLINT *MessageLen)
{
  SQLSMALLINT HandleType= 0;
  SQLHANDLE   Handle=     NULL;
  MADB_Error *error;

  if (Stmt)
  {
    MDBUG_C_ENTER(((MADB_Stmt*)Stmt)->Connection, "SQLError->SQLGetDiagRec");

    MDBUG_C_DUMP(((MADB_Stmt*)Stmt)->Connection, Env, 0x);
    MDBUG_C_DUMP(((MADB_Stmt*)Stmt)->Connection, Dbc, 0x);
    MDBUG_C_DUMP(((MADB_Stmt*)Stmt)->Connection, Stmt, 0x);

    Handle= Stmt;
    HandleType= SQL_HANDLE_STMT;
    error= &((MADB_Stmt*)Stmt)->Error;
  }
  else if (Dbc)
  {
    MDBUG_C_ENTER((MADB_Dbc*)Dbc, "SQLError->SQLGetDiagRec");

    MDBUG_C_DUMP((MADB_Dbc*)Dbc, Env, 0x);
    MDBUG_C_DUMP((MADB_Dbc*)Dbc, Dbc, 0x);
    MDBUG_C_DUMP((MADB_Dbc*)Dbc, Stmt, 0x);

    Handle= Dbc;
    HandleType= SQL_HANDLE_DBC;
    error= &((MADB_Dbc*)Dbc)->Error;
  }
  else
  {
    MDBUG_ENTER("SQLError->SQLGetDiagRec");
    MDBUG_DUMP(Env, 0x);
    MDBUG_DUMP(Dbc, 0x);
    MDBUG_DUMP(Stmt, 0x);

    Handle= Env;
    HandleType= SQL_HANDLE_ENV;
    error= &((MADB_Env*)Env)->Error;
  }

  return MA_SQLGetDiagRec(HandleType, Handle, ++error->ErrorNum, Sqlstate, NativeError, Message, MessageMax, MessageLen);
}
/* }}} */

/* {{{ MA_SQLGetDiagRecW */
SQLRETURN SQL_API MA_SQLGetDiagRecW(SQLSMALLINT HandleType,
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

/*{{{ SQLErrorW */
SQLRETURN SQL_API
SQLErrorW(SQLHENV Env, SQLHDBC Dbc, SQLHSTMT Stmt, SQLWCHAR *Sqlstate,
          SQLINTEGER *NativeError, SQLWCHAR *Message, SQLSMALLINT MessageMax,
          SQLSMALLINT *MessageLen)

{
  SQLSMALLINT HandleType= 0;
  SQLHANDLE   Handle=     NULL;
  MADB_Error *error;

    if (Stmt)
  {
    Handle= Stmt;
    HandleType= SQL_HANDLE_STMT;
    error= &((MADB_Stmt*)Stmt)->Error;
  }
  else if (Dbc)
  {
    Handle= Dbc;
    HandleType= SQL_HANDLE_DBC;
    error= &((MADB_Dbc*)Dbc)->Error;
  }
  else
  {
    Handle= Env;
    HandleType= SQL_HANDLE_ENV;
    error= &((MADB_Env*)Env)->Error;
  }

  return MA_SQLGetDiagRecW(HandleType, Handle, ++error->ErrorNum, Sqlstate, NativeError, Message, MessageMax, MessageLen);
}
/* }}} */

/* {{{ SQLTransact */
SQLRETURN SQL_API SQLTransact(SQLHENV Env, SQLHDBC Dbc, SQLUSMALLINT CompletionType)
{
  if (Env != SQL_NULL_HENV)
  {
    MADB_CLEAR_ERROR(&((MADB_Env*)Env)->Error);
    return MA_SQLEndTran(SQL_HANDLE_ENV, Env, CompletionType);
  }
  else if (Dbc != SQL_NULL_HDBC)
  {
    MADB_CLEAR_ERROR(&((MADB_Dbc*)Dbc)->Error);
    return MA_SQLEndTran(SQL_HANDLE_DBC, Dbc, CompletionType);
  }
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

  if (!Stmt)
    ret= SQL_INVALID_HANDLE;
  else
    ret= Stmt->Methods->ExecDirect(Stmt, (char *)StatementText, TextLength);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLExecDirectW */
SQLRETURN SQL_API SQLExecDirectW(SQLHSTMT StatementHandle,
    SQLWCHAR *StatementText,
    SQLINTEGER TextLength)
{
  char      *CpStmt;
  SQLULEN   StmtLength;
  SQLRETURN ret;
  BOOL      ConversionError;

  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLExecDirectW");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);

  CpStmt= MADB_ConvertFromWChar(StatementText, TextLength, &StmtLength, Stmt->Connection->ConnOrSrcCharset, &ConversionError);
  MDBUG_C_DUMP(Stmt->Connection, CpStmt, s);
  if (ConversionError)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_22018, NULL, 0);
    ret= Stmt->Error.ReturnValue;
  }
  else
    ret= Stmt->Methods->ExecDirect(Stmt, CpStmt, (SQLINTEGER)StmtLength);
  MADB_FREE(CpStmt);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

SQLRETURN SQL_API SQLExecute(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (StatementHandle == SQL_NULL_HSTMT)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLExecute");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);

  return Stmt->Methods->Execute(Stmt, FALSE);
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

  SQLULEN *SaveRowsProcessedPtr= Stmt->Ird->Header.RowsProcessedPtr;
  SQLUSMALLINT *SaveArrayStatusPtr= Stmt->Ird->Header.ArrayStatusPtr;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLExtendedFetch");
  MDBUG_C_DUMP(Stmt->Connection, FetchOrientation, u);
  MDBUG_C_DUMP(Stmt->Connection, FetchOffset, d);
  MDBUG_C_DUMP(Stmt->Connection, RowCountPtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, RowStatusArray, 0x);
   
  Stmt->Ird->Header.RowsProcessedPtr= RowCountPtr;
  Stmt->Ird->Header.ArrayStatusPtr= RowStatusArray;
  ret=  Stmt->Methods->FetchScroll(Stmt, FetchOrientation, FetchOffset);

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

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLFetch */
SQLRETURN SQL_API SQLFetch(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt;
  
  if (StatementHandle == SQL_NULL_HSTMT)
    return SQL_INVALID_HANDLE;

  Stmt= (MADB_Stmt *)StatementHandle;

  MDBUG_C_ENTER(Stmt->Connection, "SQLFetch");
  MADB_CLEAR_ERROR(&Stmt->Error);

  /* SQLFetch is equivalent of SQLFetchScroll(SQL_FETCH_NEXT), 3rd parameter is ignored for SQL_FETCH_NEXT */
  MDBUG_C_RETURN(Stmt->Connection, Stmt->Methods->FetchScroll(Stmt, SQL_FETCH_NEXT, 1), &Stmt->Error);
}
/* }}} */

/* {{{ SQLFetchScroll */
SQLRETURN SQL_API SQLFetchScroll(SQLHSTMT StatementHandle,
    SQLSMALLINT FetchOrientation,
    SQLLEN FetchOffset)
{
  MADB_Stmt *Stmt;

  if (StatementHandle == SQL_NULL_HSTMT)
    return SQL_INVALID_HANDLE;

  Stmt= (MADB_Stmt *)StatementHandle;

  MDBUG_C_ENTER(Stmt->Connection, "SQLFetchScroll");
  MDBUG_C_DUMP(Stmt->Connection, FetchOrientation, d);

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_RETURN(Stmt->Connection, Stmt->Methods->FetchScroll(Stmt, FetchOrientation, FetchOffset), &Stmt->Error);
}
/* }}} */

/* {{{ SQLFreeHandle */
SQLRETURN SQL_API SQLFreeHandle(SQLSMALLINT HandleType,
                                SQLHANDLE Handle)
{
  SQLRETURN ret;
  MADB_CHECK_HANDLE_CLEAR_ERROR(HandleType, Handle);

  switch (HandleType)
  {
  case SQL_HANDLE_ENV:
      MDBUG_ENTER("SQLFreeHandle");
      MDBUG_DUMP(HandleType, d);
      MDBUG_DUMP(Handle, 0x);

      ret= MADB_EnvFree((MADB_Env *)Handle);
      break;
  case SQL_HANDLE_DBC:
    {
      MADB_Dbc *Dbc= (MADB_Dbc *)Handle;

      MDBUG_C_ENTER(Dbc, "SQLFreeHandle");
      MDBUG_C_DUMP(Dbc, HandleType, d);
      MDBUG_C_DUMP(Dbc, Handle, 0x);

      ret= MADB_DbcFree(Dbc);
      return ret;
      /*MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);*/
    }
  case SQL_HANDLE_DESC:
    {
      MADB_Desc *Desc= (MADB_Desc *)Handle;
      MADB_Dbc  *Dbc=  Desc->Dbc;

      MDBUG_C_ENTER(Dbc, "SQLFreeHandle");
      MDBUG_C_DUMP(Dbc, HandleType, d);
      MDBUG_C_DUMP(Dbc, Handle, 0x);

      /* Error if the descriptor does not belong to application(was automatically alliocated by the driver)
         Basically DM is supposed to take care of this. Keeping in mind direct linking */
      if (!Desc->AppType)
      {
        MADB_SetError(&Desc->Error, MADB_ERR_HY017, NULL, 0);
        MDBUG_C_RETURN(Dbc, Desc->Error.ReturnValue, &Desc->Error);
      }
      ret= MADB_DescFree(Desc, FALSE);
      MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
    }
  case SQL_HANDLE_STMT:
    {
      MADB_Stmt *Stmt= (MADB_Stmt *)Handle;
      MADB_Dbc  *Dbc=  Stmt->Connection;

      MDBUG_C_ENTER(Dbc, "SQLFreeHandle");
      MDBUG_C_DUMP(Dbc, HandleType, d);
      MDBUG_C_DUMP(Dbc, Handle, 0x);

      ret= MA_SQLFreeStmt(Stmt, SQL_DROP);

      MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
    }
  }

  MDBUG_RETURN(ret);
}
/* }}} */

/* {{{ SQLFreeEnv */
SQLRETURN SQL_API SQLFreeEnv(SQLHANDLE henv)
{
  if (henv == SQL_NULL_HENV)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&((MADB_Env*)henv)->Error);

  return MADB_EnvFree((MADB_Env *)henv);
}
/* }}} */

/* {{{ SQLFreeConnect */
SQLRETURN SQL_API SQLFreeConnect(SQLHANDLE hdbc)
{
  if (hdbc == SQL_NULL_HDBC)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&((MADB_Dbc*)hdbc)->Error);

  return MADB_DbcFree((MADB_Dbc*)hdbc);
}
/* }}} */

/* {{{ SQLFreeStmt */
SQLRETURN MA_SQLFreeStmt(SQLHSTMT StatementHandle,
           SQLUSMALLINT Option)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  MDBUG_C_PRINT(Stmt->Connection, "%sMA_SQLFreeStmt","\t->");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, Option, d);

  return Stmt->Methods->StmtFree(Stmt, Option);
}

SQLRETURN SQL_API SQLFreeStmt(SQLHSTMT StatementHandle,
           SQLUSMALLINT Option)
{
  if (StatementHandle== SQL_NULL_HSTMT)
    return SQL_INVALID_HANDLE;
  MDBUG_C_ENTER(((MADB_Stmt*)StatementHandle)->Connection, "SQLFreeStmt");
  MADB_CLEAR_ERROR(&((MADB_Stmt*)StatementHandle)->Error);

  return MA_SQLFreeStmt(StatementHandle, Option);
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
  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLForeignKeys");

  ret= Stmt->Methods->ForeignKeys(Stmt, (char *)PKCatalogName, NameLength1, (char *)PKSchemaName, NameLength2,
                                    (char *)PKTableName, NameLength3, (char *)FKCatalogName, NameLength4,
                                    (char *)FKSchemaName, NameLength4, (char *)FKTableName, NameLength6);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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
  SQLULEN CpLength1, CpLength2, CpLength3,
             CpLength4, CpLength5, CpLength6;
  SQLRETURN ret;
  if(!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLForeignKeysW");

  CpPkCatalog= MADB_ConvertFromWChar(PKCatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpPkSchema= MADB_ConvertFromWChar(PKSchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpPkTable= MADB_ConvertFromWChar(PKTableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpFkCatalog= MADB_ConvertFromWChar(FKCatalogName, NameLength4, &CpLength4, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpFkSchema= MADB_ConvertFromWChar(FKSchemaName, NameLength5, &CpLength5, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpFkTable= MADB_ConvertFromWChar(FKTableName, NameLength6, &CpLength6, Stmt->Connection->ConnOrSrcCharset, NULL);

  ret= Stmt->Methods->ForeignKeys(Stmt, CpPkCatalog, (SQLSMALLINT)CpLength1, CpPkSchema, (SQLSMALLINT)CpLength2,
                                  CpPkTable, (SQLSMALLINT)CpLength3, CpFkCatalog, (SQLSMALLINT)CpLength4, 
                                  CpFkSchema, (SQLSMALLINT)CpLength5, CpFkTable, (SQLSMALLINT)CpLength6);
  MADB_FREE(CpPkCatalog);
  MADB_FREE(CpPkSchema);
  MADB_FREE(CpPkTable);
  MADB_FREE(CpFkCatalog);
  MADB_FREE(CpFkSchema);
  MADB_FREE(CpFkTable);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLGetConnectAttr */
SQLRETURN MA_SQLGetConnectAttr(SQLHDBC ConnectionHandle,
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

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}


SQLRETURN SQL_API SQLGetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  if (ConnectionHandle == SQL_NULL_HDBC)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&((MADB_Dbc *)ConnectionHandle)->Error);

  return MA_SQLGetConnectAttr(ConnectionHandle, Attribute, ValuePtr, BufferLength, StringLengthPtr);
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
  MADB_CLEAR_ERROR(&Dbc->Error);

  MDBUG_C_ENTER(Dbc, "SQLGetConnectAttr");
  MDBUG_C_DUMP(Dbc, Attribute, d);
  MDBUG_C_DUMP(Dbc, ValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, BufferLength, d);
  MDBUG_C_DUMP(Dbc, StringLengthPtr, 0x);

  ret= Dbc->Methods->GetAttr(Dbc, Attribute, ValuePtr, BufferLength, StringLengthPtr, TRUE);

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ SQLGetConnectOption */
SQLRETURN SQL_API SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER ValuePtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  
  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Dbc->Error);

  return MA_SQLGetConnectAttr(ConnectionHandle, Option, ValuePtr,
                           Option == SQL_ATTR_CURRENT_CATALOG ? SQL_MAX_OPTION_STRING_LENGTH : 0, NULL);
}
/* }}} */

/* {{{ SQLGetConnectOptionW */
SQLRETURN SQL_API SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER ValuePtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Dbc->Error);
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
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  MADB_Stmt *Stmt= (MADB_Stmt*)StatementHandle;
  unsigned int i;
  MADB_DescRecord *IrdRec;

  if (StatementHandle== SQL_NULL_HSTMT)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  /* In case we don't have DM(it check for that) */
  if (TargetValuePtr == NULL)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY009, NULL, 0);
  }

  /* Bookmark */
  if (Col_or_Param_Num == 0)
  {
    return MADB_GetBookmark(Stmt, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
  }

  /* We don't need this to be checked in case of "internal" use of the GetData, i.e. for internal needs we should always get the data */
  if ( Stmt->CharOffset[Col_or_Param_Num - 1] > 0
    && Stmt->CharOffset[Col_or_Param_Num - 1] >= Stmt->Lengths[Col_or_Param_Num - 1])
  {
    return SQL_NO_DATA;
  }

  if (BufferLength < 0)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY090, NULL, 0);
  }

  /* reset offsets for other columns. Doing that here since "internal" calls should not do that */
  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    if (i != Col_or_Param_Num - 1)
    {
      IrdRec= MADB_DescGetInternalRecord(Stmt->Ird, i, MADB_DESC_READ);
      if (IrdRec)
      {
        MADB_FREE(IrdRec->InternalBuffer);
      }
      Stmt->CharOffset[i]= 0;
    }
  }

  return Stmt->Methods->GetData(StatementHandle, Col_or_Param_Num, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr, FALSE);
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
  MADB_CLEAR_ERROR(&((MADB_Desc*)DescriptorHandle)->Error);

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
  if (!DescriptorHandle)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&((MADB_Desc*)DescriptorHandle)->Error);

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
  MADB_CLEAR_ERROR(&Desc->Error);

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
  MADB_CLEAR_ERROR(&Desc->Error);

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
SQLRETURN MA_SQLGetDiagRec(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLCHAR *SQLState,
    SQLINTEGER *NativeErrorPtr,
    SQLCHAR *MessageText,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *TextLengthPtr)
{
  SQLRETURN ret= SQL_ERROR;

  if (!Handle)
    MDBUG_RETURN(SQL_INVALID_HANDLE);

  if (RecNumber < 1 || BufferLength < 0)
    MDBUG_RETURN(SQL_ERROR);

  /* Maria ODBC driver doesn't support error lists, so only the first record can be retrieved */
  if (RecNumber != 1)
    MDBUG_RETURN(SQL_NO_DATA_FOUND);
  
  switch (HandleType) {
    case SQL_HANDLE_DBC:
      {
        MADB_Dbc *Dbc= (MADB_Dbc *)Handle;

        MDBUG_C_ENTER(Dbc, "SQLGetDiagRec");
        MDBUG_C_DUMP(Dbc, HandleType, d);
        MDBUG_C_DUMP(Dbc, Handle, 0x);
        MDBUG_C_DUMP(Dbc, MessageText, 0x);
        MDBUG_C_DUMP(Dbc, BufferLength, d);
        MDBUG_C_DUMP(Dbc, TextLengthPtr, 0x);

        ret= MADB_GetDiagRec(&Dbc->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                              (void *) MessageText, BufferLength, TextLengthPtr, FALSE,
                              Dbc->Environment->OdbcVersion);
      }
      break;
    case SQL_HANDLE_STMT:
      {
        MADB_Stmt *Stmt= (MADB_Stmt *)Handle;

        MDBUG_C_ENTER(Stmt->Connection, "SQLGetDiagRec");
        MDBUG_C_DUMP(Stmt->Connection, HandleType, d);
        MDBUG_C_DUMP(Stmt->Connection, Handle, 0x);
        MDBUG_C_DUMP(Stmt->Connection, MessageText, 0x);
        MDBUG_C_DUMP(Stmt->Connection, BufferLength, d);
        MDBUG_C_DUMP(Stmt->Connection, TextLengthPtr, 0x);

        ret= MADB_GetDiagRec(&Stmt->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, FALSE,
                               Stmt->Connection->Environment->OdbcVersion);
      }
      break;
    case SQL_HANDLE_DESC:
      {
        MADB_Desc *Desc= (MADB_Desc *)Handle;

        MDBUG_C_ENTER(Desc->Dbc, "SQLGetDiagRec");
        MDBUG_C_DUMP(Desc->Dbc, HandleType, d);
        MDBUG_C_DUMP(Desc->Dbc, Handle, 0x);
        MDBUG_C_DUMP(Desc->Dbc, MessageText, 0x);
        MDBUG_C_DUMP(Desc->Dbc, BufferLength, d);
        MDBUG_C_DUMP(Desc->Dbc, TextLengthPtr, 0x);

        ret= MADB_GetDiagRec(&Desc->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, FALSE,
                               SQL_OV_ODBC3);
      }
      break;
    case SQL_HANDLE_ENV:
      {
        MADB_Env *Env= (MADB_Env *)Handle;
        ret= MADB_GetDiagRec(&Env->Error, RecNumber, (void *)SQLState, NativeErrorPtr,
                               (void *)MessageText, BufferLength, TextLengthPtr, FALSE,
                               Env->OdbcVersion);
      }
      break;
  }

  MDBUG_RETURN(ret);
}
SQLRETURN SQL_API SQLGetDiagRec(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLCHAR *SQLState,
    SQLINTEGER *NativeErrorPtr,
    SQLCHAR *MessageText,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *TextLengthPtr)
{
  return MA_SQLGetDiagRec(HandleType, Handle, RecNumber, SQLState, NativeErrorPtr,
                          MessageText, BufferLength, TextLengthPtr);
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
  return MA_SQLGetDiagRecW(HandleType, Handle, RecNumber, SQLState, NativeErrorPtr, MessageText,
                           BufferLength, TextLengthPtr);
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
  {
    MADB_CLEAR_ERROR(&Env->Error);
    ret= MADB_EnvGetAttr(Env, Attribute, ValuePtr, BufferLength, StringLengthPtr);
  }

  MDBUG_DUMP(ret, d);
  MDBUG_RETURN(ret);
}
/* {{{ SQLGetFunctions */
SQLRETURN SQL_API SQLGetFunctions(SQLHDBC ConnectionHandle,
    SQLUSMALLINT FunctionId,
    SQLUSMALLINT *SupportedPtr)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;

  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Dbc->Error);

  MDBUG_C_ENTER(Dbc, "SQLGetFunctions");
  MDBUG_C_DUMP(Dbc, FunctionId, d);
  MDBUG_C_DUMP(Dbc, SupportedPtr, 0x);
  ret= Dbc->Methods->GetFunctions(Dbc, FunctionId, SupportedPtr);

  MDBUG_C_RETURN(Dbc,ret, &Dbc->Error);
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
  MADB_CLEAR_ERROR(&Dbc->Error);

  MDBUG_C_ENTER(Dbc, "SQLGetInfo");
  MDBUG_C_DUMP(Dbc, InfoType, d);
  ret= Dbc->Methods->GetInfo(Dbc, InfoType, InfoValuePtr, BufferLength, StringLengthPtr, FALSE);

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
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
  MADB_CLEAR_ERROR(&Dbc->Error);

  MDBUG_C_ENTER(Dbc, "SQLGetInfo");
  MDBUG_C_DUMP(Dbc, InfoType, d);
  MDBUG_C_DUMP(Dbc, InfoValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, StringLengthPtr, 0x);
  ret= Dbc->Methods->GetInfo(Dbc, InfoType, InfoValuePtr, BufferLength, StringLengthPtr, TRUE);

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ SQLGetStmtAttr */
SQLRETURN MA_SQLGetStmtAttr(SQLHSTMT StatementHandle,
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
SQLRETURN SQL_API SQLGetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  if (StatementHandle == SQL_NULL_HSTMT)
  {
    return SQL_INVALID_HANDLE;
  }
  MADB_CLEAR_ERROR(&((MADB_Stmt*)StatementHandle)->Error);

  return MA_SQLGetStmtAttr(StatementHandle, Attribute, ValuePtr, BufferLength, StringLengthPtr);
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
  MADB_CLEAR_ERROR(&Stmt->Error);
 
  return Stmt->Methods->GetAttr(Stmt, Attribute, ValuePtr, BufferLength, StringLengthPtr);
}
/* }}} */

/* {{{ SQLGetStmtOption */
SQLRETURN SQL_API SQLGetStmtOption(SQLHSTMT StatementHandle,
                                    SQLUSMALLINT Option, SQLPOINTER Value)
{
  if (StatementHandle == SQL_NULL_HSTMT)
  {
    return SQL_INVALID_HANDLE;
  }
  MADB_CLEAR_ERROR(&((MADB_Stmt*)StatementHandle)->Error);

  return MA_SQLGetStmtAttr(StatementHandle, Option, Value, SQL_NTS, (SQLINTEGER *)NULL);
}

/* }}} */

/* {{{ SQLGetTypeInfo */
SQLRETURN SQL_API SQLGetTypeInfo(SQLHSTMT StatementHandle,
    SQLSMALLINT DataType)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  MADB_CLEAR_ERROR(&Stmt->Error);

  return MADB_GetTypeInfo(Stmt, DataType);}
/* }}} */

/* {{{ SQLMoreResults */
SQLRETURN SQL_API SQLMoreResults(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  Length= (SQLINTEGER)MADB_SetString(0, OutStatementText, BufferLength, (char *)InStatementText, TextLength1, &Dbc->Error);
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
  MADB_Dbc  *Conn=   (MADB_Dbc *)ConnectionHandle;
  SQLINTEGER Length= (TextLength1 == SQL_NTS) ? SqlwcsCharLen(InStatementText, (SQLLEN)-1) : TextLength1;

  if (!Conn)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Conn->Error);

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
  MADB_CLEAR_ERROR(&Stmt->Error);

  return Stmt->Methods->ParamCount(Stmt, ParameterCountPtr);
}
/* }}} */

/* {{{ SQLNumResultCols */
SQLRETURN SQL_API SQLNumResultCols(SQLHSTMT StatementHandle,
    SQLSMALLINT *ColumnCountPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  MADB_CHECK_STMT_HANDLE(Stmt, stmt);
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  MADB_CLEAR_ERROR(&Stmt->Error);

  return Stmt->Methods->ParamData(Stmt, ValuePtrPtr);
}
/* }}} */


SQLRETURN SQL_API SQLPrepare(SQLHSTMT StatementHandle,
    SQLCHAR *StatementText,
    SQLINTEGER TextLength)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  if (StatementHandle == SQL_NULL_HSTMT)
  {
    return SQL_INVALID_HANDLE;
  }

  MDBUG_C_ENTER(Stmt->Connection, "SQLPrepare");

  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, StatementText, s);
  MDBUG_C_DUMP(Stmt->Connection, TextLength, d);

  /* Prepare method clears error */

  return Stmt->Methods->Prepare(Stmt, (char *)StatementText, TextLength, FALSE);
}
/* }}} */

/* {{{ SQLPrepareW */
SQLRETURN SQL_API SQLPrepareW(SQLHSTMT StatementHandle,
    SQLWCHAR *StatementText,
    SQLINTEGER TextLength)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  char *StmtStr;
  SQLULEN StmtLength;
  SQLRETURN ret;
  BOOL ConversionError;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLPrepareW");

  StmtStr= MADB_ConvertFromWChar(StatementText, TextLength, &StmtLength, Stmt->Connection->ConnOrSrcCharset, &ConversionError);

  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, StmtStr, s);
  MDBUG_C_DUMP(Stmt->Connection, TextLength, d);

  if (ConversionError)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_22018, NULL, 0);
    ret= Stmt->Error.ReturnValue;
  }
  else
    ret= Stmt->Methods->Prepare(Stmt, StmtStr, (SQLINTEGER)StmtLength, FALSE);
  MADB_FREE(StmtStr);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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
  {
    MADB_CLEAR_ERROR(&Stmt->Error);
    ret= Stmt->Methods->PrimaryKeys(Stmt, (char *)CatalogName, NameLength1, (char *)SchemaName, NameLength2,
                                    (char *)TableName, NameLength3);
  }

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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
  SQLULEN CpLength1, CpLength2, CpLength3;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);

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

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  SQLULEN CpLength1, CpLength2, CpLength3, CpLength4;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpProc= MADB_ConvertFromWChar(ProcName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpColumn= MADB_ConvertFromWChar(ColumnName, NameLength4, &CpLength4, Stmt->Connection->ConnOrSrcCharset, NULL);

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
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  SQLULEN CpLength1, CpLength2, CpLength3;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpProc= MADB_ConvertFromWChar(ProcName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
  
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
  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLPutData");
  MDBUG_C_DUMP(Stmt->Connection, DataPtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, StrLen_or_Ind, d);

  ret= Stmt->Methods->PutData(Stmt, DataPtr, StrLen_or_Ind);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLRowCount */
SQLRETURN SQL_API SQLRowCount(SQLHSTMT StatementHandle,
    SQLLEN *RowCountPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;

  MADB_CHECK_STMT_HANDLE(Stmt, stmt);
  MADB_CLEAR_ERROR(&Stmt->Error);

  return Stmt->Methods->RowCount(Stmt, RowCountPtr);
}
/* }}} */

/* {{{ SQLSetConnectAttr */
SQLRETURN MA_SQLSetConnectAttr(SQLHDBC ConnectionHandle,
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

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */
SQLRETURN SQL_API SQLSetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  if (ConnectionHandle == SQL_NULL_HDBC)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&((MADB_Dbc *)ConnectionHandle)->Error);

  return MA_SQLSetConnectAttr(ConnectionHandle, Attribute, ValuePtr, StringLength);
}

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
  MADB_CLEAR_ERROR(&Dbc->Error);

  MDBUG_C_ENTER(Dbc, "SetConnectAttrW");
  MDBUG_C_DUMP(Dbc, Dbc, 0x);
  MDBUG_C_DUMP(Dbc, Attribute, d);
  MDBUG_C_DUMP(Dbc, ValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, StringLength, d);
  ret= Dbc->Methods->SetAttr(Dbc, Attribute, ValuePtr, StringLength, TRUE);

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ SQLSetConnectOption */
SQLRETURN SQL_API SQLSetConnectOption(SQLHDBC Hdbc, SQLUSMALLINT Option, SQLULEN Param)
{
  SQLINTEGER StringLength= 0;
  SQLRETURN ret;

  if (!Hdbc)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&((MADB_Dbc*)Hdbc)->Error);

  /* todo: do we have more string options ? */
  if (Option == SQL_ATTR_CURRENT_CATALOG)
    StringLength= SQL_NTS;
  ret= MA_SQLSetConnectAttr(Hdbc, Option, (SQLPOINTER)Param, StringLength);
  return ret;
}
/* }}} */

/* {{{ SQLSetConnectOptionW */
SQLRETURN SQL_API SQLSetConnectOptionW(SQLHDBC Hdbc, SQLUSMALLINT Option, SQLULEN Param)
{
  SQLINTEGER StringLength= 0;
  SQLRETURN ret;
  MADB_Dbc *Dbc= (MADB_Dbc *)Hdbc;

  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Dbc->Error);

  MDBUG_C_ENTER(Dbc, "SetSetConnectOptionW");
  MDBUG_C_DUMP(Dbc, Option, d);
  MDBUG_C_DUMP(Dbc, Param, u);
  /* todo: do we have more string options ? */
  if (Option == SQL_ATTR_CURRENT_CATALOG)
    StringLength= SQL_NTS;

  ret= Dbc->Methods->SetAttr(Dbc, Option, (SQLPOINTER)Param, StringLength, TRUE);

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
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
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  SQLULEN Length;
  SQLRETURN rc;

  if (!Stmt)
  {
    return SQL_INVALID_HANDLE;
  }
  MADB_CLEAR_ERROR(&Stmt->Error);

  CpName= MADB_ConvertFromWChar(CursorName, NameLength, &Length, Stmt->Connection->ConnOrSrcCharset, NULL);
  rc= Stmt->Methods->SetCursorName(Stmt, (char *)CpName, (SQLINTEGER)Length);

  MADB_FREE(CpName);

  return rc;
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
  MADB_CLEAR_ERROR(&Desc->Error);

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
  MADB_CLEAR_ERROR(&Desc->Error);

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
  {
    MADB_CLEAR_ERROR(&Env->Error);
    ret= MADB_EnvSetAttr(Env, Attribute, ValuePtr, StringLength);
  }
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
  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLSetPos");
  MDBUG_C_DUMP(Stmt->Connection, RowNumber, d);
  MDBUG_C_DUMP(Stmt->Connection, Operation, u);
  MDBUG_C_DUMP(Stmt->Connection, LockType, d);

  ret= Stmt->Methods->SetPos(Stmt, RowNumber, Operation, LockType, 0);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
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
  MADB_CLEAR_ERROR(&((MADB_Stmt*)stmt)->Error);
  return MA_SQLBindParameter(stmt, par, SQL_PARAM_INPUT_OUTPUT, type, sqltype, coldef,
                             scale, val, SQL_SETPARAM_VALUE_MAX, nval);
}
/* }}} */

/* {{{ SQLBindParam - we need it for direct linking mainly */
SQLRETURN  SQL_API SQLBindParam(SQLHSTMT StatementHandle,
                                SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                                SQLSMALLINT ParameterType, SQLULEN LengthPrecision,
                                SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue,
                                SQLLEN *StrLen_or_Ind)
{
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&((MADB_Stmt*)StatementHandle)->Error);

  return MA_SQLBindParameter(StatementHandle, ParameterNumber, SQL_PARAM_INPUT_OUTPUT, ValueType, ParameterType, LengthPrecision, ParameterScale,
                      ParameterValue, SQL_SETPARAM_VALUE_MAX, StrLen_or_Ind);

}
/* }}} */

/* {{{ SQLSetStmtAttr */
SQLRETURN MA_SQLSetStmtAttr(SQLHSTMT StatementHandle,
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

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}

SQLRETURN SQL_API SQLSetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  if (StatementHandle == SQL_NULL_HSTMT)
  {
    return SQL_INVALID_HANDLE;
  }
  MADB_CLEAR_ERROR(&((MADB_Stmt*)StatementHandle)->Error);

  return MA_SQLSetStmtAttr(StatementHandle, Attribute, ValuePtr, StringLength);
}
/* }}} */

/* {{{ SQLSetStmtAttrW */
SQLRETURN SQL_API SQLSetStmtAttrW(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  if (StatementHandle == SQL_NULL_HSTMT)
  {
    return SQL_INVALID_HANDLE;
  }
  MADB_CLEAR_ERROR(&((MADB_Stmt*)StatementHandle)->Error);

  return MA_SQLSetStmtAttr(StatementHandle, Attribute, ValuePtr, StringLength); 
}
/* }}} */

/* {{{ SQLSetStmtOption */
SQLRETURN SQL_API SQLSetStmtOption(SQLHSTMT StatementHandle,
                                    SQLUSMALLINT Option, SQLULEN Value)
{
  if (StatementHandle == SQL_NULL_HSTMT)
  {
    return SQL_INVALID_HANDLE;
  }
  MADB_CLEAR_ERROR(&((MADB_Stmt*)StatementHandle)->Error);
  return MA_SQLSetStmtAttr(StatementHandle, Option, (SQLPOINTER)Value, SQL_NTS);
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
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  SQLULEN CpLength1, CpLength2, CpLength3;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);

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
  MADB_CLEAR_ERROR(&Stmt->Error);

  return Stmt->Methods->Statistics(Stmt, (char *)CatalogName, NameLength1, (char *)SchemaName, NameLength2,
                                   (char *)TableName, NameLength3, Unique, Reserved);
}
/* }}} */

/* {{{ SQLStatisticsW */
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
  SQLULEN CpLength1, CpLength2, CpLength3;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
  CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);

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
  MADB_CLEAR_ERROR(&Stmt->Error);

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
       *CpTable= NULL;
  SQLULEN CpLength1= 0, CpLength3= 0;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  if (CatalogName != NULL)
  {
    CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  }  
  if (TableName != NULL)
  {
    CpTable=   MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
  }

  ret= Stmt->Methods->TablePrivileges(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, NULL, 0, CpTable, (SQLSMALLINT)CpLength3);

  MADB_FREE(CpCatalog);
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
  MADB_CLEAR_ERROR(&Stmt->Error);

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
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0, CpLength4= 0;
  SQLRETURN ret;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  if (CatalogName)
  {
    CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
  }
  if (SchemaName)
  {
    CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
  }
  if (TableName)
  {
    CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
  }
  if (TableType)
  {
    CpType= MADB_ConvertFromWChar(TableType, NameLength4, &CpLength4, Stmt->Connection->ConnOrSrcCharset, NULL);
  }

  ret= Stmt->Methods->Tables(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2, 
                               CpTable, (SQLSMALLINT)CpLength3, CpType, (SQLSMALLINT)CpLength4);
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  MADB_FREE(CpType);
  return ret;
}
/* }}} */

/* {{{ SQLSetScrollOptions */
SQLRETURN SQL_API SQLSetScrollOptions(SQLHSTMT     hstmt,
                                      SQLUSMALLINT Concurrency,
                                      SQLLEN       crowKeySet,
                                      SQLUSMALLINT crowRowSet)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)hstmt;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  return MADB_DescSetField(Stmt->Ard, 0, SQL_DESC_ARRAY_SIZE, (SQLPOINTER)(SQLULEN)crowKeySet, SQL_IS_USMALLINT, 0);
}
/* }}} */

/* {{{ SQLParamOptions */
SQLRETURN SQL_API SQLParamOptions(
    SQLHSTMT hstmt,
    SQLULEN  crow,
    SQLULEN  *pirow)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)hstmt;
  SQLRETURN result;
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  result= MADB_DescSetField(Stmt->Apd, 0, SQL_DESC_ARRAY_SIZE, (SQLPOINTER)crow, SQL_IS_UINTEGER, 0);

  if (SQL_SUCCEEDED(result))
  {
    result= MADB_DescSetField(Stmt->Ipd, 0, SQL_DESC_ROWS_PROCESSED_PTR, (SQLPOINTER)pirow, SQL_IS_POINTER, 0);
  }

  return result;
}
/* }}} */
