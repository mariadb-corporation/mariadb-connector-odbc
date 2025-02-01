/************************************************************************************
   Copyright (C) 2020,2022 MariaDB Corporation AB
   
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

/**
 * "Internal" ODBC API functions - functions, that have to be called internally if API 
 * function needs to be executed
 *
 * Calling SQLFunction itself inside the connector on non-Windows platforms will result
 * in the driver manager function instead of our own function.
 */

#include "interface/PreparedStatement.h"
#include "interface/ResultSet.h"
#include "ma_odbc.h"
#include "class/ResultSetMetaData.h"
#include "class/Protocol.h"

extern Client_Charset utf8;

SQLRETURN MA_NotImplemented(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
  switch (HandleType) {
  case SQL_HANDLE_DESC:
    return MADB_SetError(&((MADB_Desc*)Handle)->Error, MADB_ERR_IM001, NULL, 0);
  }
  return SQL_ERROR;
}

void MA_ClearError(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
  switch (HandleType) {
  case SQL_HANDLE_DBC:
    MADB_CLEAR_ERROR(&((MADB_Dbc*)Handle)->Error);
    break;
  case SQL_HANDLE_DESC:
    MADB_CLEAR_ERROR(&((MADB_Desc*)Handle)->Error);
    break;
  case SQL_HANDLE_ENV:
    MADB_CLEAR_ERROR(&((MADB_Env*)Handle)->Error);
    break;
  case SQL_HANDLE_STMT:
    MADB_CLEAR_ERROR(&((MADB_Stmt*)Handle)->Error);
    break;
  }
}

/* {{{ MA_SQLAllocHandle */
SQLRETURN MA_SQLAllocHandle(SQLSMALLINT HandleType,
    SQLHANDLE InputHandle,
    SQLHANDLE *OutputHandlePtr)
{
  SQLRETURN ret= SQL_ERROR;
  MADB_Error *Error= nullptr;

  try
  {
    switch (HandleType) {
    case SQL_HANDLE_DBC:
      Error= &((MADB_Env *)InputHandle)->Error;
      MADB_CLEAR_ERROR(Error);
      if ((*OutputHandlePtr= (SQLHANDLE)MADB_DbcInit((MADB_Env *)InputHandle)) != NULL)
      {
        ret= SQL_SUCCESS;
      }
      break;
    case SQL_HANDLE_DESC:
    {
      MDBUG_C_DUMP(InputHandle, InputHandle, 0x);
      MDBUG_C_DUMP(InputHandle, OutputHandlePtr, 0x);
      Error= &((MADB_Dbc *)InputHandle)->Error;
      std::lock_guard<std::mutex> localScopeLock(((MADB_Dbc *)InputHandle)->ListsCs);
      MADB_CLEAR_ERROR(Error);
      if ((*OutputHandlePtr= (SQLHANDLE)MADB_DescInit((MADB_Dbc *)InputHandle, MADB_DESC_UNKNOWN, TRUE)) != NULL)
      {
        ret= SQL_SUCCESS;
      }
      break;
    }
    case SQL_HANDLE_ENV:
      if ((*OutputHandlePtr= (SQLHANDLE)MADB_EnvInit()) != NULL)
      {
        ret= SQL_SUCCESS;
      }
      break;
    case SQL_HANDLE_STMT:
    {
      MDBUG_C_DUMP(InputHandle, InputHandle, 0x);
      MDBUG_C_DUMP(InputHandle, OutputHandlePtr, 0x);
      MADB_Dbc *Connection= (MADB_Dbc *)InputHandle;
      MDBUG_C_ENTER(InputHandle, "MA_SQLAllocHandle(Stmt)");
      Error= &Connection->Error;
      MADB_CLEAR_ERROR(Error);

      if (!Connection->CheckConnection())
      {
        MADB_SetError(&Connection->Error, MADB_ERR_08003, NULL, 0);
        break;
      }

      ret= MADB_StmtInit(Connection, OutputHandlePtr);
      MDBUG_C_DUMP(InputHandle, *OutputHandlePtr, 0x);
      MDBUG_C_RETURN(InputHandle, ret, &Connection->Error);
    }
    break;
    default:
      /* todo: set error message */
      break;
    }
  }
  catch (std::bad_alloc &/*e*/)
  {
    if (Error)
    {
      return MADB_SetError(Error, MADB_ERR_HY001, NULL, 0);
    }
    return SQL_ERROR;
  }
  return ret;
}
/* }}} */

/* {{{ MA_SQLBindCol */
SQLRETURN MA_SQLBindCol(SQLHSTMT StatementHandle,
  SQLUSMALLINT ColumnNumber,
  SQLSMALLINT TargetType,
  SQLPOINTER TargetValuePtr,
  SQLLEN BufferLength,
  SQLLEN* StrLen_or_Ind)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Stmt->Connection, "SQLBindCol");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  MDBUG_C_DUMP(Stmt->Connection, TargetType, d);
  MDBUG_C_DUMP(Stmt->Connection, BufferLength, d);
  MDBUG_C_DUMP(Stmt->Connection, StrLen_or_Ind, 0x);

  try
  {
    ret= Stmt->Methods->BindColumn(Stmt, ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_Ind);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }

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

  MADB_CLEAR_ERROR(&((MADB_Stmt*)StatementHandle)->Error);

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
    
  try
  {
    ret= Stmt->Methods->BindParam(Stmt, ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize, DecimalDigits,
                                  ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLBrowseConnect */
SQLRETURN MA_SQLBrowseConnect(SQLHDBC ConnectionHandle,
  SQLCHAR* InConnectionString,
  SQLSMALLINT StringLength1,
  SQLCHAR* OutConnectionString,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* StringLength2Ptr)
{
  MADB_Dbc* Dbc= (MADB_Dbc*)ConnectionHandle;
  MDBUG_C_ENTER(Dbc, "SQLBrowseConnect");
  SQLRETURN ret= MADB_SetError(&Dbc->Error, MADB_ERR_IM001, NULL, 0);;

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ MA_SQLBulkOperations */
SQLRETURN MA_SQLBulkOperations(SQLHSTMT StatementHandle,
  SQLSMALLINT Operation)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLBulkOperations");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, Operation, d);

  try
  {
    ret= Stmt->Methods->BulkOperations(Stmt, Operation);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLCancel */
SQLRETURN MA_SQLCancel(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret= SQL_ERROR;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLCancel");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);

  auto& lock= Stmt->Connection->guard->getLock();
  
  if (lock.try_lock())
  {
    lock.unlock();
    try
    {
      ret= Stmt->Methods->StmtFree(Stmt, SQL_CLOSE);
    }
    catch (std::bad_alloc &/*e*/)
    {
      ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    }
    catch (SQLException &e)
    {
      ret= MADB_FromException(Stmt->Error, e);
    }

    MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
  }
  else
  {
    // There is nothing, that can throw here
    MYSQL *MariaDb;
    
    char StmtStr[30];

    if (!(MariaDb= mysql_init(NULL)))
    {
      ret= SQL_ERROR;
      goto end;
    }
    if (!SQL_SUCCEEDED(Stmt->Connection->CoreConnect(MariaDb, Stmt->Connection->Dsn, &Stmt->Error)))
    {
      mysql_close(MariaDb);
      goto end;
    }
    
    unsigned long len= static_cast<unsigned long>(_snprintf(StmtStr, 30, "KILL QUERY %ld", mysql_thread_id(Stmt->Connection->mariadb)));
    if (!mysql_real_query(MariaDb, StmtStr, len))
    {
      ret= SQL_SUCCESS;
    }
    mysql_close(MariaDb);
  }
end:
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLCancelDbc */
SQLRETURN MA_SQLCancelDbc(SQLHANDLE Handle)
{
  MADB_Stmt Stmt(static_cast<MADB_Dbc*>(Handle));
  return MA_SQLCancel((SQLHSTMT)&Stmt);
}
/* }}} */

/* {{{ MA_SQLCloseCursor */
SQLRETURN MA_SQLCloseCursor(SQLHSTMT StatementHandle)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLCloseCursor");
  MDBUG_C_DUMP(Stmt->Connection, StatementHandle, 0x);

  if (!Stmt->rs && Stmt->Connection->Environment->OdbcVersion >= SQL_OV_ODBC3)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_24000, NULL, 0);
    ret= Stmt->Error.ReturnValue;
  }
  else
  {
    try
    {
      ret= Stmt->Methods->StmtFree(Stmt, SQL_CLOSE);
    }
    catch (std::bad_alloc &/*e*/)
    {
      ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    }
    catch (SQLException &e)
    {
      ret= MADB_FromException(Stmt->Error, e);
    }
  }
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLColAttribute */
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
)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLColAttribute");
  MDBUG_C_DUMP(Stmt->Connection, StatementHandle, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);
  MDBUG_C_DUMP(Stmt->Connection, FieldIdentifier, u);
  MDBUG_C_DUMP(Stmt->Connection, CharacterAttributePtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, BufferLength, d);
  MDBUG_C_DUMP(Stmt->Connection, StringLengthPtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, NumericAttributePtr, 0x);

  try
  {
    ret= Stmt->Methods->ColAttribute(Stmt, ColumnNumber, FieldIdentifier, CharacterAttributePtr,
      BufferLength, StringLengthPtr, static_cast<SQLLEN*>(NumericAttributePtr), isWchar);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLColumnPrivileges */
SQLRETURN MA_SQLColumnPrivileges(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLCHAR* ColumnName,
  SQLSMALLINT NameLength4)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumnPrivileges");
  try
  {
    ret= Stmt->Methods->ColumnPrivileges(Stmt, (char*)CatalogName, NameLength1, (char*)SchemaName, NameLength2,
      (char*)TableName, NameLength3, (char*)ColumnName, NameLength4);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLColumnPrivilegesW */
SQLRETURN MA_SQLColumnPrivilegesW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLWCHAR* ColumnName,
  SQLSMALLINT NameLength4)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0, CpLength4= 0;
  char* CpCatalog= NULL,
    * CpSchema= NULL,
    * CpTable= NULL,
    * CpColumn= NULL;
  SQLRETURN ret;

  if (!StatementHandle)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumnPrivilegesW");
  try
  {
    if (CatalogName != NULL)
    {
      CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (SchemaName != NULL)
    {
      CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (TableName != NULL)
    {
      CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (ColumnName != NULL)
    {
      CpColumn= MADB_ConvertFromWChar(ColumnName, NameLength4, &CpLength4, Stmt->Connection->ConnOrSrcCharset, NULL);
    }

    ret= Stmt->Methods->ColumnPrivileges(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
      CpTable, (SQLSMALLINT)CpLength3, CpColumn, (SQLSMALLINT)CpLength4);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  MADB_FREE(CpColumn);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLColumns */
SQLRETURN MA_SQLColumns(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLCHAR* ColumnName,
  SQLSMALLINT NameLength4)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumns");
  try
  {
    ret= Stmt->Methods->Columns(Stmt, (char*)CatalogName, NameLength1, (char*)SchemaName, NameLength2,
      (char*)TableName, NameLength3, (char*)ColumnName, NameLength4);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLColumnsW */
SQLRETURN MA_SQLColumnsW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLWCHAR* ColumnName,
  SQLSMALLINT NameLength4)
{
  char* CpCatalog= NULL,
    * CpSchema= NULL,
    * CpTable= NULL,
    * CpColumn= NULL;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0, CpLength4= 0;
  SQLRETURN ret;
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLColumns");

  try
  {
    // Converting cannot throw so far, but that can change
    if (CatalogName != NULL)
    {
      CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (SchemaName != NULL)
    {
      CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (TableName != NULL)
    {
      CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (ColumnName != NULL)
    {
      CpColumn= MADB_ConvertFromWChar(ColumnName, NameLength4, &CpLength4, Stmt->Connection->ConnOrSrcCharset, NULL);
    }

    ret= Stmt->Methods->Columns(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
      CpTable, (SQLSMALLINT)CpLength3, CpColumn, (SQLSMALLINT)CpLength4);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }

  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  MADB_FREE(CpColumn);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ SQLConnectCommon */
SQLRETURN SQLConnectCommon(SQLHDBC ConnectionHandle,
  SQLCHAR* ServerName,
  SQLSMALLINT NameLength1,
  SQLCHAR* UserName,
  SQLSMALLINT NameLength2,
  SQLCHAR* Authentication,
  SQLSMALLINT NameLength3)
{
  MADB_Dbc* Connection= (MADB_Dbc*)ConnectionHandle;
  MADB_Dsn* Dsn;
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

  try
  {
    if (Connection->CheckConnection())
    {
      return MADB_SetError(&Connection->Error, MADB_ERR_08002, NULL, 0);
    }

    if (!(Dsn= MADB_DSN_Init(NULL)))
    {
      return MADB_SetError(&Connection->Error, MADB_ERR_HY001, NULL, 0);
    }

    if (ServerName && !ServerName[0])
    {
      MADB_SetError(&Connection->Error, MADB_ERR_HY000, "Invalid DSN", 0);
      MADB_DSN_Free(Dsn);
      return Connection->Error.ReturnValue;
    }

    MADB_DSN_SET_STR(Dsn, DSNName, (char*)ServerName, NameLength1);
    DsnFound= MADB_ReadDSN(Dsn, NULL, TRUE);

    MADB_DSN_SET_STR(Dsn, UserName, (char*)UserName, NameLength2);
    MADB_DSN_SET_STR(Dsn, Password, (char*)Authentication, NameLength3);

    ret= Connection->ConnectDB(Dsn);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Connection->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Connection->Error, e);
  }

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

/* {{{ MA_SQLConnectW */
SQLRETURN MA_SQLConnectW(SQLHDBC ConnectionHandle,
  SQLWCHAR* ServerName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* UserName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* Authentication,
  SQLSMALLINT NameLength3)
{
  char* MBServerName= NULL, * MBUserName= NULL, * MBAuthentication= NULL;
  SQLRETURN ret;
  MADB_Dbc* Dbc= (MADB_Dbc*)ConnectionHandle;

  MADB_CLEAR_ERROR(&Dbc->Error);

  // There are traps in SQLConnectCommon, and conwerting cannot throw atm
  /*try
  {*/
  /* Convert parameters to Cp */
  if (ServerName)
    MBServerName= MADB_ConvertFromWChar(ServerName, NameLength1, 0, Dbc->IsAnsi ? Dbc->ConnOrSrcCharset : &utf8, NULL);
  if (UserName)
    MBUserName= MADB_ConvertFromWChar(UserName, NameLength2, 0, Dbc->IsAnsi ? Dbc->ConnOrSrcCharset : &utf8, NULL);
  if (Authentication)
    MBAuthentication= MADB_ConvertFromWChar(Authentication, NameLength3, 0, Dbc->IsAnsi ? Dbc->ConnOrSrcCharset : &utf8, NULL);

  ret= SQLConnectCommon(ConnectionHandle, (SQLCHAR*)MBServerName, SQL_NTS, (SQLCHAR*)MBUserName, SQL_NTS,
    (SQLCHAR*)MBAuthentication, SQL_NTS);
  MADB_FREE(MBServerName);
  MADB_FREE(MBUserName);
  MADB_FREE(MBAuthentication);
  return ret;
}
/* }}} */

/* {{{ MA_SQLDescribeCol */
SQLRETURN MA_SQLDescribeCol(SQLHSTMT StatementHandle,
  SQLUSMALLINT ColumnNumber,
  void* ColumnName,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* NameLengthPtr,
  SQLSMALLINT* DataTypePtr,
  SQLULEN* ColumnSizePtr,
  SQLSMALLINT* DecimalDigitsPtr,
  SQLSMALLINT* NullablePtr,
  char isWchar)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLDescribeCol");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, ColumnNumber, u);

  try
  {
    ret= Stmt->Methods->DescribeCol(Stmt, ColumnNumber, (void*)ColumnName, BufferLength,
      NameLengthPtr, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr,
      NullablePtr, isWchar);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLDriverConnect */
SQLRETURN MA_SQLDriverConnect(SQLHDBC ConnectionHandle,
  SQLHWND WindowHandle,
  SQLCHAR* InConnectionString,
  SQLSMALLINT StringLength1,
  SQLCHAR* OutConnectionString,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* StringLength2Ptr,
  SQLUSMALLINT DriverCompletion)
{
  MADB_Dbc* Dbc= (MADB_Dbc*)ConnectionHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Dbc, "SQLDriverConnect");
  MDBUG_C_DUMP(Dbc, Dbc, 0x);
  MDBUG_C_DUMP(Dbc, InConnectionString, s);
  MDBUG_C_DUMP(Dbc, StringLength1, d);
  MDBUG_C_DUMP(Dbc, OutConnectionString, 0x);
  MDBUG_C_DUMP(Dbc, BufferLength, d);
  MDBUG_C_DUMP(Dbc, StringLength2Ptr, 0x);
  MDBUG_C_DUMP(Dbc, DriverCompletion, d);
  try
  {
    ret= Dbc->DriverConnect(WindowHandle, InConnectionString, StringLength1, OutConnectionString,
      BufferLength, StringLength2Ptr, DriverCompletion);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Dbc->Error, e);
  }
  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ MA_SQLDriverConnectW */
SQLRETURN MA_SQLDriverConnectW(SQLHDBC ConnectionHandle,
  SQLHWND      WindowHandle,
  SQLWCHAR* InConnectionString,
  SQLSMALLINT  StringLength1,
  SQLWCHAR* OutConnectionString,
  SQLSMALLINT  BufferLength,
  SQLSMALLINT* StringLength2Ptr,
  SQLUSMALLINT DriverCompletion)
{
  SQLRETURN   ret= SQL_ERROR;
  SQLULEN     Length= 0; /* Since we need bigger(in bytes) buffer for utf8 string, the length may be > max SQLSMALLINT */
  char       *InConnStrA= nullptr;
  SQLULEN     InStrAOctLen= 0;
  char       *OutConnStrA= nullptr;
  MADB_Dbc   *Dbc= (MADB_Dbc*)ConnectionHandle;

  MDBUG_C_ENTER(Dbc, "SQLDriverConnectW");

  try
  {
    InConnStrA= MADB_ConvertFromWChar(InConnectionString, StringLength1, &InStrAOctLen, Dbc->IsAnsi ? Dbc->ConnOrSrcCharset : &utf8, nullptr);
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
      Length= BufferLength * 4 /* Max bytes per utf8 character */;
      OutConnStrA= (char*)MADB_CALLOC(Length);

      if (OutConnStrA == nullptr)
      {
        ret= MADB_SetError(&Dbc->Error, MADB_ERR_HY001, nullptr, 0);
        goto end;
      }
    }
    SQLSMALLINT OutStrLengthA= 0;
    if (!StringLength2Ptr)
    {
      StringLength2Ptr= &OutStrLengthA;
    }

    ret= Dbc->DriverConnect(WindowHandle, (SQLCHAR*)InConnStrA, InStrAOctLen, (SQLCHAR*)OutConnStrA,
      Length, StringLength2Ptr, DriverCompletion);
    MDBUG_C_DUMP(Dbc, ret, d);
    if (!SQL_SUCCEEDED(ret))
      goto end;

    // If we have what to transcode. And we would have something, if we had where to write it
    if (OutConnStrA)
    {
      //Above we've made sure, that StringLength2Ptr is not null
      *StringLength2Ptr= static_cast<SQLSMALLINT>(MADB_SetString(&utf8, OutConnectionString, BufferLength,
        OutConnStrA, *StringLength2Ptr, &((MADB_Dbc*)ConnectionHandle)->Error));
    }
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Dbc->Error, e);
  }

end:
  MADB_FREE(OutConnStrA);
  MADB_FREE(InConnStrA);
  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ MA_SQLEndTran */
SQLRETURN MA_SQLEndTran(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT CompletionType)
{

  switch (HandleType) {
  case SQL_HANDLE_ENV:
    {
      MADB_Env *Env= (MADB_Env *)Handle;

      for (auto& con: Env->Dbcs)
      {
        try
        {
          if (con->mariadb)
          {
            con->EndTran(CompletionType);
          }
        }
        catch (std::bad_alloc &/*e*/)
        {
          // Probably does not make much sense to continue
          return MADB_SetError(&Env->Error, MADB_ERR_HY001, NULL, 0);
        }
        catch (SQLException &)
        {
          // TODO: what should happen here if error occurs on some connection?
        }
      }
    }
    break;
  case SQL_HANDLE_DBC:
    {
      MADB_Dbc *Dbc= (MADB_Dbc *)Handle;
      if (!Dbc->mariadb)
      {
        return MADB_SetError(&Dbc->Error, MADB_ERR_08002, NULL, 0);
      }
      else
      {
        try
        {
          Dbc->EndTran(CompletionType);
        }
        catch (std::bad_alloc &/*e*/)
        {
          return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
        }
        catch (SQLException &e)
        {
          return MADB_FromException(Dbc->Error, e);
        }
      }
    }
    break;
  default:
    /* todo: Do we need to set an error ?! */
    break;
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLError */
SQLRETURN MA_SQLError(SQLHENV Env, SQLHDBC Dbc, SQLHSTMT Stmt,
  void* Sqlstate, SQLINTEGER* NativeError,
  void* Message, SQLSMALLINT MessageMax,
  SQLSMALLINT* MessageLen, int isWchar)
{
  SQLSMALLINT HandleType= 0;
  SQLHANDLE   Handle= NULL;
  MADB_Error* error;

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

  try
  {
    if (isWchar)
    {
      return MA_SQLGetDiagRecW(HandleType, Handle, ++error->ErrorNum, (SQLWCHAR*)Sqlstate, NativeError, (SQLWCHAR*)Message, MessageMax, MessageLen);
    }
    return MA_SQLGetDiagRec(HandleType, Handle, ++error->ErrorNum, (SQLCHAR*)Sqlstate, NativeError, (SQLCHAR*)Message, MessageMax, MessageLen);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(*error, e);
  }
  return SQL_SUCCESS;
}

/* {{{ MA_SQLExecDirect */
SQLRETURN MA_SQLExecDirect(SQLHSTMT StatementHandle,
  SQLCHAR* StatementText,
  SQLINTEGER TextLength)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  if (!Stmt)
    ret= SQL_INVALID_HANDLE;
  else
  {
    try
    {
      ret= Stmt->Methods->ExecDirect(Stmt, (char*)StatementText, TextLength);
    }
    catch (SQLException &e)
    {
      ret= MADB_FromException(Stmt->Error, e);
    }
    catch (const std::bad_alloc&)
    {
      ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    }
  }

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLExecDirectW */
SQLRETURN MA_SQLExecDirectW(SQLHSTMT StatementHandle,
  SQLWCHAR* StatementText,
  SQLINTEGER TextLength)
{
  char* CpStmt;
  SQLULEN   StmtLength;
  SQLRETURN ret;
  BOOL      ConversionError;

  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;

  MDBUG_C_ENTER(Stmt->Connection, "SQLExecDirectW");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);

  try
  {
    CpStmt= MADB_ConvertFromWChar(StatementText, TextLength, &StmtLength, Stmt->Connection->ConnOrSrcCharset, &ConversionError);
    MDBUG_C_DUMP(Stmt->Connection, CpStmt, s);
    if (ConversionError)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_22018, NULL, 0);
      ret= Stmt->Error.ReturnValue;
    }
    else
      ret= Stmt->Methods->ExecDirect(Stmt, CpStmt, (SQLINTEGER)StmtLength);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  catch (const std::bad_alloc&)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  MADB_FREE(CpStmt);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLExecute */
SQLRETURN MA_SQLExecute(SQLHSTMT StatementHandle)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;

  MDBUG_C_ENTER(Stmt->Connection, "SQLExecute");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  
  try
  {
    return Stmt->Methods->Execute(Stmt, false);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  catch (MADB_Error &Err)
  {
    // Assuming that this is Err from the handle, and we do not need to copy anything
    return Err.ReturnValue;
  }
  catch (const std::bad_alloc&)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
}
/* }}} */

/* {{{ MA_SQLExtendedFetch */
SQLRETURN MA_SQLExtendedFetch(SQLHSTMT StatementHandle,
  SQLUSMALLINT FetchOrientation,
  SQLLEN FetchOffset,
  SQLULEN* RowCountPtr,
  SQLUSMALLINT* RowStatusArray)
{
  SQLRETURN ret;
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;

  SQLULEN* SaveRowsProcessedPtr= Stmt->Ird->Header.RowsProcessedPtr;
  SQLUSMALLINT* SaveArrayStatusPtr= Stmt->Ird->Header.ArrayStatusPtr;

  MDBUG_C_ENTER(Stmt->Connection, "SQLExtendedFetch");
  MDBUG_C_DUMP(Stmt->Connection, FetchOrientation, u);
  MDBUG_C_DUMP(Stmt->Connection, FetchOffset, d);
  MDBUG_C_DUMP(Stmt->Connection, RowCountPtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, RowStatusArray, 0x);

  Stmt->Ird->Header.RowsProcessedPtr= RowCountPtr;
  Stmt->Ird->Header.ArrayStatusPtr= RowStatusArray;
  try
  {
    ret= Stmt->Methods->FetchScroll(Stmt, FetchOrientation, FetchOffset);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }

  if (RowStatusArray && SaveArrayStatusPtr)
  {
    SQLUINTEGER i;
    for (i= 0; i < Stmt->Ard->Header.ArraySize; i++)
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

/* {{{ MA_SQLFetch */
SQLRETURN MA_SQLFetch(SQLHSTMT StatementHandle)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;

  MDBUG_C_ENTER(Stmt->Connection, "SQLFetch");

  /* SQLFetch is equivalent of SQLFetchScroll(SQL_FETCH_NEXT), 3rd parameter is ignored for SQL_FETCH_NEXT */
  try
  {
    MDBUG_C_RETURN(Stmt->Connection, Stmt->Methods->FetchScroll(Stmt, SQL_FETCH_NEXT, 1), &Stmt->Error);
  }
  catch (std::bad_alloc &/*e*/)
  {
    MDBUG_C_RETURN(Stmt->Connection, MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0), &Stmt->Error);
  }
  catch (SQLException &e)
  {
    MDBUG_C_RETURN(Stmt->Connection, MADB_FromException(Stmt->Error, e), &Stmt->Error);
  }
  catch (MADB_Error &Err)
  {
    // Assuming that this is Err from the handle, and we do not need to copy anything
    return Err.ReturnValue;
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLFetchScroll */
SQLRETURN MA_SQLFetchScroll(SQLHSTMT StatementHandle,
  SQLSMALLINT FetchOrientation,
  SQLLEN FetchOffset)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;

  MDBUG_C_ENTER(Stmt->Connection, "SQLFetchScroll");
  MDBUG_C_DUMP(Stmt->Connection, FetchOrientation, d);

  try
  {
    MDBUG_C_RETURN(Stmt->Connection, Stmt->Methods->FetchScroll(Stmt, FetchOrientation, FetchOffset), &Stmt->Error);
  }
  catch (std::bad_alloc &/*e*/)
  {
    MDBUG_C_RETURN(Stmt->Connection, MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0), &Stmt->Error);
  }
  catch (SQLException &e)
  {
    MDBUG_C_RETURN(Stmt->Connection, MADB_FromException(Stmt->Error, e), &Stmt->Error);
  }
  catch (MADB_Error &Err)
  {
    // Assuming that this is Err from the handle, and we do not need to copy anything
    return Err.ReturnValue;
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLFreeHandle */
SQLRETURN MA_SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
  SQLRETURN ret= SQL_INVALID_HANDLE;
  switch (HandleType)
  {
  case SQL_HANDLE_ENV:
    MDBUG_ENTER("SQLFreeHandle");
    MDBUG_DUMP(HandleType, d);
    MDBUG_DUMP(Handle, 0x);

    ret= MADB_EnvFree((MADB_Env*)Handle);
    break;
  case SQL_HANDLE_DBC:
  {
    MADB_Dbc* Dbc= (MADB_Dbc*)Handle;

    MDBUG_C_ENTER(Dbc, "SQLFreeHandle");
    MDBUG_C_DUMP(Dbc, HandleType, d);
    MDBUG_C_DUMP(Dbc, Handle, 0x);

    ret= MADB_DbcFree(Dbc);
    return ret;
    /*MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);*/
  }
  case SQL_HANDLE_DESC:
  {
    MADB_Desc* Desc= (MADB_Desc*)Handle;
    MADB_Dbc* Dbc= Desc->Dbc;

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
    MADB_Stmt* Stmt= (MADB_Stmt*)Handle;
    MADB_Dbc* Dbc= Stmt->Connection;

    MDBUG_C_ENTER(Dbc, "SQLFreeHandle");
    MDBUG_C_DUMP(Dbc, HandleType, d);
    MDBUG_C_DUMP(Dbc, Handle, 0x);

    try
    {
      ret= Stmt->Methods->StmtFree(Stmt, SQL_DROP);
    }
    catch (std::bad_alloc &/*e*/)
    {
      ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    }
    catch (SQLException &e)
    {
      ret= MADB_FromException(Stmt->Error, e);
    }
    MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
  }
  }
  MDBUG_RETURN(ret);
}
/* }}} */

/* {{{ MA_SQLFreeStmt */
SQLRETURN MA_SQLFreeStmt(SQLHSTMT StatementHandle,
  SQLUSMALLINT Option)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;

  MDBUG_C_ENTER(((MADB_Stmt*)StatementHandle)->Connection, "SQLFreeStmt");
  MDBUG_C_DUMP(Stmt->Connection, StatementHandle, 0x);
  MDBUG_C_DUMP(Stmt->Connection, Option, d);

  try
  {
    return Stmt->Methods->StmtFree(Stmt, Option);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLForeignKeys */
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
  SQLSMALLINT NameLength6)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Stmt->Connection, "SQLForeignKeys");

  try
  {
    ret= Stmt->Methods->ForeignKeys(Stmt, (char*)PKCatalogName, NameLength1, (char*)PKSchemaName, NameLength2,
      (char*)PKTableName, NameLength3, (char*)FKCatalogName, NameLength4,
      (char*)FKSchemaName, NameLength4, (char*)FKTableName, NameLength6);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLForeignKeysW */
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
  SQLSMALLINT NameLength6)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  char* CpPkCatalog= NULL,
    * CpPkSchema= NULL,
    * CpPkTable= NULL,
    * CpFkCatalog= NULL,
    * CpFkSchema= NULL,
    * CpFkTable= NULL;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0,
    CpLength4= 0, CpLength5= 0, CpLength6= 0;
  SQLRETURN ret;

  MDBUG_C_ENTER(Stmt->Connection, "SQLForeignKeysW");

  try
  {
    if (PKCatalogName != NULL)
    {
      CpPkCatalog= MADB_ConvertFromWChar(PKCatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (PKSchemaName != NULL)
    {
      CpPkSchema= MADB_ConvertFromWChar(PKSchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (PKTableName != NULL)
    {
      CpPkTable= MADB_ConvertFromWChar(PKTableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (FKCatalogName != NULL)
    {
      CpFkCatalog= MADB_ConvertFromWChar(FKCatalogName, NameLength4, &CpLength4, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (FKSchemaName != NULL)
    {
      CpFkSchema= MADB_ConvertFromWChar(FKSchemaName, NameLength5, &CpLength5, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (FKTableName != NULL)
    {
      CpFkTable= MADB_ConvertFromWChar(FKTableName, NameLength6, &CpLength6, Stmt->Connection->ConnOrSrcCharset, NULL);
    }

    ret= Stmt->Methods->ForeignKeys(Stmt, CpPkCatalog, (SQLSMALLINT)CpLength1, CpPkSchema, (SQLSMALLINT)CpLength2,
      CpPkTable, (SQLSMALLINT)CpLength3, CpFkCatalog, (SQLSMALLINT)CpLength4,
      CpFkSchema, (SQLSMALLINT)CpLength5, CpFkTable, (SQLSMALLINT)CpLength6);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MADB_FREE(CpPkCatalog);
  MADB_FREE(CpPkSchema);
  MADB_FREE(CpPkTable);
  MADB_FREE(CpFkCatalog);
  MADB_FREE(CpFkSchema);
  MADB_FREE(CpFkTable);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLGetConnectAttrW */
SQLRETURN MA_SQLGetConnectAttrW(SQLHDBC ConnectionHandle,
  SQLINTEGER Attribute,
  SQLPOINTER ValuePtr,
  SQLINTEGER BufferLength,
  SQLINTEGER* StringLengthPtr)
{
  MADB_Dbc* Dbc= (MADB_Dbc*)ConnectionHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Dbc, "SQLGetConnectAttr");
  MDBUG_C_DUMP(Dbc, Attribute, d);
  MDBUG_C_DUMP(Dbc, ValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, BufferLength, d);
  MDBUG_C_DUMP(Dbc, StringLengthPtr, 0x);
  try
  {
    ret= Dbc->GetAttr(Attribute, ValuePtr, BufferLength, StringLengthPtr, true);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Dbc->Error, e);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ MA_SQLGetCursorName */
SQLRETURN MA_SQLGetCursorName(
  SQLHSTMT StatementHandle,
  void* CursorName,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* NameLengthPtr,
  int isWstr)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  try
  {
    return Stmt->Methods->GetCursorName(Stmt, CursorName, BufferLength, NameLengthPtr, isWstr);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLGetData */
SQLRETURN MA_SQLGetData(SQLHSTMT StatementHandle,
  SQLUSMALLINT Col_or_Param_Num,
  SQLSMALLINT TargetType,
  SQLPOINTER TargetValuePtr,
  SQLLEN BufferLength,
  SQLLEN* StrLen_or_IndPtr)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  unsigned int i;
  MADB_DescRecord* IrdRec;

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
  if (Stmt->CharOffset[Col_or_Param_Num - 1] > 0
    && Stmt->CharOffset[Col_or_Param_Num - 1] >= Stmt->Lengths[Col_or_Param_Num - 1])
  {
    return SQL_NO_DATA;
  }

  if (BufferLength < 0)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY090, NULL, 0);
  }

  /* reset offsets for other columns. Doing that here since "internal" calls should not do that */
  for (i= 0; i < Stmt->metadata->getColumnCount(); i++)
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

  try
  {
    return Stmt->Methods->GetData(StatementHandle, Col_or_Param_Num, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr, false);
  }
  catch (MADB_Error &Err)
  {
    // Assuming that this is Err from the handle, and we do not need to copy anything
    return Err.ReturnValue;
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  // Just to calm down compilers
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLGetDiagRecW */
SQLRETURN MA_SQLGetDiagRecW(SQLSMALLINT HandleType,
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

/* {{{ MA_SQLGetConnectAttr */
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

  try
  {
    ret= Dbc->GetAttr(Attribute, ValuePtr, BufferLength, StringLengthPtr, FALSE);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Dbc->Error, e);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
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
/* }}} */

/* {{{ MA_SQLGetStmtAttr */
SQLRETURN MA_SQLGetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  // GetAttr method will figure out if this is wide or ansi string
  try
  {
    return Stmt->Methods->GetAttr(Stmt, Attribute, ValuePtr, BufferLength, StringLengthPtr);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLGetEnvAttr */
SQLRETURN MA_SQLGetEnvAttr(SQLHENV EnvironmentHandle,
  SQLINTEGER Attribute,
  SQLPOINTER ValuePtr,
  SQLINTEGER BufferLength,
  SQLINTEGER* StringLengthPtr)
{
  MADB_Env* Env= (MADB_Env*)EnvironmentHandle;
  SQLRETURN ret;

  MDBUG_ENTER("SQLGetEnvAttr");
  MDBUG_DUMP(Attribute, d);
  MDBUG_DUMP(ValuePtr, 0x);
  MDBUG_DUMP(BufferLength, d);
  MDBUG_DUMP(StringLengthPtr, 0x);

  ret= MADB_EnvGetAttr(Env, Attribute, ValuePtr, BufferLength, StringLengthPtr);

  MDBUG_DUMP(ret, d);
  MDBUG_RETURN(ret);
}

/* {{{ MA_SQLGetFunctions */
SQLRETURN MA_SQLGetFunctions(SQLHDBC ConnectionHandle,
  SQLUSMALLINT FunctionId,
  SQLUSMALLINT* SupportedPtr)
{
  MADB_Dbc* Dbc= (MADB_Dbc*)ConnectionHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Dbc, "SQLGetFunctions");
  MDBUG_C_DUMP(Dbc, FunctionId, d);
  MDBUG_C_DUMP(Dbc, SupportedPtr, 0x);
  try
  {
    ret= Dbc->GetFunctions(FunctionId, SupportedPtr);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ MA_SQLGetInfo */
SQLRETURN MA_SQLGetInfo(SQLHDBC ConnectionHandle,
  SQLUSMALLINT InfoType,
  SQLPOINTER InfoValuePtr,
  SQLSMALLINT BufferLength,
  SQLSMALLINT* StringLengthPtr,
  int isWstr)
{
  MADB_Dbc* Dbc= (MADB_Dbc*)ConnectionHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Dbc, "SQLGetInfo");
  MDBUG_C_DUMP(Dbc, InfoType, d);
  try
  {
    ret= Dbc->GetInfo(InfoType, InfoValuePtr, BufferLength, StringLengthPtr, isWstr);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Dbc->Error, e);
  }

  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ MA_SQLNativeSql */
SQLRETURN MA_SQLNativeSql(SQLHDBC ConnectionHandle,
  SQLCHAR* InStatementText,
  SQLINTEGER TextLength1,
  SQLCHAR* OutStatementText,
  SQLINTEGER BufferLength,
  SQLINTEGER* TextLength2Ptr)
{
  MADB_Dbc* Dbc= (MADB_Dbc*)ConnectionHandle;
  SQLINTEGER Length;
  if (!Dbc)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Dbc->Error);

  if (!TextLength2Ptr && (!OutStatementText || !BufferLength))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_01004, NULL, 0);
    return Dbc->Error.ReturnValue;
  }
  Length= (SQLINTEGER)MADB_SetString(0, OutStatementText, BufferLength, (char*)InStatementText, TextLength1, &Dbc->Error);
  if (TextLength2Ptr)
    *TextLength2Ptr= Length;
  return Dbc->Error.ReturnValue;
}
/* }}} */

/* {{{ MA_SQLNativeSqlW */
SQLRETURN MA_SQLNativeSqlW(SQLHDBC ConnectionHandle,
  SQLWCHAR* InStatementText,
  SQLINTEGER TextLength1,
  SQLWCHAR* OutStatementText,
  SQLINTEGER BufferLength,
  SQLINTEGER* TextLength2Ptr)
{
  MADB_Dbc* Conn= (MADB_Dbc*)ConnectionHandle;
  SQLINTEGER Length= (TextLength1 == SQL_NTS) ? SqlwcsCharLen(InStatementText, (SQLLEN)-1) : TextLength1;

  if (TextLength2Ptr)
    *TextLength2Ptr= Length;

  if (OutStatementText && BufferLength < Length)
    MADB_SetError(&Conn->Error, MADB_ERR_01004, NULL, 0);

  if (OutStatementText && BufferLength < Length)
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

/* {{{ MA_SQLNumParams */
SQLRETURN MA_SQLNumParams(SQLHSTMT StatementHandle,
  SQLSMALLINT* ParameterCountPtr)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  return Stmt->Methods->ParamCount(Stmt, ParameterCountPtr);
}
/* }}} */

/* {{{ MA_SQLNumResultCols */
SQLRETURN MA_SQLNumResultCols(SQLHSTMT StatementHandle,
  SQLSMALLINT* ColumnCountPtr)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  return Stmt->Methods->ColumnCount(Stmt, ColumnCountPtr);
}
/* }}} */

/* {{{ MA_SQLParamData */
SQLRETURN MA_SQLParamData(SQLHSTMT StatementHandle,
  SQLPOINTER* ValuePtrPtr)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  try
  {
    return Stmt->Methods->ParamData(Stmt, ValuePtrPtr);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLPrepare */
SQLRETURN MA_SQLPrepare(SQLHSTMT StatementHandle,
  SQLCHAR* StatementText,
  SQLINTEGER TextLength)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;

  MDBUG_C_ENTER(Stmt->Connection, "SQLPrepare");

  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);
  MDBUG_C_DUMP(Stmt->Connection, StatementText, s);
  MDBUG_C_DUMP(Stmt->Connection, TextLength, d);
  try
  {
    return Stmt->Prepare((char*)StatementText, TextLength, !Stmt->Options.PrepareOnClient);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  catch (const std::bad_alloc&)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLPrepareW */
SQLRETURN MA_SQLPrepareW(SQLHSTMT StatementHandle,
  SQLWCHAR* StatementText,
  SQLINTEGER TextLength)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  char* StmtStr;
  SQLULEN StmtLength;
  SQLRETURN ret;
  BOOL ConversionError;

  MDBUG_C_ENTER(Stmt->Connection, "SQLPrepareW");

  try // Currently string transcoding cannot throw, but in future it most cetainly will
  {
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
    {
      ret= Stmt->Prepare(StmtStr, (SQLINTEGER)StmtLength, !Stmt->Options.PrepareOnClient);
    }
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  catch (const std::bad_alloc&)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  MADB_FREE(StmtStr);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLPrimaryKeys */
SQLRETURN MA_SQLPrimaryKeys(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Stmt->Connection, "SQLPrimaryKeys");
  MDBUG_C_DUMP(Stmt->Connection, StatementHandle, 0x);
  MDBUG_C_DUMP(Stmt->Connection, CatalogName, s);
  MDBUG_C_DUMP(Stmt->Connection, NameLength1, d);
  MDBUG_C_DUMP(Stmt->Connection, SchemaName, s);
  MDBUG_C_DUMP(Stmt->Connection, NameLength2, d);
  MDBUG_C_DUMP(Stmt->Connection, TableName, s);
  MDBUG_C_DUMP(Stmt->Connection, NameLength3, d);

  try
  {
    ret= Stmt->Methods->PrimaryKeys(Stmt, (char*)CatalogName, NameLength1, (char*)SchemaName, NameLength2,
      (char*)TableName, NameLength3);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  catch (const std::bad_alloc&)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLPrimaryKeysW */
SQLRETURN MA_SQLPrimaryKeysW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  char* CpCatalog= NULL,
    * CpSchema= NULL,
    * CpTable= NULL;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0;
  SQLRETURN ret;

  try
  {
    if (CatalogName != NULL)
    {
      CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (SchemaName != NULL)
    {
      CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (TableName != NULL)
    {
      CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
    }

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
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLProcedureColumns */
SQLRETURN MA_SQLProcedureColumns(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* ProcName,
  SQLSMALLINT NameLength3,
  SQLCHAR* ColumnName,
  SQLSMALLINT NameLength4)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  try
  {
    return Stmt->Methods->ProcedureColumns(Stmt, (char*)CatalogName, NameLength1, (char*)SchemaName, NameLength2,
      (char*)ProcName, NameLength3, (char*)ColumnName, NameLength4);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLProcedureColumnsW */
SQLRETURN MA_SQLProcedureColumnsW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* ProcName,
  SQLSMALLINT NameLength3,
  SQLWCHAR* ColumnName,
  SQLSMALLINT NameLength4)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;
  char* CpCatalog= NULL,
    * CpSchema= NULL,
    * CpProc= NULL,
    * CpColumn= NULL;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0, CpLength4= 0;

  try
  {
    if (CatalogName != NULL)
    {
      CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (SchemaName != NULL)
    {
      CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (ProcName != NULL)
    {
      CpProc= MADB_ConvertFromWChar(ProcName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (ColumnName != NULL)
    {
      CpColumn= MADB_ConvertFromWChar(ColumnName, NameLength4, &CpLength4, Stmt->Connection->ConnOrSrcCharset, NULL);
    }

    ret= Stmt->Methods->ProcedureColumns(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
      CpProc, (SQLSMALLINT)CpLength3, CpColumn, (SQLSMALLINT)CpLength4);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpProc);
  MADB_FREE(CpColumn);

  return ret;
}
/* }}} */

/* {{{ SMA_QLProcedures */
SQLRETURN MA_SQLProcedures(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* ProcName,
  SQLSMALLINT NameLength3)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  try
  {
    return Stmt->Methods->Procedures(Stmt, (char*)CatalogName, NameLength1, (char*)SchemaName,
      NameLength2, (char*)ProcName, NameLength3);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLProceduresW */
SQLRETURN MA_SQLProceduresW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* ProcName,
  SQLSMALLINT NameLength3)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;
  char* CpCatalog= NULL,
    * CpSchema= NULL,
    * CpProc= NULL;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  MADB_CLEAR_ERROR(&Stmt->Error);

  try
  {
    if (CatalogName != NULL)
    {
      CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (SchemaName != NULL)
    {
      CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (ProcName != NULL)
    {
      CpProc= MADB_ConvertFromWChar(ProcName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
    }

    ret= Stmt->Methods->Procedures(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
      CpProc, (SQLSMALLINT)CpLength3);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpProc);
  return ret;
}
/* }}} */

/* {{{ MA_SQLPutData */
SQLRETURN MA_SQLPutData(SQLHSTMT StatementHandle,
  SQLPOINTER DataPtr,
  SQLLEN StrLen_or_Ind)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Stmt->Connection, "SQLPutData");
  MDBUG_C_DUMP(Stmt->Connection, DataPtr, 0x);
  MDBUG_C_DUMP(Stmt->Connection, StrLen_or_Ind, d);

  try
  {
    ret= Stmt->Methods->PutData(Stmt, DataPtr, StrLen_or_Ind);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLRowCount */
SQLRETURN MA_SQLRowCount(SQLHSTMT StatementHandle,
  SQLLEN* RowCountPtr)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  return Stmt->Methods->RowCount(Stmt, RowCountPtr);
}
/* }}} */

/* {{{ MA_SQLSetConnectAttr */
SQLRETURN MA_SQLSetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength,
    int isWchar)
{
  MADB_Dbc *Dbc= (MADB_Dbc *)ConnectionHandle;
  SQLRETURN ret;

  MDBUG_C_ENTER(Dbc, "SQLSetConnectAttr");
  MDBUG_C_DUMP(Dbc, Attribute, d);
  MDBUG_C_DUMP(Dbc, ValuePtr, 0x);
  MDBUG_C_DUMP(Dbc, StringLength, d);

  try
  {
    ret= Dbc->SetAttr(Attribute, ValuePtr, StringLength, isWchar);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Dbc->Error, e);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
  }
  MDBUG_C_RETURN(Dbc, ret, &Dbc->Error);
}
/* }}} */

/* {{{ MA_SQLSetCursorName */
SQLRETURN MA_SQLSetCursorName(SQLHSTMT StatementHandle,
  SQLCHAR* CursorName,
  SQLSMALLINT NameLength)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  return Stmt->Methods->SetCursorName(Stmt, (char*)CursorName, NameLength);
}
/* }}} */

/* {{{ MA_SQLSetCursorNameW */
SQLRETURN MA_SQLSetCursorNameW(SQLHSTMT StatementHandle,
  SQLWCHAR* CursorName,
  SQLSMALLINT NameLength)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  char* CpName= NULL;
  SQLULEN Length;
  SQLRETURN ret;

  try
  {
    CpName= MADB_ConvertFromWChar(CursorName, NameLength, &Length, Stmt->Connection->ConnOrSrcCharset, NULL);
    ret= Stmt->Methods->SetCursorName(Stmt, (char*)CpName, (SQLINTEGER)Length);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }

  MADB_FREE(CpName);

  return ret;
}
/* }}} */

/* {{{ MA_SQLSetEnvAttr */
SQLRETURN MA_SQLSetEnvAttr(SQLHENV EnvironmentHandle,
  SQLINTEGER Attribute,
  SQLPOINTER ValuePtr,
  SQLINTEGER StringLength)
{
  MADB_Env* Env= (MADB_Env*)EnvironmentHandle;
  SQLRETURN ret;
  MDBUG_ENTER("SQLSetEnvAttr");
  MDBUG_DUMP(Attribute, d);
  MDBUG_DUMP(ValuePtr, 0x);


  ret= MADB_EnvSetAttr(Env, Attribute, ValuePtr, StringLength);

  MDBUG_DUMP(ret, d);
  MDBUG_RETURN(ret);
}
/* }}} */

/* {{{ MA_SQLSetPos */
SQLRETURN MA_SQLSetPos(SQLHSTMT StatementHandle,
  SQLSETPOSIROW RowNumber,
  SQLUSMALLINT Operation,
  SQLUSMALLINT LockType)
{
  MADB_Stmt* Stmt= static_cast<MADB_Stmt*>(StatementHandle);

  MDBUG_C_ENTER(Stmt->Connection, "SQLSetPos");
  MDBUG_C_DUMP(Stmt->Connection, RowNumber, d);
  MDBUG_C_DUMP(Stmt->Connection, Operation, u);
  MDBUG_C_DUMP(Stmt->Connection, LockType, d);

  try
  {
    MDBUG_C_RETURN(Stmt->Connection, Stmt->Methods->SetPos(Stmt, RowNumber, Operation, LockType, 0), &Stmt->Error);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  return SQL_SUCCESS;
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

  try
  {
    ret= Stmt->Methods->SetAttr(Stmt, Attribute, ValuePtr, StringLength);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

/* {{{ MA_SQLSpecialColumns */
SQLRETURN MA_SQLSpecialColumns(SQLHSTMT StatementHandle,
  SQLUSMALLINT IdentifierType,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLUSMALLINT Scope,
  SQLUSMALLINT Nullable)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  try
  {
    return Stmt->Methods->SpecialColumns(Stmt, IdentifierType, (char*)CatalogName, NameLength1,
      (char*)SchemaName, NameLength2,
      (char*)TableName, NameLength3, Scope, Nullable);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLSpecialColumnsW */
SQLRETURN MA_SQLSpecialColumnsW(SQLHSTMT StatementHandle,
  SQLUSMALLINT IdentifierType,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLUSMALLINT Scope,
  SQLUSMALLINT Nullable)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;
  char* CpCatalog= NULL,
    * CpSchema= NULL,
    * CpTable= NULL;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0;

  try
  {
    if (CatalogName != NULL)
    {
      CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (SchemaName != NULL)
    {
      CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (TableName != NULL)
    {
      CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
    }

    ret= Stmt->Methods->SpecialColumns(Stmt, IdentifierType, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema,
      (SQLSMALLINT)CpLength2, CpTable, (SQLSMALLINT)CpLength3, Scope, Nullable);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  return ret;
}
/* }}} */

/* {{{ MA_SQLStatistics */
SQLRETURN MA_SQLStatistics(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLUSMALLINT Unique,
  SQLUSMALLINT Reserved)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  try
  {
    return Stmt->Methods->Statistics(Stmt, (char*)CatalogName, NameLength1, (char*)SchemaName, NameLength2,
      (char*)TableName, NameLength3, Unique, Reserved);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLStatisticsW */
SQLRETURN MA_SQLStatisticsW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLUSMALLINT Unique,
  SQLUSMALLINT Reserved)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;
  char *CpCatalog= NULL,
        *CpSchema= NULL,
        *CpTable=  NULL;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  try
  {
    if (CatalogName != NULL)
    {
      CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (SchemaName != NULL)
    {
      CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (TableName != NULL)
    {
      CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
    }


    ret= Stmt->Methods->Statistics(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2,
      CpTable, (SQLSMALLINT)CpLength3, Unique, Reserved);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  return ret;
}
/* }}} */

/* {{{ MA_SQLTablePrivileges */
SQLRETURN MA_SQLTablePrivileges(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  try
  {
    return Stmt->Methods->TablePrivileges(Stmt, (char*)CatalogName, NameLength1, (char*)SchemaName, NameLength2,
      (char*)TableName, NameLength3);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  return SQL_SUCCESS;
    
}
/* }}} */

/* {{{ MA_SQLTablePrivilegesW */
SQLRETURN MA_SQLTablePrivilegesW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret;
  char *CpCatalog= NULL,
       *CpTable= NULL, *CpSchema= NULL;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0;

  try
  {
    if (CatalogName != NULL)
    {
      CpCatalog= MADB_ConvertFromWChar(CatalogName, NameLength1, &CpLength1, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (SchemaName != NULL)
    {
      CpSchema= MADB_ConvertFromWChar(SchemaName, NameLength2, &CpLength2, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
    if (TableName != NULL)
    {
      CpTable= MADB_ConvertFromWChar(TableName, NameLength3, &CpLength3, Stmt->Connection->ConnOrSrcCharset, NULL);
    }
  
    ret= Stmt->Methods->TablePrivileges(Stmt, CpCatalog, (SQLSMALLINT)CpLength1, CpSchema, (SQLSMALLINT)CpLength2, CpTable, (SQLSMALLINT)CpLength3);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  MADB_FREE(CpCatalog);
  MADB_FREE(CpTable);
  return ret;
}
/* }}} */

/* {{{ MA_SQLTables */
SQLRETURN MA_SQLTables(SQLHSTMT StatementHandle,
  SQLCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLCHAR* TableType,
  SQLSMALLINT NameLength4)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  try
  {
    return Stmt->Methods->Tables(Stmt, (char*)CatalogName, NameLength1, (char*)SchemaName, NameLength2,
      (char*)TableName, NameLength3, (char*)TableType, NameLength4);
  }
  catch (SQLException &e)
  {
    return MADB_FromException(Stmt->Error, e);
  }
  catch (std::bad_alloc &/*e*/)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MA_SQLTablesW */
SQLRETURN MA_SQLTablesW(SQLHSTMT StatementHandle,
  SQLWCHAR* CatalogName,
  SQLSMALLINT NameLength1,
  SQLWCHAR* SchemaName,
  SQLSMALLINT NameLength2,
  SQLWCHAR* TableName,
  SQLSMALLINT NameLength3,
  SQLWCHAR* TableType,
  SQLSMALLINT NameLength4)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)StatementHandle;
  char *CpCatalog= NULL,
       *CpSchema=  NULL,
       *CpTable=   NULL,
       *CpType=    NULL;
  SQLULEN CpLength1= 0, CpLength2= 0, CpLength3= 0, CpLength4= 0;
  SQLRETURN ret;

  try
  {
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
  }
  catch (SQLException &e)
  {
    ret= MADB_FromException(Stmt->Error, e);
  }
  catch (std::bad_alloc &/*e*/)
  {
    ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);

  }
  MADB_FREE(CpCatalog);
  MADB_FREE(CpSchema);
  MADB_FREE(CpTable);
  MADB_FREE(CpType);
  return ret;
}

/* {{{ MA_SQLSetScrollOptions */
SQLRETURN MA_SQLSetScrollOptions(SQLHSTMT hstmt,
  SQLUSMALLINT Concurrency,
  SQLLEN       crowKeySet,
  SQLUSMALLINT crowRowSet)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)hstmt;
  return MADB_DescSetField(Stmt->Ard, 0, SQL_DESC_ARRAY_SIZE, (SQLPOINTER)(SQLULEN)crowKeySet, SQL_IS_USMALLINT, 0);
}
/* }}} */

/* {{{ MA_SQLParamOptions */
SQLRETURN MA_SQLParamOptions(
  SQLHSTMT hstmt,
  SQLULEN  crow,
  SQLULEN* pirow)
{
  MADB_Stmt* Stmt= (MADB_Stmt*)hstmt;
  SQLRETURN result;

  result= MADB_DescSetField(Stmt->Apd, 0, SQL_DESC_ARRAY_SIZE, (SQLPOINTER)crow, SQL_IS_UINTEGER, 0);

  if (SQL_SUCCEEDED(result))
  {
    result= MADB_DescSetField(Stmt->Ipd, 0, SQL_DESC_ROWS_PROCESSED_PTR, (SQLPOINTER)pirow, SQL_IS_POINTER, 0);
  }

  return result;
}
/* }}} */

