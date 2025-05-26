/************************************************************************************
   Copyright (C) 2020,2024 MariaDB Corporation plc
   
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

#include <ma_odbc.h>


/* {{{ MA_SQLAllocHandle */
SQLRETURN MA_SQLAllocHandle(SQLSMALLINT HandleType,
    SQLHANDLE InputHandle,
    SQLHANDLE *OutputHandlePtr)
{
  SQLRETURN ret= SQL_ERROR;

  switch(HandleType) {
    case SQL_HANDLE_DBC:
      EnterCriticalSection(&((MADB_Env *)InputHandle)->cs);
      MADB_CLEAR_ERROR(&((MADB_Env *)InputHandle)->Error);
      if ((*OutputHandlePtr= (SQLHANDLE)MADB_DbcInit((MADB_Env *)InputHandle)) != NULL)
      {
        ret= SQL_SUCCESS;
      }
      LeaveCriticalSection(&((MADB_Env *)InputHandle)->cs);
      break;
    case SQL_HANDLE_DESC:
      EnterCriticalSection(&((MADB_Dbc *)InputHandle)->cs);
      MADB_CLEAR_ERROR(&((MADB_Dbc *)InputHandle)->Error);
      if ((*OutputHandlePtr= (SQLHANDLE)MADB_DescInit((MADB_Dbc *)InputHandle, MADB_DESC_UNKNOWN, TRUE)) != NULL)
      {
        ret= SQL_SUCCESS;
      }
      LeaveCriticalSection(&((MADB_Dbc *)InputHandle)->cs);
      break;
    case SQL_HANDLE_ENV:
      if ((*OutputHandlePtr= (SQLHANDLE)MADB_EnvInit()) != NULL)
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

/* {{{ MA_SQLCancel */
SQLRETURN MA_SQLCancel(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt *)StatementHandle;
  SQLRETURN ret= SQL_SUCCESS;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  // Technically, if the application does not do good syncronization of stmt use here, we can have
  // Stmt already freed at this point, and crash right away on reading the mutex in Stmt
  EnterCriticalSection(&globalLock);
  if (CheckDeletedStmt(Stmt) != NULL)
  {
    LeaveCriticalSection(&globalLock);
    return ret;// i.e. SQL_SUCCESS. Stmt has been deleted, nothing to cancel already
  }
  if (!TryEnterCriticalSection(&Stmt->CancelDropSwitch))
  {
    LeaveCriticalSection(&globalLock);
    // This is not clear what is right here - success or error
    return SQL_SUCCESS;
  }
  // We can release the lock. SQL_DROP will get it, but stop at the Stmt->CancelDropSwitch
  // that we already own
  LeaveCriticalSection(&globalLock);
  MADB_CLEAR_ERROR(&Stmt->Error);

  MDBUG_C_ENTER(Stmt->Connection, "SQLCancel");
  MDBUG_C_DUMP(Stmt->Connection, Stmt, 0x);

  /* In ODBC 2.x, if an application calls SQLCancel when no processing is being done on the statement,
     SQLCancel has the same effect as SQLFreeStmt with the SQL_CLOSE option */
  if (Stmt->Connection->Environment->OdbcVersion == SQL_OV_ODBC2)
  {
    // Leaving locking and killing part here for compatibility. cuz we had it. TODO: but something to be removed in next version
    if (TryEnterCriticalSection(&Stmt->Connection->cs))
    {
      MADB_CloseCursor(Stmt);
      UNLOCK_MARIADB(Stmt->Connection);
    }
    else
    {
      ret= MADB_KillAtServer(Stmt);
    }
  }
  else /* if application is ODBC3 */
  {
    /* SQLCancel can cancel the following types of processing on a statement:
       - A function running asynchronously on the statement -- those we do not have
       - A function on a statement that needs data -- so first check this and reset it if needed
       - A function running on the statement on another thread -- or killing process on server otherwise */
    if (Stmt->PutParam > MADB_NO_DATA_NEEDED && !DAE_DONE(Stmt))
    {
      RESET_DAE_STATUS(Stmt);
    } // Else we are canceling function running in other thread.
    else if (TryEnterCriticalSection(&Stmt->Connection->cs))
    {
      Stmt->canceled= '\1';
      /* "If a SQL statement is being executed when SQLCancel is called on another thread to cancel the
          statement execution, it is possible for the execution to succeed and return SQL_SUCCESS while the
          cancel is also successful. In this case, the Driver Manager assumes that the cursor opened by the
          statement execution is closed by the cancel, so the application will not be able to use the cursor."
       */
      MADB_CloseCursor(Stmt);
      UNLOCK_MARIADB(Stmt->Connection);
    }
    else {
      ret= MADB_KillAtServer(Stmt);
    }
  }
  LeaveCriticalSection(&Stmt->CancelDropSwitch);
  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}

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

  ret= Dbc->Methods->GetAttr(Dbc, Attribute, ValuePtr, BufferLength, StringLengthPtr, FALSE);

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
  if (!Stmt)
    return SQL_INVALID_HANDLE;
  return Stmt->Methods->GetAttr(Stmt, Attribute, ValuePtr, BufferLength, StringLengthPtr);
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
/* }}} */

