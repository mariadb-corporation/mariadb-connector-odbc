/************************************************************************************
   Copyright (C) 2013,2021 MariaDB Corporation AB
   
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

extern const char* DefaultPluginLocation;
static const char* utf8mb3 = "utf8mb3";
static const unsigned int selectedIntOption= 1, unselectedIntOption= 0;

struct st_madb_isolation MADB_IsolationLevel[] =
{
  {SQL_TRANSACTION_REPEATABLE_READ, "REPEATABLE READ", "REPEATABLE-READ"},
  {SQL_TRANSACTION_READ_COMMITTED, "READ COMMITTED", "READ-COMMITTED"},
  {SQL_TRANSACTION_READ_UNCOMMITTED, "READ UNCOMMITTED", "READ-UNCOMMITTED"},
  {SQL_TRANSACTION_SERIALIZABLE, "SERIALIZABLE", "SERIALIZABLE"},
  {0, 0}
};


static long TranslateTxIsolation(const char* txIsolation, size_t len)
{
  unsigned int i;
  for (i = 0; i < 4; i++)
  {
    if (strncmp(txIsolation, MADB_IsolationLevel[i].StrIsolation, len) == 0 || strncmp(txIsolation, MADB_IsolationLevel[i].TrackStr, len) == 0)
    {
      return MADB_IsolationLevel[i].SqlIsolation;
      break;
    }
  }
  return SQL_TRANSACTION_REPEATABLE_READ;
}


/* used by SQLGetFunctions */
SQLUSMALLINT MADB_supported_api[]=
{
  SQL_API_SQLALLOCCONNECT,
  SQL_API_SQLALLOCENV,
  SQL_API_SQLALLOCHANDLE,
  SQL_API_SQLALLOCSTMT,
  SQL_API_SQLBINDCOL,
  SQL_API_SQLBINDPARAM,
  SQL_API_SQLCANCEL,
  SQL_API_SQLCLOSECURSOR,
  SQL_API_SQLCOLATTRIBUTE,
  SQL_API_SQLCOLUMNS,
  SQL_API_SQLCONNECT,
  SQL_API_SQLCOPYDESC,
  SQL_API_SQLDATASOURCES,
  SQL_API_SQLDESCRIBECOL,
  SQL_API_SQLDISCONNECT,
  SQL_API_SQLENDTRAN,
  SQL_API_SQLERROR,
  SQL_API_SQLEXECDIRECT,
  SQL_API_SQLEXECUTE,
  SQL_API_SQLFETCH,
  SQL_API_SQLFETCHSCROLL,
  SQL_API_SQLFREECONNECT,
  SQL_API_SQLFREEENV,
  SQL_API_SQLFREEHANDLE,
  SQL_API_SQLFREESTMT,
  SQL_API_SQLGETCONNECTATTR,
  SQL_API_SQLGETCONNECTOPTION,
  SQL_API_SQLGETCURSORNAME,
  SQL_API_SQLGETDATA,
  SQL_API_SQLGETDESCFIELD,
  SQL_API_SQLGETDESCREC,
  SQL_API_SQLGETDIAGFIELD,
  SQL_API_SQLGETDIAGREC,
  SQL_API_SQLGETENVATTR,
  SQL_API_SQLGETFUNCTIONS,
  SQL_API_SQLGETINFO,
  SQL_API_SQLGETSTMTATTR,
  SQL_API_SQLGETSTMTOPTION,
  SQL_API_SQLGETTYPEINFO,
  SQL_API_SQLNUMRESULTCOLS,
  SQL_API_SQLPARAMDATA,
  SQL_API_SQLPREPARE,
  SQL_API_SQLPUTDATA,
  SQL_API_SQLROWCOUNT,
  SQL_API_SQLSETCONNECTATTR,
  SQL_API_SQLSETCONNECTOPTION,
  SQL_API_SQLSETCURSORNAME,
  SQL_API_SQLSETDESCFIELD,
  SQL_API_SQLSETDESCREC,
  SQL_API_SQLSETENVATTR,
  SQL_API_SQLSETPARAM,
  SQL_API_SQLSETSTMTATTR,
  SQL_API_SQLSETSTMTOPTION,
  SQL_API_SQLSPECIALCOLUMNS,
  SQL_API_SQLSTATISTICS,
  SQL_API_SQLTABLES,
  SQL_API_SQLTRANSACT,
  SQL_API_SQLBULKOPERATIONS,
  SQL_API_SQLBINDPARAMETER,
  SQL_API_SQLBROWSECONNECT,
  SQL_API_SQLCOLATTRIBUTES,
  SQL_API_SQLCOLUMNPRIVILEGES ,
  SQL_API_SQLDESCRIBEPARAM,
  SQL_API_SQLDRIVERCONNECT,
  SQL_API_SQLDRIVERS,
  SQL_API_SQLEXTENDEDFETCH,
  SQL_API_SQLFOREIGNKEYS,
  SQL_API_SQLMORERESULTS,
  SQL_API_SQLNATIVESQL,
  SQL_API_SQLNUMPARAMS,
  SQL_API_SQLPARAMOPTIONS,
  SQL_API_SQLPRIMARYKEYS,
  SQL_API_SQLPROCEDURECOLUMNS,
  SQL_API_SQLPROCEDURES,
  SQL_API_SQLSETPOS,
  SQL_API_SQLSETSCROLLOPTIONS,
  SQL_API_SQLTABLES,
  SQL_API_SQLTABLEPRIVILEGES

};


struct st_ma_connection_methods MADB_Dbc_Methods; /* declared at the end of file */
SQLRETURN MADB_DbcDummyTrackSession(MADB_Dbc* Dbc);
SQLRETURN MADB_DbcGetServerTxIsolation(MADB_Dbc* Dbc, SQLINTEGER*txIsolation);

/* Stub for stream caching - i.e. cahing is desabled by default, and we just raise the error, if some stmt is streaming atm
 * Stmt - the statement, that has requested the operation, and where the error has to be set 
 */
int MADB_Dbc_ErrorOnStreaming(MADB_Stmt* Stmt)
{
  MADB_SetError(&Stmt->Error, MADB_ERR_HY000, "The requested operation cannot be executed at the moment: it would break "
                                              "the protocol since other statement is currently streaming its resultset", 0);
  return 1;
}


/* Main method for stream caching - i.e. cahing is desabled by default, and we are capable to store and operate the rest
   of the RS.
   Stmt - the statement, that has requested the operation, and where the error has to be set */
int MADB_Dbc_CacheRestOfCurrentRsStream(MADB_Stmt* Stmt)
{
  MADB_Dbc* Dbc = Stmt->Connection;
  if (Dbc->Streamer != NULL)
  {
    if (Dbc->Streamer->RsOps->CacheRestOfRs(Dbc->Streamer))
    {
      return MADB_Dbc_ErrorOnStreaming(Stmt);
    }
    Dbc->Streamer= NULL;
    return 0;
  }
  return 0;
}


my_bool CheckConnection(MADB_Dbc *Dbc)
{
  if (!Dbc->mariadb)
    return FALSE;
  if (mysql_get_socket(Dbc->mariadb) == MARIADB_INVALID_SOCKET)
  {
    /* Check if reconnect option was set */
    if (DSN_OPTION(Dbc, MADB_OPT_FLAG_AUTO_RECONNECT))
    {
      if (!mysql_ping(Dbc->mariadb))
        return TRUE;
    }
    return FALSE;
  }
  return TRUE;
}

/* {{{ MADB_DbcSetAttr */
SQLRETURN MADB_DbcSetAttr(MADB_Dbc *Dbc, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength, my_bool isWChar)
{
  MADB_CLEAR_ERROR(&Dbc->Error);

  if (!Dbc)
  {
    /* Todo: check */
    if (Attribute != SQL_ATTR_TRACE &&
        Attribute != SQL_ATTR_TRACEFILE)
      return SQL_INVALID_HANDLE;
    return SQL_SUCCESS;
  } 

  switch(Attribute) {
  case SQL_ATTR_ACCESS_MODE:
    if ((SQLPOINTER)SQL_MODE_READ_WRITE != ValuePtr)
      MADB_SetError(&Dbc->Error, MADB_ERR_01S02, NULL, 0);
    Dbc->AccessMode= SQL_MODE_READ_WRITE;
    break;
#if (ODBCVER >= 0x0351)
  case SQL_ATTR_ANSI_APP:
    if (ValuePtr != NULL)
    {
      Dbc->IsAnsi= 1;
      Dbc->ConnOrSrcCharset= &SourceAnsiCs;
      CopyClientCharset(&SourceAnsiCs, &Dbc->Charset);
    }
    else
    {
      Dbc->IsAnsi= 0;
    }
    break;
#endif
  case SQL_ATTR_ASYNC_ENABLE:
     if ((SQLPOINTER)SQL_ASYNC_ENABLE_OFF != ValuePtr)
      MADB_SetError(&Dbc->Error, MADB_ERR_01S02, NULL, 0);
     Dbc->AsyncEnable= SQL_ASYNC_ENABLE_OFF;
    break;
  case SQL_ATTR_AUTO_IPD:
    /* read only */
    MADB_SetError(&Dbc->Error, MADB_ERR_HY092, NULL, 0);
    break;
  case SQL_ATTR_AUTOCOMMIT:
    {
      SQLULEN ValidAttrs[]= {2, SQL_AUTOCOMMIT_ON, SQL_AUTOCOMMIT_OFF};
      MADB_CHECK_ATTRIBUTE(Dbc, ValuePtr, ValidAttrs);
      /* if a connection is open, try to apply setting to the connection */
      if (Dbc->mariadb)
      {
        if (Dbc->EnlistInDtc) {
          return MADB_SetError(&Dbc->Error, MADB_ERR_25000, NULL, 0);
        }
        if (mysql_autocommit(Dbc->mariadb, (my_bool)(size_t)ValuePtr))
        {
          return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, mysql_error(Dbc->mariadb), mysql_errno(Dbc->mariadb));
        }
      }
      Dbc->AutoCommit= (SQLUINTEGER)(SQLULEN)ValuePtr;
    }
    break;
  case SQL_ATTR_CONNECTION_DEAD:
    /* read only! */
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY092, NULL, 0);
  case SQL_ATTR_CURRENT_CATALOG:
    {
      MADB_FREE(Dbc->CatalogName);
      if (isWChar)
      {
        /* IsAnsi will be set before this, even if it is set before connection */
        Dbc->CatalogName= MADB_ConvertFromWChar((SQLWCHAR *)ValuePtr, StringLength, NULL, Dbc->ConnOrSrcCharset, NULL);
      }
      else
        Dbc->CatalogName= _strdup((char *)ValuePtr);

      if (Dbc->mariadb &&
          mysql_select_db(Dbc->mariadb, Dbc->CatalogName))
      {
        return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, mysql_error(Dbc->mariadb), mysql_errno(Dbc->mariadb));
      }
      MADB_RESET(Dbc->CurrentSchema, Dbc->CatalogName);
    }
    break;
  case SQL_ATTR_LOGIN_TIMEOUT:
    Dbc->LoginTimeout= (SQLUINTEGER)(SQLULEN)ValuePtr;
    break;
  case SQL_ATTR_METADATA_ID:
    Dbc->MetadataId= (SQLUINTEGER)(SQLULEN)ValuePtr;
    break;
  case SQL_ATTR_ODBC_CURSORS:
    {
      SQLULEN ValidAttrs[]= {3, SQL_CUR_USE_IF_NEEDED, SQL_CUR_USE_ODBC, SQL_CUR_USE_DRIVER};
      MADB_CHECK_ATTRIBUTE(Dbc, ValuePtr, ValidAttrs);
      if ((SQLULEN)ValuePtr != SQL_CUR_USE_ODBC)
        MADB_SetError(&Dbc->Error, MADB_ERR_01S02, NULL, 0);
      Dbc->OdbcCursors= SQL_CUR_USE_ODBC;
    }
    break;
  case SQL_ATTR_ENLIST_IN_DTC:
    /* MS Distributed Transaction Coordinator not supported */
    return MADB_SetError(&Dbc->Error, MADB_ERR_HYC00, NULL, 0);
  case SQL_ATTR_PACKET_SIZE:
    /* if connection was made, return HY001 */
    if (Dbc->mariadb)
    {
      return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, NULL, 0);
    }
    Dbc->PacketSize= (SQLUINTEGER)(SQLULEN)ValuePtr;
    break;
  case SQL_ATTR_QUIET_MODE:
    Dbc->QuietMode= (HWND)ValuePtr;
    break;
  case SQL_ATTR_TRACE:
    break;
  case SQL_ATTR_TRACEFILE:
    break;
  case SQL_ATTR_TRANSLATE_LIB:
    break;
  case SQL_ATTR_TRANSLATE_OPTION:
    break;
  case SQL_ATTR_TXN_ISOLATION:
    if (Dbc->mariadb)
    {
      my_bool ValidTx= FALSE;
      unsigned int i;
      for (i=0; i < 4; i++)
      {
        if (MADB_IsolationLevel[i].SqlIsolation == (SQLLEN)ValuePtr)
        {
          char StmtStr[128];
          int len= _snprintf(StmtStr, sizeof(StmtStr), "SET SESSION TRANSACTION ISOLATION LEVEL %s",
                      MADB_IsolationLevel[i].StrIsolation);
          LOCK_MARIADB(Dbc);
          if (mysql_real_query(Dbc->mariadb, StmtStr, (unsigned long)len))
          {
            UNLOCK_MARIADB(Dbc);
            return MADB_SetError(&Dbc->Error, MADB_ERR_HY001, mysql_error(Dbc->mariadb), mysql_errno(Dbc->mariadb));
          }
          Dbc->Methods->TrackSession(Dbc);
          UNLOCK_MARIADB(Dbc);
          ValidTx= TRUE;
          break;
        }
      }
      if (!ValidTx)
      {
        return MADB_SetError(&Dbc->Error, MADB_ERR_HY024, NULL, 0);
      }
    }
    Dbc->TxnIsolation= (SQLINTEGER)(SQLLEN)ValuePtr;
    break;
  default:
    break;
  }
  return Dbc->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_DbcHetAttr */
SQLRETURN MADB_DbcGetAttr(MADB_Dbc *Dbc, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength, SQLINTEGER *StringLengthPtr, my_bool isWChar)
{
  MADB_CLEAR_ERROR(&Dbc->Error);

  if (!Dbc)
    return SQL_INVALID_HANDLE;

  if (!ValuePtr && Attribute != SQL_ATTR_CURRENT_CATALOG)
    return SQL_SUCCESS;
  if (Attribute == SQL_ATTR_CURRENT_CATALOG && !StringLengthPtr && 
      (!ValuePtr || !BufferLength))
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_01004, NULL, 0);
  }

  switch(Attribute) {
  case SQL_ATTR_ACCESS_MODE:
    *(SQLUINTEGER *)ValuePtr= SQL_MODE_READ_WRITE;
    break;
  case SQL_ATTR_ASYNC_ENABLE:
    *(SQLULEN *)ValuePtr= SQL_ASYNC_ENABLE_OFF;
    break;
  case SQL_ATTR_AUTO_IPD:
    *(SQLUINTEGER *)ValuePtr= SQL_FALSE;
    break;
  case SQL_ATTR_AUTOCOMMIT:
    /* Not sure why Dbc->AutoCommit is initialized with 4. Probably the idea has been lost in time. But keeping it so far, and workarounding here. Looks like all DMs but iOdbc corrects the value to SQL_AUTOCOMMIT_ON, when returning */
    *(SQLUINTEGER *)ValuePtr= (Dbc->AutoCommit != 0 ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF);
    break;
  case SQL_ATTR_CONNECTION_DEAD:
    /* ping may fail if status isn't ready, so we need to check errors */
    if (mysql_ping(Dbc->mariadb))
      *(SQLUINTEGER *)ValuePtr= (mysql_errno(Dbc->mariadb) == CR_SERVER_GONE_ERROR ||
                                 mysql_errno(Dbc->mariadb) == CR_SERVER_LOST) ? SQL_CD_TRUE : SQL_CD_FALSE;
    else
      *(SQLUINTEGER *)ValuePtr= SQL_CD_FALSE;
    break;
  case SQL_ATTR_CURRENT_CATALOG:
  {
    SQLSMALLINT StrLen;
    SQLRETURN   ret;

    ret= Dbc->Methods->GetCurrentDB(Dbc, ValuePtr, BufferLength, &StrLen, isWChar);
    /* if we weren't able to determine the current db, we will return the cached catalog name */
    if (!SQL_SUCCEEDED(ret) && Dbc->CatalogName)
    {
      MADB_CLEAR_ERROR(&Dbc->Error);
      StrLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : 0, ValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar),
                                          Dbc->CatalogName, strlen(Dbc->CatalogName), &Dbc->Error);
      ret= SQL_SUCCESS;
    }
    if (StringLengthPtr != NULL)
    {
      *StringLengthPtr= (SQLINTEGER)StrLen;
    }
    return ret;
  }
  case SQL_ATTR_LOGIN_TIMEOUT:
    *(SQLUINTEGER *)ValuePtr= Dbc->LoginTimeout;
    break;
  case SQL_ATTR_CONNECTION_TIMEOUT:
    *(SQLUINTEGER *)ValuePtr= 0;
    break;
  case SQL_ATTR_METADATA_ID:
    /* SQL_ATTR_METADATA_ID is SQLUINTEGER attribute on connection level, but SQLULEN on statement level :/ */
    *(SQLUINTEGER *)ValuePtr= Dbc->MetadataId;
  case SQL_ATTR_ODBC_CURSORS:
    *(SQLULEN*)ValuePtr= SQL_CUR_USE_ODBC;
    break;
  case SQL_ATTR_ENLIST_IN_DTC:
    /* MS Distributed Transaction Coordinator not supported */
    MADB_SetError(&Dbc->Error, MADB_ERR_HYC00, NULL, 0);
    break;
  case SQL_ATTR_PACKET_SIZE:
    {
      unsigned long packet_size= 0;
      mysql_get_option(Dbc->mariadb, MYSQL_OPT_NET_BUFFER_LENGTH, &packet_size);
      *(SQLUINTEGER *)ValuePtr= (SQLUINTEGER)packet_size;
    }
    break;
  case SQL_ATTR_QUIET_MODE:
#ifdef _WIN32
  (HWND)ValuePtr= Dbc->QuietMode;
#endif // _WIN32
    break;
  case SQL_ATTR_TRACE:
    break;
  case SQL_ATTR_TRACEFILE:
    break;
  case SQL_ATTR_TRANSLATE_LIB:
    break;
  case SQL_ATTR_TRANSLATE_OPTION:
    break;
  case SQL_ATTR_TXN_ISOLATION:
    /* TxnIsolation wasn't set before we retrieve it from open connection or
       assume a default of REPETABLE_READ */
    if (!Dbc->TxnIsolation)
    {
      Dbc->TxnIsolation= SQL_TRANSACTION_REPEATABLE_READ;
      if (Dbc->mariadb)
      {
        Dbc->Methods->GetTxIsolation(Dbc, (SQLINTEGER*)ValuePtr);
      }
    }
    else 
      *(SQLINTEGER*)ValuePtr= Dbc->TxnIsolation;
    break;

  default:
    MADB_SetError(&Dbc->Error, MADB_ERR_HYC00, NULL, 0);
    break;
  }
  return Dbc->Error.ReturnValue;
}
/* }}} */


/* {{{ MADB_DbcInit() */
MADB_Dbc *MADB_DbcInit(MADB_Env *Env)
{
  MADB_Dbc *Connection= NULL;

  MADB_CLEAR_ERROR(&Env->Error);

  if (!(Connection = (MADB_Dbc *)MADB_CALLOC(sizeof(MADB_Dbc))))
    goto cleanup;

  Connection->AutoCommit= 4;
  Connection->Environment= Env;
  Connection->Methods= &MADB_Dbc_Methods;
  //CopyClientCharset(&SourceAnsiCs, &Connection->Charset);
  InitializeCriticalSection(&Connection->cs);
  InitializeCriticalSection(&Connection->ListsCs);
  /* Not sure that critical section is really needed here - this init routine is called when
     no one has the handle yet */
  EnterCriticalSection(&Connection->Environment->cs);

  /* Save connection in Environment list */
  Connection->ListItem.data= (void *)Connection;
  Connection->Environment->Dbcs= MADB_ListAdd(Connection->Environment->Dbcs, &Connection->ListItem);

  LeaveCriticalSection(&Connection->Environment->cs);

  MADB_PutErrorPrefix(NULL, &Connection->Error);

  return Connection;      
cleanup:
  if (Connection)
    free(Connection);
  else
    MADB_SetError(&Env->Error, MADB_ERR_HY001, NULL, 0);

  return NULL;
}
/* }}} */

/* {{{ MADB_DbcFree() */
SQLRETURN MADB_DbcFree(MADB_Dbc *Connection)
{
  MADB_Env *Env= NULL;

  if (!Connection)
    return SQL_ERROR;
  MDBUG_C_PRINT(Connection, "%sMADB_DbcFree", "\t->");
  MDBUG_C_DUMP(Connection, Connection, 0x);

  Env= Connection->Environment;

  /* TODO: If somebody uses connection it won't help if lock it here. At least it requires
           more fingers movements
    LOCK_MARIADB(Dbc);*/
  if (Connection->mariadb)
  {
    mysql_close(Connection->mariadb);
    Connection->mariadb= NULL;
  }
  /*UNLOCK_MARIADB(Dbc);*/

  /* todo: delete all descriptors */

  EnterCriticalSection(&Env->cs);
  Connection->Environment->Dbcs= MADB_ListDelete(Connection->Environment->Dbcs, &Connection->ListItem);
  LeaveCriticalSection(&Env->cs);

  MADB_FREE(Connection->CatalogName);
  CloseClientCharset(&Connection->Charset);
  MADB_FREE(Connection->CurrentSchema);
  MADB_DSN_Free(Connection->Dsn);
  DeleteCriticalSection(&Connection->cs);

  free(Connection);
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_DbcGetCurrentDB
   Fetches current DB. For use without session tracking
*/
SQLRETURN MADB_DbcGetCurrentDB(MADB_Dbc *Connection, SQLPOINTER CurrentDB, SQLINTEGER CurrentDBLength, 
                                SQLSMALLINT *StringLengthPtr, my_bool isWChar) 
{
  SQLLEN Size;
  MYSQL_RES *res;
  MYSQL_ROW row;

  MADB_CLEAR_ERROR(&Connection->Error);
  if (CheckConnection(Connection) == FALSE)
  {
    return MADB_SetError(&Connection->Error, MADB_ERR_08003, NULL, 0);
  }

  LOCK_MARIADB(Connection);

  if (mysql_real_query(Connection->mariadb, "SELECT DATABASE()", 17) || (res= mysql_store_result(Connection->mariadb)) == NULL)
  {
    MADB_SetNativeError(&Connection->Error, SQL_HANDLE_DBC, Connection->mariadb);;
    goto end;
  }
  
  row = mysql_fetch_row(res);

  Size= (SQLSMALLINT)MADB_SetString(isWChar ? & Connection->Charset : 0, 
                                     (void *)CurrentDB, BUFFER_CHAR_LEN(CurrentDBLength, isWChar), row[0] != NULL ? row[0] : "null",
                                     SQL_NTS, &Connection->Error);
  mysql_free_result(res);

  if (StringLengthPtr)
    *StringLengthPtr= isWChar ? (SQLSMALLINT)Size * sizeof(SQLWCHAR) : (SQLSMALLINT)Size;
  
end:
  UNLOCK_MARIADB(Connection);
  return Connection->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_DbcGetTrackedCurrentDB
   Fetches current DB. For use without session tracking
*/
SQLRETURN MADB_DbcGetTrackedCurrentDB(MADB_Dbc* Dbc, SQLPOINTER CurrentDB, SQLINTEGER CurrentDBLength,
  SQLSMALLINT* StringLengthPtr, my_bool isWChar)
{
  SQLLEN Size;

  MADB_CLEAR_ERROR(&Dbc->Error);
  Size = (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : 0,
    (void*)CurrentDB, BUFFER_CHAR_LEN(CurrentDBLength, isWChar), Dbc->CurrentSchema != NULL ? Dbc->CurrentSchema : "null",
    SQL_NTS, &Dbc->Error);

  if (StringLengthPtr)
    *StringLengthPtr = isWChar ? (SQLSMALLINT)Size * sizeof(SQLWCHAR) : (SQLSMALLINT)Size;
  return Dbc->Error.ReturnValue;
}
/* }}} */

BOOL MADB_SqlMode(MADB_Dbc *Connection, enum enum_madb_sql_mode SqlMode)
{
  unsigned int ServerStatus;

  mariadb_get_infov(Connection->mariadb, MARIADB_CONNECTION_SERVER_STATUS, (void*)&ServerStatus);
  switch (SqlMode)
  {
  case MADB_NO_BACKSLASH_ESCAPES:
    return test(ServerStatus & SERVER_STATUS_NO_BACKSLASH_ESCAPES);
  case MADB_ANSI_QUOTES:
    return test(ServerStatus & SERVER_STATUS_ANSI_QUOTES);
  }
  return FALSE;
}
/* }}} */

/* {{{ MADB_DbcEndTran */
SQLRETURN MADB_DbcEndTran(MADB_Dbc *Dbc, SQLSMALLINT CompletionType)
{
  MADB_CLEAR_ERROR(&Dbc->Error);
  if (!Dbc)
    return SQL_INVALID_HANDLE;

  LOCK_MARIADB(Dbc);
  switch (CompletionType) {
  case SQL_ROLLBACK:
    if (Dbc->mariadb && mysql_rollback(Dbc->mariadb))
      MADB_SetNativeError(&Dbc->Error, SQL_HANDLE_DBC, Dbc->mariadb);
    break;
  case SQL_COMMIT:
    if (Dbc->mariadb && mysql_commit(Dbc->mariadb))
      MADB_SetNativeError(&Dbc->Error, SQL_HANDLE_DBC, Dbc->mariadb);
    break;
  default:
    MADB_SetError(&Dbc->Error, MADB_ERR_HY012, NULL, 0);
  }
  Dbc->Methods->TrackSession(Dbc);
  UNLOCK_MARIADB(Dbc);

  return Dbc->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_AddInitCommand
*/
static int MADB_AddInitCommand(MYSQL* mariadb, MADB_DynString *InitCmd, unsigned long MultiStmtAllowed, const char *StmtToAdd)
{
  if (MultiStmtAllowed == 0)
  {
    mysql_optionsv(mariadb, MYSQL_INIT_COMMAND, StmtToAdd);
    return 0;
  }
  else
  {
    if (InitCmd->length != 0)
    {
      if (MADB_DynstrAppendMem(InitCmd, ";", 1))
      {
        return 1;
      }
    }
    return MADB_DynstrAppend(InitCmd, StmtToAdd);
  }
}
/* }}} */

/* {{{ MADB_Dbc_ConnectDB
       Mind that this function is used for establishing connection from the setup lib
*/
SQLRETURN MADB_DbcConnectDB(MADB_Dbc *Connection,
    MADB_Dsn *Dsn)
{
  char StmtStr[128];
  my_bool ReportDataTruncation= 1;
  unsigned int i;
  unsigned long client_flags= CLIENT_MULTI_RESULTS;
  my_bool my_reconnect= 1;
  int protocol = MYSQL_PROTOCOL_TCP;
  MADB_DynString InitCmd;
  const char* defaultSchema= NULL;
  
  if (!Connection || !Dsn)
    return SQL_ERROR;

  MADB_CLEAR_ERROR(&Connection->Error);

  if (Connection->mariadb == NULL)
  {
    if (!(Connection->mariadb= mysql_init(NULL)))
    {
      MADB_SetError(&Connection->Error, MADB_ERR_HY001, NULL, 0);
      goto end;
    }
  }

  if( !MADB_IS_EMPTY(Dsn->ConnCPluginsDir))
  {
    mysql_optionsv(Connection->mariadb, MYSQL_PLUGIN_DIR, Dsn->ConnCPluginsDir);
  }
  else
  {
    if (DefaultPluginLocation != NULL)
    {
      mysql_optionsv(Connection->mariadb, MYSQL_PLUGIN_DIR, DefaultPluginLocation);
    }
  }

  if (Dsn->ReadMycnf != '\0')
  {
    mysql_optionsv(Connection->mariadb, MYSQL_READ_DEFAULT_GROUP, (void *)"odbc");
  }
  /* If a client character set was specified in DSN, we will always use it.
     Otherwise for ANSI applications we will use the current character set,
     for unicode connections we use utf8
  */
  {
    const char* cs_name= NULL;

    if (!MADB_IS_EMPTY(Dsn->CharacterSet))
    {
      if (strcmp(Dsn->CharacterSet, "utf8") == 0)
      {
        cs_name= utf8mb3;
      }
     cs_name= Dsn->CharacterSet;
    }
    else if (Connection->IsAnsi)
    {
      MARIADB_CHARSET_INFO *cs= mariadb_get_charset_by_name("auto");
      cs_name= cs->csname;
    }

    if (InitClientCharset(&Connection->Charset, MADB_IS_EMPTY(cs_name) ? "utf8mb4" : cs_name))
    {
      /* Memory allocation error */
      MADB_SetError(&Connection->Error, MADB_ERR_HY001, NULL, 0);
      goto end;
    }
    if (iOdbc() && strcmp(Connection->Charset.cs_info->csname, "swe7") == 0)
    {
      MADB_SetError(&Connection->Error, MADB_ERR_HY000, "Charset SWE7 is not supported with iODBC", 0);
      goto end;

    }
    if (!Connection->IsAnsi || iOdbc())
    {
      /* If application is not ansi, we should convert wchar into connection string */
      Connection->ConnOrSrcCharset= &Connection->Charset;
    }
  }

  /* todo: error handling */
  mysql_optionsv(Connection->mariadb, MYSQL_SET_CHARSET_NAME, Connection->Charset.cs_info->csname);

  /* This should go before any DSN_OPTION macro use. I don't know why can't we use Dsn directly, though */
  Connection->Options = Dsn->Options;

  if (DSN_OPTION(Connection, MADB_OPT_FLAG_MULTI_STATEMENTS))
  {
    client_flags|= CLIENT_MULTI_STATEMENTS;
    MADB_InitDynamicString(&InitCmd, "", 1024, 1024);
  }

  if (Dsn->InitCommand && Dsn->InitCommand[0])
  {
    MADB_AddInitCommand(Connection->mariadb, &InitCmd, DSN_OPTION(Connection, MADB_OPT_FLAG_MULTI_STATEMENTS), Dsn->InitCommand);
  }
  /* Turn sql_auto_is_null behavior off.
    For more details see: http://bugs.mysql.com/bug.php?id=47005 */
  MADB_AddInitCommand(Connection->mariadb, &InitCmd, DSN_OPTION(Connection, MADB_OPT_FLAG_MULTI_STATEMENTS), "SET SESSION SQL_AUTO_IS_NULL=0");

  /* set autocommit behavior */
  if (Connection->AutoCommit != 0)
  {
    MADB_AddInitCommand(Connection->mariadb, &InitCmd, DSN_OPTION(Connection, MADB_OPT_FLAG_MULTI_STATEMENTS), "SET autocommit=1");
  }
  else
  {
    MADB_AddInitCommand(Connection->mariadb, &InitCmd, DSN_OPTION(Connection, MADB_OPT_FLAG_MULTI_STATEMENTS), "SET autocommit=0");
  }

  /* Set isolation level */
  if (Connection->IsolationLevel)
  {
    for (i = 0; i < 4; i++)
    {
      if (MADB_IsolationLevel[i].SqlIsolation == Connection->IsolationLevel)
      {
        _snprintf(StmtStr, sizeof(StmtStr), "SET SESSION TRANSACTION ISOLATION LEVEL %s",
          MADB_IsolationLevel[i].StrIsolation);
        MADB_AddInitCommand(Connection->mariadb, &InitCmd, DSN_OPTION(Connection, MADB_OPT_FLAG_MULTI_STATEMENTS), StmtStr);
        break;
      }
    }
  }

  /* If multistmts allowed - we've put all queries to run in InitCmd. Now need to set it to MYSQL_INIT_COMMAND option */
  if (DSN_OPTION(Connection, MADB_OPT_FLAG_MULTI_STATEMENTS))
  {
    mysql_optionsv(Connection->mariadb, MYSQL_INIT_COMMAND, InitCmd.str);
  }

  if (Dsn->ConnectionTimeout)
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&Dsn->ConnectionTimeout);

  if (Dsn->ReadTimeout)
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_READ_TIMEOUT, (const char *)&Dsn->ReadTimeout);

  if (Dsn->WriteTimeout)
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_WRITE_TIMEOUT, (const char *)&Dsn->WriteTimeout);

  if (DSN_OPTION(Connection, MADB_OPT_FLAG_AUTO_RECONNECT))
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_RECONNECT, &my_reconnect);

  if (DSN_OPTION(Connection, MADB_OPT_FLAG_NO_SCHEMA))
    client_flags|= CLIENT_NO_SCHEMA;
  if (DSN_OPTION(Connection, MADB_OPT_FLAG_IGNORE_SPACE))
    client_flags|= CLIENT_IGNORE_SPACE;

  if (DSN_OPTION(Connection, MADB_OPT_FLAG_FOUND_ROWS))
    client_flags|= CLIENT_FOUND_ROWS;
  if (DSN_OPTION(Connection, MADB_OPT_FLAG_COMPRESSED_PROTO))
    client_flags|= CLIENT_COMPRESS;
  

  if (Dsn->InteractiveClient)
  {
    mysql_optionsv(Connection->mariadb, MARIADB_OPT_INTERACTIVE, 1);
  }
  /* enable truncation reporting */
  mysql_optionsv(Connection->mariadb, MYSQL_REPORT_DATA_TRUNCATION, &ReportDataTruncation);

  if (Dsn->Socket)
  {
    protocol= MYSQL_PROTOCOL_SOCKET;
  }
  else if (Dsn->IsNamedPipe) /* DSN_OPTION(Connection, MADB_OPT_FLAG_NAMED_PIPE) */
  {
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_NAMED_PIPE, (void*)Dsn->ServerName);
    protocol = MYSQL_PROTOCOL_PIPE;
  }
  else if (Dsn->Port > 0 || Dsn->IsTcpIp)
  {
    protocol = MYSQL_PROTOCOL_TCP;
    if (Dsn->Port == 0)
    {
      Dsn->Port= 3306;
    }
  }
  mysql_optionsv(Connection->mariadb, MYSQL_OPT_PROTOCOL, (void*)&protocol);

  {
    /* I don't think it's possible to have empty strings or only spaces in the string here, but I prefer
       to have this paranoid check to make sure we dont' them */
    const char *SslKey=    ltrim(Dsn->SslKey);
    const char *SslCert=   ltrim(Dsn->SslCert);
    const char *SslCa=     ltrim(Dsn->SslCa);
    const char *SslCaPath= ltrim(Dsn->SslCaPath);
    const char *SslCipher= ltrim(Dsn->SslCipher);

    if (!MADB_IS_EMPTY(SslCa)
     || !MADB_IS_EMPTY(SslCaPath)
     || !MADB_IS_EMPTY(SslCipher)
     || !MADB_IS_EMPTY(SslCert)
     || !MADB_IS_EMPTY(SslKey))
    {
      char Enable= 1;
      mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_ENFORCE, &Enable);

      if (!MADB_IS_EMPTY(SslKey))
      {
        mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_KEY, SslKey);
      }
      if (!MADB_IS_EMPTY(SslCert))
      {
        mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_CERT, SslCert);
      }
      if (!MADB_IS_EMPTY(SslCa))
      {
        mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_CA, SslCa);
      }
      if (!MADB_IS_EMPTY(SslCaPath))
      {
        mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_CAPATH, SslCaPath);
      }
      if (!MADB_IS_EMPTY(SslCipher))
      {
        mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_CIPHER, SslCipher);
      }

      if (Dsn->TlsVersion > 0)
      {
        char TlsVersion[sizeof(TlsVersionName) + sizeof(TlsVersionBits) - 1], *Ptr= TlsVersion; /* All names + (n-1) comma */
        unsigned int i, NeedComma= 0;

        for (i= 0; i < sizeof(TlsVersionBits); ++i)
        {
          if (Dsn->TlsVersion & TlsVersionBits[i])
          {
            if (NeedComma != 0)
            {
              *Ptr++= ',';
            }
            else
            {
              NeedComma= 1;
            }
            strcpy(Ptr, TlsVersionName[i]);
            Ptr += strlen(TlsVersionName[i]);
          }
        }
        mysql_optionsv(Connection->mariadb, MARIADB_OPT_TLS_VERSION, (void *)TlsVersion);
      }
    }

    if (Dsn->SslVerify)
    {
      const unsigned int verify= 0x01010101;
      mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, (const char*)&verify);
    }
    else
    {
      const unsigned int verify= 0;
      mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, (const char*)&verify);
    }
  }
  
  if (Dsn->ForceTls != '\0')
  {
    const unsigned int ForceTls= 0x01010101;
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_ENFORCE, (const char*)&ForceTls);
  }

  if (!MADB_IS_EMPTY(Dsn->SslCrl))
  {
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_CRL, Dsn->SslCrl);
  }
  if (!MADB_IS_EMPTY(Dsn->SslCrlPath))
  {
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_SSL_CRLPATH, Dsn->SslCrlPath);
  }

  if (!MADB_IS_EMPTY(Dsn->ServerKey))
  {
    mysql_optionsv(Connection->mariadb, MYSQL_SERVER_PUBLIC_KEY, Dsn->ServerKey);
  }

  if (!MADB_IS_EMPTY(Dsn->TlsPeerFp))
  {
    mysql_optionsv(Connection->mariadb, MARIADB_OPT_TLS_PEER_FP, (void*)Dsn->TlsPeerFp);
  }
  if (!MADB_IS_EMPTY(Dsn->TlsPeerFpList))
  {
    mysql_optionsv(Connection->mariadb, MARIADB_OPT_TLS_PEER_FP_LIST, (void*)Dsn->TlsPeerFpList);
  }

  if (!MADB_IS_EMPTY(Dsn->TlsKeyPwd))
  {
    mysql_optionsv(Connection->mariadb, MARIADB_OPT_TLS_PASSPHRASE, (void*)Dsn->TlsKeyPwd);
  }

  if (Dsn->DisableLocalInfile != '\0')
  {
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_LOCAL_INFILE, &unselectedIntOption);
  }
  else
  {
    mysql_optionsv(Connection->mariadb, MYSQL_OPT_LOCAL_INFILE, &selectedIntOption);
  }

  /* set default catalog. If we have value set via SQL_ATTR_CURRENT_CATALOG - it takes priority. Otherwise - DSN */
  if (Connection->CatalogName && Connection->CatalogName[0])
  {
    defaultSchema= Connection->CatalogName;
  }
  else if (Dsn->Catalog && Dsn->Catalog[0])
  {
    //MADB_RESET(Connection->CatalogName, Dsn->Catalog);
    defaultSchema= Dsn->Catalog;
  }

  if (!mysql_real_connect(Connection->mariadb,
      Dsn->Socket ? "localhost" : Dsn->ServerName, Dsn->UserName, Dsn->Password, defaultSchema, Dsn->Port, Dsn->Socket, client_flags))
  {
    goto err;
  }
  
  /* I guess it is better not to do that at all. Besides SQL_ATTR_PACKET_SIZE is actually not for max packet size */
  if (Connection->PacketSize)
  {
    /*_snprintf(StmtStr, 128, "SET GLOBAL max_allowed_packet=%ld", Connection-> PacketSize);
    if (mysql_query(Connection->mariadb, StmtStr))
      goto err;*/
  }

  MADB_SetCapabilities(Connection, mysql_get_server_version(Connection->mariadb), mysql_get_server_name(Connection->mariadb));

  if (DSN_OPTION(Connection, MADB_OPT_FLAG_NO_CACHE))
  {
    Connection->Methods->CacheRestOfCurrentRsStream= &MADB_Dbc_CacheRestOfCurrentRsStream;
  }

  if (MADB_ServerSupports(Connection, MADB_SESSION_TRACKING) == TRUE)
  {
    int len;
    if (DSN_OPTION(Connection, MADB_OPT_FLAG_MULTI_STATEMENTS))
    {
      char buffer[sizeof("SET session_track_schema= ON;SET session_track_system_variables='autocommit'") + 1/*','*/ + 21/*transaction_isolation*/];
      len= _snprintf(buffer, sizeof(buffer), "SET session_track_schema= ON;SET session_track_system_variables='autocommit,%s'", MADB_GetTxIsolationVarName(Connection));
      if (mysql_real_query(Connection->mariadb, buffer, (unsigned long)len) ||
        mysql_next_result(Connection->mariadb))
      {
        goto sessionTrackinErr;
      }
    }
    else
    {
      char buffer[sizeof("SET session_track_system_variables='autocommit'") + 1/*','*/ + 21/*transaction_isolation*/];
      len= _snprintf(buffer, sizeof(buffer), "SET session_track_system_variables='autocommit,%s'", MADB_GetTxIsolationVarName(Connection));
      if (mysql_real_query(Connection->mariadb, "SET session_track_schema= ON", 28) ||
        mysql_real_query(Connection->mariadb, buffer, (unsigned long)len))
      {
        goto sessionTrackinErr;
      }
    }
    if (defaultSchema != NULL)
    {
      Connection->CurrentSchema= _strdup(defaultSchema);
    }
    goto end;
  }

sessionTrackinErr:
  /* Default methods are for use of session tracking */
  Connection->Methods->GetCurrentDB= &MADB_DbcGetCurrentDB;
  Connection->Methods->TrackSession= &MADB_DbcDummyTrackSession;
  Connection->Methods->GetTxIsolation= &MADB_DbcGetServerTxIsolation;

  goto end;

err:
  MADB_SetNativeError(&Connection->Error, SQL_HANDLE_DBC, Connection->mariadb);
      
end:
  if (Connection->Error.ReturnValue == SQL_ERROR && Connection->mariadb)
  {
    mysql_close(Connection->mariadb);
    Connection->mariadb= NULL;
  }

  return Connection->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_DbcGetFunctions */
SQLRETURN MADB_DbcGetFunctions(MADB_Dbc *Dbc, SQLUSMALLINT FunctionId, SQLUSMALLINT *SupportedPtr)
{
  unsigned int i, Elements= sizeof(MADB_supported_api) / sizeof(SQLUSMALLINT);
  
  switch(FunctionId) {
  case SQL_API_ODBC3_ALL_FUNCTIONS:
    /* clear ptr */ 
    memset(SupportedPtr, 0, sizeof(SQLUSMALLINT) * SQL_API_ODBC3_ALL_FUNCTIONS_SIZE);
    for (i=0; i < Elements; ++i)
    {
      SQLUSMALLINT function= MADB_supported_api[i]; 
      SupportedPtr[function >> 4]|= (1 << (function & 0x000F));
    }
    break;
  case SQL_API_ALL_FUNCTIONS:
    /* Set all to SQL_FALSE (0) */
    memset(SupportedPtr, 0, sizeof(SQLUSMALLINT) * 100);
    for (i=0; i < Elements; i++)
      if (MADB_supported_api[i] < 100)
        SupportedPtr[MADB_supported_api[i]]= SQL_TRUE;
    break;
  default:
    *SupportedPtr= SQL_FALSE;
    for (i=0; i < Elements; i++)
      if (MADB_supported_api[i] == FunctionId)
      {
        *SupportedPtr= SQL_TRUE;
        break;
      }
    break;
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ IsStringInfoType */
int IsStringInfoType(SQLSMALLINT InfoType)
{
  switch (InfoType)
  {
    case SQL_ACCESSIBLE_PROCEDURES:
    case SQL_ACCESSIBLE_TABLES:
    case SQL_CATALOG_NAME:
    case SQL_CATALOG_NAME_SEPARATOR:
    case SQL_CATALOG_TERM:
    case SQL_COLLATION_SEQ:
    case SQL_COLUMN_ALIAS:
    case SQL_DATA_SOURCE_NAME:
    case SQL_DATABASE_NAME:
    case SQL_DBMS_NAME:
    case SQL_DBMS_VER:
    case SQL_DESCRIBE_PARAMETER:
    case SQL_DRIVER_NAME:
    case SQL_DRIVER_ODBC_VER:
    case SQL_DRIVER_VER:
    case SQL_EXPRESSIONS_IN_ORDERBY:
    case SQL_INTEGRITY:
    case SQL_KEYWORDS:
    case SQL_LIKE_ESCAPE_CLAUSE:
    case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
    case SQL_MULT_RESULT_SETS:
    case SQL_MULTIPLE_ACTIVE_TXN:
    case SQL_NEED_LONG_DATA_LEN:
    case SQL_ORDER_BY_COLUMNS_IN_SELECT:
    case SQL_PROCEDURE_TERM:
    case SQL_PROCEDURES:
    case SQL_ROW_UPDATES:
    case SQL_SCHEMA_TERM:
    case SQL_SEARCH_PATTERN_ESCAPE:
    case SQL_SERVER_NAME:
    case SQL_SPECIAL_CHARACTERS:
    case SQL_TABLE_TERM:
    case SQL_USER_NAME:
    case SQL_XOPEN_CLI_YEAR:
    case SQL_DATA_SOURCE_READ_ONLY:
    case SQL_IDENTIFIER_QUOTE_CHAR:
      return 1;
  }

  return 0;
}
/* }}} */

/* {{{ MADB_DbcGetInfo */
SQLRETURN MADB_DbcGetInfo(MADB_Dbc *Dbc, SQLUSMALLINT InfoType, SQLPOINTER InfoValuePtr,
                          SQLSMALLINT BufferLength, SQLSMALLINT *StringLengthPtr, my_bool isWChar)
{
  SQLSMALLINT SLen= 0;
  extern Client_Charset utf8;

  if (!InfoValuePtr && !StringLengthPtr)
    return SQL_SUCCESS;

  /* Prety special case - on Windows DM passes NULL instead of InfoValuePtr and own pointer instead of StringLengthPtr.
     The logic here is not quite clear - I would imagine that truncated status is more appropriate.
     But UnixODBC does not do so, and we are making connector's behavior consistent */
  if (InfoValuePtr != NULL && BufferLength == 0 && StringLengthPtr == NULL && IsStringInfoType(InfoType))
  {
    return SQL_SUCCESS;
  }
 
  MADB_CLEAR_ERROR(&Dbc->Error);
  switch(InfoType) {
  case SQL_ACCESSIBLE_PROCEDURES:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, 
                                     (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), "N", SQL_NTS, &Dbc->Error);
    break;
  case SQL_ACCESSIBLE_TABLES:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL,
                                     (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), "N", SQL_NTS, &Dbc->Error);
    break;
  case SQL_ACTIVE_ENVIRONMENTS:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_AGGREGATE_FUNCTIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_AF_ALL | SQL_AF_AVG | SQL_AF_COUNT | SQL_AF_DISTINCT |
                                                SQL_AF_MAX | SQL_AF_MIN | SQL_AF_SUM, StringLengthPtr);
    break;
  case SQL_ALTER_DOMAIN:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_ALTER_TABLE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_AT_ADD_COLUMN | SQL_AT_DROP_COLUMN, StringLengthPtr);
    break;
#ifdef SQL_ASYNC_DBC_FUNCTIONS
  case SQL_ASYNC_DBC_FUNCTIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_ASYNC_DBC_NOT_CAPABLE, StringLengthPtr);
    break;
#endif
#ifdef SQL_ASYNC_MODE
  case SQL_ASYNC_MODE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_AM_NONE, StringLengthPtr);
    break;
#endif
#ifdef SQL_ASYNC_NOTIFICATION
  case SQL_ASYNC_NOTIFICATION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_ASYNC_NOTIFICATION_NOT_CAPABLE, StringLengthPtr);
    break;
#endif
  case SQL_BATCH_ROW_COUNT:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_BRC_EXPLICIT, StringLengthPtr);
    break;
  case SQL_BATCH_SUPPORT:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_BS_SELECT_EXPLICIT | SQL_BS_ROW_COUNT_EXPLICIT |
                                                SQL_BS_SELECT_PROC | SQL_BS_ROW_COUNT_PROC,
                     StringLengthPtr);
    break;
  case SQL_BOOKMARK_PERSISTENCE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CATALOG_LOCATION:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_CL_START, StringLengthPtr);
    break;
  case SQL_CATALOG_NAME:
    /* Todo: MyODBC Driver has a DSN configuration for diabling catalog usage:
       but it's not implemented in MAODBC */
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, 
                                     (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), "Y", SQL_NTS, &Dbc->Error);
    break;
  case SQL_CATALOG_NAME_SEPARATOR:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, 
                                     (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), ".", SQL_NTS, &Dbc->Error);
    break;
  case SQL_CATALOG_TERM:
    /* todo: See comment for SQL_CATALOG_NAME */
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, 
                                     (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), "database", SQL_NTS, &Dbc->Error);
    break;
  case SQL_CATALOG_USAGE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_CU_DML_STATEMENTS | 
                                                SQL_CU_INDEX_DEFINITION |
                                                SQL_CU_PROCEDURE_INVOCATION | 
                                                SQL_CU_PRIVILEGE_DEFINITION |
                                                SQL_CU_TABLE_DEFINITION,
                      StringLengthPtr);
    break;
  case SQL_COLLATION_SEQ:
  {
    MY_CHARSET_INFO cs;
    mariadb_get_infov(Dbc->mariadb, MARIADB_CONNECTION_MARIADB_CHARSET_INFO, (void*)&cs);
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL,
      (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar),
      cs.name, SQL_NTS, &Dbc->Error);
    break;
  }
  case SQL_COLUMN_ALIAS:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, 
                           (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), "Y", SQL_NTS, &Dbc->Error);
    break;
  case SQL_CONCAT_NULL_BEHAVIOR:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_CB_NULL, StringLengthPtr);
    break;
  case SQL_CONVERT_BIGINT:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_BINARY:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CONVERT_BIT:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_CHAR:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_WCHAR:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
#ifdef SQL_CONVERT_GUID
  case SQL_CONVERT_GUID:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
#endif
  case SQL_CONVERT_DATE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_DECIMAL:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_DOUBLE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_FLOAT:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_INTEGER:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_INTERVAL_YEAR_MONTH:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CONVERT_INTERVAL_DAY_TIME:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CONVERT_LONGVARBINARY:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CONVERT_LONGVARCHAR:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_WLONGVARCHAR:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_NUMERIC:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_REAL:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_SMALLINT:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_TIME:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_TIMESTAMP:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_TINYINT:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_VARBINARY:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CONVERT_VARCHAR:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_WVARCHAR:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, MADB_SUPPORTED_CONVERSIONS, StringLengthPtr);
    break;
  case SQL_CONVERT_FUNCTIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CORRELATION_NAME:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_CN_DIFFERENT, StringLengthPtr);
    break;
  case SQL_CREATE_ASSERTION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CREATE_CHARACTER_SET:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CREATE_COLLATION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CREATE_DOMAIN:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CREATE_SCHEMA:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CREATE_TABLE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_CT_COLUMN_COLLATION | SQL_CT_COLUMN_DEFAULT |
                                                SQL_CT_COMMIT_DELETE | SQL_CT_CREATE_TABLE |
                                                SQL_CT_LOCAL_TEMPORARY,
                     StringLengthPtr);
    break;
  case SQL_CREATE_TRANSLATION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_CREATE_VIEW:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_CV_CASCADED | SQL_CV_CHECK_OPTION |
                                                SQL_CV_CREATE_VIEW, StringLengthPtr);
    break;
  case SQL_CURSOR_COMMIT_BEHAVIOR:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_CB_PRESERVE, StringLengthPtr);
    break;
  case SQL_CURSOR_ROLLBACK_BEHAVIOR:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_CB_PRESERVE, StringLengthPtr);
    break;
  case SQL_CURSOR_SENSITIVITY:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_UNSPECIFIED, StringLengthPtr);
    break;
  case SQL_DATA_SOURCE_NAME:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar),
                                     Dbc->Dsn ? Dbc->Dsn->DSNName : "", SQL_NTS, &Dbc->Error);
    break;
  case SQL_DATABASE_NAME:
    return Dbc->Methods->GetCurrentDB(Dbc, InfoValuePtr, BufferLength, (SQLSMALLINT *)StringLengthPtr, isWChar);
    break;
  case SQL_DATETIME_LITERALS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_DL_SQL92_DATE | SQL_DL_SQL92_TIME |
                                                SQL_DL_SQL92_TIMESTAMP, StringLengthPtr);
    break;
  case SQL_DBMS_NAME:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar),
                                     Dbc->mariadb ? (char *)mysql_get_server_name(Dbc->mariadb) : "MariaDB",
                                     SQL_NTS, &Dbc->Error);
    break;
  case SQL_DBMS_VER:
    {
      char Version[13];
      unsigned long ServerVersion= 0L;
      
      if (Dbc->mariadb)
      {
        ServerVersion= mysql_get_server_version(Dbc->mariadb);
        _snprintf(Version, sizeof(Version), "%02u.%02u.%06u", ServerVersion / 10000,
                    (ServerVersion % 10000) / 100, ServerVersion % 100);
      }
      else
        Version[0]= 0;
      SLen= (SQLSMALLINT)MADB_SetString(isWChar ?  &utf8 : 0, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                       Version[0] ? Version : "", SQL_NTS, &Dbc->Error);
    }
    break;
  case SQL_DDL_INDEX:
     MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_DI_CREATE_INDEX | SQL_DI_DROP_INDEX, StringLengthPtr);
    break;
  case SQL_DEFAULT_TXN_ISOLATION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_DESCRIBE_PARAMETER:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "N", SQL_NTS, &Dbc->Error);
    break;
#ifdef SQL_DRIVER_AWARE_POOLING_SUPPORTED
  case SQL_DRIVER_AWARE_POOLING_SUPPORTED:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_DRIVER_AWARE_POOLING_NOT_CAPABLE, StringLengthPtr);
    break;
#endif
    /* Handled by driver manager */
  case SQL_DRIVER_HDBC:
    break;
  case SQL_DRIVER_HENV:
    break;
  case SQL_DRIVER_HLIB:
    break;
  case SQL_DRIVER_HSTMT:
    break;
  case SQL_DRIVER_NAME:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL,
                                     (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     MADB_DRIVER_NAME, SQL_NTS, &Dbc->Error);
    break;
  case SQL_DRIVER_ODBC_VER:
    {
      char *OdbcVersion = "03.51";
      /* DM requests this info before Dbc->Charset initialized. Thus checking if it is, and use utf8 by default
         The other way would be to use utf8 when Dbc initialized */
      SLen= (SQLSMALLINT)MADB_SetString(isWChar ? (Dbc->Charset.cs_info ? &Dbc->Charset : &utf8 ): NULL,
                                     (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     OdbcVersion, SQL_NTS, &Dbc->Error);
    }
    break;
  case SQL_DRIVER_VER:
     SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL,
                                     (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     MARIADB_ODBC_VERSION, SQL_NTS, &Dbc->Error);
    break;
    /*******************************/
  case SQL_DROP_ASSERTION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_DROP_CHARACTER_SET:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_DROP_COLLATION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_DROP_DOMAIN:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_DROP_SCHEMA:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_DROP_TABLE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_DT_CASCADE | SQL_DT_DROP_TABLE | 
                                                SQL_DT_RESTRICT, StringLengthPtr);
    break;
  case SQL_DROP_TRANSLATION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_DROP_VIEW:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_DV_CASCADE | SQL_DV_DROP_VIEW | 
                                                SQL_DV_RESTRICT, StringLengthPtr);
    break;
  case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_CA1_ABSOLUTE |
                                                SQL_CA1_BULK_ADD |
                                                SQL_CA1_LOCK_NO_CHANGE |
                                                SQL_CA1_NEXT |
                                                SQL_CA1_POSITIONED_DELETE |
                                                SQL_CA1_POSITIONED_UPDATE |
                                                SQL_CA1_POS_DELETE |
                                                SQL_CA1_POS_POSITION |
                                                SQL_CA1_POS_REFRESH |
                                                SQL_CA1_POS_UPDATE |
                                                SQL_CA1_RELATIVE, StringLengthPtr);
    break;
  case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_CA2_CRC_EXACT | 
                                                SQL_CA2_MAX_ROWS_DELETE |
                                                SQL_CA2_MAX_ROWS_INSERT |
                                                SQL_CA2_MAX_ROWS_SELECT |
                                                SQL_CA2_MAX_ROWS_UPDATE |
                                                SQL_CA2_SENSITIVITY_ADDITIONS |
                                                SQL_CA2_SENSITIVITY_DELETIONS |
                                                SQL_CA2_SENSITIVITY_UPDATES |
                                                SQL_CA2_SIMULATE_TRY_UNIQUE, StringLengthPtr);
    break;
  case SQL_EXPRESSIONS_IN_ORDERBY:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "Y", SQL_NTS, &Dbc->Error);
    break;
  case SQL_FILE_USAGE:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_FILE_NOT_SUPPORTED, StringLengthPtr);
    break;
  case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_CA1_ABSOLUTE |
                                                SQL_CA1_BULK_ADD |
                                                SQL_CA1_LOCK_NO_CHANGE |
                                                SQL_CA1_NEXT |
                                                SQL_CA1_POSITIONED_DELETE |
                                                SQL_CA1_POSITIONED_UPDATE |
                                                SQL_CA1_POS_DELETE |
                                                SQL_CA1_POS_POSITION |
                                                SQL_CA1_POS_REFRESH |
                                                SQL_CA1_POS_UPDATE |
                                                SQL_CA1_RELATIVE, StringLengthPtr);
    break;
  case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_CA2_CRC_EXACT |
                                                SQL_CA2_MAX_ROWS_DELETE |
                                                SQL_CA2_MAX_ROWS_INSERT |
                                                SQL_CA2_MAX_ROWS_SELECT |
                                                SQL_CA2_MAX_ROWS_UPDATE, StringLengthPtr);
    break;
  case SQL_GETDATA_EXTENSIONS:
  {
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER |
                                                SQL_GD_BLOCK | SQL_GD_BOUND, StringLengthPtr);
    break;
  }
  case SQL_GROUP_BY:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_GB_NO_RELATION, StringLengthPtr);
    break;
  case SQL_IDENTIFIER_CASE:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_IC_MIXED, StringLengthPtr);
    break;
  case SQL_IDENTIFIER_QUOTE_CHAR:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
      MADB_SqlMode(Dbc, MADB_ANSI_QUOTES) ? "\"" : "`", SQL_NTS, &Dbc->Error);
    break;
  case SQL_INDEX_KEYWORDS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_IK_ALL, StringLengthPtr);
    break;
  case SQL_INFO_SCHEMA_VIEWS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_ISV_CHARACTER_SETS | SQL_ISV_COLLATIONS |
                                                SQL_ISV_COLUMNS | SQL_ISV_COLUMN_PRIVILEGES |
                                                SQL_ISV_KEY_COLUMN_USAGE | SQL_ISV_REFERENTIAL_CONSTRAINTS |
                                                SQL_ISV_TABLES | SQL_ISV_TABLE_PRIVILEGES |
                                                SQL_ISV_TABLE_CONSTRAINTS | SQL_ISV_VIEWS, StringLengthPtr);
    break;
  case SQL_INSERT_STATEMENT:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_IS_INSERT_LITERALS | SQL_IS_INSERT_SEARCHED |
                                                SQL_IS_SELECT_INTO, StringLengthPtr);
    break;
  case SQL_INTEGRITY:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "N", SQL_NTS, &Dbc->Error);
    break;
  case SQL_KEYSET_CURSOR_ATTRIBUTES1:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_KEYSET_CURSOR_ATTRIBUTES2:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_KEYWORDS:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar),
                                     "ACCESSIBLE,ANALYZE,ASENSITIVE,BEFORE,BIGINT,BINARY,BLOB,CALL,"
                                     "CHANGE,CONDITION,DATABASE,DATABASES,DAY_HOUR,DAY_MICROSECOND,"
                                     "DAY_MINUTE,DAY_SECOND,DELAYED,DETERMINISTIC,DISTINCTROW,DIV,"
                                     "DUAL,EACH,ELSEIF,ENCLOSED,ESCAPED,EXIT,EXPLAIN,FLOAT4,FLOAT8,"
                                     "FORCE,FULLTEXT,HIGH_PRIORITY,HOUR_MICROSECOND,HOUR_MINUTE,"
                                     "HOUR_SECOND,IF,IGNORE,INFILE,INOUT,INT1,INT2,INT3,INT4,INT8,"
                                     "ITERATE,KEY,KEYS,KILL,LEAVE,LIMIT,LINEAR,LINES,LOAD,LOCALTIME,"
                                     "LOCALTIMESTAMP,LOCK,LONG,LONGBLOB,LONGTEXT,LOOP,LOW_PRIORITY,"
                                     "MEDIUMBLOB,MEDIUMINT,MEDIUMTEXT,MIDDLEINT,MINUTE_MICROSECOND,"
                                     "MINUTE_SECOND,MOD,MODIFIES,NO_WRITE_TO_BINLOG,OPTIMIZE,OPTIONALLY,"
                                     "OUT,OUTFILE,PURGE,RANGE,READS,READ_ONLY,READ_WRITE,REGEXP,RELEASE,"
                                     "RENAME,REPEAT,REPLACE,REQUIRE,RETURN,RLIKE,SCHEMAS,"
                                     "SECOND_MICROSECOND,SENSITIVE,SEPARATOR,SHOW,SPATIAL,SPECIFIC,"
                                     "SQLEXCEPTION,SQL_BIG_RESULT,SQL_CALC_FOUND_ROWS,SQL_SMALL_RESULT,"
                                     "SSL,STARTING,STRAIGHT_JOIN,TERMINATED,TINYBLOB,TINYINT,TINYTEXT,"
                                     "TRIGGER,UNDO,UNLOCK,UNSIGNED,USE,UTC_DATE,UTC_TIME,UTC_TIMESTAMP,"
                                     "VARBINARY,VARCHARACTER,WHILE,X509,XOR,YEAR_MONTH,ZEROFILL,GENERAL,"
                                     "IGNORE_SERVER_IDS,MASTER_HEARTBEAT_PERIOD,MAXVALUE,RESIGNAL,SIGNAL,"
                                     "SLOW", SQL_NTS, &Dbc->Error);
    break;
  case SQL_LIKE_ESCAPE_CLAUSE:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "Y", SQL_NTS, &Dbc->Error);
    break;
  case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_BINARY_LITERAL_LEN:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_CATALOG_NAME_LEN:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, NAME_CHAR_LEN * SYSTEM_MB_MAX_CHAR_LENGTH, StringLengthPtr);
    break;
  case SQL_MAX_CHAR_LITERAL_LEN:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_COLUMN_NAME_LEN:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, NAME_CHAR_LEN * SYSTEM_MB_MAX_CHAR_LENGTH - 1, StringLengthPtr);
    break;
  case SQL_MAX_COLUMNS_IN_GROUP_BY:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_COLUMNS_IN_INDEX:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 32, StringLengthPtr);
    break;
  case SQL_MAX_COLUMNS_IN_ORDER_BY:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_COLUMNS_IN_SELECT:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_COLUMNS_IN_TABLE:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_CONCURRENT_ACTIVITIES:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_CURSOR_NAME_LEN:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, MADB_MAX_CURSOR_NAME, StringLengthPtr);
    break;
  case SQL_MAX_DRIVER_CONNECTIONS:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_IDENTIFIER_LEN:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, NAME_CHAR_LEN * SYSTEM_MB_MAX_CHAR_LENGTH, StringLengthPtr);
    break;
  case SQL_MAX_INDEX_SIZE:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 3072, StringLengthPtr);
    break;
  case SQL_MAX_PROCEDURE_NAME_LEN:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, NAME_CHAR_LEN * SYSTEM_MB_MAX_CHAR_LENGTH, StringLengthPtr);
    break;
  case SQL_MAX_ROW_SIZE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "Y", SQL_NTS, &Dbc->Error);
    break;
  case SQL_MAX_SCHEMA_NAME_LEN:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_MAX_STATEMENT_LEN:
    {
      size_t max_packet_size;
      mariadb_get_infov(Dbc->mariadb, MARIADB_MAX_ALLOWED_PACKET, (void*)&max_packet_size);
      MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, (SQLUINTEGER)max_packet_size, StringLengthPtr);
    }
    break;
  case SQL_MAX_TABLE_NAME_LEN:
     MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, NAME_CHAR_LEN * SYSTEM_MB_MAX_CHAR_LENGTH, StringLengthPtr);
    break;
  case SQL_MAX_TABLES_IN_SELECT:
     MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, 63, StringLengthPtr);
    break;
  case SQL_MAX_USER_NAME_LEN:
     MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, USERNAME_LENGTH, StringLengthPtr);
    break;
  case SQL_MULT_RESULT_SETS:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "Y", SQL_NTS, &Dbc->Error);
    break;
  case SQL_MULTIPLE_ACTIVE_TXN:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "Y", SQL_NTS, &Dbc->Error);
    break;
  case SQL_NEED_LONG_DATA_LEN:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "N", SQL_NTS, &Dbc->Error);
    break;
  case SQL_NON_NULLABLE_COLUMNS:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_NNC_NON_NULL, StringLengthPtr);
    break;
  case SQL_NULL_COLLATION:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_NC_LOW, StringLengthPtr);
    break;
  case SQL_NUMERIC_FUNCTIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_FN_NUM_ABS | SQL_FN_NUM_ACOS | SQL_FN_NUM_ASIN |
                                                SQL_FN_NUM_ATAN | SQL_FN_NUM_ATAN2 | SQL_FN_NUM_CEILING |
                                                SQL_FN_NUM_COS | SQL_FN_NUM_COT | SQL_FN_NUM_EXP |
                                                SQL_FN_NUM_FLOOR | SQL_FN_NUM_LOG | SQL_FN_NUM_MOD |
                                                SQL_FN_NUM_SIGN | SQL_FN_NUM_SIN | SQL_FN_NUM_SQRT |
                                                SQL_FN_NUM_TAN | SQL_FN_NUM_PI | SQL_FN_NUM_RAND |
                                                SQL_FN_NUM_DEGREES | SQL_FN_NUM_LOG10 | SQL_FN_NUM_POWER |
                                                SQL_FN_NUM_RADIANS | SQL_FN_NUM_ROUND | SQL_FN_NUM_TRUNCATE,
                     StringLengthPtr);
    break;
  case SQL_ODBC_API_CONFORMANCE:
    MADB_SET_NUM_VAL(SQLSMALLINT, InfoValuePtr, SQL_OAC_LEVEL1, StringLengthPtr);
    break;
  case SQL_ODBC_INTERFACE_CONFORMANCE:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_OIC_CORE, StringLengthPtr);
    break;
  case SQL_ODBC_SQL_CONFORMANCE:
    MADB_SET_NUM_VAL(SQLSMALLINT, InfoValuePtr, SQL_OSC_CORE, StringLengthPtr);
    break;
  case SQL_ODBC_VER:
    break;
  case SQL_OUTER_JOINS:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, 
                                     (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), "Y", SQL_NTS, &Dbc->Error);
    break;
  case SQL_OJ_CAPABILITIES:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_OJ_LEFT | SQL_OJ_RIGHT |
                                                SQL_OJ_NESTED | SQL_OJ_INNER, StringLengthPtr);
    break;
  case SQL_ORDER_BY_COLUMNS_IN_SELECT:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "N", SQL_NTS, &Dbc->Error);
    break;
  case SQL_PARAM_ARRAY_ROW_COUNTS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_PARC_NO_BATCH, StringLengthPtr);
    break;
  case SQL_PARAM_ARRAY_SELECTS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_PAS_NO_BATCH, StringLengthPtr);
    break;
  case SQL_PROCEDURE_TERM:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "stored procedure", SQL_NTS, &Dbc->Error);
    break;
  case SQL_PROCEDURES:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "Y", SQL_NTS, &Dbc->Error);
    break;
  case SQL_QUOTED_IDENTIFIER_CASE:  
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_IC_SENSITIVE, StringLengthPtr);
    break;
  case SQL_ROW_UPDATES:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, 
                                      BUFFER_CHAR_LEN(BufferLength, isWChar),
                                     "N", SQL_NTS, &Dbc->Error);
    break;
  case SQL_SCHEMA_TERM:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr,
                                      BUFFER_CHAR_LEN(BufferLength, isWChar),
                                     "", SQL_NTS, &Dbc->Error);
    break;
  case SQL_SCHEMA_USAGE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_SCROLL_OPTIONS:
    {
      SQLUINTEGER Options= SQL_SO_FORWARD_ONLY;
      if (!MA_ODBC_CURSOR_FORWARD_ONLY(Dbc))
        Options|= SQL_SO_STATIC;
      if (MA_ODBC_CURSOR_DYNAMIC(Dbc))
        Options|= SQL_SO_DYNAMIC;
      MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, Options, StringLengthPtr);
    }
    break;
  case SQL_SEARCH_PATTERN_ESCAPE:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr,
                                      BUFFER_CHAR_LEN(BufferLength, isWChar),
                                     "\\", SQL_NTS, &Dbc->Error);
    break;
  case SQL_SERVER_NAME:
  {
    const char *Host= "";
    if (Dbc->mariadb)
    {
      mariadb_get_infov(Dbc->mariadb, MARIADB_CONNECTION_HOST, (void*)&Host);
    }
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr,
      BUFFER_CHAR_LEN(BufferLength, isWChar),
      Host, SQL_NTS, &Dbc->Error);
    break;
  }
  case SQL_SPECIAL_CHARACTERS:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr, 
                                      BUFFER_CHAR_LEN(BufferLength, isWChar),
                                     "\"\\/", SQL_NTS, &Dbc->Error);
    break;
  case SQL_SQL_CONFORMANCE:
     MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SC_SQL92_INTERMEDIATE, StringLengthPtr);
    break;
  case SQL_SQL92_DATETIME_FUNCTIONS:
     MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SDF_CURRENT_DATE | SQL_SDF_CURRENT_TIME |
                                                 SQL_SDF_CURRENT_TIMESTAMP, StringLengthPtr);
    break;
  case SQL_SQL92_FOREIGN_KEY_DELETE_RULE:
     MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_SQL92_GRANT:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SG_DELETE_TABLE | SQL_SG_INSERT_COLUMN |
                                                SQL_SG_INSERT_TABLE | SQL_SG_REFERENCES_COLUMN |
                                                SQL_SG_REFERENCES_TABLE | SQL_SG_SELECT_TABLE |
                                                SQL_SG_UPDATE_COLUMN | SQL_SG_UPDATE_TABLE |
                                                SQL_SG_WITH_GRANT_OPTION, StringLengthPtr);
    break;
  case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SNVF_BIT_LENGTH | SQL_SNVF_CHARACTER_LENGTH |
                                                SQL_SNVF_CHAR_LENGTH | SQL_SNVF_EXTRACT |
                                                SQL_SNVF_OCTET_LENGTH | SQL_SNVF_POSITION,
                     StringLengthPtr);
    break;
  case SQL_SQL92_PREDICATES:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SP_BETWEEN | SQL_SP_COMPARISON |
                                                SQL_SP_EXISTS | SQL_SP_IN | SQL_SP_ISNOTNULL |
                                                SQL_SP_ISNULL | SQL_SP_LIKE | SQL_SP_QUANTIFIED_COMPARISON,
                     StringLengthPtr);
    break;
  case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SRJO_CROSS_JOIN | SQL_SRJO_INNER_JOIN |
                                                SQL_SRJO_LEFT_OUTER_JOIN | SQL_SRJO_RIGHT_OUTER_JOIN |
                                                SQL_SRJO_NATURAL_JOIN,
                     StringLengthPtr);
    break;
  case SQL_SQL92_REVOKE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SR_DELETE_TABLE | SQL_SR_INSERT_COLUMN |
                                                SQL_SR_INSERT_TABLE | SQL_SR_REFERENCES_COLUMN |
                                                SQL_SR_REFERENCES_TABLE | SQL_SR_SELECT_TABLE |
                                                SQL_SR_UPDATE_COLUMN | SQL_SR_UPDATE_TABLE,
                     StringLengthPtr);
    break;
  case SQL_SQL92_ROW_VALUE_CONSTRUCTOR:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SRVC_DEFAULT | SQL_SRVC_NULL |
                                                 SQL_SRVC_ROW_SUBQUERY | SQL_SRVC_VALUE_EXPRESSION,
                     StringLengthPtr);
    break;
  case SQL_SQL92_STRING_FUNCTIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SSF_CONVERT | SQL_SSF_LOWER |
                                                SQL_SSF_SUBSTRING | SQL_SSF_TRANSLATE |
                                                SQL_SSF_TRIM_BOTH | SQL_SSF_TRIM_LEADING |
                                                SQL_SSF_TRIM_TRAILING | SQL_SSF_UPPER,
                     StringLengthPtr);
    break;
  case SQL_SQL92_VALUE_EXPRESSIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SVE_CASE | SQL_SVE_CAST | SQL_SVE_COALESCE |
                                                SQL_SVE_NULLIF, StringLengthPtr);
    break;
  case SQL_STANDARD_CLI_CONFORMANCE:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SCC_ISO92_CLI, StringLengthPtr);
    break;
  case SQL_STATIC_CURSOR_ATTRIBUTES1:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_CA1_ABSOLUTE | /* SQL_CA1_BOOKMARK | */
                                                SQL_CA1_NEXT | SQL_CA1_RELATIVE,
                     StringLengthPtr);
    break;
  case SQL_STATIC_CURSOR_ATTRIBUTES2:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_CA2_MAX_ROWS_SELECT, StringLengthPtr);
    break;
  case SQL_STRING_FUNCTIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_FN_STR_ASCII | SQL_FN_STR_BIT_LENGTH |
                                                SQL_FN_STR_CHAR | SQL_FN_STR_CHAR_LENGTH |
                                                SQL_FN_STR_CONCAT | SQL_FN_STR_INSERT | 
                                                SQL_FN_STR_LCASE | SQL_FN_STR_LEFT | 
                                                SQL_FN_STR_LENGTH | SQL_FN_STR_LOCATE |
                                                SQL_FN_STR_LOCATE_2 | SQL_FN_STR_LTRIM |
                                                SQL_FN_STR_OCTET_LENGTH | SQL_FN_STR_POSITION |
                                                SQL_FN_STR_REPEAT | SQL_FN_STR_REPLACE |
                                                SQL_FN_STR_RIGHT | SQL_FN_STR_RTRIM |
                                                SQL_FN_STR_SOUNDEX | SQL_FN_STR_SPACE |
                                                SQL_FN_STR_SUBSTRING | SQL_FN_STR_UCASE,
                     StringLengthPtr);
    break;
  case SQL_SUBQUERIES:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_SQ_COMPARISON | SQL_SQ_CORRELATED_SUBQUERIES | 
                                                SQL_SQ_EXISTS | SQL_SQ_IN | SQL_SQ_QUANTIFIED,
                     StringLengthPtr);
    break;
  case SQL_SYSTEM_FUNCTIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_FN_SYS_DBNAME | SQL_FN_SYS_IFNULL |
                                                SQL_FN_SYS_USERNAME, StringLengthPtr);
    break;
  case SQL_TABLE_TERM:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr,
                                      BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                     "table", SQL_NTS, &Dbc->Error);
    break;
  case SQL_TIMEDATE_ADD_INTERVALS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_TIMEDATE_DIFF_INTERVALS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, 0, StringLengthPtr);
    break;
  case SQL_TIMEDATE_FUNCTIONS:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_FN_TD_CURDATE | SQL_FN_TD_CURRENT_DATE | 
                                                SQL_FN_TD_CURRENT_TIME | SQL_FN_TD_CURRENT_TIMESTAMP | 
                                                SQL_FN_TD_CURTIME | SQL_FN_TD_DAYNAME |
                                                SQL_FN_TD_DAYOFMONTH | SQL_FN_TD_DAYOFWEEK |
                                                SQL_FN_TD_DAYOFYEAR | SQL_FN_TD_EXTRACT | 
                                                SQL_FN_TD_HOUR | SQL_FN_TD_MINUTE | 
                                                SQL_FN_TD_MONTH | SQL_FN_TD_MONTHNAME | 
                                                SQL_FN_TD_NOW | SQL_FN_TD_QUARTER | 
                                                SQL_FN_TD_SECOND | SQL_FN_TD_TIMESTAMPADD |
                                                SQL_FN_TD_TIMESTAMPDIFF | SQL_FN_TD_WEEK | SQL_FN_TD_YEAR,
                      StringLengthPtr);
    break;
  case SQL_TXN_CAPABLE:
    MADB_SET_NUM_VAL(SQLUSMALLINT, InfoValuePtr, SQL_TC_DDL_COMMIT, StringLengthPtr);
    break;
  case SQL_TXN_ISOLATION_OPTION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_TXN_READ_COMMITTED | SQL_TXN_READ_UNCOMMITTED |
                                                SQL_TXN_REPEATABLE_READ | SQL_TXN_SERIALIZABLE,
                      StringLengthPtr);
    break;
  case SQL_UNION:
    MADB_SET_NUM_VAL(SQLUINTEGER, InfoValuePtr, SQL_U_UNION | SQL_U_UNION_ALL, StringLengthPtr);
    break;
  case SQL_USER_NAME:
  {
    const char *User= "";
    if (Dbc->mariadb)
    {
      mariadb_get_infov(Dbc->mariadb, MARIADB_CONNECTION_USER, (void *)&User);
    }
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr,
                                     BUFFER_CHAR_LEN(BufferLength, isWChar), 
                                      User, SQL_NTS, &Dbc->Error);
    break;
  }
  case SQL_XOPEN_CLI_YEAR:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr,
                                     BUFFER_CHAR_LEN(BufferLength, isWChar),
                                     "1992", SQL_NTS, &Dbc->Error);
    break;
  case SQL_DATA_SOURCE_READ_ONLY:
    SLen= (SQLSMALLINT)MADB_SetString(isWChar ? &Dbc->Charset : NULL, (void *)InfoValuePtr,
                                     BUFFER_CHAR_LEN(BufferLength, isWChar),
                                     "N", SQL_NTS, &Dbc->Error);
    break;
  /* 2.0 types */
  case SQL_POS_OPERATIONS:
    MADB_SET_NUM_VAL(SQLINTEGER, InfoValuePtr, SQL_POS_POSITION | SQL_POS_REFRESH | SQL_POS_UPDATE | SQL_POS_DELETE | SQL_POS_ADD,
                     StringLengthPtr);
    break;
  case SQL_STATIC_SENSITIVITY:
    MADB_SET_NUM_VAL(SQLINTEGER, InfoValuePtr, SQL_SS_DELETIONS | SQL_SS_UPDATES, StringLengthPtr);
    break;
  case SQL_LOCK_TYPES:
    MADB_SET_NUM_VAL(SQLINTEGER, InfoValuePtr, SQL_LCK_NO_CHANGE, StringLengthPtr);
    break;
  case SQL_SCROLL_CONCURRENCY:
    MADB_SET_NUM_VAL(SQLINTEGER, InfoValuePtr, SQL_SCCO_READ_ONLY | SQL_SCCO_OPT_VALUES, StringLengthPtr);
    break;
default:
    MADB_SetError(&Dbc->Error, MADB_ERR_HY096, NULL, 0);
    return Dbc->Error.ReturnValue;
  }
  if (isWChar && SLen)
  {
    SLen*= sizeof(SQLWCHAR);
  }
  if (IsStringInfoType(InfoType) && StringLengthPtr)
  {
    *StringLengthPtr= SLen;
  }
  
  return SQL_SUCCESS;
}
/* }}} */


/* {{{ MADB_DriverSideMemalocate */
char * MADB_DriverSideAllocate(size_t size)
{
  return (char *)MADB_CALLOC(size);
}
/* }}} */


/* {{{ MADB_DriverSideFree */
void MADB_DriverSideFree(void *ptr)
{
  MADB_FREE(ptr);
}
/* }}} */


/* {{{ MADB_DriverConnect */
SQLRETURN MADB_DriverConnect(MADB_Dbc *Dbc, SQLHWND WindowHandle, SQLCHAR *InConnectionString,
                             SQLULEN StringLength1, SQLCHAR *OutConnectionString,
                             SQLULEN BufferLength, SQLSMALLINT *StringLength2Ptr,
                             SQLUSMALLINT DriverCompletion)
{
  MADB_Dsn   *Dsn;
  MADB_Drv   *Drv=       NULL;
  SQLRETURN   ret=       SQL_SUCCESS;
  MADB_Prompt DSNPrompt= { NULL, NULL };
  SQLULEN     Length;

  if (!Dbc)
    return SQL_INVALID_HANDLE;

  MADB_CLEAR_ERROR(&Dbc->Error);

  Dsn= MADB_DSN_Init();

  if (!MADB_ReadConnString(Dsn, (char *)InConnectionString, StringLength1, ';'))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_HY000, "Error while parsing DSN", 0);
    goto error;
  }

  /* if DSN prompt is off, adjusting DriverCompletion */
  if (Dsn->ConnectPrompt)
    DriverCompletion= SQL_DRIVER_NOPROMPT;

  switch (DriverCompletion) {
  case SQL_DRIVER_COMPLETE_REQUIRED:
  case SQL_DRIVER_COMPLETE:
  case SQL_DRIVER_NOPROMPT:

    if (SQL_SUCCEEDED(MADB_DbcConnectDB(Dbc, Dsn)))
    {
      goto end;
    }
    else if (DriverCompletion == SQL_DRIVER_NOPROMPT)
    {
      /* For SQL_DRIVER_COMPLETE(_REQUIRED) this is not the end - will show prompt for user */
      goto error;
    }
    /* If we got here, it means that we had unsuccessful connect attempt with SQL_DRIVER_COMPLETE(_REQUIRED) completion
       Have to clean that error */
    MADB_CLEAR_ERROR(&Dbc->Error);
    break;

  case SQL_DRIVER_PROMPT:
    break;
  default:
    MADB_SetError(&Dbc->Error, MADB_ERR_HY110, NULL, 0);
    goto error;
    break;
  }

  /* Without window handle we can't show a dialog */
  if (DriverCompletion != SQL_DRIVER_NOPROMPT && !WindowHandle)
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_IM008, NULL, 0);
    goto error;
  }
  
  if (DriverCompletion == SQL_DRIVER_COMPLETE_REQUIRED)
    Dsn->isPrompt= MAODBC_PROMPT_REQUIRED;
  else
    Dsn->isPrompt= MAODBC_PROMPT;

  /* We need to obtain the driver name to load maodbcs.dll, if it's not stored inside DSN, 
     error IM007 (dialog prohibited) will be returned */
  if (!Dsn->Driver)
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_IM007, NULL, 0);
    goto error;
  }
  
  if (!(Drv= MADB_DriverGet(Dsn->Driver)))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_IM003, NULL, 0);
    goto error;
  }
  if (!Drv->SetupLibrary)
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_HY000, "Couldn't determine setup library", 0);
    goto error;
  }
 
  switch (DSNPrompt_Lookup(&DSNPrompt, Drv->SetupLibrary))
  {
  case 0: break;
  case MADB_PROMPT_NOT_SUPPORTED:
    MADB_SetError(&Dbc->Error, MADB_ERR_HY000, "Prompting is not supported on this platform", 0);
    goto error;
  case MADB_PROMPT_COULDNT_LOAD:
    MADB_SetError(&Dbc->Error, MADB_ERR_HY000, "Couldn't load the setup library", 0);
    goto error;
  }

  Dsn->allocator= MADB_DriverSideAllocate;
  Dsn->free=      MADB_DriverSideFree;

  if (DSNPrompt.Call((HWND)WindowHandle, Dsn) == FALSE)
  {
    /* If user cancels prompt, SQLDriverConnect should return SQL_NO_DATA */
    Dbc->Error.ReturnValue= SQL_NO_DATA;
    goto error;
  }

  DSNPrompt_Free(&DSNPrompt);

  ret= MADB_DbcConnectDB(Dbc, Dsn);
  if (!SQL_SUCCEEDED(ret))
  {
    goto error;
  }

end:
  Dbc->Dsn= Dsn;
  /* Dialog returns bitmap - syncing corresponding properties */
  MADB_DsnUpdateOptionsFields(Dsn);
  if (Dsn->isPrompt)
  {
    char *PreservePwd;

    /* DM should do that on its own, but we still better also remove pwd from the string being saved in the file DSN */
    if (Dsn->SaveFile != NULL)
    {
      PreservePwd= Dsn->Password;
      Dsn->Password= NULL;
    }
    /* If prompt/complete(_required), and dialog was succusefully showed - we generate string from the result DSN */
    Length= MADB_DsnToString(Dsn, (char *)OutConnectionString, BufferLength);

    if (Dsn->SaveFile != NULL)
    {
      Dsn->Password= PreservePwd;
    }
  }
  else
  {
    if (StringLength1 == SQL_NTS)
    {
      StringLength1= (SQLSMALLINT)strlen((const char*)InConnectionString);
    }
    if (OutConnectionString && BufferLength)
    {
      /* Otherwise we are supposed to simply copy incoming connection string */
      strncpy_s((char *)OutConnectionString, BufferLength, (const char*)InConnectionString, StringLength1);
    }
    Length= StringLength1;
  }
  if (StringLength2Ptr)
    *StringLength2Ptr= (SQLSMALLINT)Length;

  if (OutConnectionString && BufferLength && Length > BufferLength)
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_01004, NULL, 0);
    return Dbc->Error.ReturnValue;
  }
  return ret;
error:
  DSNPrompt_Free(&DSNPrompt);
  MADB_DSN_Free(Dsn);
  MADB_DriverFree(Drv);
  return Dbc->Error.ReturnValue;
}
/* }}} */

SQLRETURN MADB_DbcTrackSession(MADB_Dbc* Dbc)
{
  const char *key, *value;
  size_t keyLength, length;

  if (mysql_session_track_get_first(Dbc->mariadb, SESSION_TRACK_SCHEMA, &value, &length) == 0)
  {
    MADB_FREE(Dbc->CurrentSchema);
    Dbc->CurrentSchema= strndup(value, length);
  }

  if (mysql_session_track_get_first(Dbc->mariadb, SESSION_TRACK_SYSTEM_VARIABLES, &key, &keyLength) == 0)
  {
    do {
      mysql_session_track_get_next(Dbc->mariadb, SESSION_TRACK_SYSTEM_VARIABLES, &value, &length);
      if (strncmp(key, "autocommit", keyLength) == 0)
      {
        /* Seemingly it's ON or OFF, but checking also lowercase and '0' or '1' */
        Dbc->AutoCommit= test(length > 1 && (value[1] == 'N' || value[1] == 'n') || length == 1 && *value == '1');
      }
      else if (strncmp(key, MADB_GetTxIsolationVarName(Dbc), keyLength) == 0)
      {
        Dbc->TxnIsolation= TranslateTxIsolation(value, length);
      }
    } while (mysql_session_track_get_next(Dbc->mariadb, SESSION_TRACK_SYSTEM_VARIABLES, &key, &keyLength) == 0);
  }
  return SQL_SUCCESS;
}

SQLRETURN MADB_DbcDummyTrackSession(MADB_Dbc* Dbc)
{
  return SQL_SUCCESS;
}


/* {{{ MADB_DbcGetTrackedTxIsolatin
   
*/
SQLRETURN MADB_DbcGetTrackedTxIsolatin(MADB_Dbc* Dbc, SQLINTEGER* txIsolation)
{
  MADB_CLEAR_ERROR(&Dbc->Error);

  *txIsolation= Dbc->TxnIsolation;

  return SQL_SUCCESS;
}


/* {{{ MADB_DbcGetServerTxIsolation
   
*/
SQLRETURN MADB_DbcGetServerTxIsolation(MADB_Dbc* Dbc, SQLINTEGER* txIsolation)
{
  MYSQL_RES* result;
  MYSQL_ROW row;
  const char* StmtString = MADB_GetTxIsolationQuery(Dbc);

  LOCK_MARIADB(Dbc);
  if (mysql_real_query(Dbc->mariadb, StmtString, 21))
  {
    UNLOCK_MARIADB(Dbc);
    return MADB_SetNativeError(&Dbc->Error, SQL_HANDLE_DBC, Dbc->mariadb);
  }
  result = mysql_store_result(Dbc->mariadb);
  UNLOCK_MARIADB(Dbc);
  if ( result != NULL && (row = mysql_fetch_row(result)))
  {
    *txIsolation= Dbc->TxnIsolation= TranslateTxIsolation(row[0], strlen(row[0]));
    mysql_free_result(result);
  }
  else
  {
    return MADB_SetNativeError(&Dbc->Error, SQL_HANDLE_DBC, Dbc->mariadb);
  }

  return SQL_SUCCESS;
}

/* Default method for current stream caching - for the case caching is not disabled, and stream caching is not needed */
int MADB_Dbc_StreamingIsNotUsed(MADB_Stmt* Stmt)
{
  return 0;
}

struct st_ma_connection_methods MADB_Dbc_Methods =
{ 
  MADB_DbcSetAttr,
  MADB_DbcGetAttr,
  MADB_DbcConnectDB,
  MADB_DbcEndTran,
  MADB_DbcGetFunctions,
  MADB_DbcGetInfo,
  MADB_DriverConnect,
  MADB_DbcGetTrackedCurrentDB,
  MADB_DbcTrackSession,
  MADB_DbcGetTrackedTxIsolatin,
  MADB_Dbc_StreamingIsNotUsed /* and thus there is nothing to cache */
};
