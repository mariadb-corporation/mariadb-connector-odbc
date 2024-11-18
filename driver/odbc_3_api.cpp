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

#include "ma_c_stuff.h"
#include "ma_api_internal.h"
#include "ma_debug.h"

extern "C" {
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


SQLRETURN SQL_API SQLAllocConnect(SQLHANDLE InputHandle,
                                  SQLHANDLE *OutputHandlePtr)
{
  return MA_SQLAllocHandle(SQL_HANDLE_DBC, InputHandle, OutputHandlePtr);;
}
/* }}} */

/* {{{ SQLAllocStmt */
SQLRETURN SQL_API SQLAllocStmt(SQLHANDLE InputHandle,
                               SQLHANDLE *OutputHandlePtr)
{
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLBindCol(StatementHandle, ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_Ind);
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
  return MA_SQLBrowseConnect(ConnectionHandle, InConnectionString, StringLength1, OutConnectionString,
    BufferLength, StringLength2Ptr);
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
  return MA_SQLBrowseConnect(ConnectionHandle, NULL, StringLength1, NULL, BufferLength, StringLength2Ptr);
}
/* }}} */

/* {{{ SQLBulkOperations */
SQLRETURN SQL_API SQLBulkOperations(SQLHSTMT StatementHandle,
    SQLSMALLINT Operation)
{
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;
  
  return MA_SQLBulkOperations(StatementHandle, Operation);
}
/* }}} */

/* {{{ SQLCancel */
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
  if (!Handle)
    return SQL_INVALID_HANDLE;

  switch(HandleType) {
  case SQL_HANDLE_DBC:
    return MA_SQLCancelDbc(Handle);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;

  return MA_SQLCloseCursor(StatementHandle);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;
  return MA_SQLColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttributePtr, BufferLength,
    StringLengthPtr, NumericAttributePtr, FALSE);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;
  return MA_SQLColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttributePtr, BufferLength,
    StringLengthPtr, NumericAttributePtr, TRUE);
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
  if (!hstmt)
    return SQL_INVALID_HANDLE;

  return MA_SQLColAttribute(hstmt, icol, MapColAttributeDescType(fDescType), rgbDesc,
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
  if (!hstmt)
    return SQL_INVALID_HANDLE;
  return MA_SQLColAttribute(hstmt, icol, MapColAttributeDescType(fDescType), rgbDesc, cbDescMax, pcbDesc, pfDesc, TRUE);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;

  return MA_SQLColumnPrivileges(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
    NameLength3, ColumnName, NameLength4);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;
  return MA_SQLColumnPrivilegesW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
    NameLength3, ColumnName, NameLength4);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;

  return MA_SQLColumns(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
    TableName, NameLength3, ColumnName, NameLength4);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;
  return MA_SQLColumnsW(StatementHandle, CatalogName, NameLength1, SchemaName,
    NameLength2, TableName, NameLength3, ColumnName, NameLength4);
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
  if (!ConnectionHandle)
    return SQL_INVALID_HANDLE;
  return MA_SQLConnectW(ConnectionHandle, ServerName, NameLength1, UserName, NameLength2,
    Authentication, NameLength3);
}
/* }}} */

/* {{{ SQLCopyDesc */
SQLRETURN SQL_API SQLCopyDesc(SQLHDESC SourceDescHandle,
    SQLHDESC TargetDescHandle)
{
  if (!SourceDescHandle)
    return SQL_INVALID_HANDLE;
  return MADB_DescCopyDesc(SourceDescHandle, TargetDescHandle);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;

  return MA_SQLDescribeCol(StatementHandle, ColumnNumber, (void*)ColumnName, BufferLength, NameLengthPtr,
    DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr, FALSE);
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
  if (!StatementHandle)
    return SQL_INVALID_HANDLE;
  return MA_SQLDescribeCol(StatementHandle, ColumnNumber, (void*)ColumnName, BufferLength, NameLengthPtr,
    DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr, TRUE);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);

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
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MADB_SQLDisconnect(ConnectionHandle);
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
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);

  return MA_SQLDriverConnect(ConnectionHandle, WindowHandle, InConnectionString,
    StringLength1, OutConnectionString, BufferLength, StringLength2Ptr, DriverCompletion);
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
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MA_SQLDriverConnectW(ConnectionHandle, WindowHandle, InConnectionString, StringLength1,
    OutConnectionString, BufferLength, StringLength2Ptr, DriverCompletion);
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
  return MA_SQLError(Env, Dbc, Stmt, Sqlstate, NativeError, Message, MessageMax,
    MessageLen, FALSE);
}
/* }}} */

/*{{{ SQLErrorW */
SQLRETURN SQL_API
SQLErrorW(SQLHENV Env, SQLHDBC Dbc, SQLHSTMT Stmt, SQLWCHAR *Sqlstate,
          SQLINTEGER *NativeError, SQLWCHAR *Message, SQLSMALLINT MessageMax,
          SQLSMALLINT *MessageLen)

{
  return MA_SQLError(Env, Dbc, Stmt, Sqlstate, NativeError, Message, MessageMax,
    MessageLen, TRUE);
}
/* }}} */

/* {{{ SQLTransact */
SQLRETURN SQL_API SQLTransact(SQLHENV Env, SQLHDBC Dbc, SQLUSMALLINT CompletionType)
{
  if (Env != SQL_NULL_HENV)
  {
    MA_ClearError(SQL_HANDLE_ENV, Env);
    return MA_SQLEndTran(SQL_HANDLE_ENV, Env, CompletionType);
  }
  else if (Dbc != SQL_NULL_HDBC)
  {
    MA_ClearError(SQL_HANDLE_DBC, Dbc);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLExecDirect(StatementHandle, StatementText, TextLength);
}
/* }}} */

/* {{{ SQLExecDirectW */
SQLRETURN SQL_API SQLExecDirectW(SQLHSTMT StatementHandle,
    SQLWCHAR *StatementText,
    SQLINTEGER TextLength)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLExecDirectW(StatementHandle, StatementText, TextLength);
}
/* }}} */

/* {{{ SQLExecute */
SQLRETURN SQL_API SQLExecute(SQLHSTMT StatementHandle)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLExecute(StatementHandle);
}
/* }}} */

/* {{{ SQLExtendedFetch */
SQLRETURN SQL_API SQLExtendedFetch(SQLHSTMT StatementHandle,
    SQLUSMALLINT FetchOrientation,
    SQLLEN FetchOffset,
    SQLULEN *RowCountPtr,
    SQLUSMALLINT *RowStatusArray)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLExtendedFetch(StatementHandle, FetchOrientation, FetchOffset, RowCountPtr,
    RowStatusArray);
}
/* }}} */

/* {{{ SQLFetch */
SQLRETURN SQL_API SQLFetch(SQLHSTMT StatementHandle)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLFetch(StatementHandle);
}
/* }}} */

/* {{{ SQLFetchScroll */
SQLRETURN SQL_API SQLFetchScroll(SQLHSTMT StatementHandle,
    SQLSMALLINT FetchOrientation,
    SQLLEN FetchOffset)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLFetchScroll(StatementHandle, FetchOrientation, FetchOffset);
}
/* }}} */

/* {{{ SQLFreeHandle */
SQLRETURN SQL_API SQLFreeHandle(SQLSMALLINT HandleType,
                                SQLHANDLE Handle)
{
  MADB_CHECK_HANDLE_CLEAR_ERROR(HandleType, Handle);
  return MA_SQLFreeHandle(HandleType, Handle);
}
/* }}} */

/* {{{ SQLFreeEnv */
SQLRETURN SQL_API SQLFreeEnv(SQLHANDLE henv)
{
  CHECK_ENV_CLEAR_ERROR(henv);

  return MA_SQLFreeHandle(SQL_HANDLE_ENV, henv);
}
/* }}} */

/* {{{ SQLFreeConnect */
SQLRETURN SQL_API SQLFreeConnect(SQLHANDLE hdbc)
{
  CHECK_DBC_CLEAR_ERROR(hdbc);
  return MA_SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
}
/* }}} */

/* {{{ SQLFreeStmt */
SQLRETURN SQL_API SQLFreeStmt(SQLHSTMT StatementHandle,
           SQLUSMALLINT Option)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);

  return MA_SQLForeignKeys(StatementHandle, PKCatalogName, NameLength1, PKSchemaName,
    NameLength2, PKTableName, NameLength3, FKCatalogName, NameLength4, FKSchemaName,
    NameLength5, FKTableName, NameLength6);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);

  return MA_SQLForeignKeysW(StatementHandle, PKCatalogName, NameLength1, PKSchemaName,
    NameLength2, PKTableName, NameLength3, FKCatalogName, NameLength4, FKSchemaName,
    NameLength5, FKTableName, NameLength6);
}
/* }}} */

/* {{{ SQLGetConnectAttr */
SQLRETURN SQL_API SQLGetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);

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
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);

  return MA_SQLGetConnectAttrW(ConnectionHandle, Attribute, ValuePtr, BufferLength,
    StringLengthPtr);
}
/* }}} */

/* {{{ SQLGetConnectOption */
SQLRETURN SQL_API SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER ValuePtr)
{
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);

  return MA_SQLGetConnectAttr(ConnectionHandle, Option, ValuePtr,
                           Option == SQL_ATTR_CURRENT_CATALOG ? SQL_MAX_OPTION_STRING_LENGTH : 0, NULL);
}
/* }}} */

/* {{{ SQLGetConnectOptionW */
SQLRETURN SQL_API SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER ValuePtr)
{
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MA_SQLGetConnectAttrW(ConnectionHandle, Option, ValuePtr,
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);

  return MA_SQLGetCursorName(StatementHandle, CursorName, BufferLength,
    NameLengthPtr, FALSE);
}
/* }}} */

/* {{{ SQLGetCursorNameW */
SQLRETURN SQL_API SQLGetCursorNameW(
     SQLHSTMT        StatementHandle,
     SQLWCHAR *      CursorName,
     SQLSMALLINT     BufferLength,
     SQLSMALLINT *   NameLengthPtr)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLGetCursorName(StatementHandle, CursorName, BufferLength,
    NameLengthPtr, TRUE);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLGetData(StatementHandle, Col_or_Param_Num, TargetType,
    TargetValuePtr, BufferLength, StrLen_or_IndPtr);
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
  CHECK_DESC_CLEAR_ERROR(DescriptorHandle);
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
  CHECK_DESC_CLEAR_ERROR(DescriptorHandle);
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
  CHECK_STMT_CLEAR_ERROR(DescriptorHandle);
  return MADB_DescGetRec(DescriptorHandle, RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr,
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
  CHECK_STMT_CLEAR_ERROR(DescriptorHandle);
  return MADB_DescGetRec(DescriptorHandle, RecNumber, (SQLCHAR *)Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr,
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
  if (!Handle)
    return SQL_INVALID_HANDLE;
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
  CHECK_ENV_CLEAR_ERROR(EnvironmentHandle);
  return MA_SQLGetEnvAttr(EnvironmentHandle, Attribute, ValuePtr, BufferLength, StringLengthPtr);
}
/* {{{ SQLGetFunctions */
SQLRETURN SQL_API SQLGetFunctions(SQLHDBC ConnectionHandle,
    SQLUSMALLINT FunctionId,
    SQLUSMALLINT *SupportedPtr)
{
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MA_SQLGetFunctions(ConnectionHandle, FunctionId, SupportedPtr);
}
/* }}} */

/* {{{ SQLGetInfo */
SQLRETURN SQL_API SQLGetInfo(SQLHDBC ConnectionHandle,
    SQLUSMALLINT InfoType,
    SQLPOINTER InfoValuePtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr)
{
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MA_SQLGetInfo(ConnectionHandle, InfoType, InfoValuePtr, BufferLength,
    StringLengthPtr, FALSE);
}
/* }}} */

/* {{{ SQLGetInfoW */
SQLRETURN SQL_API SQLGetInfoW(SQLHDBC ConnectionHandle,
    SQLUSMALLINT InfoType,
    SQLPOINTER InfoValuePtr,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr)
{
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MA_SQLGetInfo(ConnectionHandle, InfoType, InfoValuePtr, BufferLength,
    StringLengthPtr, TRUE);
}
/* }}} */

/* {{{ SQLGetStmtAttr */
SQLRETURN SQL_API SQLGetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLGetStmtAttr(StatementHandle, Attribute, ValuePtr, BufferLength, StringLengthPtr);
}
/* }}} */

/* {{{ SQLGetStmtOption */
SQLRETURN SQL_API SQLGetStmtOption(SQLHSTMT StatementHandle,
                                    SQLUSMALLINT Option, SQLPOINTER Value)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLGetStmtAttr(StatementHandle, Option, Value, SQL_NTS, (SQLINTEGER *)NULL);
}

/* }}} */

/* {{{ SQLGetTypeInfo */
SQLRETURN SQL_API SQLGetTypeInfo(SQLHSTMT StatementHandle,
    SQLSMALLINT DataType)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MADB_GetTypeInfo(StatementHandle, DataType);
}
/* }}} */

/* {{{ SQLGetTypeInfoW */
SQLRETURN SQL_API SQLGetTypeInfoW(SQLHSTMT StatementHandle,
    SQLSMALLINT DataType)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MADB_GetTypeInfo(StatementHandle, DataType);}
/* }}} */

/* {{{ SQLMoreResults */
SQLRETURN SQL_API SQLMoreResults(SQLHSTMT StatementHandle)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MADB_StmtMoreResults(StatementHandle);
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
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MA_SQLNativeSql(ConnectionHandle, InStatementText, TextLength1, OutStatementText,
    BufferLength, TextLength2Ptr);
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
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MA_SQLNativeSqlW(ConnectionHandle, InStatementText, TextLength1, OutStatementText,
    BufferLength, TextLength2Ptr);
}
/* }}} */

/* {{{ SQLNumParams */
SQLRETURN SQL_API SQLNumParams(SQLHSTMT StatementHandle,
    SQLSMALLINT *ParameterCountPtr)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLNumParams(StatementHandle, ParameterCountPtr);
}
/* }}} */

/* {{{ SQLNumResultCols */
SQLRETURN SQL_API SQLNumResultCols(SQLHSTMT StatementHandle,
    SQLSMALLINT *ColumnCountPtr)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLNumResultCols(StatementHandle, ColumnCountPtr);
}
/* }}} */

/* {{{ SQLParamData */
SQLRETURN SQL_API SQLParamData(SQLHSTMT StatementHandle,
    SQLPOINTER *ValuePtrPtr)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLParamData(StatementHandle, ValuePtrPtr);
}
/* }}} */


SQLRETURN SQL_API SQLPrepare(SQLHSTMT StatementHandle,
    SQLCHAR *StatementText,
    SQLINTEGER TextLength)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLPrepare(StatementHandle, StatementText, TextLength);
}
/* }}} */

/* {{{ SQLPrepareW */
SQLRETURN SQL_API SQLPrepareW(SQLHSTMT StatementHandle,
    SQLWCHAR *StatementText,
    SQLINTEGER TextLength)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLPrepareW(StatementHandle, StatementText, TextLength);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLPrimaryKeys(StatementHandle, CatalogName, NameLength1, SchemaName,
    NameLength2, TableName, NameLength3);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLPrimaryKeysW(StatementHandle, CatalogName, NameLength1, SchemaName,
    NameLength2, TableName, NameLength3);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLProcedureColumns(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
    ProcName, NameLength3, ColumnName, NameLength4);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLProcedureColumnsW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
    ProcName, NameLength3, ColumnName, NameLength4);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLProcedures(StatementHandle, CatalogName, NameLength1, SchemaName, 
                                   NameLength2, ProcName, NameLength3);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLProceduresW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
    ProcName, NameLength3);
}
/* }}} */

/* {{{ SQLPutData */
SQLRETURN SQL_API SQLPutData(SQLHSTMT StatementHandle,
    SQLPOINTER DataPtr,
    SQLLEN StrLen_or_Ind)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLPutData(StatementHandle, DataPtr, StrLen_or_Ind);
}
/* }}} */

/* {{{ SQLRowCount */
SQLRETURN SQL_API SQLRowCount(SQLHSTMT StatementHandle,
    SQLLEN *RowCountPtr)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLRowCount(StatementHandle, RowCountPtr);
}
/* }}} */

/* {{{ SQLSetConnectAttr */
SQLRETURN SQL_API SQLSetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MA_SQLSetConnectAttr(ConnectionHandle, Attribute, ValuePtr, StringLength, FALSE);
}

/* {{{ SQLSetConnectAttrW */
SQLRETURN SQL_API SQLSetConnectAttrW(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  CHECK_DBC_CLEAR_ERROR(ConnectionHandle);
  return MA_SQLSetConnectAttr(ConnectionHandle, Attribute, ValuePtr, StringLength, TRUE);
}
/* }}} */

/* {{{ SQLSetConnectOption */
SQLRETURN SQL_API SQLSetConnectOption(SQLHDBC Hdbc, SQLUSMALLINT Option, SQLULEN Param)
{
  SQLINTEGER StringLength= 0;

  CHECK_DBC_CLEAR_ERROR(Hdbc);

  /* todo: do we have more string options ? */
  if (Option == SQL_ATTR_CURRENT_CATALOG)
    StringLength= SQL_NTS;
  return MA_SQLSetConnectAttr(Hdbc, Option, (SQLPOINTER)Param, StringLength, FALSE);
}
/* }}} */

/* {{{ SQLSetConnectOptionW */
SQLRETURN SQL_API SQLSetConnectOptionW(SQLHDBC Hdbc, SQLUSMALLINT Option, SQLULEN Param)
{
  SQLINTEGER StringLength= 0;

  CHECK_DBC_CLEAR_ERROR(Hdbc);
  /* todo: do we have more string options ? */
  if (Option == SQL_ATTR_CURRENT_CATALOG)
    StringLength= SQL_NTS;
  return MA_SQLSetConnectAttr(Hdbc, Option, (SQLPOINTER)Param, StringLength, TRUE);
}
/* }}} */

/* {{{ SQLSetCursorName */
SQLRETURN SQL_API SQLSetCursorName(SQLHSTMT StatementHandle,
    SQLCHAR *CursorName,
    SQLSMALLINT NameLength)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLSetCursorName(StatementHandle,CursorName, NameLength);
}
/* }}} */

/* {{{ SQLSetCursorNameW */
SQLRETURN SQL_API SQLSetCursorNameW(SQLHSTMT StatementHandle,
    SQLWCHAR *CursorName,
    SQLSMALLINT NameLength)
{
  
  return MA_SQLSetCursorNameW(StatementHandle,CursorName, NameLength);
}
/* }}} */

/* {{{ SQLSetDescField */
SQLRETURN SQL_API SQLSetDescField(SQLHDESC DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength)
{
  CHECK_DESC_CLEAR_ERROR(DescriptorHandle);
  if (MADB_ValidateDescFieldAccess(DescriptorHandle, FieldIdentifier, ValuePtr))
  {
    return SQL_ERROR;
  }
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
  CHECK_DESC_CLEAR_ERROR(DescriptorHandle);
  if (MADB_ValidateDescFieldAccess(DescriptorHandle, FieldIdentifier, ValuePtr))
  {
    return SQL_ERROR; /* (MADB_Desc*)DescriptorHandle->Error.ReturnValue would be better, but SQL_ERROR is good enough,
                         as this is the only possible value */
  }
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
  return MA_NotImplemented(SQL_HANDLE_DESC, DescriptorHandle);
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
  return MA_NotImplemented(SQL_HANDLE_DESC, DescriptorHandle);
}
/* }}} */

/* {{{ SQLSetEnvAttr */
SQLRETURN SQL_API SQLSetEnvAttr(SQLHENV EnvironmentHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  CHECK_ENV_CLEAR_ERROR(EnvironmentHandle);
  return MA_SQLSetEnvAttr(EnvironmentHandle, Attribute, ValuePtr, StringLength);
}
/* }}} */

/* {{{ SQLSetPos */
SQLRETURN SQL_API SQLSetPos(SQLHSTMT StatementHandle,
    SQLSETPOSIROW RowNumber,
    SQLUSMALLINT Operation,
    SQLUSMALLINT LockType)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLSetPos(StatementHandle, RowNumber, Operation, LockType);
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
  CHECK_STMT_CLEAR_ERROR(stmt);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLBindParameter(StatementHandle, ParameterNumber, SQL_PARAM_INPUT, ValueType, ParameterType, LengthPrecision, ParameterScale,
                      ParameterValue, SQL_SETPARAM_VALUE_MAX, StrLen_or_Ind);

}
/* }}} */

/* {{{ SQLSetStmtAttr */
SQLRETURN SQL_API SQLSetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLSetStmtAttr(StatementHandle, Attribute, ValuePtr, StringLength);
}
/* }}} */

/* {{{ SQLSetStmtAttrW */
SQLRETURN SQL_API SQLSetStmtAttrW(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLSetStmtAttr(StatementHandle, Attribute, ValuePtr, StringLength); 
}
/* }}} */

/* {{{ SQLSetStmtOption */
SQLRETURN SQL_API SQLSetStmtOption(SQLHSTMT StatementHandle,
                                    SQLUSMALLINT Option, SQLULEN Value)
{
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLSpecialColumns(StatementHandle, IdentifierType, CatalogName, NameLength1,
    SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable);
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
  
  return MA_SQLSpecialColumnsW(StatementHandle, IdentifierType, CatalogName, NameLength1,
    SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLStatistics(StatementHandle, CatalogName, NameLength1,SchemaName, NameLength2,
                          TableName, NameLength3, Unique, Reserved);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLStatisticsW(StatementHandle, CatalogName, NameLength1,SchemaName, NameLength2,
                          TableName, NameLength3, Unique, Reserved);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLTablePrivileges(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
                               TableName, NameLength3);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLTablePrivilegesW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
    TableName, NameLength3);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLTables(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, 
                      TableName,NameLength3, TableType, NameLength4);
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
  CHECK_STMT_CLEAR_ERROR(StatementHandle);
  return MA_SQLTablesW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
    TableName, NameLength3, TableType, NameLength4);
}
/* }}} */

/* {{{ SQLSetScrollOptions */
SQLRETURN SQL_API SQLSetScrollOptions(SQLHSTMT     hstmt,
                                      SQLUSMALLINT Concurrency,
                                      SQLLEN       crowKeySet,
                                      SQLUSMALLINT crowRowSet)
{
  CHECK_STMT_CLEAR_ERROR(hstmt);
  return MA_SQLSetScrollOptions(hstmt, Concurrency, crowKeySet, crowRowSet);
}
/* }}} */

/* {{{ SQLParamOptions */
SQLRETURN SQL_API SQLParamOptions(
    SQLHSTMT hstmt,
    SQLULEN  crow,
    SQLULEN  *pirow)
{
  CHECK_STMT_CLEAR_ERROR(hstmt);
  return MA_SQLParamOptions(hstmt, crow, pirow);
}
}
/* }}} */
