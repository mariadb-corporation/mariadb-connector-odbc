/************************************************************************************
   Copyright (C) 2013, 2022 MariaDB Corporation AB
   
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
#ifndef _ma_statement_h_
#define _ma_statement_h_

#include "ma_catalog.h"


struct st_ma_stmt_methods
{
  SQLRETURN(*Prepare)(MADB_Stmt* Stmt, char* StatementText, SQLINTEGER TextLength, BOOL ExecDirect);
  SQLRETURN(*Execute)(MADB_Stmt* Stmt, BOOL ExecDirect);
  SQLRETURN(*Fetch)(MADB_Stmt* Stmt);
  SQLRETURN(*BindColumn)(MADB_Stmt* Stmt, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
    SQLPOINTER TargetValuePtr, SQLLEN BufferLength, SQLLEN* StrLen_or_Ind);
  SQLRETURN(*BindParam)(MADB_Stmt* Stmt, SQLUSMALLINT ParameterNumber, SQLSMALLINT InputOutputType, SQLSMALLINT ValueType,
    SQLSMALLINT ParameterType, SQLULEN ColumnSize, SQLSMALLINT DecimalDigits, SQLPOINTER ParameterValuePtr,
    SQLLEN BufferLength, SQLLEN* StrLen_or_IndPtr);
  SQLRETURN(*ExecDirect)(MADB_Stmt* Stmt, char* StatementText, SQLINTEGER TextLength);
  SQLRETURN(*GetData)(SQLHSTMT StatementHandle, SQLUSMALLINT Col_or_Param_Num, SQLSMALLINT TargetType,
    SQLPOINTER TargetValuePtr, SQLLEN BufferLength, SQLLEN* StrLen_or_IndPtr, BOOL InternalUse);
  SQLRETURN(*RowCount)(MADB_Stmt* Stmt, SQLLEN* RowCountPtr);
  SQLRETURN(*ParamCount)(MADB_Stmt* Stmt, SQLSMALLINT* ParamCountPtr);
  SQLRETURN(*ColumnCount)(MADB_Stmt* Stmt, SQLSMALLINT* ColumnCountPtr);
  SQLRETURN(*GetAttr)(MADB_Stmt* Stmt, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength,
    SQLINTEGER* StringLength);
  SQLRETURN(*SetAttr)(MADB_Stmt* Stmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength);
  SQLRETURN(*StmtFree)(MADB_Stmt* Stmt, SQLUSMALLINT Option);
  SQLRETURN(*ColAttribute)(MADB_Stmt* Stmt, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttributePtr,
    SQLSMALLINT BufferLength, SQLSMALLINT* StringLengthPtr, SQLLEN* NumericAttributePtr,
    my_bool isWchar);
  SQLRETURN(*ColumnPrivileges)(MADB_Stmt* Stmt, char* CatalogName, SQLSMALLINT NameLength1,
    char* SchemaName, SQLSMALLINT NameLength2, char* TableName,
    SQLSMALLINT NameLength3, char* ColumnName, SQLSMALLINT NameLength4);
  SQLRETURN(*TablePrivileges)(MADB_Stmt* Stmt, char* CatalogName, SQLSMALLINT NameLength1,
    char* SchemaName, SQLSMALLINT NameLength2,
    char* TableName, SQLSMALLINT NameLength3);
  SQLRETURN(*Tables)(MADB_Stmt* Stmt, char* CatalogName, SQLSMALLINT NameLength1,
    char* SchemaName, SQLSMALLINT NameLength2, char* TableName,
    SQLSMALLINT NameLength3, char* TableType, SQLSMALLINT NameLength4);
  SQLRETURN(*Statistics)(MADB_Stmt* Stmt, char* CatalogName, SQLSMALLINT NameLength1,
    char* SchemaName, SQLSMALLINT NameLength2,
    char* TableName, SQLSMALLINT NameLength3,
    SQLUSMALLINT Unique, SQLUSMALLINT Reserved);
  SQLRETURN(*Columns)(MADB_Stmt* Stmt, char* CatalogName, SQLSMALLINT NameLength1,
    char* SchemaName, SQLSMALLINT NameLength2,
    char* TableName, SQLSMALLINT NameLength3,
    char* ColumnName, SQLSMALLINT NameLength4);
  SQLRETURN(*ProcedureColumns)(MADB_Stmt* Stmt, char* CatalogName, SQLSMALLINT NameLength1,
    char* SchemaName, SQLSMALLINT NameLength2, char* ProcName,
    SQLSMALLINT NameLength3, char* ColumnName, SQLSMALLINT NameLength4);
  SQLRETURN(*PrimaryKeys)(MADB_Stmt* Stmt, char* CatalogName, SQLSMALLINT NameLength1,
    char* SchemaName, SQLSMALLINT NameLength2, char* TableName,
    SQLSMALLINT NameLength3);
  SQLRETURN(*SpecialColumns)(MADB_Stmt* Stmt, SQLUSMALLINT IdentifierType,
    char* CatalogName, SQLSMALLINT NameLength1,
    char* SchemaName, SQLSMALLINT NameLength2,
    char* TableName, SQLSMALLINT NameLength3,
    SQLUSMALLINT Scope, SQLUSMALLINT Nullable);
  SQLRETURN(*Procedures)(MADB_Stmt* Stmt, char* CatalogName, SQLSMALLINT NameLength1,
    char* SchemaName, SQLSMALLINT NameLength2, char* ProcName,
    SQLSMALLINT NameLength3);
  SQLRETURN(*ForeignKeys)(MADB_Stmt* Stmt, char* PKCatalogName, SQLSMALLINT NameLength1,
    char* PKSchemaName, SQLSMALLINT NameLength2, char* PKTableName,
    SQLSMALLINT NameLength3, char* FKCatalogName, SQLSMALLINT NameLength4,
    char* FKSchemaName, SQLSMALLINT NameLength5, char* FKTableName,
    SQLSMALLINT NameLength6);
  SQLRETURN(*DescribeCol)(MADB_Stmt* Stmt, SQLUSMALLINT ColumnNumber, void* ColumnName,
    SQLSMALLINT BufferLength, SQLSMALLINT* NameLengthPtr,
    SQLSMALLINT* DataTypePtr, SQLULEN* ColumnSizePtr, SQLSMALLINT* DecimalDigitsPtr,
    SQLSMALLINT* NullablePtr, my_bool isWChar);
  SQLRETURN(*SetCursorName)(MADB_Stmt* Stmt, char* Buffer, SQLINTEGER BufferLength);
  SQLRETURN(*GetCursorName)(MADB_Stmt* Stmt, void* CursorName, SQLSMALLINT BufferLength,
    SQLSMALLINT* NameLengthPtr, my_bool isWChar);
  SQLRETURN(*SetPos)(MADB_Stmt* Stmt, SQLSETPOSIROW Row, SQLUSMALLINT Operation,
    SQLUSMALLINT LockType, int ArrayOffset);
  SQLRETURN(*FetchScroll)(MADB_Stmt* Stmt, SQLSMALLINT FetchOrientation,
    SQLLEN FetchOffset);
  SQLRETURN(*ParamData)(MADB_Stmt* Stmt, SQLPOINTER* ValuePtrPtr);
  SQLRETURN(*PutData)(MADB_Stmt* Stmt, SQLPOINTER DataPtr, SQLLEN StrLen_or_Ind);
  SQLRETURN(*BulkOperations)(MADB_Stmt* Stmt, SQLSMALLINT Operation);
  SQLRETURN(*RefreshDynamicCursor)(MADB_Stmt* Stmt);
  SQLRETURN(*RefreshRowPtrs)(MADB_Stmt* Stmt);
  SQLRETURN(*GetOutParams)(MADB_Stmt* Stmt, int CurrentOffset);
};

struct st_ma_stmt_rsstore
{
  int       (*CacheRs)(MADB_Stmt* Stmt);
  /* Transfer current row data from C/C side and store locally. It can be not just the row, but the current rowset,
     probably.*/
  int       (*CacheLocallyCurrentRow)(MADB_Stmt* Stmt);
  int       (*CacheRestOfRs)(MADB_Stmt* Stmt);
  void      (*FreeRs)(MADB_Stmt* Stmt);
};

SQLRETURN    MADB_StmtInit          (MADB_Dbc *Connection, SQLHANDLE *pHStmt);
SQLUSMALLINT MapColAttributeDescType(SQLUSMALLINT FieldIdentifier);
MYSQL_RES*   FetchMetadata          (MADB_Stmt *Stmt);
SQLRETURN    MADB_DoExecute         (MADB_Stmt *Stmt, BOOL ExecDirect);
void         MakeStmtCacher         (MADB_Stmt* Stmt);

#define MADB_MAX_CURSOR_NAME 64 * 4 + 1
#define MADB_CHECK_STMT_HANDLE(a,b)\
  if (!(a) || !(a)->b)\
    return SQL_INVALID_HANDLE

#define MADB_STMT_COLUMN_COUNT(aStmt) (aStmt)->Ird->Header.Count
#define MADB_RESET_COLUMT_COUNT(aStmt) (aStmt)->Ird->Header.Count= 0
#define MADB_STMT_PARAM_COUNT(aStmt)  (aStmt)->ParamCount
#define MADB_POSITIONED_COMMAND(aStmt) ((aStmt)->PositionedCommand && (aStmt)->PositionedCursor)
#define MADB_STMT_HAS_UNIQUE_IDX(aStmt) (aStmt->UniqueIndex != NULL && aStmt->UniqueIndex[0] > 0)
#define MADB_POS_COMM_IDX_FIELD_COUNT(aStmt) (MADB_STMT_HAS_UNIQUE_IDX((aStmt)->PositionedCursor)?(aStmt)->PositionedCursor->UniqueIndex[0]:MADB_STMT_COLUMN_COUNT((aStmt)->PositionedCursor))
#define MADB_STMT_FORGET_NEXT_POS(aStmt) (aStmt)->Cursor.Next= NULL
#define MADB_STMT_RESET_CURSOR(aStmt) (aStmt)->Cursor.Position= -1; MADB_STMT_FORGET_NEXT_POS(aStmt)
#define MADB_STMT_CLOSE_STMT(aStmt)   mysql_stmt_close((aStmt)->stmt);(aStmt)->stmt= NULL
#define MADB_STMT_SHOULD_STREAM(_a) (DSN_OPTION((_a)->Connection, MADB_OPT_FLAG_NO_CACHE) &&\
  (_a)->Options.CursorType == SQL_CURSOR_FORWARD_ONLY)
/* Checks if given Stmt handle is currently streaming (prev variant (MADB_STMT_SHOULD_STREAM(_a) && STMT_EXECUTED(_a) && MADB_STMT_COLUMN_COUNT(_a) > 0) */
#define MADB_STMT_IS_STREAMING(_a) ((_a)->Connection->Streamer == _a)
/* Set given Stmt handle as s current RS streamer on the connection */
#define MADB_STMT_SET_CURRENT_STREAMER(_a) (_a)->Connection->Streamer= (_a)

#endif
