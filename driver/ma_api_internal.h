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

#ifndef _MA_API_INTERNAL_
#define _MA_API_INTERNAL_
/**
 * "Internal" ODBC API functions - functions, that have to be called internally if API 
 * function needs to be executed
 *
 * Calling SQLFunction itself inside the connector on non-Windows platforms will result
 * in the driver manager function instead of our own function.
 * 
 * Also contains couple of basic helper functions to be used by API functions, that can not,
 * and thus should not know about hanlde classes implementations
 */


SQLRETURN MA_NotImplemented(SQLSMALLINT handleType, SQLHANDLE handle);
void MA_ClearError(SQLSMALLINT handleType, SQLHANDLE handle);

#define MADB_CHECK_HANDLE_CLEAR_ERROR(handle_type, handle) \
  if (handle == 0) return SQL_INVALID_HANDLE;\
  MA_ClearError(handle_type, handle)

#define CHECK_STMT_CLEAR_ERROR(HANDLE) MADB_CHECK_HANDLE_CLEAR_ERROR(SQL_HANDLE_STMT, (HANDLE))
#define CHECK_DBC_CLEAR_ERROR(HANDLE) MADB_CHECK_HANDLE_CLEAR_ERROR(SQL_HANDLE_DBC, (HANDLE))
#define CHECK_DESC_CLEAR_ERROR(HANDLE) MADB_CHECK_HANDLE_CLEAR_ERROR(SQL_HANDLE_DESC, (HANDLE))
#define CHECK_ENV_CLEAR_ERROR(HANDLE) MADB_CHECK_HANDLE_CLEAR_ERROR(SQL_HANDLE_ENV, (HANDLE))


SQLUSMALLINT MapColAttributeDescType(SQLUSMALLINT FieldIdentifier);

SQLRETURN MADB_DescCopyDesc(SQLHDESC SrcDesc, SQLHDESC DestDesc);
SQLRETURN MADB_SQLDisconnect(SQLHDBC ConnectionHandle);
SQLRETURN MADB_DescGetField(SQLHDESC DescriptorHandle,
  SQLSMALLINT RecNumber,
  SQLSMALLINT FieldIdentifier,
  SQLPOINTER ValuePtr,
  SQLINTEGER BufferLength,
  SQLINTEGER * StringLengthPtr,
  int isWChar);

SQLRETURN MADB_DescGetRec(SQLHDESC Handle,
  SQLSMALLINT RecNumber,
  SQLCHAR * Name,
  SQLSMALLINT BufferLength,
  SQLSMALLINT * StringLengthPtr,
  SQLSMALLINT * TypePtr,
  SQLSMALLINT * SubTypePtr,
  SQLLEN * LengthPtr,
  SQLSMALLINT * PrecisionPtr,
  SQLSMALLINT * ScalePtr,
  SQLSMALLINT * NullablePtr,
  int isWChar);

SQLRETURN MADB_GetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle,
  SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier, SQLPOINTER
  DiagInfoPtr, SQLSMALLINT BufferLength,
  SQLSMALLINT * StringLengthPtr, int isWChar);

SQLRETURN MADB_GetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType);
SQLRETURN MADB_StmtMoreResults(SQLHSTMT StatementHandle);
SQLRETURN MADB_DescSetField(SQLHDESC DescriptorHandle,
  SQLSMALLINT RecNumber,
  SQLSMALLINT FieldIdentifier,
  SQLPOINTER ValuePtr,
  SQLINTEGER BufferLength,
  int isWChar);

SQLRETURN MA_SQLAllocHandle(SQLSMALLINT HandleType,
    SQLHANDLE InputHandle,
    SQLHANDLE *OutputHandlePtr);

SQLRETURN MA_SQLBindCol(SQLHSTMT StatementHandle,
  SQLUSMALLINT ColumnNumber,
  SQLSMALLINT TargetType,
  SQLPOINTER TargetValuePtr,
  SQLLEN BufferLength,
  SQLLEN* StrLen_or_Ind);

SQLRETURN MA_SQLBindParameter(SQLHSTMT StatementHandle,
    SQLUSMALLINT ParameterNumber,
    SQLSMALLINT InputOutputType,
    SQLSMALLINT ValueType,
    SQLSMALLINT ParameterType,
    SQLULEN ColumnSize,
    SQLSMALLINT DecimalDigits,
    SQLPOINTER ParameterValuePtr,
    SQLLEN BufferLength,
    SQLLEN *StrLen_or_IndPtr);

SQLRETURN MA_SQLBrowseConnect(SQLHDBC ConnectionHandle,
  SQLCHAR* InConnectionString,
  SQLSMALLINT StringLength1,
  SQLCHAR* OutConnectionString,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* StringLength2Ptr);

SQLRETURN MA_SQLBulkOperations(SQLHSTMT StatementHandle,
  SQLSMALLINT Operation);

SQLRETURN MA_SQLCancel(SQLHSTMT StatementHandle);

SQLRETURN MA_SQLCancelDbc(SQLHANDLE Handle);

SQLRETURN MA_SQLCloseCursor(SQLHSTMT StatementHandle);

SQLRETURN MA_SQLColAttribute(SQLHSTMT StatementHandle,
  SQLUSMALLINT ColumnNumber,
  SQLUSMALLINT FieldIdentifier,
  SQLPOINTER CharacterAttributePtr,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* StringLengthPtr,
#ifdef SQLCOLATTRIB_SQLPOINTER
  SQLPOINTER NumericAttributePtr
#else
  SQLLEN* NumericAttributePtr
#endif
  , int isWchar
);

SQLRETURN MA_SQLColumnPrivileges(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLCHAR* ColumnName,
  SQLSMALLINT NameLength4);

SQLRETURN MA_SQLColumnPrivilegesW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLWCHAR* ColumnName,
  SQLSMALLINT NameLength4);

SQLRETURN MA_SQLColumns(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLCHAR* ColumnName,
  SQLSMALLINT NameLength4);

SQLRETURN MA_SQLColumnsW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLWCHAR* ColumnName,
  SQLSMALLINT NameLength4);

SQLRETURN SQLConnectCommon(SQLHDBC ConnectionHandle,
  SQLCHAR* ServerName,
  SQLSMALLINT NameLength1,
  SQLCHAR* UserName,
  SQLSMALLINT NameLength2,
  SQLCHAR* Authentication,
  SQLSMALLINT NameLength3);

SQLRETURN MA_SQLConnectW(SQLHDBC ConnectionHandle,
  SQLWCHAR* ServerName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* UserName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* Authentication,
  SQLSMALLINT NameLength3);

SQLRETURN MA_SQLDescribeCol(SQLHSTMT StatementHandle,
  SQLUSMALLINT ColumnNumber,
  void* ColumnName,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* NameLengthPtr,
  SQLSMALLINT* DataTypePtr,
  SQLULEN* ColumnSizePtr,
  SQLSMALLINT* DecimalDigitsPtr,
  SQLSMALLINT* NullablePtr,
  char isWchar);

SQLRETURN MA_SQLDriverConnect(SQLHDBC ConnectionHandle,
  SQLHWND WindowHandle,
  SQLCHAR* InConnectionString,
  SQLSMALLINT StringLength1,
  SQLCHAR* OutConnectionString,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* StringLength2Ptr,
  SQLUSMALLINT DriverCompletion);

SQLRETURN MA_SQLDriverConnectW(SQLHDBC      ConnectionHandle,
  SQLHWND      WindowHandle,
  SQLWCHAR* InConnectionString,
  SQLSMALLINT  StringLength1,
  SQLWCHAR* OutConnectionString,
  SQLSMALLINT  BufferLength,
  SQLSMALLINT* StringLength2Ptr,
  SQLUSMALLINT DriverCompletion);

SQLRETURN MA_SQLEndTran(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT CompletionType);

SQLRETURN MA_SQLError(SQLHENV Env, SQLHDBC Dbc, SQLHSTMT Stmt,
  void* Sqlstate, SQLINTEGER* NativeError,
  void* Message, SQLSMALLINT MessageMax,
  SQLSMALLINT* MessageLen, int isWchar);

SQLRETURN MA_SQLExecDirect(SQLHSTMT StatementHandle,
  SQLCHAR* StatementText,
  SQLINTEGER TextLength);
SQLRETURN MA_SQLExecDirectW(SQLHSTMT StatementHandle,
  SQLWCHAR* StatementText,
  SQLINTEGER TextLength);

SQLRETURN MA_SQLExecute(SQLHSTMT StatementHandle);

SQLRETURN MA_SQLExtendedFetch(SQLHSTMT StatementHandle,
  SQLUSMALLINT FetchOrientation,
  SQLLEN FetchOffset,
  SQLULEN* RowCountPtr,
  SQLUSMALLINT* RowStatusArray);

SQLRETURN MA_SQLFetch(SQLHSTMT StatementHandle);
SQLRETURN MA_SQLFetchScroll(SQLHSTMT StatementHandle,
  SQLSMALLINT FetchOrientation,
  SQLLEN FetchOffset);

SQLRETURN MA_SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle);
SQLRETURN MA_SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option);

SQLRETURN MA_SQLForeignKeys(SQLHSTMT StatementHandle,
  SQLCHAR* PKCatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* PKSchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* PKTableName,
  SQLSMALLINT NameLength3,
  SQLCHAR* FKCatalogName,
  SQLSMALLINT NameLength4,
  SQLCHAR* FKSchemaName,
  SQLSMALLINT NameLength5,
  SQLCHAR* FKTableName,
  SQLSMALLINT NameLength6);
SQLRETURN MA_SQLForeignKeysW(SQLHSTMT StatementHandle,
  SQLWCHAR* PKCatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* PKSchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* PKTableName,
  SQLSMALLINT NameLength3,
  SQLWCHAR* FKCatalogName,
  SQLSMALLINT NameLength4,
  SQLWCHAR* FKSchemaName,
  SQLSMALLINT NameLength5,
  SQLWCHAR* FKTableName,
  SQLSMALLINT NameLength6);

SQLRETURN MA_SQLGetConnectAttrW(SQLHDBC ConnectionHandle,
  SQLINTEGER Attribute,
  SQLPOINTER ValuePtr,
  SQLINTEGER BufferLength,
  SQLINTEGER* StringLengthPtr);

SQLRETURN MA_SQLGetCursorName(
  SQLHSTMT StatementHandle,
  void* CursorName,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* NameLengthPtr,
  int isWstr);

SQLRETURN MA_SQLGetData(SQLHSTMT StatementHandle,
  SQLUSMALLINT Col_or_Param_Num,
  SQLSMALLINT TargetType,
  SQLPOINTER TargetValuePtr,
  SQLLEN BufferLength,
  SQLLEN* StrLen_or_IndPtr);

SQLRETURN MA_SQLGetDiagRecW(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLWCHAR *SQLState,
    SQLINTEGER *NativeErrorPtr,
    SQLWCHAR *MessageText,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *TextLengthPtr);

SQLRETURN MA_SQLGetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr);

SQLRETURN MA_SQLGetDiagRec(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLCHAR *SQLState,
    SQLINTEGER *NativeErrorPtr,
    SQLCHAR *MessageText,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *TextLengthPtr);

SQLRETURN MA_SQLGetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr);

SQLRETURN MA_SQLGetEnvAttr(SQLHENV EnvironmentHandle,
  SQLINTEGER Attribute,
  SQLPOINTER ValuePtr,
  SQLINTEGER BufferLength,
  SQLINTEGER* StringLengthPtr);

SQLRETURN MA_SQLGetFunctions(SQLHDBC ConnectionHandle,
  SQLUSMALLINT FunctionId,
  SQLUSMALLINT* SupportedPtr);

SQLRETURN MA_SQLGetInfo(SQLHDBC ConnectionHandle,
  SQLUSMALLINT InfoType,
  SQLPOINTER InfoValuePtr,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* StringLengthPtr,
  int isWstr);

SQLRETURN MA_SQLNativeSql(SQLHDBC ConnectionHandle,
  SQLCHAR* InStatementText,
  SQLINTEGER TextLength1,
  SQLCHAR* OutStatementText,
  SQLINTEGER BufferLength,
  SQLINTEGER* TextLength2Ptr);
SQLRETURN MA_SQLNativeSqlW(SQLHDBC ConnectionHandle,
  SQLWCHAR* InStatementText,
  SQLINTEGER TextLength1,
  SQLWCHAR* OutStatementText,
  SQLINTEGER BufferLength,
  SQLINTEGER* TextLength2Ptr);

SQLRETURN MA_SQLNumParams(SQLHSTMT StatementHandle,
  SQLSMALLINT* ParameterCountPtr);

SQLRETURN MA_SQLNumResultCols(SQLHSTMT StatementHandle,
  SQLSMALLINT* ColumnCountPtr);

SQLRETURN MA_SQLParamData(SQLHSTMT StatementHandle,
  SQLPOINTER* ValuePtrPtr);

SQLRETURN MA_SQLPrepare(SQLHSTMT StatementHandle,
  SQLCHAR* StatementText,
  SQLINTEGER TextLength);
SQLRETURN MA_SQLPrepareW(SQLHSTMT StatementHandle,
  SQLWCHAR* StatementText,
  SQLINTEGER TextLength);

SQLRETURN MA_SQLPrimaryKeys(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3);
SQLRETURN MA_SQLPrimaryKeysW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3);

SQLRETURN MA_SQLProcedureColumns(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* ProcName,
  SQLSMALLINT NameLength3,
  SQLCHAR* ColumnName,
  SQLSMALLINT NameLength4);
SQLRETURN MA_SQLProcedureColumnsW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* ProcName,
  SQLSMALLINT NameLength3,
  SQLWCHAR* ColumnName,
  SQLSMALLINT NameLength4);

SQLRETURN MA_SQLProcedures(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* ProcName,
  SQLSMALLINT NameLength3);
SQLRETURN MA_SQLProceduresW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* ProcName,
  SQLSMALLINT NameLength3);

SQLRETURN MA_SQLPutData(SQLHSTMT StatementHandle,
  SQLPOINTER DataPtr,
  SQLLEN StrLen_or_Ind);

SQLRETURN MA_SQLRowCount(SQLHSTMT StatementHandle,
  SQLLEN* RowCountPtr);

SQLRETURN MA_SQLSetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength,
    int        isWstr);

SQLRETURN MA_SQLSetCursorName(SQLHSTMT StatementHandle,
  SQLCHAR* CursorName,
  SQLSMALLINT NameLength);
SQLRETURN MA_SQLSetCursorNameW(SQLHSTMT StatementHandle,
  SQLWCHAR* CursorName,
  SQLSMALLINT NameLength);

SQLRETURN MA_SQLSetEnvAttr(SQLHENV EnvironmentHandle,
  SQLINTEGER Attribute,
  SQLPOINTER ValuePtr,
  SQLINTEGER StringLength);

SQLRETURN MA_SQLSetPos(SQLHSTMT StatementHandle,
  SQLSETPOSIROW RowNumber,
  SQLUSMALLINT Operation,
  SQLUSMALLINT LockType);

SQLRETURN MA_SQLSetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength);

SQLRETURN MA_SQLSpecialColumns(SQLHSTMT StatementHandle,
  SQLUSMALLINT IdentifierType,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLUSMALLINT Scope,
  SQLUSMALLINT Nullable);
SQLRETURN MA_SQLSpecialColumnsW(SQLHSTMT StatementHandle,
  SQLUSMALLINT IdentifierType,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLUSMALLINT Scope,
  SQLUSMALLINT Nullable);

SQLRETURN MA_SQLStatistics(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLUSMALLINT Unique,
  SQLUSMALLINT Reserved);
SQLRETURN MA_SQLStatisticsW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLUSMALLINT Unique,
  SQLUSMALLINT Reserved);

SQLRETURN MA_SQLTablePrivileges(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3);
SQLRETURN MA_SQLTablePrivilegesW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3);

SQLRETURN MA_SQLTables(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLCHAR* TableType,
  SQLSMALLINT NameLength4);
SQLRETURN MA_SQLTablesW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLWCHAR* TableType,
  SQLSMALLINT NameLength4);

SQLRETURN MA_SQLSetScrollOptions(SQLHSTMT hstmt,
  SQLUSMALLINT Concurrency,
  SQLLEN       crowKeySet,
  SQLUSMALLINT crowRowSet);

SQLRETURN MA_SQLParamOptions(
  SQLHSTMT hstmt,
  SQLULEN  crow,
  SQLULEN* pirow);

#endif
