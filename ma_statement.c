/************************************************************************************
   Copyright (C) 2013,2019 MariaDB Corporation AB
   
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

#define MADB_MIN_QUERY_LEN 5

struct st_ma_stmt_methods MADB_StmtMethods; /* declared at the end of file */

/* {{{ MADB_StmtInit */
SQLRETURN MADB_StmtInit(MADB_Dbc *Connection, SQLHANDLE *pHStmt)
{
  MADB_Stmt *Stmt= NULL;

  if (!(Stmt = (MADB_Stmt *)MADB_CALLOC(sizeof(MADB_Stmt))))
    goto error;
 
  MADB_PutErrorPrefix(Connection, &Stmt->Error);
  *pHStmt= Stmt;
  Stmt->Connection= Connection;
 
  LOCK_MARIADB(Connection);

  if (!(Stmt->stmt= MADB_NewStmtHandle(Stmt)) ||
    !(Stmt->IApd= MADB_DescInit(Connection, MADB_DESC_APD, FALSE)) ||
    !(Stmt->IArd= MADB_DescInit(Connection, MADB_DESC_ARD, FALSE)) ||
    !(Stmt->IIpd= MADB_DescInit(Connection, MADB_DESC_IPD, FALSE)) ||
    !(Stmt->IIrd= MADB_DescInit(Connection, MADB_DESC_IRD, FALSE)))
  {
    UNLOCK_MARIADB(Stmt->Connection);
    goto error;
  }

  MDBUG_C_PRINT(Stmt->Connection, "-->inited %0x", Stmt->stmt);
  UNLOCK_MARIADB(Connection);
  Stmt->PutParam= -1;
  Stmt->Methods= &MADB_StmtMethods;
  /* default behaviour is SQL_CURSOR_STATIC */
  Stmt->Options.CursorType= SQL_CURSOR_STATIC;
  Stmt->Options.UseBookmarks= SQL_UB_OFF;
  Stmt->Options.MetadataId= Connection->MetadataId;

  Stmt->Apd= Stmt->IApd;
  Stmt->Ard= Stmt->IArd;
  Stmt->Ipd= Stmt->IIpd;
  Stmt->Ird= Stmt->IIrd;
  
  Stmt->ListItem.data= (void *)Stmt;
  EnterCriticalSection(&Stmt->Connection->ListsCs);
  Stmt->Connection->Stmts= MADB_ListAdd(Stmt->Connection->Stmts, &Stmt->ListItem);
  LeaveCriticalSection(&Stmt->Connection->ListsCs);

  Stmt->Ard->Header.ArraySize= 1;

  return SQL_SUCCESS;

error:
  if (Stmt && Stmt->stmt)
  {
    MADB_STMT_CLOSE_STMT(Stmt);
  }
  MADB_DescFree(Stmt->IApd, TRUE);
  MADB_DescFree(Stmt->IArd, TRUE);
  MADB_DescFree(Stmt->IIpd, TRUE);
  MADB_DescFree(Stmt->IIrd, TRUE);
  MADB_FREE(Stmt);
  return SQL_ERROR;
}
/* }}} */

/* {{{ MADB_ExecuteQuery */
SQLRETURN MADB_ExecuteQuery(MADB_Stmt * Stmt, char *StatementText, SQLINTEGER TextLength)
{
  SQLRETURN ret= SQL_ERROR;
  
  LOCK_MARIADB(Stmt->Connection);
  if (StatementText)
  {
    MDBUG_C_PRINT(Stmt->Connection, "mysql_real_query(%0x,%s,%lu)", Stmt->Connection->mariadb, StatementText, TextLength);
    if(!mysql_real_query(Stmt->Connection->mariadb, StatementText, TextLength))
    {
      ret= SQL_SUCCESS;
      MADB_CLEAR_ERROR(&Stmt->Error);

      Stmt->AffectedRows= mysql_affected_rows(Stmt->Connection->mariadb);
    }
    else
    {
      MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_DBC, Stmt->Connection->mariadb);
    }
  }
  else
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, mysql_error(Stmt->Connection->mariadb), 
                            mysql_errno(Stmt->Connection->mariadb));
  UNLOCK_MARIADB(Stmt->Connection);

  return ret;
}
/* }}} */

/* {{{ MADB_StmtBulkOperations */
SQLRETURN MADB_StmtBulkOperations(MADB_Stmt *Stmt, SQLSMALLINT Operation)
{
  MADB_CLEAR_ERROR(&Stmt->Error);
  switch(Operation)
  {
  case SQL_ADD:
     return Stmt->Methods->SetPos(Stmt, 0, SQL_ADD, SQL_LOCK_NO_CHANGE, 0);
  default:
    return SQL_ERROR;
  }
}
/* }}} */

/* {{{ RemoveStmtRefFromDesc
       Helper function removing references to the stmt in the descriptor when explisitly allocated descriptor is substituted
       by some other descriptor */
void RemoveStmtRefFromDesc(MADB_Desc *desc, MADB_Stmt *Stmt, BOOL all)
{
  if (desc->AppType)
  {
    unsigned int i;
    for (i=0; i < desc->Stmts.elements; ++i)
    {
      MADB_Stmt **refStmt= ((MADB_Stmt **)desc->Stmts.buffer) + i;
      if (Stmt == *refStmt)
      {
        MADB_DeleteDynamicElement(&desc->Stmts, i);

        if (!all)
        {
          return;
        }
      }
    }
  }
}
/* }}} */

/* {{{ ResetMetadata */
void ResetMetadata(MYSQL_RES** metadata, MYSQL_RES* new_metadata)
{
  if (*metadata != NULL)
  {
    mysql_free_result(*metadata);
  }

  *metadata= new_metadata;
}
/* }}} */

/* {{{ MADB_StmtFree */
SQLRETURN MADB_StmtFree(MADB_Stmt *Stmt, SQLUSMALLINT Option)
{
  if (!Stmt)
    return SQL_INVALID_HANDLE;

  switch (Option) {
  case SQL_CLOSE:
    if (Stmt->stmt)
    {
      if (Stmt->Ird)
        MADB_DescFree(Stmt->Ird, TRUE);
      if (Stmt->State > MADB_SS_PREPARED && !QUERY_IS_MULTISTMT(Stmt->Query))
      {
        MDBUG_C_PRINT(Stmt->Connection, "mysql_stmt_free_result(%0x)", Stmt->stmt);
        mysql_stmt_free_result(Stmt->stmt);
        LOCK_MARIADB(Stmt->Connection);
        MDBUG_C_PRINT(Stmt->Connection, "-->resetting %0x", Stmt->stmt);
        mysql_stmt_reset(Stmt->stmt);
        UNLOCK_MARIADB(Stmt->Connection);
      }
      if (QUERY_IS_MULTISTMT(Stmt->Query) && Stmt->MultiStmts)
      {
        unsigned int i;
        LOCK_MARIADB(Stmt->Connection);
        for (i=0; i < STMT_COUNT(Stmt->Query); ++i)
        {
          if (Stmt->MultiStmts[i] != NULL)
          {
            MDBUG_C_PRINT(Stmt->Connection, "-->resetting %0x(%u)", Stmt->MultiStmts[i], i);
            mysql_stmt_reset(Stmt->MultiStmts[i]);
          }
        }
        UNLOCK_MARIADB(Stmt->Connection);
      }

      ResetMetadata(&Stmt->metadata, NULL);
     
      MADB_FREE(Stmt->result);
      MADB_FREE(Stmt->CharOffset);
      MADB_FREE(Stmt->Lengths);

      RESET_STMT_STATE(Stmt);
      RESET_DAE_STATUS(Stmt);
    }
    break;
  case SQL_UNBIND:
    MADB_FREE(Stmt->result);
    MADB_DescFree(Stmt->Ard, TRUE);
    break;
  case SQL_RESET_PARAMS:
    MADB_FREE(Stmt->params);
    MADB_DescFree(Stmt->Apd, TRUE);
    RESET_DAE_STATUS(Stmt);
    break;
  case SQL_DROP:
    MADB_FREE(Stmt->params);
    MADB_FREE(Stmt->result);
    MADB_FREE(Stmt->Cursor.Name);
    MADB_FREE(Stmt->CatalogName);
    MADB_FREE(Stmt->TableName);
    ResetMetadata(&Stmt->metadata, NULL);

    /* For explicit descriptors we only remove reference to the stmt*/
    if (Stmt->Apd->AppType)
    {
      EnterCriticalSection(&Stmt->Connection->ListsCs);
      RemoveStmtRefFromDesc(Stmt->Apd, Stmt, TRUE);
      LeaveCriticalSection(&Stmt->Connection->ListsCs);
      MADB_DescFree(Stmt->IApd, FALSE);
    }
    else
    {
      MADB_DescFree( Stmt->Apd, FALSE);
    }
    if (Stmt->Ard->AppType)
    {
      EnterCriticalSection(&Stmt->Connection->ListsCs);
      RemoveStmtRefFromDesc(Stmt->Ard, Stmt, TRUE);
      LeaveCriticalSection(&Stmt->Connection->ListsCs);
      MADB_DescFree(Stmt->IArd, FALSE);
    }
    else
    {
      MADB_DescFree(Stmt->Ard, FALSE);
    }
    MADB_DescFree(Stmt->Ipd, FALSE);
    MADB_DescFree(Stmt->Ird, FALSE);

    MADB_FREE(Stmt->CharOffset);
    MADB_FREE(Stmt->Lengths);
    ResetMetadata(&Stmt->DefaultsResult, NULL);

    if (Stmt->DaeStmt != NULL)
    {
      Stmt->DaeStmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
      Stmt->DaeStmt= NULL;
    }
    EnterCriticalSection(&Stmt->Connection->cs);
    /* TODO: if multistatement was prepared, but not executed, we would get here Stmt->stmt leaked. Unlikely that is very probable scenario,
             thus leaving this for new version */
    if (QUERY_IS_MULTISTMT(Stmt->Query) && Stmt->MultiStmts)
    {
      unsigned int i;
      for (i= 0; i < STMT_COUNT(Stmt->Query); ++i)
      {
        /* This dirty hack allows to avoid crash in case stmt object was not allocated
           TODO: The better place for this check would be where MultiStmts was not allocated
           to avoid inconsistency(MultiStmtCount > 0 and MultiStmts is NULL */
        if (Stmt->MultiStmts!= NULL && Stmt->MultiStmts[i] != NULL)
        {
          MDBUG_C_PRINT(Stmt->Connection, "-->closing %0x(%u)", Stmt->MultiStmts[i], i);
          mysql_stmt_close(Stmt->MultiStmts[i]);
        }
      }
      MADB_FREE(Stmt->MultiStmts);
      Stmt->MultiStmtNr= 0;
    }
    else if (Stmt->stmt != NULL)
    {
      MDBUG_C_PRINT(Stmt->Connection, "-->closing %0x", Stmt->stmt);
      MADB_STMT_CLOSE_STMT(Stmt);
    }
    /* Query has to be deleted after multistmt handles are closed, since the depends on info in the Query */
    MADB_DeleteQuery(&Stmt->Query);
    LeaveCriticalSection(&Stmt->Connection->cs);
    EnterCriticalSection(&Stmt->Connection->ListsCs);
    Stmt->Connection->Stmts= MADB_ListDelete(Stmt->Connection->Stmts, &Stmt->ListItem);
    LeaveCriticalSection(&Stmt->Connection->ListsCs);
    
    MADB_FREE(Stmt);
  } /* End of switch (Option) */
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_CheckIfExecDirectPossible
       Checking if we can deploy mariadb_stmt_execute_direct */
BOOL MADB_CheckIfExecDirectPossible(MADB_Stmt *Stmt)
{
  return MADB_ServerSupports(Stmt->Connection, MADB_CAPABLE_EXEC_DIRECT)
      && !(Stmt->Apd->Header.ArraySize > 1)                              /* With array of parameters exec_direct will be not optimal */
      && MADB_FindNextDaeParam(Stmt->Apd, -1, 1) == MADB_NOPARAM;
}
/* }}} */

/* {{{ MADB_BulkInsertPossible
       Checking if we can deploy mariadb_stmt_execute_direct */
BOOL MADB_BulkInsertPossible(MADB_Stmt *Stmt)
{
  return MADB_ServerSupports(Stmt->Connection, MADB_CAPABLE_PARAM_ARRAYS)
      && (Stmt->Apd->Header.ArraySize > 1)
      && (Stmt->Apd->Header.BindType == SQL_PARAM_BIND_BY_COLUMN)        /* First we support column-wise binding */
      && (Stmt->Query.QueryType == MADB_QUERY_INSERT || Stmt->Query.QueryType == MADB_QUERY_UPDATE)
      && MADB_FindNextDaeParam(Stmt->Apd, -1, 1) == MADB_NOPARAM;        /* TODO: should be not very hard ot optimize to use bulk in this
                                                                         case for chunks of the array, delimitered by param rows with DAE
                                                                         In particular, MADB_FindNextDaeParam should consider Stmt->ArrayOffset */
}
/* }}} */
/* {{{ MADB_StmtExecDirect */
SQLRETURN MADB_StmtExecDirect(MADB_Stmt *Stmt, char *StatementText, SQLINTEGER TextLength)
{
  SQLRETURN ret;
  BOOL      ExecDirect= TRUE;

  ret= Stmt->Methods->Prepare(Stmt, StatementText, TextLength, ExecDirect);
  /* In case statement is not supported, we use mysql_query instead */
  if (!SQL_SUCCEEDED(ret))
  {
    /* This is not quite good - 1064 may simply mean that syntax is wrong. we are screwed then */
    if ((Stmt->Error.NativeError == 1295/*ER_UNSUPPORTED_PS*/ ||
         Stmt->Error.NativeError == 1064/*ER_PARSE_ERROR*/))
    {
      Stmt->State= MADB_SS_EMULATED;
    }
    else
    {
      return ret;
    }
  }

  /* For multistmt we don't use mariadb_stmt_execute_direct so far */
  if (QUERY_IS_MULTISTMT(Stmt->Query))
  {
    ExecDirect= FALSE;
  }

  return Stmt->Methods->Execute(Stmt, ExecDirect);
}
/* }}} */

/* {{{ MADB_FindCursor */
MADB_Stmt *MADB_FindCursor(MADB_Stmt *Stmt, const char *CursorName)
{
  MADB_Dbc *Dbc= Stmt->Connection;
  MADB_List *LStmt, *LStmtNext;

  for (LStmt= Dbc->Stmts; LStmt; LStmt= LStmtNext)
  {
    MADB_Cursor *Cursor= &((MADB_Stmt *)LStmt->data)->Cursor;
    LStmtNext= LStmt->next;

    if (Stmt != (MADB_Stmt *)LStmt->data &&
        Cursor->Name && _stricmp(Cursor->Name, CursorName) == 0)
    {
      return (MADB_Stmt *)LStmt->data;
    }
  }
  MADB_SetError(&Stmt->Error, MADB_ERR_34000, NULL, 0);
  return NULL;
}
/* }}} */

/* {{{ FetchMetadata */
MYSQL_RES* FetchMetadata(MADB_Stmt *Stmt)
{
  ResetMetadata(&Stmt->metadata, mysql_stmt_result_metadata(Stmt->stmt));
  return Stmt->metadata;
}
/* }}} */

/* {{{ MADB_StmtReset - reseting Stmt handler for new use. Has to be called inside a lock */
void MADB_StmtReset(MADB_Stmt *Stmt)
{
  if (!QUERY_IS_MULTISTMT(Stmt->Query) || Stmt->MultiStmts == NULL)
  {
    if (Stmt->State > MADB_SS_PREPARED)
    {
      MDBUG_C_PRINT(Stmt->Connection, "mysql_stmt_free_result(%0x)", Stmt->stmt);
      mysql_stmt_free_result(Stmt->stmt);
    }

    if (Stmt->State >= MADB_SS_PREPARED)
    {
      MDBUG_C_PRINT(Stmt->Connection, "-->closing %0x", Stmt->stmt);
      MADB_STMT_CLOSE_STMT(Stmt);
      Stmt->stmt= MADB_NewStmtHandle(Stmt);

      MDBUG_C_PRINT(Stmt->Connection, "-->inited %0x", Stmt->stmt);
    }
  }
  else
  {
    CloseMultiStatements(Stmt);
    Stmt->stmt= MADB_NewStmtHandle(Stmt);

    MDBUG_C_PRINT(Stmt->Connection, "-->inited %0x", Stmt->stmt);
  }

  switch (Stmt->State)
  {
  case MADB_SS_EXECUTED:
  case MADB_SS_OUTPARAMSFETCHED:

    MADB_FREE(Stmt->result);
    MADB_FREE(Stmt->CharOffset);
    MADB_FREE(Stmt->Lengths);
    RESET_DAE_STATUS(Stmt);

  case MADB_SS_PREPARED:
    ResetMetadata(&Stmt->metadata, NULL);

    Stmt->PositionedCursor= NULL;
    Stmt->Ird->Header.Count= 0;

  case MADB_SS_EMULATED:
  /* We can have the case, then query did not succeed, and in case of direct execution we wouldn't
     have ane state set, but some of stuff still needs to be cleaned. Perhaps we could introduce a state
     for such case, smth like DIREXEC_PREPARED. Would be more proper, but yet overkill */

    if (QUERY_IS_MULTISTMT(Stmt->Query))
    {
      while (mysql_more_results(Stmt->Connection->mariadb))
      {
        mysql_next_result(Stmt->Connection->mariadb);
      }
    }
  default:
    Stmt->PositionedCommand= 0;
    Stmt->State= MADB_SS_INITED;
    MADB_CLEAR_ERROR(&Stmt->Error);
  }
}
/* }}} */

/* {{{ MADB_EDPrepare - Method called from SQLPrepare in case it is SQLExecDirect and if server >= 10.2
      (i.e. we gonna do mariadb_stmt_exec_direct) */
SQLRETURN MADB_EDPrepare(MADB_Stmt *Stmt)
{
  /* TODO: In case of positioned command it shouldn't be always*/
  if ((Stmt->ParamCount= Stmt->Apd->Header.Count + (MADB_POSITIONED_COMMAND(Stmt) ? MADB_POS_COMM_IDX_FIELD_COUNT(Stmt) : 0)) != 0)
  {
    if (Stmt->params)
    {
      MADB_FREE(Stmt->params);
    }
    /* If we have "WHERE CURRENT OF", we will need bind additionaly parameters for each field in the index */
    Stmt->params= (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * Stmt->ParamCount);
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_RegularPrepare - Method called from SQLPrepare in case it is SQLExecDirect and if !(server > 10.2)
(i.e. we aren't going to do mariadb_stmt_exec_direct) */
SQLRETURN MADB_RegularPrepare(MADB_Stmt *Stmt)
{
  LOCK_MARIADB(Stmt->Connection);

  MDBUG_C_PRINT(Stmt->Connection, "mysql_stmt_prepare(%0x,%s)", Stmt->stmt, STMT_STRING(Stmt));
  if (mysql_stmt_prepare(Stmt->stmt, STMT_STRING(Stmt), (unsigned long)strlen(STMT_STRING(Stmt))))
  {
    /* Need to save error first */
    MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
    /* We need to close the stmt here, or it becomes unusable like in ODBC-21 */
    MDBUG_C_PRINT(Stmt->Connection, "mysql_stmt_close(%0x)", Stmt->stmt);
    MADB_STMT_CLOSE_STMT(Stmt);
    Stmt->stmt= MADB_NewStmtHandle(Stmt);

    UNLOCK_MARIADB(Stmt->Connection);

    MDBUG_C_PRINT(Stmt->Connection, "mysql_stmt_init(%0x)->%0x", Stmt->Connection->mariadb, Stmt->stmt);

    return Stmt->Error.ReturnValue;
  }
  UNLOCK_MARIADB(Stmt->Connection);

  Stmt->State= MADB_SS_PREPARED;

  /* If we have result returning query - fill descriptor records with metadata */
  if (mysql_stmt_field_count(Stmt->stmt) > 0)
  {
    MADB_DescSetIrdMetadata(Stmt, mysql_fetch_fields(FetchMetadata(Stmt)), mysql_stmt_field_count(Stmt->stmt));
  }

  if ((Stmt->ParamCount= (SQLSMALLINT)mysql_stmt_param_count(Stmt->stmt)))
  {
    if (Stmt->params)
    {
      MADB_FREE(Stmt->params);
    }
    Stmt->params= (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * Stmt->ParamCount);
  }

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_StmtPrepare */
SQLRETURN MADB_StmtPrepare(MADB_Stmt *Stmt, char *StatementText, SQLINTEGER TextLength, BOOL ExecDirect)
{
  char          *CursorName= NULL;
  unsigned int  WhereOffset;
  BOOL          HasParameters= 0;

  MDBUG_C_PRINT(Stmt->Connection, "%sMADB_StmtPrepare", "\t->");

  LOCK_MARIADB(Stmt->Connection);

  MADB_StmtReset(Stmt);

  /* After this point we can't have SQL_NTS*/
  ADJUST_LENGTH(StatementText, TextLength);

  /* There is no need to send anything to the server to find out there is syntax error here */
  if (TextLength < MADB_MIN_QUERY_LEN)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_42000, NULL, 0);
  }
  MADB_ResetParser(Stmt, StatementText, TextLength);
  MADB_ParseQuery(&Stmt->Query);

  if ((Stmt->Query.QueryType == MADB_QUERY_INSERT || Stmt->Query.QueryType == MADB_QUERY_UPDATE || Stmt->Query.QueryType == MADB_QUERY_DELETE)
    && MADB_FindToken(&Stmt->Query, "RETURNING"))
  {
    Stmt->Query.ReturnsResult= '\1';
  }

  /* if we have multiple statements we save single statements in Stmt->StrMultiStmt
     and store the number in Stmt->MultiStmts */
  if (QueryIsPossiblyMultistmt(&Stmt->Query) && QUERY_IS_MULTISTMT(Stmt->Query) &&
    (Stmt->Query.ReturnsResult || Stmt->Query.HasParameters) && Stmt->Query.BatchAllowed) /* If neither of statements returns result,
                                                                          and does not have parameters, we will run them
                                                                          using text protocol */
  {
    if (ExecDirect != FALSE)
    {
      return MADB_EDPrepare(Stmt);
    }
    /* We had error preparing any of statements */
    else if (GetMultiStatements(Stmt, ExecDirect))
    {
      return Stmt->Error.ReturnValue;
    }

    /* all statemtens successfully prepared */
    UNLOCK_MARIADB(Stmt->Connection);
    return SQL_SUCCESS;
  }

  UNLOCK_MARIADB(Stmt->Connection);

  if (!MADB_ValidateStmt(&Stmt->Query))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY000, "SQL command SET NAMES is not allowed", 0);
    return Stmt->Error.ReturnValue;
  }

  /* Transform WHERE CURRENT OF [cursorname]:
     Append WHERE with Parameter Markers
     In StmtExecute we will call SQLSetPos with update or delete:
     */

  if ((CursorName = MADB_ParseCursorName(&Stmt->Query, &WhereOffset)))
  {
    MADB_DynString StmtStr;
    char *TableName;

    /* Make sure we have a delete or update statement
       MADB_QUERY_DELETE and MADB_QUERY_UPDATE defined in the enum to have the same value
       as SQL_UPDATE and SQL_DELETE, respectively */
    if (Stmt->Query.QueryType == MADB_QUERY_DELETE || Stmt->Query.QueryType == MADB_QUERY_UPDATE)
    {
      Stmt->PositionedCommand= 1;
    }
    else
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_42000, "Invalid SQL Syntax: DELETE or UPDATE expected for positioned update", 0);
      return Stmt->Error.ReturnValue;
    }

    if (!(Stmt->PositionedCursor= MADB_FindCursor(Stmt, CursorName)))
      return Stmt->Error.ReturnValue;

    TableName= MADB_GetTableName(Stmt->PositionedCursor);
    MADB_InitDynamicString(&StmtStr, "", 8192, 1024);
    MADB_DynstrAppendMem(&StmtStr, Stmt->Query.RefinedText, WhereOffset);
    MADB_DynStrGetWhere(Stmt->PositionedCursor, &StmtStr, TableName, TRUE);
    
    MADB_RESET(STMT_STRING(Stmt), StmtStr.str);
    /* Constructed query we've copied for execution has parameters */
    Stmt->Query.HasParameters= 1;
    MADB_DynstrFree(&StmtStr);
  }

  if (Stmt->Options.MaxRows)
  {
    /* TODO: LIMIT is not always the last clause. And not applicable to each query type.
       Thus we need to check query type and last tokens, and possibly put limit before them */
    char *p;
    STMT_STRING(Stmt)= realloc((char *)STMT_STRING(Stmt), strlen(STMT_STRING(Stmt)) + 40);
    p= STMT_STRING(Stmt) + strlen(STMT_STRING(Stmt));
    _snprintf(p, 40, " LIMIT %zd", Stmt->Options.MaxRows);
  }

  if (!Stmt->Query.ReturnsResult && !Stmt->Query.HasParameters &&
    /* If have multistatement query, and this is not allowed, we want to do normal prepare.
       To give it last chance. And to return correct error otherwise */
    ! (QUERY_IS_MULTISTMT(Stmt->Query) && !Stmt->Query.BatchAllowed))
  {
    Stmt->State= MADB_SS_EMULATED;
    return SQL_SUCCESS;
  }

  if (ExecDirect && MADB_CheckIfExecDirectPossible(Stmt))
  {
    return MADB_EDPrepare(Stmt);
  }
  else
  {
    return MADB_RegularPrepare(Stmt);
  }
}
/* }}} */

/* {{{ MADB_StmtParamData */ 
SQLRETURN MADB_StmtParamData(MADB_Stmt *Stmt, SQLPOINTER *ValuePtrPtr)
{
  MADB_Desc *Desc;
  MADB_DescRecord *Record;
  int ParamCount;
  int i;
  SQLRETURN ret;

  if (Stmt->DataExecutionType == MADB_DAE_NORMAL)
  {
    if (!Stmt->Apd || !(ParamCount= Stmt->ParamCount))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY010, NULL, 0);
      return Stmt->Error.ReturnValue;
    }
    Desc= Stmt->Apd;
  }
  else
  {
    if (!Stmt->Ard || !(ParamCount= Stmt->DaeStmt->ParamCount))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY010, NULL, 0);
      return Stmt->Error.ReturnValue;
    }
    Desc= Stmt->DaeStmt->Apd;
  }

  /* If we have last DAE param(Stmt->PutParam), we are starting from the next one. Otherwise from first */
  for (i= Stmt->PutParam > -1 ? Stmt->PutParam + 1 : 0; i < ParamCount; i++)
  {
    if ((Record= MADB_DescGetInternalRecord(Desc, i, MADB_DESC_READ)))
    {
      if (Record->OctetLengthPtr)
      {
        /* Stmt->DaeRowNumber is 1 based */
        SQLLEN *OctetLength = (SQLLEN *)GetBindOffset(Desc, Record, Record->OctetLengthPtr, Stmt->DaeRowNumber > 1 ? Stmt->DaeRowNumber - 1 : 0, sizeof(SQLLEN));
        if (PARAM_IS_DAE(OctetLength))
        {
          Stmt->PutDataRec= Record;
          *ValuePtrPtr = GetBindOffset(Desc, Record, Record->DataPtr, Stmt->DaeRowNumber > 1 ? Stmt->DaeRowNumber - 1 : 0, Record->OctetLength);
          Stmt->PutParam= i;
          Stmt->Status= SQL_NEED_DATA;

          return SQL_NEED_DATA;
        }
      }
    }
  }

  /* reset status, otherwise SQLSetPos and SQLExecute will fail */
  MARK_DAE_DONE(Stmt);
  if (Stmt->DataExecutionType == MADB_DAE_ADD || Stmt->DataExecutionType == MADB_DAE_UPDATE)
  {
    MARK_DAE_DONE(Stmt->DaeStmt);
  }

  switch (Stmt->DataExecutionType) {
  case MADB_DAE_NORMAL:
    ret= Stmt->Methods->Execute(Stmt, FALSE);
    RESET_DAE_STATUS(Stmt);
    break;
  case MADB_DAE_UPDATE:
    ret= Stmt->Methods->SetPos(Stmt, Stmt->DaeRowNumber, SQL_UPDATE, SQL_LOCK_NO_CHANGE, 1);
    RESET_DAE_STATUS(Stmt);
    break;
  case MADB_DAE_ADD:
    ret= Stmt->DaeStmt->Methods->Execute(Stmt->DaeStmt, FALSE);
    MADB_CopyError(&Stmt->Error, &Stmt->DaeStmt->Error);
    RESET_DAE_STATUS(Stmt->DaeStmt);
    break;
  default:
    ret= SQL_ERROR;
  }
  /* Interesting should we reset if execution failed? */

  return ret;
}
/* }}} */

/* {{{ MADB_StmtPutData */
SQLRETURN MADB_StmtPutData(MADB_Stmt *Stmt, SQLPOINTER DataPtr, SQLLEN StrLen_or_Ind)
{
  MADB_DescRecord *Record;
  MADB_Stmt       *MyStmt= Stmt;
  SQLPOINTER      ConvertedDataPtr= NULL;
  SQLULEN         Length= 0;

  MADB_CLEAR_ERROR(&Stmt->Error);

  if (DataPtr != NULL && StrLen_or_Ind < 0 && StrLen_or_Ind != SQL_NTS && StrLen_or_Ind != SQL_NULL_DATA)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY090, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  if (Stmt->DataExecutionType != MADB_DAE_NORMAL)
  {
    MyStmt= Stmt->DaeStmt;
  }
  Record= MADB_DescGetInternalRecord(MyStmt->Apd, Stmt->PutParam, MADB_DESC_READ);
  assert(Record);

  if (StrLen_or_Ind == SQL_NULL_DATA)
  {
    /* Check if we've already sent any data */
    if (MyStmt->stmt->params[Stmt->PutParam].long_data_used)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY011, NULL, 0);
      return Stmt->Error.ReturnValue;
    }
    Record->Type= SQL_TYPE_NULL;
    return SQL_SUCCESS;
  }

  /* This normally should be enforced by DM */
  if (DataPtr == NULL && StrLen_or_Ind != 0)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, NULL, 0);
    return Stmt->Error.ReturnValue;
  }
/*
  if (StrLen_or_Ind == SQL_NTS)
  {
    if (Record->ConciseType == SQL_C_WCHAR)
      StrLen_or_Ind= wcslen((SQLWCHAR *)DataPtr);
    else
      StrLen_or_Ind= strlen((char *)DataPtr);
  }
 */
  if (Record->ConciseType == SQL_C_WCHAR)
  {
    /* Conn cs */
    ConvertedDataPtr= MADB_ConvertFromWChar((SQLWCHAR *)DataPtr, (SQLINTEGER)(StrLen_or_Ind/sizeof(SQLWCHAR)), &Length, &Stmt->Connection->Charset, NULL);

    if ((ConvertedDataPtr == NULL || Length == 0) && StrLen_or_Ind > 0)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      return Stmt->Error.ReturnValue;
    }
  }
  else
  {
    if (StrLen_or_Ind == SQL_NTS)
    {
      Length= strlen((char *)DataPtr);
    }
    else
    {
      Length= StrLen_or_Ind;
    }
  }

  /* To make sure that we will not consume the doble amount of memory, we need to send
     data via mysql_send_long_data directly to the server instead of allocating a separate
     buffer. This means we need to process Update and Insert statements row by row. */
  if (mysql_stmt_send_long_data(MyStmt->stmt, Stmt->PutParam, (ConvertedDataPtr ? (char *)ConvertedDataPtr : DataPtr), (unsigned long)Length))
  {
    MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, MyStmt->stmt);
  }
  else
  {
    Record->InternalLength+= (unsigned long)Length;
  }

  MADB_FREE(ConvertedDataPtr);
  return Stmt->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_ExecutePositionedUpdate */
SQLRETURN MADB_ExecutePositionedUpdate(MADB_Stmt *Stmt, BOOL ExecDirect)
{
  SQLSMALLINT   j;
  SQLRETURN     ret;
  MADB_DynArray DynData;
  MADB_Stmt     *SaveCursor;

  char *p;

  MADB_CLEAR_ERROR(&Stmt->Error);
  if (!Stmt->PositionedCursor->result)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_34000, "Cursor has no result set or is not open", 0);
    return Stmt->Error.ReturnValue;
  }
  MADB_StmtDataSeek(Stmt->PositionedCursor, Stmt->PositionedCursor->Cursor.Position);
  Stmt->Methods->RefreshRowPtrs(Stmt->PositionedCursor);

  memcpy(&Stmt->Apd->Header, &Stmt->Ard->Header, sizeof(MADB_Header));
  
  Stmt->AffectedRows= 0;
  
  MADB_InitDynamicArray(&DynData, sizeof(char *), 8, 8);

  for (j= 1; j < MADB_POS_COMM_IDX_FIELD_COUNT(Stmt) + 1; ++j)
  {
    SQLLEN Length;
    MADB_DescRecord *Rec= MADB_DescGetInternalRecord(Stmt->PositionedCursor->Ard, j, MADB_DESC_READ);
    Length= Rec->OctetLength;
 /*   if (Rec->inUse)
      MA_SQLBindParameter(Stmt, j+1, SQL_PARAM_INPUT, Rec->ConciseType, Rec->Type, Rec->DisplaySize, Rec->Scale, Rec->DataPtr, Length, Rec->OctetLengthPtr);
    else */
    {
      Stmt->Methods->GetData(Stmt->PositionedCursor, j, SQL_CHAR, NULL, 0, &Length, TRUE);
      p= (char *)MADB_CALLOC(Length + 2);
      MADB_InsertDynamic(&DynData, (char *)&p);
      Stmt->Methods->GetData(Stmt->PositionedCursor, j, SQL_CHAR, p, Length + 1, NULL, TRUE);
      Stmt->Methods->BindParam(Stmt, j + (Stmt->ParamCount - MADB_POS_COMM_IDX_FIELD_COUNT(Stmt)), SQL_PARAM_INPUT, SQL_CHAR, SQL_CHAR, 0, 0, p, Length, NULL);

    }
  }

  SaveCursor= Stmt->PositionedCursor;
  Stmt->PositionedCursor= NULL;

  ret= Stmt->Methods->Execute(Stmt, ExecDirect);

  Stmt->PositionedCursor= SaveCursor;

  /* For the case of direct execution we need to restore number of parameters bound by application, for the case when application
     re-uses handle with same parameters for another query. Otherwise we won't know that number (of application's parameters) */
  if (ExecDirect)
  {
    Stmt->Apd->Header.Count-= MADB_POS_COMM_IDX_FIELD_COUNT(Stmt);
  }

  for (j=0; j < (int)DynData.elements; j++)
  {
    MADB_GetDynamic(&DynData, (char *)&p, j);
    MADB_FREE(p);
  }
  MADB_DeleteDynamic(&DynData);

  if (Stmt->PositionedCursor->Options.CursorType == SQL_CURSOR_DYNAMIC && 
     (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO))
  {
    SQLRETURN rc;
    rc= Stmt->Methods->RefreshDynamicCursor(Stmt->PositionedCursor);
    if (!SQL_SUCCEEDED(rc))
    {
      MADB_CopyError(&Stmt->Error, &Stmt->PositionedCursor->Error);
      return Stmt->Error.ReturnValue;
    }
    if (Stmt->Query.QueryType == SQL_DELETE)
    {
      MADB_STMT_RESET_CURSOR(Stmt->PositionedCursor);
    }
      
  }
  //MADB_FREE(DataPtr);
  return ret;
}
/* }}} */

/* {{{ MADB_GetOutParams */
SQLRETURN MADB_GetOutParams(MADB_Stmt *Stmt, int CurrentOffset)
{
  MYSQL_BIND *Bind;
  unsigned int i=0, ParameterNr= 0;

  /* Since Outparams are only one row, we use store result */
  if (mysql_stmt_store_result(Stmt->stmt))
  {
    return MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt);
  }

  Bind= (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * mysql_stmt_field_count(Stmt->stmt));
  
  for (i=0; i < (unsigned int)Stmt->ParamCount && ParameterNr < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    MADB_DescRecord *IpdRecord, *ApdRecord;
    if ((IpdRecord= MADB_DescGetInternalRecord(Stmt->Ipd, i, MADB_DESC_READ))!= NULL)
    {
      if (IpdRecord->ParameterType == SQL_PARAM_INPUT_OUTPUT ||
          IpdRecord->ParameterType == SQL_PARAM_OUTPUT)
      {
        ApdRecord= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ);
        Bind[ParameterNr].buffer= GetBindOffset(Stmt->Apd, ApdRecord, ApdRecord->DataPtr, CurrentOffset, ApdRecord->OctetLength);
        if (ApdRecord->OctetLengthPtr)
        {
          Bind[ParameterNr].length= (unsigned long *)GetBindOffset(Stmt->Apd, ApdRecord, ApdRecord->OctetLengthPtr,
                                                        CurrentOffset, sizeof(SQLLEN));
        }
        Bind[ParameterNr].buffer_length= (unsigned long)ApdRecord->OctetLength;
        Bind[ParameterNr].buffer_type= Stmt->stmt->params[i].buffer_type;
        ParameterNr++;
      }
    }
  }
  mysql_stmt_bind_result(Stmt->stmt, Bind);
  mysql_stmt_fetch(Stmt->stmt);
   
  mysql_stmt_data_seek(Stmt->stmt, 0);
  MADB_FREE(Bind);

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ ResetInternalLength */
static void ResetInternalLength(MADB_Stmt *Stmt, unsigned int ParamOffset)
{
  unsigned int i;
  MADB_DescRecord *ApdRecord;

  for (i= ParamOffset; i < ParamOffset + Stmt->ParamCount; ++i)
  {
    if ((ApdRecord= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ)))
    {
      ApdRecord->InternalLength= 0;
    }
  }
}
/* }}} */

/* {{{ MADB_DoExecute */
/* Actually executing on the server, doing required actions with C API, and processing execution result */
SQLRETURN MADB_DoExecute(MADB_Stmt *Stmt, BOOL ExecDirect)
{
  SQLRETURN ret= SQL_SUCCESS;

  /**************************** mysql_stmt_bind_param **********************************/
  if (ExecDirect)
  {
    mysql_stmt_attr_set(Stmt->stmt, STMT_ATTR_PREBIND_PARAMS, &Stmt->ParamCount);
  }

  mysql_stmt_attr_set(Stmt->stmt, STMT_ATTR_ARRAY_SIZE, (void*)&Stmt->Bulk.ArraySize);

  if (Stmt->ParamCount)
  {
    mysql_stmt_bind_param(Stmt->stmt, Stmt->params);
  }
  ret= SQL_SUCCESS;

  /**************************** mysql_stmt_execute *************************************/

  MDBUG_C_PRINT(Stmt->Connection, ExecDirect ? "mariadb_stmt_execute_direct(%0x,%s)"
    : "mariadb_stmt_execute(%0x)(%s)", Stmt->stmt, STMT_STRING(Stmt));

  if ((ExecDirect && mariadb_stmt_execute_direct(Stmt->stmt, STMT_STRING(Stmt), strlen(STMT_STRING(Stmt))))
    || (!ExecDirect && mysql_stmt_execute(Stmt->stmt)))
  {
    ret= MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
    MDBUG_C_PRINT(Stmt->Connection, "mysql_stmt_execute:ERROR%s", "");
  }
  else
  {
    unsigned int ServerStatus;

    Stmt->State= MADB_SS_EXECUTED;

    mariadb_get_infov(Stmt->Connection->mariadb, MARIADB_CONNECTION_SERVER_STATUS, (void*)&ServerStatus);
    if (ServerStatus & SERVER_PS_OUT_PARAMS)
    {
      Stmt->State= MADB_SS_OUTPARAMSFETCHED;
      ret= Stmt->Methods->GetOutParams(Stmt, 0);
    }
  }
  return ret;
}
/* }}} */

void MADB_SetStatusArray(MADB_Stmt *Stmt, SQLUSMALLINT Status)
{
  if (Stmt->Ipd->Header.ArrayStatusPtr != NULL)
  {
    memset(Stmt->Ipd->Header.ArrayStatusPtr, 0x00ff & Status, Stmt->Apd->Header.ArraySize*sizeof(SQLUSMALLINT));
    if (Stmt->Apd->Header.ArrayStatusPtr != NULL)
    {
      unsigned int i;
      for (i= 0; i < Stmt->Apd->Header.ArraySize; ++i)
      {
        if (Stmt->Apd->Header.ArrayStatusPtr[i] == SQL_PARAM_IGNORE)
        {
          Stmt->Ipd->Header.ArrayStatusPtr[i]= SQL_PARAM_UNUSED;
        }
      }
    }
  }
}
/* {{{ MADB_StmtExecute */
SQLRETURN MADB_StmtExecute(MADB_Stmt *Stmt, BOOL ExecDirect)
{
  unsigned int          i;
  MYSQL_RES   *DefaultResult= NULL;
  SQLRETURN    ret=           SQL_SUCCESS;
  unsigned int ErrorCount=    0;
  unsigned int StatementNr;
  unsigned int ParamOffset=   0; /* for multi statements */
               /* Will use it for STMT_ATTR_ARRAY_SIZE and as indicator if we are deploying MariaDB bulk insert feature */
  unsigned int MariadbArrSize= MADB_BulkInsertPossible(Stmt) != FALSE ? (unsigned int)Stmt->Apd->Header.ArraySize : 0;
  SQLULEN      j, Start=      0;
  /* For multistatement direct execution */
  char        *CurQuery= Stmt->Query.RefinedText, *QueriesEnd= Stmt->Query.RefinedText + Stmt->Query.RefinedLength;

  MDBUG_C_PRINT(Stmt->Connection, "%sMADB_StmtExecute", "\t->");

  MADB_CLEAR_ERROR(&Stmt->Error);

  if (Stmt->State == MADB_SS_EMULATED)
  {
    return MADB_ExecuteQuery(Stmt, STMT_STRING(Stmt), (SQLINTEGER)strlen(STMT_STRING(Stmt)));
  }

  if (MADB_POSITIONED_COMMAND(Stmt))
  {
    return MADB_ExecutePositionedUpdate(Stmt, ExecDirect);
  }

  /* Stmt->params was allocated during prepare, but could be cleared
     by SQLResetStmt. In latter case we need to allocate it again */
  if (!Stmt->params &&
    !(Stmt->params = (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * MADB_STMT_PARAM_COUNT(Stmt))))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  /* Normally this check is done by a DM. We are doing that too, keeping in mind direct linking.
     If exectution routine called from the SQLParamData, DataExecutionType has been reset */
  if (Stmt->Status == SQL_NEED_DATA && !DAE_DONE(Stmt))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY010, NULL, 0);
  }

  LOCK_MARIADB(Stmt->Connection);
  Stmt->AffectedRows= 0;
  Start+= Stmt->ArrayOffset;

  if (Stmt->Ipd->Header.RowsProcessedPtr)
  {
    *Stmt->Ipd->Header.RowsProcessedPtr= 0;
  }
 
  if (MariadbArrSize > 1)
  {
    if (MADB_DOING_BULK_OPER(Stmt))
    {
      //MADB_CleanBulkOperationData(Stmt);
    }
    Stmt->Bulk.ArraySize=  MariadbArrSize;
    Stmt->Bulk.HasRowsToSkip= 0;
  }

  for (StatementNr= 0; StatementNr < STMT_COUNT(Stmt->Query); ++StatementNr)
  {
    if (QUERY_IS_MULTISTMT(Stmt->Query))
    {
      if (Stmt->MultiStmts && Stmt->MultiStmts[StatementNr] != NULL)
      {
        Stmt->stmt= Stmt->MultiStmts[StatementNr];
      }
      else
      {
        /* We have direct execution, since otherwise it'd already prepared, and thus Stmt->MultiStmts would be set */
        if (CurQuery >= QueriesEnd)
        {
          /* Something went wrong(with parsing). But we've got here, and everything worked. Giving it chance to fail later.
             This shouldn't really happen */
          MDBUG_C_PRINT(Stmt->Connection, "Got past end of query direct-executing %s on stmt #%u", Stmt->Query.RefinedText, StatementNr);
          continue;
        }
        if (StatementNr > 0)
        {
          Stmt->stmt= MADB_NewStmtHandle(Stmt);
        }
        else
        {
          Stmt->MultiStmts= (MYSQL_STMT **)MADB_CALLOC(sizeof(MYSQL_STMT) * STMT_COUNT(Stmt->Query));
        }

        Stmt->MultiStmts[StatementNr]= Stmt->stmt;

        if (mysql_stmt_prepare(Stmt->stmt, CurQuery, (unsigned long)strlen(CurQuery)))
        {
          return MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
        }
        CurQuery+= strlen(CurQuery) + 1;
      }
      
      Stmt->RebindParams= TRUE;

      if (Stmt->ParamCount != mysql_stmt_param_count(Stmt->stmt))
      {
        Stmt->ParamCount= (SQLSMALLINT)mysql_stmt_param_count(Stmt->stmt);
        Stmt->params= (MYSQL_BIND*)MADB_REALLOC(Stmt->params, sizeof(MYSQL_BIND) * MADB_STMT_PARAM_COUNT(Stmt));
      }

      memset(Stmt->params, 0, sizeof(MYSQL_BIND) * MADB_STMT_PARAM_COUNT(Stmt));
    }

    if (MADB_DOING_BULK_OPER(Stmt))
    {
      if (!SQL_SUCCEEDED(MADB_ExecuteBulk(Stmt, ParamOffset)))
      {
        /* Doing just the same thing as we would do in general case */
        MADB_CleanBulkOperData(Stmt, ParamOffset);
        ErrorCount= (unsigned int)Stmt->Apd->Header.ArraySize;
        MADB_SetStatusArray(Stmt, SQL_PARAM_DIAG_UNAVAILABLE);
        goto end;
      }
      else if (!mysql_stmt_field_count(Stmt->stmt) && !Stmt->MultiStmts)
      {
        Stmt->AffectedRows+= mysql_stmt_affected_rows(Stmt->stmt);
      }
      /* Suboptimal, but more reliable and simple */
      MADB_CleanBulkOperData(Stmt, ParamOffset);
      Stmt->ArrayOffset+= (int)Stmt->Apd->Header.ArraySize;
      if (Stmt->Ipd->Header.RowsProcessedPtr)
      {
        *Stmt->Ipd->Header.RowsProcessedPtr= *Stmt->Ipd->Header.RowsProcessedPtr + Stmt->Apd->Header.ArraySize;
      }
      MADB_SetStatusArray(Stmt, SQL_PARAM_SUCCESS);
    }
    else
    {
      /* Convert and bind parameters */
      for (j= Start; j < Start + Stmt->Apd->Header.ArraySize; ++j)
      {
        /* "... In an IPD, this SQLUINTEGER * header field points to a buffer containing the number
           of sets of parameters that have been processed, including error sets. ..." */
        if (Stmt->Ipd->Header.RowsProcessedPtr)
        {
          *Stmt->Ipd->Header.RowsProcessedPtr= *Stmt->Ipd->Header.RowsProcessedPtr + 1;
        }

        if (Stmt->Apd->Header.ArrayStatusPtr &&
          Stmt->Apd->Header.ArrayStatusPtr[j-Start] == SQL_PARAM_IGNORE)
        {
          if (Stmt->Ipd->Header.ArrayStatusPtr)
          {
            Stmt->Ipd->Header.ArrayStatusPtr[j-Start]= SQL_PARAM_UNUSED;
          }
          continue;
        }

        for (i= ParamOffset; i < ParamOffset + MADB_STMT_PARAM_COUNT(Stmt); ++i)
        {
          MADB_DescRecord *ApdRecord, *IpdRecord;

          if ((ApdRecord= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ)) &&
            (IpdRecord= MADB_DescGetInternalRecord(Stmt->Ipd, i, MADB_DESC_READ)))
          {
            /* check if parameter was bound */
            if (!ApdRecord->inUse)
            {
              ret= MADB_SetError(&Stmt->Error, MADB_ERR_07002, NULL, 0);
              goto end;
            }

            if (MADB_ConversionSupported(ApdRecord, IpdRecord) == FALSE)
            {
              ret= MADB_SetError(&Stmt->Error, MADB_ERR_07006, NULL, 0);
              goto end;
            }

            Stmt->params[i-ParamOffset].length= NULL;

            ret= MADB_C2SQL(Stmt, ApdRecord, IpdRecord, j - Start, &Stmt->params[i-ParamOffset]);
            if (!SQL_SUCCEEDED(ret))
            {
              goto end;
            }
          }
        }                 /* End of for() on parameters */

        if (Stmt->RebindParams && MADB_STMT_PARAM_COUNT(Stmt))
        {
          Stmt->stmt->bind_param_done= 1;
          Stmt->RebindParams= FALSE;
        }

        ret= MADB_DoExecute(Stmt, ExecDirect && MADB_CheckIfExecDirectPossible(Stmt));

        if (!SQL_SUCCEEDED(ret))
        {
          ++ErrorCount;
        }
        /* We need to unset InternalLength, i.e. reset dae length counters for next stmt.
           However that length is not used anywhere, and is not clear what is it needed for */
        ResetInternalLength(Stmt, ParamOffset);

        if (Stmt->Ipd->Header.ArrayStatusPtr)
        {
          Stmt->Ipd->Header.ArrayStatusPtr[j-Start]= SQL_SUCCEEDED(ret) ? SQL_PARAM_SUCCESS :
            (j == Stmt->Apd->Header.ArraySize - 1) ? SQL_PARAM_ERROR : SQL_PARAM_DIAG_UNAVAILABLE;
        }
        if (!mysql_stmt_field_count(Stmt->stmt) && SQL_SUCCEEDED(ret) && !Stmt->MultiStmts)
        {
          Stmt->AffectedRows+= mysql_stmt_affected_rows(Stmt->stmt);
        }
        ++Stmt->ArrayOffset;
        if (!SQL_SUCCEEDED(ret) && j == Start + Stmt->Apd->Header.ArraySize)
        {
          goto end;
        }
      }     /* End of for() thru paramsets(parameters array) */
    }       /* End of if (bulk/not bulk) execution */

    if (QUERY_IS_MULTISTMT(Stmt->Query))
    {
      /* If we optimize memory allocation, then we will need to free bulk operation data here(among other places) */
      /* MADB_CleanBulkOperData(Stmt, ParamOffset); */
      ParamOffset+= MADB_STMT_PARAM_COUNT(Stmt);

      if (mysql_stmt_field_count(Stmt->stmt))
      {
        mysql_stmt_store_result(Stmt->stmt);
      }
    }
  }       /* End of for() on statements(Multistatmt) */
  
  /* All rows processed, so we can unset ArrayOffset */
  Stmt->ArrayOffset= 0;

  if (Stmt->MultiStmts)
  {
    Stmt->MultiStmtNr= 0;
    MADB_InstallStmt(Stmt, Stmt->MultiStmts[Stmt->MultiStmtNr]);
  }
  else if (mysql_stmt_field_count(Stmt->stmt) > 0)
  {
    MADB_StmtResetResultStructures(Stmt);

    /* Todo: for SQL_CURSOR_FORWARD_ONLY we should use cursor and prefetch rows */
    /*************************** mysql_stmt_store_result ******************************/
    /*If we did OUT params already, we should not store */
    if (Stmt->State == MADB_SS_EXECUTED && mysql_stmt_store_result(Stmt->stmt) != 0)
    {
      UNLOCK_MARIADB(Stmt->Connection);
      if (DefaultResult)
      {
        mysql_free_result(DefaultResult);
      }

      return MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
    }
    
    /* I don't think we can reliably establish the fact that we do not need to re-fetch the metadata, thus we are re-fetching always
       The fact that we have resultset has been established above in "if" condition(fields count is > 0) */
    MADB_DescSetIrdMetadata(Stmt, mysql_fetch_fields(FetchMetadata(Stmt)), mysql_stmt_field_count(Stmt->stmt));

    Stmt->AffectedRows= -1;
  }
end:
  UNLOCK_MARIADB(Stmt->Connection);
  Stmt->LastRowFetched= 0;

  if (DefaultResult)
    mysql_free_result(DefaultResult);

  if (ErrorCount)
  {
    if (ErrorCount < Stmt->Apd->Header.ArraySize)
      ret= SQL_SUCCESS_WITH_INFO;
    else
      ret= SQL_ERROR;
  }

  return ret;
}
/* }}} */

/* {{{ MADB_StmtBindCol */
SQLRETURN MADB_StmtBindCol(MADB_Stmt *Stmt, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
    SQLPOINTER TargetValuePtr, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
  MADB_Desc *Ard= Stmt->Ard;
  MADB_DescRecord *Record;

  if ((ColumnNumber < 1 && Stmt->Options.UseBookmarks == SQL_UB_OFF) || 
      (mysql_stmt_field_count(Stmt->stmt) &&
       Stmt->stmt->state > MYSQL_STMT_PREPARED && 
       ColumnNumber > mysql_stmt_field_count(Stmt->stmt)))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07009, NULL, 0);
    return SQL_ERROR;
  }

  /* Bookmark */
  if (ColumnNumber == 0)
  {
    if (TargetType == SQL_C_BOOKMARK || TargetType == SQL_C_VARBOOKMARK)
    {
      Stmt->Options.BookmarkPtr=     TargetValuePtr;
      Stmt->Options.BookmarkLength = BufferLength;
      Stmt->Options.BookmarkType=    TargetType;
      return SQL_SUCCESS;
    }
    MADB_SetError(&Stmt->Error, MADB_ERR_07006, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  if (!(Record= MADB_DescGetInternalRecord(Ard, ColumnNumber - 1, MADB_DESC_WRITE)))
  {
    MADB_CopyError(&Stmt->Error, &Ard->Error);
    return Stmt->Error.ReturnValue;
  }

  /* check if we need to unbind and delete a record */
  if (!TargetValuePtr && !StrLen_or_Ind)
  {
    int i;
    Record->inUse= 0;
    /* Update counter */
    for (i= Ard->Records.elements; i > 0; i--)
    {
      MADB_DescRecord *Rec= MADB_DescGetInternalRecord(Ard, i-1, MADB_DESC_READ);
      if (Rec && Rec->inUse)
      {
        Ard->Header.Count= i;
        return SQL_SUCCESS;
      }
    }
    Ard->Header.Count= 0;
    return SQL_SUCCESS;
  }

  if (!SQL_SUCCEEDED(MADB_DescSetField(Ard, ColumnNumber, SQL_DESC_TYPE, (SQLPOINTER)(SQLLEN)TargetType, SQL_IS_SMALLINT, 0)) ||
      !SQL_SUCCEEDED(MADB_DescSetField(Ard, ColumnNumber, SQL_DESC_OCTET_LENGTH_PTR, (SQLPOINTER)StrLen_or_Ind, SQL_IS_POINTER, 0)) ||
      !SQL_SUCCEEDED(MADB_DescSetField(Ard, ColumnNumber, SQL_DESC_INDICATOR_PTR, (SQLPOINTER)StrLen_or_Ind, SQL_IS_POINTER, 0)) ||
      !SQL_SUCCEEDED(MADB_DescSetField(Ard, ColumnNumber, SQL_DESC_OCTET_LENGTH, (SQLPOINTER)MADB_GetTypeLength(TargetType, BufferLength), SQL_IS_INTEGER, 0)) ||
      !SQL_SUCCEEDED(MADB_DescSetField(Ard, ColumnNumber, SQL_DESC_DATA_PTR, TargetValuePtr, SQL_IS_POINTER, 0)))
  {
    MADB_CopyError(&Stmt->Error, &Ard->Error);
    return Stmt->Error.ReturnValue;
  }
   
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_StmtBindParam */
SQLRETURN MADB_StmtBindParam(MADB_Stmt *Stmt,  SQLUSMALLINT ParameterNumber,
                             SQLSMALLINT InputOutputType, SQLSMALLINT ValueType,
                             SQLSMALLINT ParameterType, SQLULEN ColumnSize,
                             SQLSMALLINT DecimalDigits, SQLPOINTER ParameterValuePtr,
                             SQLLEN BufferLength, SQLLEN *StrLen_or_IndPtr)
{
   MADB_Desc *Apd= Stmt->Apd, 
             *Ipd= Stmt->Ipd;
   MADB_DescRecord *ApdRecord, *IpdRecord;
   SQLRETURN ret= SQL_SUCCESS;

   MADB_CLEAR_ERROR(&Stmt->Error);
   if (!(ApdRecord= MADB_DescGetInternalRecord(Apd, ParameterNumber - 1, MADB_DESC_WRITE)))
   {
     MADB_CopyError(&Stmt->Error, &Apd->Error);
     return Stmt->Error.ReturnValue;
   }
   if (!(IpdRecord= MADB_DescGetInternalRecord(Ipd, ParameterNumber - 1, MADB_DESC_WRITE)))
   {
     MADB_CopyError(&Stmt->Error, &Ipd->Error);
     return Stmt->Error.ReturnValue;
   }

   /* Map to the correspoinding type */
   if (ValueType == SQL_C_DEFAULT)
   {
     ValueType= MADB_GetDefaultType(ParameterType);
   }
   
   if (!(SQL_SUCCEEDED(MADB_DescSetField(Apd, ParameterNumber, SQL_DESC_CONCISE_TYPE, (SQLPOINTER)(SQLLEN)ValueType, SQL_IS_SMALLINT, 0))) ||
       !(SQL_SUCCEEDED(MADB_DescSetField(Apd, ParameterNumber, SQL_DESC_OCTET_LENGTH_PTR, (SQLPOINTER)StrLen_or_IndPtr, SQL_IS_POINTER, 0))) ||
       !(SQL_SUCCEEDED(MADB_DescSetField(Apd, ParameterNumber, SQL_DESC_OCTET_LENGTH, (SQLPOINTER)MADB_GetTypeLength(ValueType, BufferLength), SQL_IS_INTEGER, 0))) ||
       !(SQL_SUCCEEDED(MADB_DescSetField(Apd, ParameterNumber, SQL_DESC_INDICATOR_PTR, (SQLPOINTER)StrLen_or_IndPtr, SQL_IS_POINTER, 0))) ||
       !(SQL_SUCCEEDED(MADB_DescSetField(Apd, ParameterNumber, SQL_DESC_DATA_PTR, ParameterValuePtr, SQL_IS_POINTER, 0))))
   {
     MADB_CopyError(&Stmt->Error, &Apd->Error);
     return Stmt->Error.ReturnValue;
   }

   if (!(SQL_SUCCEEDED(MADB_DescSetField(Ipd, ParameterNumber, SQL_DESC_CONCISE_TYPE, (SQLPOINTER)(SQLLEN)ParameterType, SQL_IS_SMALLINT, 0))) ||
       !(SQL_SUCCEEDED(MADB_DescSetField(Ipd, ParameterNumber, SQL_DESC_PARAMETER_TYPE, (SQLPOINTER)(SQLLEN)InputOutputType, SQL_IS_SMALLINT, 0))))
   {
     MADB_CopyError(&Stmt->Error, &Ipd->Error);
     return Stmt->Error.ReturnValue;
   }

   switch(ParameterType) {
   case SQL_BINARY:
   case SQL_VARBINARY:
   case SQL_LONGVARBINARY:
   case SQL_CHAR:
   case SQL_VARCHAR:
   case SQL_LONGVARCHAR:
   case SQL_WCHAR:
   case SQL_WLONGVARCHAR:
   case SQL_WVARCHAR:
     ret= MADB_DescSetField(Ipd, ParameterNumber, SQL_DESC_LENGTH, (SQLPOINTER)ColumnSize, SQL_IS_INTEGER, 0);
     break;
   case SQL_FLOAT:
   case SQL_REAL:
   case SQL_DOUBLE:
     ret= MADB_DescSetField(Ipd, ParameterNumber, SQL_DESC_PRECISION, (SQLPOINTER)ColumnSize, SQL_IS_INTEGER, 0);
     break;
   case SQL_DECIMAL:
   case SQL_NUMERIC:
     ret= MADB_DescSetField(Ipd, ParameterNumber, SQL_DESC_PRECISION, (SQLPOINTER)ColumnSize, SQL_IS_SMALLINT, 0);
     if (SQL_SUCCEEDED(ret))
       ret= MADB_DescSetField(Ipd, ParameterNumber, SQL_DESC_SCALE, (SQLPOINTER)(SQLLEN)DecimalDigits, SQL_IS_SMALLINT, 0);
     break;
   case SQL_INTERVAL_MINUTE_TO_SECOND:
   case SQL_INTERVAL_HOUR_TO_SECOND:
   case SQL_INTERVAL_DAY_TO_SECOND:
   case SQL_INTERVAL_SECOND:
   case SQL_TYPE_TIMESTAMP:
   case SQL_TYPE_TIME:
     ret= MADB_DescSetField(Ipd, ParameterNumber, SQL_DESC_PRECISION, (SQLPOINTER)(SQLLEN)DecimalDigits, SQL_IS_SMALLINT, 0);
     break;
   }

   if(!SQL_SUCCEEDED(ret))
     MADB_CopyError(&Stmt->Error, &Ipd->Error);
   Stmt->RebindParams= TRUE;
   
   return ret;
 }
 /* }}} */

void MADB_InitStatusPtr(SQLUSMALLINT *Ptr, SQLULEN Size, SQLSMALLINT InitialValue)
{
  SQLULEN i;

  for (i=0; i < Size; i++)
    Ptr[i]= InitialValue;
}

/* Not used for now, but leaving it so far here - it may be useful */
/* BOOL MADB_NumericBufferType(SQLSMALLINT BufferType)
{
  switch (BufferType)
  {
  case SQL_C_TINYINT:
  case SQL_C_UTINYINT:
  case SQL_C_STINYINT:
  case SQL_C_SHORT:
  case SQL_C_SSHORT:
  case SQL_C_USHORT:
  case SQL_C_FLOAT:
  case SQL_C_LONG:
  case SQL_C_ULONG:
  case SQL_C_SLONG:
  case SQL_C_DOUBLE:
    return TRUE;
  default:
    return FALSE;
  }
}*/

/* {{{ MADB_BinaryFieldType */
BOOL MADB_BinaryFieldType(SQLSMALLINT FieldType)
{
  return FieldType == SQL_BINARY || FieldType == SQL_BIT;
}
/* }}} */

/* {{{ MADB_PrepareBind
       Filling bind structures in */
SQLRETURN MADB_PrepareBind(MADB_Stmt *Stmt, int RowNumber)
{
  MADB_DescRecord *IrdRec, *ArdRec;
  int             i;
  void            *DataPtr= NULL;

  for (i= 0; i < MADB_STMT_COLUMN_COUNT(Stmt); ++i)
  {
    ArdRec= MADB_DescGetInternalRecord(Stmt->Ard, i, MADB_DESC_READ);
    if (ArdRec == NULL || !ArdRec->inUse)
    {      
      Stmt->result[i].flags|= MADB_BIND_DUMMY;
      continue;
    }

    DataPtr= (SQLLEN *)GetBindOffset(Stmt->Ard, ArdRec, ArdRec->DataPtr, RowNumber, ArdRec->OctetLength);

    MADB_FREE(ArdRec->InternalBuffer);
    if (!DataPtr)
    {
      Stmt->result[i].flags|= MADB_BIND_DUMMY;
      continue;
    }
    else
    {
      Stmt->result[i].flags&= ~MADB_BIND_DUMMY;
    }

    IrdRec= MADB_DescGetInternalRecord(Stmt->Ird, i, MADB_DESC_READ);
    /* assert(IrdRec != NULL) */

    /* We can't use application's buffer directly, as it has/can have different size, than C/C needs */
    Stmt->result[i].length= &Stmt->result[i].length_value;

    switch(ArdRec->ConciseType) {
    case SQL_C_WCHAR:
      /* In worst case for 2 bytes of UTF16 in result, we need 3 bytes of utf8.
          For ASCII  we need 2 times less(for 2 bytes of UTF16 - 1 byte UTF8,
          in other cases we need same 2 of 4 bytes. */
      ArdRec->InternalBuffer=        (char *)MADB_CALLOC((size_t)((ArdRec->OctetLength)*1.5));
      Stmt->result[i].buffer=        ArdRec->InternalBuffer;
      Stmt->result[i].buffer_length= (unsigned long)(ArdRec->OctetLength*1.5);
      Stmt->result[i].buffer_type=   MYSQL_TYPE_STRING;
      break;
    case SQL_C_CHAR:
      Stmt->result[i].buffer=        DataPtr;
      Stmt->result[i].buffer_length= (unsigned long)ArdRec->OctetLength;
      Stmt->result[i].buffer_type=   MYSQL_TYPE_STRING;
      break;
    case SQL_C_NUMERIC:
      MADB_FREE(ArdRec->InternalBuffer);
      Stmt->result[i].buffer_length= MADB_DEFAULT_PRECISION + 1/*-*/ + 1/*.*/;
      ArdRec->InternalBuffer=       (char *)MADB_CALLOC(Stmt->result[i].buffer_length);
      Stmt->result[i].buffer=        ArdRec->InternalBuffer;
      
      Stmt->result[i].buffer_type=   MYSQL_TYPE_STRING;
      break;
    case SQL_TYPE_TIMESTAMP:
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIME:
    case SQL_C_TIMESTAMP:
    case SQL_C_TIME:
    case SQL_C_DATE:
      MADB_FREE(ArdRec->InternalBuffer);
      if (IrdRec->ConciseType == SQL_CHAR || IrdRec->ConciseType == SQL_VARCHAR)
      {
        ArdRec->InternalBuffer= (char *)MADB_CALLOC(Stmt->stmt->fields[i].max_length + 1);
        if (ArdRec->InternalBuffer == NULL)
        {
          return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
        }
        Stmt->result[i].buffer=        ArdRec->InternalBuffer;
        Stmt->result[i].buffer_type=   MYSQL_TYPE_STRING;
        Stmt->result[i].buffer_length= Stmt->stmt->fields[i].max_length + 1;
      }
      else
      {
        ArdRec->InternalBuffer=       (char *)MADB_CALLOC(sizeof(MYSQL_TIME));
        Stmt->result[i].buffer=        ArdRec->InternalBuffer;
        Stmt->result[i].buffer_length= sizeof(MYSQL_TIME);
        Stmt->result[i].buffer_type=   MYSQL_TYPE_TIMESTAMP;
      }
      break;
    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
    case SQL_C_INTERVAL_HOUR_TO_SECOND:
      {
        MYSQL_FIELD *Field= mysql_fetch_field_direct(Stmt->metadata, i);
        MADB_FREE(ArdRec->InternalBuffer);
        if (IrdRec->ConciseType == SQL_CHAR || IrdRec->ConciseType == SQL_VARCHAR)
        {
          ArdRec->InternalBuffer= (char *)MADB_CALLOC(Stmt->stmt->fields[i].max_length + 1);
          if (ArdRec->InternalBuffer == NULL)
          {
            return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
          }
          Stmt->result[i].buffer=        ArdRec->InternalBuffer;
          Stmt->result[i].buffer_type=   MYSQL_TYPE_STRING;
          Stmt->result[i].buffer_length= Stmt->stmt->fields[i].max_length + 1;
        }
        else
        {
          ArdRec->InternalBuffer=       (char *)MADB_CALLOC(sizeof(MYSQL_TIME));
          Stmt->result[i].buffer=        ArdRec->InternalBuffer;
          Stmt->result[i].buffer_length= sizeof(MYSQL_TIME);
          Stmt->result[i].buffer_type=   Field && Field->type == MYSQL_TYPE_TIME ? MYSQL_TYPE_TIME : MYSQL_TYPE_TIMESTAMP;
        }
      }
      break;
    case SQL_C_TINYINT:
    case SQL_C_UTINYINT:
    case SQL_C_STINYINT:
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
    case SQL_C_USHORT:
    case SQL_C_FLOAT:
    case SQL_C_LONG:
    case SQL_C_ULONG:
    case SQL_C_SLONG:
    case SQL_C_DOUBLE:
      if (MADB_BinaryFieldType(IrdRec->ConciseType))
      {
        /* To keep things simple - we will use internal buffer of the column size, and later(in the MADB_FixFetchedValues) will copy (correct part of)
           it to the application's buffer taking care of endianness. Perhaps it'd be better just not to support this type of conversion */
        MADB_FREE(ArdRec->InternalBuffer);
        ArdRec->InternalBuffer=        (char *)MADB_CALLOC(IrdRec->OctetLength);
        Stmt->result[i].buffer=        ArdRec->InternalBuffer;
        Stmt->result[i].buffer_length= (unsigned long)IrdRec->OctetLength;
        Stmt->result[i].buffer_type=   MYSQL_TYPE_BLOB;
        break;
      }
      /* else {we are falling through below} */
    default:
      if (!MADB_CheckODBCType(ArdRec->ConciseType))
      {
        return MADB_SetError(&Stmt->Error, MADB_ERR_07006, NULL, 0);
      }
      Stmt->result[i].buffer_length= (unsigned long)ArdRec->OctetLength;
      Stmt->result[i].buffer=        DataPtr;
      Stmt->result[i].buffer_type=   MADB_GetMaDBTypeAndLength(ArdRec->ConciseType,
                                                            &Stmt->result[i].is_unsigned,
                                                            &Stmt->result[i].buffer_length);
      break;
    }
  }

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ LittleEndian */
char LittleEndian()
{
  int   x= 1;
  char *c= (char*)&x;

  return *c;
}
/* }}} */

/* {{{ SwitchEndianness */
void SwitchEndianness(char *Src, SQLLEN SrcBytes, char *Dst, SQLLEN DstBytes)
{
  /* SrcBytes can only be less or equal DstBytes */
  while (SrcBytes--)
  {
    *Dst++= *(Src + SrcBytes);
  }
}
/* }}} */

#define CALC_ALL_FLDS_RC(_agg_rc, _field_rc) if (_field_rc != SQL_SUCCESS && _agg_rc != SQL_ERROR) _agg_rc= _field_rc 

/* {{{ MADB_FixFetchedValues 
       Converting and/or fixing fetched values if needed */
SQLRETURN MADB_FixFetchedValues(MADB_Stmt *Stmt, int RowNumber, MYSQL_ROW_OFFSET SaveCursor)
{
  MADB_DescRecord *IrdRec, *ArdRec;
  int             i;
  SQLLEN          *IndicatorPtr= NULL, *LengthPtr= NULL, Dummy= 0;
  void            *DataPtr=      NULL;
  SQLRETURN       rc= SQL_SUCCESS, FieldRc;

  for (i= 0; i < MADB_STMT_COLUMN_COUNT(Stmt); ++i)
  {
    if ((ArdRec= MADB_DescGetInternalRecord(Stmt->Ard, i, MADB_DESC_READ)) && ArdRec->inUse)
    {
      /* set indicator and dataptr */
      LengthPtr=    (SQLLEN *)GetBindOffset(Stmt->Ard, ArdRec, ArdRec->OctetLengthPtr, RowNumber, sizeof(SQLLEN));
      IndicatorPtr= (SQLLEN *)GetBindOffset(Stmt->Ard, ArdRec, ArdRec->IndicatorPtr,   RowNumber, sizeof(SQLLEN));
      DataPtr=      (SQLLEN *)GetBindOffset(Stmt->Ard, ArdRec, ArdRec->DataPtr,        RowNumber, ArdRec->OctetLength);

      if (LengthPtr == NULL)
      {
        LengthPtr= &Dummy;
      }
      /* clear IndicatorPtr */
      if (IndicatorPtr != NULL && IndicatorPtr != LengthPtr && *IndicatorPtr < 0)
      {
        *IndicatorPtr= 0;
      }

      IrdRec= MADB_DescGetInternalRecord(Stmt->Ird, i, MADB_DESC_READ);
      /* assert(IrdRec != NULL) */

      if (*Stmt->stmt->bind[i].is_null)
      {
        if (IndicatorPtr)
        {
          *IndicatorPtr= SQL_NULL_DATA;
        }
        else
        {
          if (SaveCursor)
          {
            mysql_stmt_row_seek(Stmt->stmt, SaveCursor);
          }
          rc= MADB_SetError(&Stmt->Error, MADB_ERR_22002, NULL, 0);
          continue;
        }
      }
      else
      {
        switch (ArdRec->ConciseType)
        {
        case SQL_C_BIT:
        {
          char *p= (char *)Stmt->result[i].buffer;
          if (p)
          {
            *p= test(*p != '\0');
          }
        }
        break;
        case SQL_C_TYPE_TIMESTAMP:
        case SQL_C_TYPE_DATE:
        case SQL_C_TYPE_TIME:
        case SQL_C_TIMESTAMP:
        case SQL_C_TIME:
        case SQL_C_DATE:
          {
            MYSQL_TIME tm, *Intermidiate;

            if (IrdRec->ConciseType == SQL_CHAR || IrdRec->ConciseType == SQL_VARCHAR)
            {
              BOOL isTime;

              FieldRc= MADB_Str2Ts(ArdRec->InternalBuffer, *Stmt->stmt->bind[i].length, &tm, FALSE, &Stmt->Error, &isTime);
              if (SQL_SUCCEEDED(FieldRc))
              {
                Intermidiate= &tm;
              }
              else
              {
                CALC_ALL_FLDS_RC(rc, FieldRc);
                break;
              }
            }
            else
            {
              Intermidiate= (MYSQL_TIME *)ArdRec->InternalBuffer;
            }

            FieldRc= MADB_CopyMadbTimestamp(Stmt, Intermidiate, DataPtr, LengthPtr, IndicatorPtr, ArdRec->Type, IrdRec->ConciseType);
            CALC_ALL_FLDS_RC(rc, FieldRc);
          }
          break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
        {
          MYSQL_TIME          *tm= (MYSQL_TIME*)ArdRec->InternalBuffer, ForConversion;
          SQL_INTERVAL_STRUCT *ts= (SQL_INTERVAL_STRUCT *)DataPtr;

          if (IrdRec->ConciseType == SQL_CHAR || IrdRec->ConciseType == SQL_VARCHAR)
          {
            BOOL isTime;

            FieldRc= MADB_Str2Ts(ArdRec->InternalBuffer, *Stmt->stmt->bind[i].length, &ForConversion, FALSE, &Stmt->Error, &isTime);
            if (SQL_SUCCEEDED(FieldRc))
            {
              tm= &ForConversion;
            }
            else
            {
              CALC_ALL_FLDS_RC(rc, FieldRc);
              break;
            }
          }

          /* If we have ts == NULL we (may) have tm also NULL, since we didn't really bind this column */
          if (ts != NULL)
          {
            if (tm->hour > 99999)
            {
              FieldRc= MADB_SetError(&Stmt->Error, MADB_ERR_22015, NULL, 0);
              CALC_ALL_FLDS_RC(rc, FieldRc);
              break;
            }

            ts->intval.day_second.hour= tm->hour;
            ts->intval.day_second.minute= tm->minute;
            ts->interval_sign= tm->neg ? SQL_TRUE : SQL_FALSE;

            if (ArdRec->Type == SQL_C_INTERVAL_HOUR_TO_MINUTE)
            {
              ts->intval.day_second.second= 0;
              ts->interval_type= SQL_INTERVAL_HOUR_TO_MINUTE;
              if (tm->second)
              {
                FieldRc= MADB_SetError(&Stmt->Error, MADB_ERR_01S07, NULL, 0);
                CALC_ALL_FLDS_RC(rc, FieldRc);
                break;
              }
            }
            else
            {
              ts->interval_type= SQL_INTERVAL_HOUR_TO_SECOND;
              ts->intval.day_second.second= tm->second;
            }
          }
          
          *LengthPtr= sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
        case SQL_C_NUMERIC:
        {
          int rc= 0;
          MADB_CLEAR_ERROR(&Stmt->Error);
          if (DataPtr != NULL && Stmt->result[i].buffer_length < Stmt->stmt->fields[i].max_length)
          {
            MADB_SetError(&Stmt->Error, MADB_ERR_22003, NULL, 0);
            ArdRec->InternalBuffer[Stmt->result[i].buffer_length - 1]= 0;
            return Stmt->Error.ReturnValue;
          }

          if ((rc= MADB_CharToSQLNumeric(ArdRec->InternalBuffer, Stmt->Ard, ArdRec, NULL, RowNumber)))
          {
            MADB_SetError(&Stmt->Error, rc, NULL, 0);
          }
          /* TODO: why is it here individually for Numeric type?! */
          if (Stmt->Ard->Header.ArrayStatusPtr)
          {
            Stmt->Ard->Header.ArrayStatusPtr[RowNumber]= Stmt->Error.ReturnValue;
          }
          *LengthPtr= sizeof(SQL_NUMERIC_STRUCT);
        }
        break;
        case SQL_C_WCHAR:
        {
          SQLLEN CharLen= MADB_SetString(&Stmt->Connection->Charset, DataPtr, ArdRec->OctetLength, (char *)Stmt->result[i].buffer,
            *Stmt->stmt->bind[i].length, &Stmt->Error);
          /* Not quite right */
          *LengthPtr= CharLen * sizeof(SQLWCHAR);
        }
        break;

        case SQL_C_TINYINT:
        case SQL_C_UTINYINT:
        case SQL_C_STINYINT:
        case SQL_C_SHORT:
        case SQL_C_SSHORT:
        case SQL_C_USHORT:
        case SQL_C_FLOAT:
        case SQL_C_LONG:
        case SQL_C_ULONG:
        case SQL_C_SLONG:
        case SQL_C_DOUBLE:
          if (MADB_BinaryFieldType(IrdRec->ConciseType))
          {
            if (DataPtr != NULL)
            {
              if (Stmt->result[i].buffer_length >= (unsigned long)ArdRec->OctetLength)
              {
                if (LittleEndian())
                {
                  /* We currently got the bigendian number. If we or littleendian machine, we need to switch bytes */
                  SwitchEndianness((char*)Stmt->result[i].buffer + Stmt->result[i].buffer_length - ArdRec->OctetLength,
                    ArdRec->OctetLength,
                    (char*)DataPtr,
                    ArdRec->OctetLength);
                }
                else
                {
                  memcpy(DataPtr, (void*)((char*)Stmt->result[i].buffer + Stmt->result[i].buffer_length - ArdRec->OctetLength), ArdRec->OctetLength);
                }
              }
              else
              {
                /* We won't write to the whole memory pointed by DataPtr, thus to need to zerofill prior to that */
                memset(DataPtr, 0, ArdRec->OctetLength);
                if (LittleEndian())
                {
                  SwitchEndianness((char*)Stmt->result[i].buffer,
                    Stmt->result[i].buffer_length,
                    (char*)DataPtr,
                    ArdRec->OctetLength);
                }
                else
                {
                  memcpy((void*)((char*)DataPtr + ArdRec->OctetLength - Stmt->result[i].buffer_length),
                    Stmt->result[i].buffer, Stmt->result[i].buffer_length);
                }
              }
              *LengthPtr= *Stmt->stmt->bind[i].length;
            }
            break;
          }
          /* else {we are falling through below} */
        default:
          if (DataPtr != NULL)
          {
            if (Stmt->Ard->Header.ArraySize > 1)
            {
              if (Stmt->Ard->Header.BindType)
              {
                Stmt->result[i].buffer= (char *)Stmt->result[i].buffer + Stmt->Ard->Header.BindType;
              }
              else
              {
                Stmt->result[i].buffer = (char *)ArdRec->DataPtr + (RowNumber + 1) * ArdRec->OctetLength;
              }
            }
            *LengthPtr= *Stmt->stmt->bind[i].length;
          }
          break;
        }
      }
    }
  }

  return rc;
}
/* }}} */
#undef CALC_ALL_FLDS_RC


SQLUSMALLINT MADB_MapToRowStatus(SQLRETURN rc)
{
  switch (rc)
  {
  case SQL_SUCCESS_WITH_INFO: return SQL_ROW_SUCCESS_WITH_INFO;
  case SQL_ERROR:             return SQL_ROW_ERROR;
  /* Assuming is that status array pre-filled with SQL_ROW_NOROW,
     and it never needs to be mapped to */
  }

  return SQL_ROW_SUCCESS;
}


void ResetDescIntBuffers(MADB_Desc *Desc)
{
  MADB_DescRecord *Rec;
  SQLSMALLINT i;

  for (i= 0; i < Desc->Header.Count; ++i)
  {
    Rec= MADB_DescGetInternalRecord(Desc, i, MADB_DESC_READ);
    if (Rec)
    {
      MADB_FREE(Rec->InternalBuffer);
    }
  }
}

/* For first row we just take its result as initial.
   For the rest, if all rows SQL_SUCCESS or SQL_ERROR - aggregated result is SQL_SUCCESS or SQL_ERROR, respectively
   Otherwise - SQL_SUCCESS_WITH_INFO */
#define CALC_ALL_ROWS_RC(_accumulated_rc, _cur_row_rc, _row_num)\
if      (_row_num == 0)                  _accumulated_rc= _cur_row_rc;\
else if (_cur_row_rc != _accumulated_rc) _accumulated_rc= SQL_SUCCESS_WITH_INFO

/* {{{ MADB_StmtFetch */
SQLRETURN MADB_StmtFetch(MADB_Stmt *Stmt)
{
  unsigned int     RowNum, j, rc;
  SQLULEN          Rows2Fetch=  Stmt->Ard->Header.ArraySize, Processed, *ProcessedPtr= &Processed;
  MYSQL_ROW_OFFSET SaveCursor= NULL;
  SQLRETURN        Result= SQL_SUCCESS, RowResult;

  MADB_CLEAR_ERROR(&Stmt->Error);

  if (!(MADB_STMT_COLUMN_COUNT(Stmt) > 0))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_24000, NULL, 0);
  }

  if ((Stmt->Options.UseBookmarks == SQL_UB_VARIABLE && Stmt->Options.BookmarkType == SQL_C_BOOKMARK) ||
      (Stmt->Options.UseBookmarks != SQL_UB_VARIABLE && Stmt->Options.BookmarkType == SQL_C_VARBOOKMARK))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07006, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  /* We don't have much to do if ArraySize == 0 */
  if (Stmt->Ard->Header.ArraySize == 0)
  {
    return SQL_SUCCESS;
  }

  Stmt->LastRowFetched= 0;

  if (Stmt->result == NULL && !(Stmt->result= (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * mysql_stmt_field_count(Stmt->stmt))))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  Rows2Fetch= MADB_RowsToFetch(&Stmt->Cursor, Stmt->Ard->Header.ArraySize, mysql_stmt_num_rows(Stmt->stmt));
  if (Rows2Fetch == 0)
  {
    return SQL_NO_DATA;
  }

  if (Stmt->Ard->Header.ArrayStatusPtr)
  {
    MADB_InitStatusPtr(Stmt->Ard->Header.ArrayStatusPtr, Stmt->Ard->Header.ArraySize, SQL_NO_DATA);
  }

  if (Stmt->Ird->Header.RowsProcessedPtr)
  {
    ProcessedPtr= Stmt->Ird->Header.RowsProcessedPtr;
  }
  if (Stmt->Ird->Header.ArrayStatusPtr)
  {
    MADB_InitStatusPtr(Stmt->Ird->Header.ArrayStatusPtr, Stmt->Ard->Header.ArraySize, SQL_ROW_NOROW);
  }

  *ProcessedPtr= 0;

  /* We need to return to 1st row in the rowset only if there are >1 rows in it. Otherwise we stay on it anyway */
  if (Rows2Fetch > 1 && Stmt->Options.CursorType != SQL_CURSOR_FORWARD_ONLY)
  {
    SaveCursor= mysql_stmt_row_tell(Stmt->stmt);
    /* Skipping current row for for reading now, it will be read when the Cursor is returned to it */
    MoveNext(Stmt, 1LL);
  }

  for (j= 0; j < Rows2Fetch; ++j)
  {
    RowResult= SQL_SUCCESS;
    /* If we need to return the cursor to 1st row in the rowset, we start to read it from 2nd, and 1st row we read the last */
    if (SaveCursor != NULL)
    {
      RowNum= j + 1;
      if (RowNum == Rows2Fetch)
      {
        RowNum= 0;
        Stmt->Cursor.Next= mysql_stmt_row_tell(Stmt->stmt);
        mysql_stmt_row_seek(Stmt->stmt, SaveCursor);
      }
    }
    else
    {
      RowNum= j;
    }
    /*************** Setting up BIND structures ********************/
    /* Basically, nothing should happen here, but if happens, then it will happen on each row.
    Thus it's ok to stop */
    RETURN_ERROR_OR_CONTINUE(MADB_PrepareBind(Stmt, RowNum));

    /************************ Bind! ********************************/  
    mysql_stmt_bind_result(Stmt->stmt, Stmt->result);

    if (Stmt->Options.UseBookmarks && Stmt->Options.BookmarkPtr != NULL)
    {
      /* TODO: Bookmark can be not only "unsigned long*", but also "unsigned char*". Can be determined by examining Stmt->Options.BookmarkType */
      long *p= (long *)Stmt->Options.BookmarkPtr;
      p+= RowNum * Stmt->Options.BookmarkLength;
      *p= (long)Stmt->Cursor.Position;
    }
    /************************ Fetch! ********************************/
    rc= mysql_stmt_fetch(Stmt->stmt);

    *ProcessedPtr += 1;

    if (Stmt->Cursor.Position < 0)
    {
      Stmt->Cursor.Position= 0;
    }

    switch(rc) {
    case 1:
      RowResult= MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
      /* If mysql_stmt_fetch returned error, there is no sense to continue */
      if (Stmt->Ird->Header.ArrayStatusPtr)
      {
        Stmt->Ird->Header.ArrayStatusPtr[RowNum]= MADB_MapToRowStatus(RowResult);
      }
      CALC_ALL_ROWS_RC(Result, RowResult, RowNum);
      return Result;

    case MYSQL_DATA_TRUNCATED:
    {
      /* We will not report truncation if a dummy buffer was bound */
      int     col;

      for (col= 0; col < MADB_STMT_COLUMN_COUNT(Stmt); ++col)
      {
        if (Stmt->stmt->bind[col].error && *Stmt->stmt->bind[col].error > 0 &&
            !(Stmt->stmt->bind[col].flags & MADB_BIND_DUMMY))
        {
          MADB_DescRecord *ArdRec= MADB_DescGetInternalRecord(Stmt->Ard, col, MADB_DESC_READ),
                          *IrdRec= MADB_DescGetInternalRecord(Stmt->Ird, col, MADB_DESC_READ);
          /* If (numeric) field value and buffer are of the same size - ignoring truncation.
          In some cases specs are not clear enough if certain column signed or not(think of catalog functions for example), and
          some apps bind signed buffer where we return unsigdned value. And in general - if application want to fetch unsigned as
          signed, or vice versa, why we should prevent that. */
          if (ArdRec->OctetLength == IrdRec->OctetLength
           && MADB_IsIntType(IrdRec->ConciseType) && MADB_IsIntType(ArdRec->ConciseType))
          {
            continue;
          }
          /* For numeric types we return either 22003 or 01S07, 01004 for the rest.
             if ird type is not fractional - we return 22003. But as a matter of fact, it's possible that we have 22003 if converting
             from fractional types */
          RowResult= MADB_SetError(&Stmt->Error, ArdRec != NULL && MADB_IsNumericType(ArdRec->ConciseType) ?
                    (MADB_IsIntType(IrdRec->ConciseType) ? MADB_ERR_22003 : MADB_ERR_01S07) : MADB_ERR_01004, NULL, 0);
          /* One found such column is enough */
          break;
        }
      }
      break;
    }
    case MYSQL_NO_DATA:
      /* We have already incremented this counter, since there was no more rows, need to decrement */
      --*ProcessedPtr;
      /* SQL_NO_DATA should be only returned if first fetched row is already beyond end of the resultset */
      if (RowNum > 0)
      {
        continue;
      }
      return SQL_NO_DATA;
    }  /* End of switch on fetch result */

    ++Stmt->LastRowFetched;
    ++Stmt->PositionedCursor;

    /*Conversion etc. At this point, after fetch we can have RowResult either SQL_SUCCESS or SQL_SUCCESS_WITH_INFO */
    switch (MADB_FixFetchedValues(Stmt, RowNum, SaveCursor))
    {
    case SQL_ERROR:
      RowResult= SQL_ERROR;
      break;
    case SQL_SUCCESS_WITH_INFO:
      RowResult= SQL_SUCCESS_WITH_INFO;
    /* And if result of conversions - success, just leaving that we had before */
    }

    CALC_ALL_ROWS_RC(Result, RowResult, RowNum);

    if (Stmt->Ird->Header.ArrayStatusPtr)
    {
      Stmt->Ird->Header.ArrayStatusPtr[RowNum]= MADB_MapToRowStatus(RowResult);
    }
  }
    
  memset(Stmt->CharOffset, 0, sizeof(long) * mysql_stmt_field_count(Stmt->stmt));
  memset(Stmt->Lengths, 0, sizeof(long) * mysql_stmt_field_count(Stmt->stmt));

  ResetDescIntBuffers(Stmt->Ird);

  return Result;
}
/* }}} */

#undef CALC_ALL_ROWS_RC

/* {{{ MADB_StmtGetAttr */ 
SQLRETURN MADB_StmtGetAttr(MADB_Stmt *Stmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
                       SQLINTEGER *StringLengthPtr)
{
  SQLINTEGER StringLength;
  SQLRETURN ret= SQL_SUCCESS;

  if (!StringLengthPtr)
    StringLengthPtr= &StringLength;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  switch(Attribute) {
  case SQL_ATTR_APP_PARAM_DESC:
    *(SQLPOINTER *)ValuePtr= Stmt->Apd;
    *StringLengthPtr= sizeof(SQLPOINTER *);
    break;
  case SQL_ATTR_APP_ROW_DESC:
    *(SQLPOINTER *)ValuePtr= Stmt->Ard;
    *StringLengthPtr= sizeof(SQLPOINTER *);
    break;
  case SQL_ATTR_IMP_PARAM_DESC:
    *(SQLPOINTER *)ValuePtr= Stmt->Ipd;
    *StringLengthPtr= sizeof(SQLPOINTER *);
    break;
  case SQL_ATTR_IMP_ROW_DESC:
    *(SQLPOINTER *)ValuePtr= Stmt->Ird;
    *StringLengthPtr= sizeof(SQLPOINTER *);
    break;
  case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
    *(SQLPOINTER *)ValuePtr= Stmt->Apd->Header.BindOffsetPtr;
    break;
  case SQL_ATTR_PARAM_BIND_TYPE:
    *(SQLULEN *)ValuePtr= Stmt->Apd->Header.BindType;
    break;
  case SQL_ATTR_PARAM_OPERATION_PTR:
    *(SQLPOINTER *)ValuePtr= (SQLPOINTER)Stmt->Apd->Header.ArrayStatusPtr;
    break;
  case SQL_ATTR_PARAM_STATUS_PTR:
    *(SQLPOINTER *)ValuePtr= (SQLPOINTER)Stmt->Ipd->Header.ArrayStatusPtr;
    break;
  case SQL_ATTR_PARAMS_PROCESSED_PTR:
    *(SQLPOINTER *)ValuePtr= (SQLPOINTER)(SQLULEN)Stmt->Ipd->Header.BindType;
    break;
  case SQL_ATTR_PARAMSET_SIZE:
    *(SQLULEN *)ValuePtr= Stmt->Apd->Header.ArraySize;
    break;
  case SQL_ATTR_ASYNC_ENABLE:
    *(SQLPOINTER *)ValuePtr= SQL_ASYNC_ENABLE_OFF;
    break;
  case SQL_ATTR_ROW_ARRAY_SIZE:
  case SQL_ROWSET_SIZE:
    *(SQLULEN *)ValuePtr= Stmt->Ard->Header.ArraySize;
    break;
  case SQL_ATTR_ROW_BIND_OFFSET_PTR:
    *(SQLPOINTER *)ValuePtr= (SQLPOINTER)Stmt->Ard->Header.BindOffsetPtr;
    break;
  case SQL_ATTR_ROW_BIND_TYPE:
    *(SQLULEN *)ValuePtr= Stmt->Ard->Header.BindType;
    break;
  case SQL_ATTR_ROW_OPERATION_PTR:
    *(SQLPOINTER *)ValuePtr= (SQLPOINTER)Stmt->Ard->Header.ArrayStatusPtr;
    break;
  case SQL_ATTR_ROW_STATUS_PTR:
    *(SQLPOINTER *)ValuePtr= (SQLPOINTER)Stmt->Ird->Header.ArrayStatusPtr;
    break;
  case SQL_ATTR_ROWS_FETCHED_PTR:
    *(SQLULEN **)ValuePtr= Stmt->Ird->Header.RowsProcessedPtr;
    break;
  case SQL_ATTR_USE_BOOKMARKS:
    *(SQLUINTEGER *)ValuePtr= Stmt->Options.UseBookmarks;
  case SQL_ATTR_SIMULATE_CURSOR:
    *(SQLULEN *)ValuePtr= Stmt->Options.SimulateCursor;
    break;
  case SQL_ATTR_CURSOR_SCROLLABLE:
    *(SQLULEN *)ValuePtr= Stmt->Options.CursorType;
    break;
  case SQL_ATTR_CURSOR_SENSITIVITY:
    *(SQLULEN *)ValuePtr= SQL_UNSPECIFIED;
    break;
  case SQL_ATTR_CURSOR_TYPE:
    *(SQLULEN *)ValuePtr= Stmt->Options.CursorType;
    break;
  case SQL_ATTR_CONCURRENCY:
    *(SQLULEN *)ValuePtr= SQL_CONCUR_READ_ONLY;
    break;
  case SQL_ATTR_ENABLE_AUTO_IPD:
    *(SQLULEN *)ValuePtr= SQL_FALSE;
    break;
  case SQL_ATTR_MAX_LENGTH:
    *(SQLULEN *)ValuePtr= Stmt->Options.MaxLength;
    break;
  case SQL_ATTR_MAX_ROWS:
    *(SQLULEN *)ValuePtr= Stmt->Options.MaxRows;
    break;
  case SQL_ATTR_METADATA_ID:
    *(SQLULEN *)ValuePtr= Stmt->Options.MetadataId;
    break;
  case SQL_ATTR_NOSCAN:
    *(SQLULEN *)ValuePtr= SQL_NOSCAN_ON;
    break;
  case SQL_ATTR_QUERY_TIMEOUT:
    *(SQLULEN *)ValuePtr= 0;
    break;
  case SQL_ATTR_RETRIEVE_DATA:
    *(SQLULEN *)ValuePtr= SQL_RD_ON;
    break;
  }
  return ret;
}
/* }}} */


/* {{{ MADB_StmtSetAttr */
SQLRETURN MADB_StmtSetAttr(MADB_Stmt *Stmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{
  SQLRETURN ret= SQL_SUCCESS;

  if (!Stmt)
    return SQL_INVALID_HANDLE;

  switch(Attribute) {
  case SQL_ATTR_APP_PARAM_DESC:
    if (ValuePtr)
    {
       MADB_Desc *Desc= (MADB_Desc *)ValuePtr;
      if (!Desc->AppType && Desc != Stmt->IApd)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY017, NULL, 0);
        return Stmt->Error.ReturnValue;
      }
      if (Desc->DescType != MADB_DESC_APD && Desc->DescType != MADB_DESC_UNKNOWN)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY024, NULL, 0);
        return Stmt->Error.ReturnValue;
      }
      RemoveStmtRefFromDesc(Stmt->Apd, Stmt, FALSE);
      Stmt->Apd= (MADB_Desc *)ValuePtr;
      Stmt->Apd->DescType= MADB_DESC_APD;
      if (Stmt->Apd != Stmt->IApd)
      {
        MADB_Stmt **IntStmt;
        IntStmt = (MADB_Stmt **)MADB_AllocDynamic(&Stmt->Apd->Stmts);
        *IntStmt= Stmt;
      }
    }
    else
    {
      RemoveStmtRefFromDesc(Stmt->Apd, Stmt, FALSE);
      Stmt->Apd= Stmt->IApd;
    }
    break;
  case SQL_ATTR_APP_ROW_DESC:
    if (ValuePtr)
    {
      MADB_Desc *Desc= (MADB_Desc *)ValuePtr;

      if (!Desc->AppType && Desc != Stmt->IArd)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY017, NULL, 0);
        return Stmt->Error.ReturnValue;
      }
      if (Desc->DescType != MADB_DESC_ARD && Desc->DescType != MADB_DESC_UNKNOWN)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY024, NULL, 0);
        return Stmt->Error.ReturnValue;
      }
      RemoveStmtRefFromDesc(Stmt->Ard, Stmt, FALSE);
      Stmt->Ard= Desc;
      Stmt->Ard->DescType= MADB_DESC_ARD;
      if (Stmt->Ard != Stmt->IArd)
      {
        MADB_Stmt **IntStmt;
        IntStmt = (MADB_Stmt **)MADB_AllocDynamic(&Stmt->Ard->Stmts);
        *IntStmt= Stmt;
      }
    }
    else
    {
      RemoveStmtRefFromDesc(Stmt->Ard, Stmt, FALSE);
      Stmt->Ard= Stmt->IArd;
    }
    break;

  case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
    Stmt->Apd->Header.BindOffsetPtr= (SQLULEN*)ValuePtr;
    break;
  case SQL_ATTR_PARAM_BIND_TYPE:
    Stmt->Apd->Header.BindType= (SQLINTEGER)(SQLLEN)ValuePtr;
    break;
  case SQL_ATTR_PARAM_OPERATION_PTR:
    Stmt->Apd->Header.ArrayStatusPtr= (SQLUSMALLINT *)ValuePtr;
    break;
  case SQL_ATTR_PARAM_STATUS_PTR:
    Stmt->Ipd->Header.ArrayStatusPtr= (SQLUSMALLINT *)ValuePtr;
    break;
  case SQL_ATTR_PARAMS_PROCESSED_PTR:
    Stmt->Ipd->Header.RowsProcessedPtr  = (SQLULEN *)ValuePtr;
    break;
  case SQL_ATTR_PARAMSET_SIZE:
    Stmt->Apd->Header.ArraySize= (SQLULEN)ValuePtr;
    break;
  case SQL_ATTR_ROW_ARRAY_SIZE:
  case SQL_ROWSET_SIZE:
    Stmt->Ard->Header.ArraySize= (SQLULEN)ValuePtr;
    break;
  case SQL_ATTR_ROW_BIND_OFFSET_PTR:
    Stmt->Ard->Header.BindOffsetPtr= (SQLULEN*)ValuePtr;
    break;
  case SQL_ATTR_ROW_BIND_TYPE:
    Stmt->Ard->Header.BindType= (SQLINTEGER)(SQLLEN)ValuePtr;
    break;
  case SQL_ATTR_ROW_OPERATION_PTR:
    Stmt->Ard->Header.ArrayStatusPtr= (SQLUSMALLINT *)ValuePtr;
    break;
  case SQL_ATTR_ROW_STATUS_PTR:
    Stmt->Ird->Header.ArrayStatusPtr= (SQLUSMALLINT *)ValuePtr;
    break;
  case SQL_ATTR_ROWS_FETCHED_PTR:
    Stmt->Ird->Header.RowsProcessedPtr= (SQLULEN*)ValuePtr;
    break;
  case SQL_ATTR_ASYNC_ENABLE:
    if ((SQLULEN)ValuePtr != SQL_ASYNC_ENABLE_OFF)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_01S02, "Option value changed to default (SQL_ATTR_ASYNC_ENABLE)", 0);
      ret= SQL_SUCCESS_WITH_INFO;
    }
    break;
  case SQL_ATTR_SIMULATE_CURSOR:
    Stmt->Options.SimulateCursor= (SQLULEN) ValuePtr;
    break;
  case SQL_ATTR_CURSOR_SCROLLABLE:
    Stmt->Options.CursorType=  ((SQLULEN)ValuePtr == SQL_NONSCROLLABLE) ?
                               SQL_CURSOR_FORWARD_ONLY : SQL_CURSOR_STATIC;
    break;
  case SQL_ATTR_CURSOR_SENSITIVITY:
    /* we only support default value = SQL_UNSPECIFIED */
    if ((SQLULEN)ValuePtr != SQL_UNSPECIFIED)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_01S02, "Option value changed to default cursor sensitivity", 0);
      ret= SQL_SUCCESS_WITH_INFO;
    }
    break;
  case SQL_ATTR_CURSOR_TYPE:
    /* We need to check global DSN/Connection settings */
    if (MA_ODBC_CURSOR_FORWARD_ONLY(Stmt->Connection) && (SQLULEN)ValuePtr != SQL_CURSOR_FORWARD_ONLY)
    {
      Stmt->Options.CursorType= SQL_CURSOR_FORWARD_ONLY;
      MADB_SetError(&Stmt->Error, MADB_ERR_01S02, "Option value changed to default (SQL_CURSOR_FORWARD_ONLY)", 0);
      return Stmt->Error.ReturnValue;
    }
    else if (MA_ODBC_CURSOR_DYNAMIC(Stmt->Connection))
    {
      if ((SQLULEN)ValuePtr == SQL_CURSOR_KEYSET_DRIVEN)
      {
        Stmt->Options.CursorType= SQL_CURSOR_STATIC;
        MADB_SetError(&Stmt->Error, MADB_ERR_01S02, "Option value changed to default (SQL_CURSOR_STATIC)", 0);
        return Stmt->Error.ReturnValue;
      }
      Stmt->Options.CursorType= (SQLUINTEGER)(SQLULEN)ValuePtr;
    }
    /* only FORWARD or Static is allowed */
    else
    {
      if ((SQLULEN)ValuePtr != SQL_CURSOR_FORWARD_ONLY &&
          (SQLULEN)ValuePtr != SQL_CURSOR_STATIC)
      {
        Stmt->Options.CursorType= SQL_CURSOR_STATIC;
        MADB_SetError(&Stmt->Error, MADB_ERR_01S02, "Option value changed to default (SQL_CURSOR_STATIC)", 0);
        return Stmt->Error.ReturnValue;
      }
      Stmt->Options.CursorType= (SQLUINTEGER)(SQLULEN)ValuePtr;
    }
    break;
  case SQL_ATTR_CONCURRENCY:
    if ((SQLULEN)ValuePtr != SQL_CONCUR_READ_ONLY)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_01S02, "Option value changed to default (SQL_CONCUR_READ_ONLY). ", 0);
      ret= SQL_SUCCESS_WITH_INFO;
    }
    break;
  case SQL_ATTR_ENABLE_AUTO_IPD:
    /* MariaDB doesn't deliver param metadata after prepare, so we can't autopopulate ird */
    MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, NULL, 0);
    return Stmt->Error.ReturnValue;
    break;
  case SQL_ATTR_MAX_LENGTH:
    Stmt->Options.MaxLength= (SQLULEN)ValuePtr;
    break;
  case SQL_ATTR_MAX_ROWS:
    Stmt->Options.MaxRows= (SQLULEN)ValuePtr;
    break;
  case SQL_ATTR_METADATA_ID:
    Stmt->Options.MetadataId= (SQLULEN)ValuePtr;
    break;
  case SQL_ATTR_NOSCAN:
    if ((SQLULEN)ValuePtr != SQL_NOSCAN_ON)
    {
       MADB_SetError(&Stmt->Error, MADB_ERR_01S02, "Option value changed to default (SQL_NOSCAN_ON)", 0);
       ret= SQL_SUCCESS_WITH_INFO;
    }
    break;
  case SQL_ATTR_QUERY_TIMEOUT:
    if ((SQLULEN)ValuePtr != 0)
    {
       MADB_SetError(&Stmt->Error, MADB_ERR_01S02, "Option value changed to default (no timeout)", 0);
       ret= SQL_SUCCESS_WITH_INFO;
    }
    break;
  case SQL_ATTR_RETRIEVE_DATA:
    if ((SQLULEN)ValuePtr != SQL_RD_ON)
    {
       MADB_SetError(&Stmt->Error, MADB_ERR_01S02, "Option value changed to default (SQL_RD_ON)", 0);
       ret= SQL_SUCCESS_WITH_INFO;
    }
    break;
  case SQL_ATTR_USE_BOOKMARKS:
    Stmt->Options.UseBookmarks= (SQLUINTEGER)(SQLULEN)ValuePtr;
   break;
  case SQL_ATTR_FETCH_BOOKMARK_PTR:
    MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, NULL, 0);
    return Stmt->Error.ReturnValue;
    break;
  default:
    MADB_SetError(&Stmt->Error, MADB_ERR_HY024, NULL, 0);
    return Stmt->Error.ReturnValue;
    break;
  }
  return ret;
}
/* }}} */

SQLRETURN MADB_GetBookmark(MADB_Stmt  *Stmt,
                           SQLSMALLINT TargetType,
                           SQLPOINTER  TargetValuePtr,
                           SQLLEN      BufferLength,
                           SQLLEN     *StrLen_or_IndPtr)
{
  if (Stmt->Options.UseBookmarks == SQL_UB_OFF)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07009, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  if ((Stmt->Options.UseBookmarks == SQL_UB_VARIABLE && TargetType != SQL_C_VARBOOKMARK) ||
    (Stmt->Options.UseBookmarks != SQL_UB_VARIABLE && TargetType == SQL_C_VARBOOKMARK))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY003, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  if (TargetValuePtr && TargetType == SQL_C_BOOKMARK && BufferLength <= sizeof(SQLULEN))
  {
    *(SQLULEN *)TargetValuePtr= Stmt->Cursor.Position;
    if (StrLen_or_IndPtr)
    {
      *StrLen_or_IndPtr= sizeof(SQLULEN);
    }
    return SQL_SUCCESS;
  }

  /* Keeping compiler happy */
  return SQL_SUCCESS;
}

/* {{{ MADB_StmtGetData */
SQLRETURN MADB_StmtGetData(SQLHSTMT StatementHandle,
                           SQLUSMALLINT Col_or_Param_Num,
                           SQLSMALLINT TargetType,
                           SQLPOINTER TargetValuePtr,
                           SQLLEN BufferLength,
                           SQLLEN * StrLen_or_IndPtr,
                           BOOL   InternalUse /* Currently this is respected for SQL_CHAR type only,
                                                 since all "internal" calls of the function need string representation of datat */)
{
  MADB_Stmt       *Stmt= (MADB_Stmt *)StatementHandle;
  SQLUSMALLINT    Offset= Col_or_Param_Num - 1;
  SQLSMALLINT     OdbcType= 0, MadbType= 0;
  MYSQL_BIND      Bind;
  my_bool         IsNull= FALSE;
  my_bool         ZeroTerminated= 0;
  unsigned long   CurrentOffset= InternalUse == TRUE ? 0 : Stmt->CharOffset[Offset]; /* We are supposed not get bookmark column here */
  my_bool         Error;
  MADB_DescRecord *IrdRec= NULL;
  MYSQL_FIELD     *Field= mysql_fetch_field_direct(Stmt->metadata, Offset);

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* Should not really happen, and is evidence of that something wrong happened in some previous call(SQLFetch?) */
  if (Stmt->stmt->bind == NULL)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY109, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  if (Stmt->stmt->bind[Offset].is_null != NULL && *Stmt->stmt->bind[Offset].is_null != '\0')
  {
    if (!StrLen_or_IndPtr)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_22002, NULL, 0);
      return Stmt->Error.ReturnValue;
    }
    *StrLen_or_IndPtr= SQL_NULL_DATA;
    return SQL_SUCCESS;
  }

  memset(&Bind, 0, sizeof(MYSQL_BIND));

  /* We might need it for SQL_C_DEFAULT type, or to obtain length of fixed length types(Access likes to have it) */
  IrdRec= MADB_DescGetInternalRecord(Stmt->Ird, Offset, MADB_DESC_READ);
  if (!IrdRec)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07009, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  switch (TargetType) {
  case SQL_ARD_TYPE:
    {
      MADB_DescRecord *Ard= MADB_DescGetInternalRecord(Stmt->Ard, Offset, MADB_DESC_READ);

      if (!Ard)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_07009, NULL, 0);
        return Stmt->Error.ReturnValue;
      }
      OdbcType= Ard->ConciseType;
    }
    break;
  case SQL_C_DEFAULT:
    {
      /* Taking type from IRD record. This way, if mysql type was fixed(currently that is mainly for catalog functions, we don't lose it.
         (Access uses default types on getting catalog functions results, and not quite happy when it gets something unexpected. Seemingly it cares about returned data lenghts even for types,
         for which standard says application should not care about */
      OdbcType= IrdRec->ConciseType;
    }
    break;
  default:
    OdbcType= TargetType;
    break;  
  }
  /* Restoring mariadb/mysql type from odbc type */
  MadbType= MADB_GetMaDBTypeAndLength(OdbcType, &Bind.is_unsigned, &Bind.buffer_length);

  /* set global values for Bind */
  Bind.error=   &Error;
  Bind.length=  &Bind.length_value;
  Bind.is_null= &IsNull;

  switch(OdbcType)
  {
  case SQL_DATE:
  case SQL_C_TYPE_DATE:
  case SQL_TIMESTAMP:
  case SQL_C_TYPE_TIMESTAMP:
  case SQL_TIME:
  case SQL_C_TYPE_TIME:
    {
      MYSQL_TIME tm;

      if (IrdRec->ConciseType == SQL_CHAR || IrdRec->ConciseType == SQL_VARCHAR)
      {
        char  *ClientValue= NULL;
        BOOL isTime;

        if (!(ClientValue = (char *)MADB_CALLOC(Stmt->stmt->fields[Offset].max_length + 1)))
        {
          return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
        }
        Bind.buffer=        ClientValue;
        Bind.buffer_type=   MYSQL_TYPE_STRING;
        Bind.buffer_length= Stmt->stmt->fields[Offset].max_length + 1;
        mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);
        RETURN_ERROR_OR_CONTINUE(MADB_Str2Ts(ClientValue, Bind.length_value, &tm, FALSE, &Stmt->Error, &isTime));
      }
      else
      {


        Bind.buffer_length= sizeof(MYSQL_TIME);
        Bind.buffer= (void *)&tm;
        /* c/c is too smart to convert hours to days and days to hours, we don't need that */
        if ((OdbcType == SQL_C_TIME || OdbcType == SQL_C_TYPE_TIME)
          && (IrdRec->ConciseType == SQL_TIME || IrdRec->ConciseType == SQL_TYPE_TIME))
        {
          Bind.buffer_type= MYSQL_TYPE_TIME;
        }
        else
        {
          Bind.buffer_type= MYSQL_TYPE_TIMESTAMP;
          
        }
        mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);
      }
      RETURN_ERROR_OR_CONTINUE(MADB_CopyMadbTimestamp(Stmt, &tm, TargetValuePtr, StrLen_or_IndPtr, StrLen_or_IndPtr, OdbcType, IrdRec->ConciseType));
      break;
    }
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
    {
      MYSQL_TIME tm;
      SQL_INTERVAL_STRUCT *ts= (SQL_INTERVAL_STRUCT *)TargetValuePtr;

      if (IrdRec->ConciseType == SQL_CHAR || IrdRec->ConciseType == SQL_VARCHAR)
      {
        char *ClientValue= NULL;
        BOOL isTime;

        if (!(ClientValue = (char *)MADB_CALLOC(Stmt->stmt->fields[Offset].max_length + 1)))
        {
          return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
        }
        Bind.buffer=        ClientValue;
        Bind.buffer_type=   MYSQL_TYPE_STRING;
        Bind.buffer_length= Stmt->stmt->fields[Offset].max_length + 1;
        mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);
        RETURN_ERROR_OR_CONTINUE(MADB_Str2Ts(ClientValue, Bind.length_value, &tm, TRUE, &Stmt->Error, &isTime));
      }
      else
      {
        Bind.buffer_length= sizeof(MYSQL_TIME);
        Bind.buffer= (void *)&tm;
        /* c/c is too smart to convert hours to days and days to hours, we don't need that */
        Bind.buffer_type= Field && Field->type == MYSQL_TYPE_TIME ? MYSQL_TYPE_TIME : MYSQL_TYPE_TIMESTAMP;
        mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);
      }

      if (tm.hour > 99999)
      {
        return MADB_SetError(&Stmt->Error, MADB_ERR_22015, NULL, 0);
      }

      ts->intval.day_second.hour= tm.hour;
      ts->intval.day_second.minute= tm.minute;
      ts->interval_sign= tm.neg ? SQL_TRUE : SQL_FALSE;

      if (TargetType == SQL_C_INTERVAL_HOUR_TO_MINUTE)
      {
        ts->intval.day_second.second= 0;
        ts->interval_type= SQL_INTERVAL_HOUR_TO_MINUTE;
        if (tm.second)
        {
          return MADB_SetError(&Stmt->Error, MADB_ERR_01S07, NULL, 0);
        }
      }
      else
      {
        ts->interval_type= SQL_INTERVAL_HOUR_TO_SECOND;
        ts->intval.day_second.second= tm.second;
      }
      if (StrLen_or_IndPtr)
      {
        *StrLen_or_IndPtr= sizeof(SQL_INTERVAL_STRUCT);
      }
    }
    break;
  case SQL_WCHAR:
  case SQL_WVARCHAR:
  case SQL_WLONGVARCHAR:
    {
      char  *ClientValue= NULL;
      size_t CharLength= 0;

      /* Kinda this it not 1st call for this value, and we have it nice and recoded */
      if (IrdRec->InternalBuffer == NULL/* && Stmt->Lengths[Offset] == 0*/)
      {
        if (!(ClientValue = (char *)MADB_CALLOC(Stmt->stmt->fields[Offset].max_length + 1)))
        {
          MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
          return Stmt->Error.ReturnValue;
        }
        Bind.buffer=        ClientValue;
        Bind.buffer_type=   MYSQL_TYPE_STRING;
        Bind.buffer_length= Stmt->stmt->fields[Offset].max_length + 1;

        if (mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, Stmt->CharOffset[Offset]))
        {
          MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
          return Stmt->Error.ReturnValue;
        }

        /* check total length: if not enough space, we need to calculate new CharOffset for next fetch */
        if (Stmt->stmt->fields[Offset].max_length)
        {
          size_t ReqBuffOctetLen;
          /* Size in chars */
          CharLength= MbstrCharLen(ClientValue, Stmt->stmt->fields[Offset].max_length - Stmt->CharOffset[Offset],
            Stmt->Connection->Charset.cs_info);
          /* MbstrCharLen gave us length in characters. For encoding of each character we might need
             2 SQLWCHARs in case of UTF16, or 1 SQLWCHAR in case of UTF32. Probably we need calcualate better
             number of required SQLWCHARs */
          ReqBuffOctetLen= (CharLength + 1)*(4/sizeof(SQLWCHAR))*sizeof(SQLWCHAR);

          if (BufferLength)
          {
            /* Buffer is not big enough. Alocating InternalBuffer.
               MADB_SetString would do that anyway if - allocate buffer fitting the whole wide string,
               and then copied its part to the application's buffer */
            if (ReqBuffOctetLen > (size_t)BufferLength)
            {
              IrdRec->InternalBuffer= (char*)MADB_CALLOC(ReqBuffOctetLen);

              if (IrdRec->InternalBuffer == 0)
              {
                MADB_FREE(ClientValue);
                return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
              }

              CharLength= MADB_SetString(&Stmt->Connection->Charset, IrdRec->InternalBuffer, (SQLINTEGER)ReqBuffOctetLen / sizeof(SQLWCHAR),
                ClientValue, Stmt->stmt->fields[Offset].max_length - Stmt->CharOffset[Offset], &Stmt->Error);
            }
            else
            {
              /* Application's buffer is big enough - writing directly there */
              CharLength= MADB_SetString(&Stmt->Connection->Charset, TargetValuePtr, (SQLINTEGER)(BufferLength / sizeof(SQLWCHAR)),
                ClientValue, Stmt->stmt->fields[Offset].max_length - Stmt->CharOffset[Offset], &Stmt->Error);
            }

            if (!SQL_SUCCEEDED(Stmt->Error.ReturnValue))
            {
              MADB_FREE(ClientValue);
              MADB_FREE(IrdRec->InternalBuffer);

              return Stmt->Error.ReturnValue;
            }
          }

          if (!Stmt->CharOffset[Offset])
          {
            Stmt->Lengths[Offset]= (unsigned long)(CharLength*sizeof(SQLWCHAR));
          }
        }
        else if (BufferLength >= sizeof(SQLWCHAR))
        {
          *(SQLWCHAR*)TargetValuePtr= 0;
        }
      }
      else  /* IrdRec->InternalBuffer == NULL && Stmt->Lengths[Offset] == 0 */
      {
        CharLength= SqlwcsLen((SQLWCHAR*)((char*)IrdRec->InternalBuffer + Stmt->CharOffset[Offset]), -1);
      }

      if (StrLen_or_IndPtr)
      {
        *StrLen_or_IndPtr= CharLength * sizeof(SQLWCHAR);
      }

      if (!BufferLength)
      {
        MADB_FREE(ClientValue);

        return MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
      }

      if (IrdRec->InternalBuffer)
      {
        /* If we have more place than only for the TN */
        if (BufferLength > sizeof(SQLWCHAR))
        {
          memcpy(TargetValuePtr, (char*)IrdRec->InternalBuffer + Stmt->CharOffset[Offset],
            MIN(BufferLength - sizeof(SQLWCHAR), CharLength*sizeof(SQLWCHAR)));
        }
        /* Terminating Null */
        *(SQLWCHAR*)((char*)TargetValuePtr + MIN(BufferLength - sizeof(SQLWCHAR), CharLength*sizeof(SQLWCHAR)))= 0;
      }

      if (CharLength >= BufferLength / sizeof(SQLWCHAR))
      {
        /* Calculate new offset and substract 1 byte for null termination */
        Stmt->CharOffset[Offset]+= (unsigned long)BufferLength - sizeof(SQLWCHAR);
        MADB_FREE(ClientValue);

        return MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
      }
      else
      {
        Stmt->CharOffset[Offset]= Stmt->Lengths[Offset];
        MADB_FREE(IrdRec->InternalBuffer);
      }

      MADB_FREE(ClientValue);
    }
    break;
  case SQL_CHAR:
  case SQL_VARCHAR:
    if (Stmt->stmt->fields[Offset].type == MYSQL_TYPE_BLOB &&
        Stmt->stmt->fields[Offset].charsetnr == 63)
    {
      if (!BufferLength && StrLen_or_IndPtr)
      {
        *StrLen_or_IndPtr= Stmt->stmt->fields[Offset].max_length * 2;
        return SQL_SUCCESS_WITH_INFO;
      }
     
#ifdef CONVERSION_TO_HEX_IMPLEMENTED
      {
        /*TODO: */
        char *TmpBuffer;
        if (!(TmpBuffer= (char *)MADB_CALLOC(BufferLength)))
        {

        }
      }
#endif
    }
    ZeroTerminated= 1;

  case SQL_LONGVARCHAR:
  case SQL_BINARY:
  case SQL_VARBINARY:
  case SQL_LONGVARBINARY:
    {
      Bind.buffer=        TargetValuePtr;
      Bind.buffer_length= (unsigned long)BufferLength;
      Bind.buffer_type=   MadbType;

      if (!(BufferLength) && StrLen_or_IndPtr)
      {
        /* Paranoid - before StrLen_or_IndPtr was used as length directly. so leaving same value in Bind.length. Unlikely needed */
        Bind.length_value= (unsigned long)*StrLen_or_IndPtr;
        Bind.length=       &Bind.length_value;

        mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, Stmt->CharOffset[Offset]);
        
        if (InternalUse) 
        {
          *StrLen_or_IndPtr= MIN(*Bind.length, Stmt->stmt->fields[Offset].max_length);
        }
        else
        {
          if (!Stmt->CharOffset[Offset])
          {
            Stmt->Lengths[Offset]= MIN(*Bind.length, Stmt->stmt->fields[Offset].max_length);
          }
          *StrLen_or_IndPtr= Stmt->Lengths[Offset] - Stmt->CharOffset[Offset];
        }
        
        MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);

        return SQL_SUCCESS_WITH_INFO;
      }
      
      if (mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, CurrentOffset))
      {
        MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
        return Stmt->Error.ReturnValue;
      }
      /* Dirty temporary hack before we know what is going on. Yes, there is nothing more eternal, than temporary
         It's not that bad, after all */
      if ((long)*Bind.length == -1)
      {
        *Bind.length= 0;
      }
      /* end of dirty hack */

      if (!InternalUse && !Stmt->CharOffset[Offset])
      {
        Stmt->Lengths[Offset]= MIN(*Bind.length, Stmt->stmt->fields[Offset].max_length);
      }
      if (ZeroTerminated)
      {
        char *p= (char *)Bind.buffer;
        if (BufferLength > (SQLLEN)*Bind.length)
        {
          p[*Bind.length]= 0;
        }
        else
        {
          p[BufferLength-1]= 0;
        }
      }
      
      if (StrLen_or_IndPtr)
      {
        *StrLen_or_IndPtr= *Bind.length - CurrentOffset;
      }
      if (InternalUse == FALSE)
      {
        /* Recording new offset only if that is API call, and not getting data for internal use */
        Stmt->CharOffset[Offset]+= MIN((unsigned long)BufferLength - ZeroTerminated, *Bind.length);
        if ((BufferLength - ZeroTerminated) && Stmt->Lengths[Offset] > Stmt->CharOffset[Offset])
        {
          MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
          return Stmt->Error.ReturnValue;
        }
      }

      if (StrLen_or_IndPtr && BufferLength - ZeroTerminated < *StrLen_or_IndPtr)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
        return SQL_SUCCESS_WITH_INFO;
      }
    }
    break;
  case SQL_NUMERIC:
  {
    SQLRETURN rc;
    char *tmp;
    MADB_DescRecord *Ard= MADB_DescGetInternalRecord(Stmt->Ard, Offset, MADB_DESC_READ);

    Bind.buffer_length= MADB_DEFAULT_PRECISION + 1/*-*/ + 1/*.*/;
    tmp=                (char *)MADB_CALLOC(Bind.buffer_length);
    Bind.buffer=        tmp;

    Bind.buffer_type=   MadbType;

    mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);

    MADB_CLEAR_ERROR(&Stmt->Error);

    if (Bind.buffer_length < Stmt->stmt->fields[Offset].max_length)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_22003, NULL, 0);
      MADB_FREE(tmp);
      return Stmt->Error.ReturnValue;
    }

    rc= MADB_CharToSQLNumeric(tmp, Stmt->Ard, Ard, TargetValuePtr, 0);

    /* Ugly */
    if (rc != SQL_SUCCESS)
    {
      MADB_SetError(&Stmt->Error, rc, NULL, 0);
      if (rc == SQL_ERROR)
      {
        return SQL_ERROR;
      }
    }

    if (StrLen_or_IndPtr != NULL)
    {
      *StrLen_or_IndPtr= sizeof(SQL_NUMERIC_STRUCT);
    }
    break;
  }
  default:
    {
       /* Set the conversion function */
      Bind.buffer_type= MadbType;
      Bind.buffer= TargetValuePtr;
      if (Bind.buffer_length == 0 && BufferLength > 0)
      {
        Bind.buffer_length= (unsigned long)BufferLength;
      }
      mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);

      if (StrLen_or_IndPtr != NULL)
      {
        /* We get here only for fixed data types. Thus, according to the specs 
           "this is the length of the data after conversion; that is, it is the size of the type to which the data was converted".
           For us that is the size of the buffer in bind structure. Not the size of the field */

        *StrLen_or_IndPtr= Bind.buffer_length;

        /* Paranoid - it was here, so leaving it in place */
        if ((long)Bind.length_value == -1)
        {
          Bind.length_value= 0;
        }
        /* We do this for catalog functions and MS Access in first turn. The thing is that for some columns in catalog functions result,
           we fix column type manually, since we can't make field of desired type in the query to I_S. Mostly that is for SQLSMALLINT
           fields, and we can cast only to int, not to short. MSAccess in its turn like to to get length for fixed length types, and
           throws error if the length is not what it expected (ODBC-131)
           Probably it makes sense to do this only for SQL_C_DEFAULT type, which MS Access uses. But atm it looks like this should
           not hurt if done for other types, too */
        if (*StrLen_or_IndPtr == 0 || (Bind.length_value > (unsigned long)IrdRec->OctetLength && *StrLen_or_IndPtr > IrdRec->OctetLength))
        {
          *StrLen_or_IndPtr= IrdRec->OctetLength;
        }
      }
    }
  }             /* End of switch(OdbcType) */

  /* Marking fixed length fields to be able to return SQL_NO_DATA on subsequent calls, as standard prescribes
     "SQLGetData cannot be used to return fixed-length data in parts. If SQLGetData is called more than one time
      in a row for a column containing fixed-length data, it returns SQL_NO_DATA for all calls after the first."
     Stmt->Lengths[Offset] would be set for variable length types */
  if (!InternalUse && Stmt->Lengths[Offset] == 0)
  {
    Stmt->CharOffset[Offset]= MAX((unsigned long)Bind.buffer_length, Bind.length_value);
  }

  if (IsNull)
  {
    if (!StrLen_or_IndPtr)
    {
      return MADB_SetError(&Stmt->Error, MADB_ERR_22002, NULL, 0);
    }
    *StrLen_or_IndPtr= SQL_NULL_DATA;
  }

  return Stmt->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_StmtRowCount */
SQLRETURN MADB_StmtRowCount(MADB_Stmt *Stmt, SQLLEN *RowCountPtr)
{
  if (Stmt->AffectedRows != -1)
    *RowCountPtr= (SQLLEN)Stmt->AffectedRows;
  else if (Stmt->stmt && Stmt->stmt->result.rows && mysql_stmt_field_count(Stmt->stmt))
    *RowCountPtr= (SQLLEN)mysql_stmt_num_rows(Stmt->stmt);
  else
    *RowCountPtr= 0;
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MapColAttributesDescType */
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
/* }}} */

/* {{{ MADB_StmtRowCount */
SQLRETURN MADB_StmtParamCount(MADB_Stmt *Stmt, SQLSMALLINT *ParamCountPtr)
{
  *ParamCountPtr= (SQLSMALLINT)mysql_stmt_param_count(Stmt->stmt);
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_StmtColumnCount */
SQLRETURN MADB_StmtColumnCount(MADB_Stmt *Stmt, SQLSMALLINT *ColumnCountPtr)
{
  /* We supposed to have that data in the descriptor by now. No sense to ask C/C API one more time for that */
  *ColumnCountPtr= (SQLSMALLINT)MADB_STMT_COLUMN_COUNT(Stmt);
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_StmtColAttr */
SQLRETURN MADB_StmtColAttr(MADB_Stmt *Stmt, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttributePtr,
             SQLSMALLINT BufferLength, SQLSMALLINT *StringLengthPtr, SQLLEN *NumericAttributePtr, my_bool IsWchar)
{
  MADB_DescRecord *Record;
  SQLSMALLINT     StringLength=     0;
  SQLLEN          NumericAttribute;
  BOOL            IsNumericAttr=    TRUE;

  if (!Stmt)
    return SQL_INVALID_HANDLE;
  
  MADB_CLEAR_ERROR(&Stmt->Error);

  if (StringLengthPtr)
    *StringLengthPtr= 0;

  if (!Stmt->stmt || !mysql_stmt_field_count(Stmt->stmt))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07005, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  if (ColumnNumber < 1 || ColumnNumber > mysql_stmt_field_count(Stmt->stmt))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07009, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  /* We start at offset zero */
  --ColumnNumber;

  if (!(Record= MADB_DescGetInternalRecord(Stmt->Ird, ColumnNumber, MADB_DESC_READ)))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07009, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  /* Mapping ODBC2 attributes to ODBC3
     TODO: it looks like it takes more than that 
     "In ODBC 3.x driver must support SQL_COLUMN_PRECISION and SQL_DESC_PRECISION, SQL_COLUMN_SCALE and SQL_DESC_SCALE,
     and SQL_COLUMN_LENGTH and SQL_DESC_LENGTH. These values are different because precision, scale, and length are defined
     differently in ODBC 3.x than they were in ODBC 2.x."
     */
  FieldIdentifier= MapColAttributeDescType(FieldIdentifier);

  switch(FieldIdentifier) {
  case SQL_DESC_AUTO_UNIQUE_VALUE:
    NumericAttribute= (SQLLEN)Record->AutoUniqueValue;
    break;
  case SQL_DESC_BASE_COLUMN_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : NULL,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->BaseColumnName, strlen(Record->BaseColumnName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_BASE_TABLE_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : NULL,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->BaseTableName, strlen(Record->BaseTableName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_CASE_SENSITIVE:
    NumericAttribute= (SQLLEN)Record->CaseSensitive;
    break;
  case SQL_DESC_CATALOG_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->CatalogName, strlen(Record->CatalogName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_SCHEMA_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              "", 0, &Stmt->Error);
    IsNumericAttr= FALSE;
  case SQL_DESC_CONCISE_TYPE:
    NumericAttribute= (SQLLEN)Record->ConciseType;
    break;
  case SQL_DESC_SEARCHABLE:
    NumericAttribute= (SQLLEN)Record->Searchable;
    break;
  case SQL_DESC_COUNT:
    NumericAttribute= (SQLLEN)Stmt->Ird->Header.Count;
    break;
  case SQL_DESC_DISPLAY_SIZE:
    NumericAttribute= (SQLLEN)Record->DisplaySize;
    break;
  case SQL_DESC_FIXED_PREC_SCALE:
    NumericAttribute= (SQLLEN)Record->FixedPrecScale;
    break;
  case SQL_DESC_PRECISION:
    NumericAttribute= (SQLLEN)Record->Precision;
    break;
  case SQL_DESC_LENGTH:
    NumericAttribute= (SQLLEN)Record->Length;
    break;
  case SQL_DESC_LITERAL_PREFIX:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->LiteralPrefix, strlen(Record->LiteralPrefix), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_LITERAL_SUFFIX:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->LiteralSuffix, strlen(Record->LiteralSuffix), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_LOCAL_TYPE_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              "", 0, &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_LABEL:
  case SQL_DESC_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->ColumnName, strlen(Record->ColumnName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_TYPE_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->TypeName, strlen(Record->TypeName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_NULLABLE:
    NumericAttribute= Record->Nullable;
    break;
  case SQL_DESC_UNNAMED:
    NumericAttribute= Record->Unnamed;
    break;
  case SQL_DESC_UNSIGNED:
    NumericAttribute= Record->Unsigned;
    break;
  case SQL_DESC_UPDATABLE:
    NumericAttribute= Record->Updateable;
    break;
  case SQL_DESC_OCTET_LENGTH:
    NumericAttribute= Record->OctetLength;
    break;
  case SQL_DESC_SCALE:
    NumericAttribute= Record->Scale;
    break;
  case SQL_DESC_TABLE_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->Charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->TableName, strlen(Record->TableName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_TYPE:
    NumericAttribute= Record->Type;
    break;
  case SQL_COLUMN_COUNT:
    NumericAttribute= mysql_stmt_field_count(Stmt->stmt);
    break;
  default:
    MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, NULL, 0);
    return Stmt->Error.ReturnValue;
  }
  /* We need to return the number of bytes, not characters! */
  if (StringLength)
  {
    if (StringLengthPtr)
      *StringLengthPtr= (SQLSMALLINT)StringLength;
    if (!BufferLength && CharacterAttributePtr)
      MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
  }
  /* We shouldn't touch application memory without purpose, writing garbage there. Thus IsNumericAttr.
     Besides .Net was quite disappointed about that */
  if (NumericAttributePtr && IsNumericAttr == TRUE)
    *NumericAttributePtr= NumericAttribute;
  if (StringLengthPtr && IsWchar)
    *StringLengthPtr*= sizeof(SQLWCHAR);
  return Stmt->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_StmtColumnPrivileges */
SQLRETURN MADB_StmtColumnPrivileges(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                                    char *SchemaName, SQLSMALLINT NameLength2, char *TableName,
                                    SQLSMALLINT NameLength3, char *ColumnName, SQLSMALLINT NameLength4)
{
  char StmtStr[1024];
  char *p;

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* TableName is mandatory */
  if (!TableName || !NameLength3)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }

  p= StmtStr;
  p+= _snprintf(StmtStr, 1024, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL as TABLE_SCHEM, TABLE_NAME,"
                                 "COLUMN_NAME, NULL AS GRANTOR, GRANTEE, PRIVILEGE_TYPE AS PRIVILEGE,"
                                 "IS_GRANTABLE FROM INFORMATION_SCHEMA.COLUMN_PRIVILEGES WHERE ");
  if (CatalogName && CatalogName[0])
    p+= _snprintf(p, 1024 - strlen(StmtStr), "TABLE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= _snprintf(p, 1024 - strlen(StmtStr), "TABLE_SCHEMA LIKE DATABASE() ");
  if (TableName && TableName[0])
    p+= _snprintf(p, 1024 - strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);
  if (ColumnName && ColumnName[0])
    p+= _snprintf(p, 1024 - strlen(StmtStr), "AND COLUMN_NAME LIKE '%s' ", ColumnName);

   p+= _snprintf(p, 1024 - strlen(StmtStr), "ORDER BY TABLE_SCHEM, TABLE_NAME, COLUMN_NAME, PRIVILEGE");
  
  return Stmt->Methods->ExecDirect(Stmt, StmtStr, (SQLINTEGER)strlen(StmtStr));
}
/* }}} */

/* {{{ MADB_StmtTablePrivileges */
SQLRETURN MADB_StmtTablePrivileges(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                                    char *SchemaName, SQLSMALLINT NameLength2,
                                    char *TableName, SQLSMALLINT NameLength3)
{
  char StmtStr[1024],
       *p;

  MADB_CLEAR_ERROR(&Stmt->Error);

  p= StmtStr;
  p += _snprintf(StmtStr, 1024, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, TABLE_NAME, "
                                  "NULL AS GRANTOR, GRANTEE, PRIVILEGE_TYPE AS PRIVILEGE, IS_GRANTABLE "
                                  "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES WHERE ");
  if (CatalogName)
    p+= _snprintf(p, 1024 - strlen(StmtStr), "TABLE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= _snprintf(p, 1024 - strlen(StmtStr), "TABLE_SCHEMA LIKE IF(DATABASE(), DATABASE(), '%%') ");
  if (TableName)
    p+= _snprintf(p, 1024 - strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);
  
  p+= _snprintf(p, 1024 - strlen(StmtStr), "ORDER BY TABLE_SCHEM, TABLE_NAME, PRIVILEGE");
  
  return Stmt->Methods->ExecDirect(Stmt, StmtStr, (SQLINTEGER)strlen(StmtStr));
}
/* }}} */

/* {{{ MADB_StmtTables */
SQLRETURN MADB_StmtTables(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT CatalogNameLength,
                          char *SchemaName, SQLSMALLINT SchemaNameLength, char *TableName,
                          SQLSMALLINT TableNameLength, char *TableType, SQLSMALLINT TableTypeLength)
{
  MADB_DynString StmtStr;
  char Quote[2];
  SQLRETURN ret;

  /*
  METADATA_ID       CatalogName     SchemaName       TableName           TableType
  ---------------------------------------------------------------------------------
  ODBC_V3:
  SQL_FALSE         Pattern         Pattern          Pattern             ValueList
  SQL_TRUE          Identifier      Identifier       Identifier          ValueList
  ODBC_V2:
                    Identifier      Identifier       Identifier          ValueList
  --------------------------------------------------------------------------------
  */

  MDBUG_C_ENTER(Stmt->Connection, "MADB_StmtTables");

  ADJUST_LENGTH(CatalogName, CatalogNameLength);
  ADJUST_LENGTH(SchemaName, SchemaNameLength);
  ADJUST_LENGTH(TableName, TableNameLength);
  ADJUST_LENGTH(TableType, TableTypeLength);

  if (CatalogNameLength > 64 || TableNameLength > 64)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY090, "Table and catalog names are limited to 64 chars", 0);
    return Stmt->Error.ReturnValue;
  }

  /* SQL_ALL_CATALOGS 
     If CatalogName is SQL_ALL_CATALOGS and SchemaName and TableName are empty strings, 
     the result set contains a list of valid catalogs for the data source. 
     (All columns except the TABLE_CAT column contain NULLs
  */
  if (CatalogName && CatalogNameLength && TableName != NULL && !TableNameLength &&
    SchemaName != NULL && SchemaNameLength == 0 && !strcmp(CatalogName, SQL_ALL_CATALOGS))
  {
    MADB_InitDynamicString(&StmtStr, "SELECT SCHEMA_NAME AS TABLE_CAT, CONVERT(NULL,CHAR(64)) AS TABLE_SCHEM, "
                                  "CONVERT(NULL,CHAR(64)) AS TABLE_NAME, NULL AS TABLE_TYPE, NULL AS REMARKS "
                                  "FROM INFORMATION_SCHEMA.SCHEMATA "
                                  "GROUP BY SCHEMA_NAME ORDER BY SCHEMA_NAME",
                                  8192, 512);
  }
  /* SQL_ALL_TABLE_TYPES
     If TableType is SQL_ALL_TABLE_TYPES and CatalogName, SchemaName, and TableName are empty strings, 
     the result set contains a list of valid table types for the data source. 
     (All columns except the TABLE_TYPE column contain NULLs.)
  */
  else if (CatalogName != NULL && !CatalogNameLength && TableName != NULL && !TableNameLength &&
    SchemaName != NULL && SchemaNameLength == 0 && TableType && TableTypeLength &&
            !strcmp(TableType, SQL_ALL_TABLE_TYPES))
  {
    MADB_InitDynamicString(&StmtStr, "SELECT NULL AS TABLE_CAT, NULL AS TABLE_SCHEM, "
                                  "NULL AS TABLE_NAME, 'TABLE' AS TABLE_TYPE, NULL AS REMARKS "
                                  "FROM DUAL "
                                  "UNION "
                                  "SELECT NULL, NULL, NULL, 'VIEW', NULL FROM DUAL "
                                  "UNION "
                                  "SELECT NULL, NULL, NULL, 'SYSTEM VIEW', NULL FROM DUAL",
                                  8192, 512); 
  }
  /* Since we treat our databases as catalogs, the only acceptable value for schema is NULL or "%"
     if that is not the special case of call for schemas list. Otherwise we return empty resultset*/
  else if (SchemaName &&
    ((!strcmp(SchemaName,SQL_ALL_SCHEMAS) && CatalogName && CatalogNameLength == 0 && TableName && TableNameLength == 0) ||
      strcmp(SchemaName, SQL_ALL_SCHEMAS)))
  {
    MADB_InitDynamicString(&StmtStr, "SELECT NULL AS TABLE_CAT, NULL AS TABLE_SCHEM, "
      "NULL AS TABLE_NAME, NULL AS TABLE_TYPE, NULL AS REMARKS "
      "FROM DUAL WHERE 1=0", 8192, 512);
  }
  else
  {
    MADB_InitDynamicString(&StmtStr, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, TABLE_NAME, "
                                  "if(TABLE_TYPE='BASE TABLE','TABLE',TABLE_TYPE) AS TABLE_TYPE ,"
                                  "TABLE_COMMENT AS REMARKS FROM INFORMATION_SCHEMA.TABLES WHERE 1=1 ",
                                  8192, 512);
    if (Stmt->Options.MetadataId== SQL_TRUE)
    {
      strcpy(Quote, "`");
    }
    else
    {
      strcpy(Quote, "'");
    }

    if (CatalogName)
    {
      MADB_DynstrAppend(&StmtStr, " AND TABLE_SCHEMA ");
      MADB_DynstrAppend(&StmtStr, "LIKE ");
      MADB_DynstrAppend(&StmtStr, Quote);
      MADB_DynstrAppend(&StmtStr, CatalogName);
      MADB_DynstrAppend(&StmtStr, Quote);
    }

    if (TableName && TableNameLength)
    {
      MADB_DynstrAppend(&StmtStr, " AND TABLE_NAME LIKE ");
      MADB_DynstrAppend(&StmtStr, Quote);
      MADB_DynstrAppend(&StmtStr, TableName);
      MADB_DynstrAppend(&StmtStr, Quote);
    }
    if (TableType && TableTypeLength && strcmp(TableType, SQL_ALL_TABLE_TYPES) != 0)
    {
      unsigned int i;
      char *myTypes[3]= {"TABLE", "VIEW", "SYNONYM"};
      MADB_DynstrAppend(&StmtStr, " AND TABLE_TYPE IN (''");
      for (i= 0; i < 3; i++)
      {
        if (strstr(TableType, myTypes[i]))
        {
          if (strstr(myTypes[i], "TABLE"))
            MADB_DynstrAppend(&StmtStr, ", 'BASE TABLE'");
          else
          {
            MADB_DynstrAppend(&StmtStr, ", '");
            MADB_DynstrAppend(&StmtStr, myTypes[i]);
            MADB_DynstrAppend(&StmtStr, "'");
          }
        }
      }
      MADB_DynstrAppend(&StmtStr, ") ");
    }
    MADB_DynstrAppend(&StmtStr, " ORDER BY TABLE_SCHEMA, TABLE_NAME, TABLE_TYPE");
  }
  MDBUG_C_PRINT(Stmt->Connection, "SQL Statement: %s", StmtStr.str);

  ret= Stmt->Methods->ExecDirect(Stmt, StmtStr.str, SQL_NTS);

  MADB_DynstrFree(&StmtStr);

  MDBUG_C_RETURN(Stmt->Connection, ret, &Stmt->Error);
}
/* }}} */

static MADB_ShortTypeInfo SqlStatsColType[13]=
                               /*1*/    {{SQL_VARCHAR, 0, SQL_NULLABLE, 0}, {SQL_VARCHAR, 0, SQL_NULLABLE, 0}, {SQL_VARCHAR, 0, SQL_NO_NULLS, 0}, {SQL_SMALLINT, 0, SQL_NULLABLE, 0},
                               /*5*/     {SQL_VARCHAR, 0, SQL_NULLABLE, 0}, {SQL_VARCHAR, 0, SQL_NULLABLE, 0}, {SQL_SMALLINT, 0, SQL_NO_NULLS, 0}, {SQL_SMALLINT, 0, SQL_NULLABLE, 0},
                               /*9*/     {SQL_VARCHAR, 0, SQL_NULLABLE, 0}, {SQL_CHAR, 0, SQL_NULLABLE, 2}, {SQL_INTEGER, 0, SQL_NULLABLE, 0}, {SQL_INTEGER, 0, SQL_NULLABLE, 0},
                               /*13*/    {SQL_VARCHAR, 0, SQL_NULLABLE, 0}};

/* {{{ MADB_StmtStatistics */
SQLRETURN MADB_StmtStatistics(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                              char *SchemaName, SQLSMALLINT NameLength2,
                              char *TableName, SQLSMALLINT NameLength3,
                              SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
  char StmtStr[1024];
  char *p= StmtStr;
  
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* TableName is mandatory */
  if (!TableName || !NameLength3)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }
 
  _snprintf(StmtStr, 1024, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, TABLE_NAME, "
                             "NON_UNIQUE, NULL AS INDEX_QUALIFIER, INDEX_NAME, "
                             "%d AS TYPE, "
                             "SEQ_IN_INDEX AS ORDINAL_POSITION, COLUMN_NAME, COLLATION AS ASC_OR_DESC, "
                             "CARDINALITY, NULL AS PAGES, NULL AS FILTER_CONDITION "
                             "FROM INFORMATION_SCHEMA.STATISTICS ",
                             SQL_INDEX_OTHER);
  p+= strlen(StmtStr);
  if (CatalogName && CatalogName[0])
    p+= _snprintf(p, 1023 - strlen(StmtStr), "WHERE TABLE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= _snprintf(p, 1023 - strlen(StmtStr), "WHERE TABLE_SCHEMA LIKE IF(DATABASE() IS NOT NULL, DATABASE(), '%%') ");
  
  if (TableName)
    p+= _snprintf(p, 1023- strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);

  if (Unique == SQL_INDEX_UNIQUE)
    p+= _snprintf(p, 1023 - strlen(StmtStr), "AND NON_UNIQUE=0 ");

  _snprintf(p, 1023 - strlen(StmtStr), "ORDER BY NON_UNIQUE, INDEX_NAME, ORDINAL_POSITION");


  ret= Stmt->Methods->ExecDirect(Stmt, StmtStr, SQL_NTS);

  if (SQL_SUCCEEDED(ret))
  {
    MADB_FixColumnDataTypes(Stmt, SqlStatsColType);
  }
  return ret;
}
/* }}} */


static MADB_ShortTypeInfo SqlColumnsColType[18]=
/*1*/    {{SQL_VARCHAR, 0, SQL_NO_NULLS, 0}, {SQL_VARCHAR, 0, SQL_NO_NULLS, 0}, {SQL_VARCHAR, 0, SQL_NULLABLE, 0}, {SQL_VARCHAR, 0, SQL_NULLABLE, 0},
/*5*/     {SQL_SMALLINT, 0, SQL_NO_NULLS, 0}, {SQL_VARCHAR, 0, SQL_NO_NULLS, 0}, {SQL_INTEGER, 0, SQL_NULLABLE, 0}, {SQL_INTEGER, 0, SQL_NULLABLE, 0},
/*9*/     {SQL_SMALLINT, 0, SQL_NULLABLE, 0}, {SQL_SMALLINT, 0, SQL_NULLABLE, 0}, {SQL_SMALLINT, 0, SQL_NO_NULLS, 0}, {SQL_VARCHAR, 0, SQL_NULLABLE, 0},
/*13*/    {SQL_VARCHAR, 0, SQL_NULLABLE, 0}, {SQL_SMALLINT, 0, SQL_NO_NULLS, 0}, {SQL_SMALLINT, 0, SQL_NULLABLE, 0},
/*16*/    {SQL_INTEGER, 0, SQL_NULLABLE, 0}, {SQL_INTEGER, 0, SQL_NO_NULLS, 0}, {SQL_VARCHAR, 0, SQL_NULLABLE, 0}};

/* {{{ MADB_StmtColumns */
SQLRETURN MADB_StmtColumns(MADB_Stmt *Stmt,
                           char *CatalogName, SQLSMALLINT NameLength1,
                           char *SchemaName,  SQLSMALLINT NameLength2,
                           char *TableName,   SQLSMALLINT NameLength3,
                           char *ColumnName,  SQLSMALLINT NameLength4)
{
  MADB_DynString StmtStr;
  SQLRETURN ret;
  size_t Length= strlen(MADB_CATALOG_COLUMNSp3);
  char *ColumnsPart= MADB_CALLOC(Length);
  unsigned int OctetsPerChar= Stmt->Connection->Charset.cs_info->char_maxlen > 0 && Stmt->Connection->Charset.cs_info->char_maxlen < 10 ? Stmt->Connection->Charset.cs_info->char_maxlen : 1;

  MDBUG_C_ENTER(Stmt->Connection, "StmtColumns");

  if (ColumnsPart == NULL)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }

  _snprintf(ColumnsPart, Length, MADB_CATALOG_COLUMNSp3, OctetsPerChar);

  MADB_InitDynamicString(&StmtStr, "", 8192, 1024);
 
  MADB_CLEAR_ERROR(&Stmt->Error);
  if (MADB_DynstrAppend(&StmtStr, MADB_CATALOG_COLUMNSp1))
    goto dynerror;
  if (MADB_DynstrAppend(&StmtStr, MADB_SQL_DATATYPE(Stmt)))
    goto dynerror;
  if (MADB_DynstrAppend(&StmtStr, ColumnsPart))
    goto dynerror;
  if (MADB_DynstrAppend(&StmtStr, MADB_DEFAULT_COLUMN(Stmt->Connection)))
    goto dynerror;

  if (MADB_DynstrAppend(&StmtStr, MADB_CATALOG_COLUMNSp4))
    goto dynerror;

  ADJUST_LENGTH(CatalogName, NameLength1);
  ADJUST_LENGTH(SchemaName, NameLength2);
  ADJUST_LENGTH(TableName, NameLength3);
  ADJUST_LENGTH(ColumnName, NameLength4);

  if(MADB_DynstrAppend(&StmtStr, "TABLE_SCHEMA = "))
    goto dynerror;

  if (CatalogName)
  {
    if (MADB_DynstrAppend(&StmtStr, "'") ||
        MADB_DynstrAppendMem(&StmtStr, CatalogName, NameLength1) ||
        MADB_DynstrAppend(&StmtStr, "' "))
      goto dynerror;
  }
  else
    if (MADB_DynstrAppend(&StmtStr, "DATABASE()"))
      goto dynerror;

  if (TableName && NameLength3)
    if (MADB_DynstrAppend(&StmtStr, "AND TABLE_NAME LIKE '") ||
        MADB_DynstrAppendMem(&StmtStr, TableName, NameLength3) ||
        MADB_DynstrAppend(&StmtStr, "' "))
    goto dynerror;

  if (ColumnName && NameLength4)
    if (MADB_DynstrAppend(&StmtStr, "AND COLUMN_NAME LIKE '") ||
        MADB_DynstrAppendMem(&StmtStr, ColumnName, NameLength4) ||
        MADB_DynstrAppend(&StmtStr, "' "))
    goto dynerror;

  if (MADB_DynstrAppend(&StmtStr, " ORDER BY TABLE_SCHEMA, TABLE_NAME, ORDINAL_POSITION"))
    goto dynerror;

  MDBUG_C_DUMP(Stmt->Connection, StmtStr.str, s);

  ret= Stmt->Methods->ExecDirect(Stmt, StmtStr.str, SQL_NTS);

  if (SQL_SUCCEEDED(ret))
  {
    MADB_FixColumnDataTypes(Stmt, SqlColumnsColType);
  }

  MADB_FREE(ColumnsPart);
  MADB_DynstrFree(&StmtStr);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);

  return ret;

dynerror:
  MADB_FREE(ColumnsPart);
  MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  return Stmt->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_StmtProcedureColumns */
SQLRETURN MADB_StmtProcedureColumns(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                                char *SchemaName, SQLSMALLINT NameLength2, char *ProcName,
                                SQLSMALLINT NameLength3, char *ColumnName, SQLSMALLINT NameLength4)
{
  char *StmtStr,
       *p;
  size_t Length= strlen(MADB_PROCEDURE_COLUMNS(Stmt)) + 1024;
  SQLRETURN ret;
  unsigned int OctetsPerChar= Stmt->Connection->Charset.cs_info->char_maxlen > 0 ? Stmt->Connection->Charset.cs_info->char_maxlen: 1;

  MADB_CLEAR_ERROR(&Stmt->Error);

  if (!(StmtStr= MADB_CALLOC(Length)))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }

  p= StmtStr;

  p+= _snprintf(p, Length, MADB_PROCEDURE_COLUMNS(Stmt), OctetsPerChar);
  
  if (CatalogName)
    p+= _snprintf(p, Length - strlen(StmtStr), "WHERE SPECIFIC_SCHEMA='%s' ", CatalogName);
  else
    p+= _snprintf(p, Length - strlen(StmtStr), "WHERE SPECIFIC_SCHEMA LIKE DATABASE() ");
  if (ProcName && ProcName[0])
    p+= _snprintf(p, Length - strlen(StmtStr), "AND SPECIFIC_NAME LIKE '%s' ", ProcName);
  if (ColumnName)
  {
    if (ColumnName[0])
    {
      p+= _snprintf(p, Length- strlen(StmtStr), "AND PARAMETER_NAME LIKE '%s' ", ColumnName);
    }
    else
    {
      p+= _snprintf(p, Length- strlen(StmtStr), "AND PARAMETER_NAME IS NULL ");
    }
  }
    
  p+= _snprintf(p, Length - strlen(StmtStr), " ORDER BY SPECIFIC_SCHEMA, SPECIFIC_NAME, ORDINAL_POSITION");

  ret= Stmt->Methods->ExecDirect(Stmt, StmtStr, SQL_NTS);

  MADB_FREE(StmtStr);

  return ret;
}
/* }}} */

/* {{{ MADB_StmtPrimaryKeys */
SQLRETURN MADB_StmtPrimaryKeys(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                                char *SchemaName, SQLSMALLINT NameLength2, char *TableName,
                                SQLSMALLINT NameLength3)
{
  char StmtStr[2048],
       *p;

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* TableName is mandatory */
  if (!TableName || !NameLength3)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }

  p= StmtStr;
  p+= _snprintf(p, 1024, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, "
                           "TABLE_NAME, COLUMN_NAME, ORDINAL_POSITION KEY_SEQ, "
                           "'PRIMARY' PK_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE "
                           "COLUMN_KEY = 'pri' AND ");
  if (CatalogName && CatalogName[0])
  {
    p+= _snprintf(p, 2048 - strlen(StmtStr), "TABLE_SCHEMA LIKE '%s' ", CatalogName);
  }
  else
  {
    p+= _snprintf(p, 2048 - strlen(StmtStr), "TABLE_SCHEMA LIKE IF(DATABASE() IS NOT NULL, DATABASE(), '%%') ");
  }
  if (TableName)
  {
    p+= _snprintf(p, 2048 - strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);
  }
  p+= _snprintf(p, 2048 - strlen(StmtStr), " ORDER BY TABLE_SCHEMA, TABLE_NAME, ORDINAL_POSITION");

  return Stmt->Methods->ExecDirect(Stmt, StmtStr, SQL_NTS);
}
/* }}} */

/* {{{ MADB_StmtSpecialColumns */
SQLRETURN MADB_StmtSpecialColumns(MADB_Stmt *Stmt, SQLUSMALLINT IdentifierType,
                                  char *CatalogName, SQLSMALLINT NameLength1, 
                                  char *SchemaName, SQLSMALLINT NameLength2,
                                  char *TableName, SQLSMALLINT NameLength3,
                                  SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
  char StmtStr[2048],
       *p= StmtStr;

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* TableName is mandatory */
  if (!TableName || !NameLength3)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }

  p+= _snprintf(p, 2048, "SELECT NULL AS SCOPE, COLUMN_NAME, %s,"
                           "DATA_TYPE TYPE_NAME,"
                           "CASE" 
                           "  WHEN DATA_TYPE in ('bit', 'tinyint', 'smallint', 'year', 'mediumint', 'int',"
                              "'bigint', 'decimal', 'float', 'double') THEN NUMERIC_PRECISION "
                           "  WHEN DATA_TYPE='date' THEN 10"
                           "  WHEN DATA_TYPE='time' THEN 8"
                           "  WHEN DATA_TYPE in ('timestamp', 'datetime') THEN 19 "
                           "END AS COLUMN_SIZE,"
                           "CHARACTER_OCTET_LENGTH AS BUFFER_LENGTH,"
                           "NUMERIC_SCALE DECIMAL_DIGITS, " 
                           XSTR(SQL_PC_UNKNOWN) " PSEUDO_COLUMN "
                           "FROM INFORMATION_SCHEMA.COLUMNS WHERE 1 ", MADB_SQL_DATATYPE(Stmt));

  if (CatalogName && CatalogName[0])
    p+= _snprintf(p, 2048 - strlen(StmtStr), "AND TABLE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= _snprintf(p, 2048 - strlen(StmtStr), "AND TABLE_SCHEMA LIKE IF(DATABASE() IS NOT NULL, DATABASE(), '%%') ");
  if (TableName && TableName[0])
    p+= _snprintf(p, 2048 - strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);

  if (Nullable == SQL_NO_NULLS)
    p+= _snprintf(p, 2048 - strlen(StmtStr), "AND IS_NULLABLE <> 'YES' ");

  if (IdentifierType == SQL_BEST_ROWID)
    p+= _snprintf(p, 2048 - strlen(StmtStr), "AND COLUMN_KEY IN ('PRI', 'UNI') ");
  else if (IdentifierType == SQL_ROWVER)
    p+= _snprintf(p, 2048 - strlen(StmtStr), "AND DATA_TYPE='timestamp' AND EXTRA LIKE '%%CURRENT_TIMESTAMP%%' ");
  p+= _snprintf(p, 2048 - strlen(StmtStr), "ORDER BY TABLE_SCHEMA, TABLE_NAME, COLUMN_KEY");

  return Stmt->Methods->ExecDirect(Stmt, StmtStr, SQL_NTS);
}
/* }}} */

/* {{{ MADB_StmtProcedures */
SQLRETURN MADB_StmtProcedures(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                              char *SchemaName, SQLSMALLINT NameLength2, char *ProcName,
                              SQLSMALLINT NameLength3)
{
   char StmtStr[2048],
       *p;

  MADB_CLEAR_ERROR(&Stmt->Error);

  p= StmtStr;

  p+= _snprintf(p, 2048, "SELECT ROUTINE_SCHEMA AS PROCEDURE_CAT, NULL AS PROCEDURE_SCHEM, "
                           "SPECIFIC_NAME PROCEDURE_NAME, NULL NUM_INPUT_PARAMS, "
                           "NULL NUM_OUTPUT_PARAMS, NULL NUM_RESULT_SETS, "
                           "ROUTINE_COMMENT REMARKS, "
                           "CASE ROUTINE_TYPE "
                           "  WHEN 'FUNCTION' THEN " XSTR(SQL_PT_FUNCTION)
                           "  WHEN 'PROCEDURE' THEN " XSTR(SQL_PT_PROCEDURE)
                           "  ELSE " XSTR(SQL_PT_UNKNOWN) " "
                           "END PROCEDURE_TYPE "
                           "FROM INFORMATION_SCHEMA.ROUTINES ");
  if (CatalogName && CatalogName[0])
    p+= _snprintf(p, 2048 - strlen(StmtStr), "WHERE ROUTINE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= _snprintf(p, 2048 - strlen(StmtStr), "WHERE ROUTINE_SCHEMA LIKE DATABASE() ");
  if (ProcName && ProcName[0])
    p+= _snprintf(p, 2048 - strlen(StmtStr), "AND SPECIFIC_NAME LIKE '%s' ", ProcName);
  
  p+= _snprintf(p, 2048 - strlen(StmtStr), " ORDER BY ROUTINE_SCHEMA, SPECIFIC_NAME");

  return Stmt->Methods->ExecDirect(Stmt, StmtStr, SQL_NTS);
}
/* }}} */

/* {{{ SQLForeignKeys */
SQLRETURN MADB_StmtForeignKeys(MADB_Stmt *Stmt, char *PKCatalogName, SQLSMALLINT NameLength1,
                               char *PKSchemaName, SQLSMALLINT NameLength2, char *PKTableName,
                               SQLSMALLINT NameLength3, char *FKCatalogName, SQLSMALLINT NameLength4,
                               char *FKSchemaName, SQLSMALLINT NameLength5,  char *FKTableName,
                               SQLSMALLINT NameLength6)
{
  SQLRETURN ret= SQL_ERROR;
  MADB_DynString StmtStr;
  char EscapeBuf[256];

  MADB_CLEAR_ERROR(&Stmt->Error);

  ADJUST_LENGTH(PKCatalogName, NameLength1);
  ADJUST_LENGTH(PKSchemaName, NameLength2);
  ADJUST_LENGTH(PKTableName, NameLength3);
  ADJUST_LENGTH(FKCatalogName, NameLength4);
  ADJUST_LENGTH(FKSchemaName, NameLength5);
  ADJUST_LENGTH(FKTableName, NameLength6);



  /* PKTableName and FKTableName are mandatory */
  if ((!PKTableName || !NameLength3) && (!FKTableName || !NameLength6))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "PKTableName or FKTableName are required", 0);
    return Stmt->Error.ReturnValue;
  }

  /* modified from JDBC driver */
  MADB_InitDynamicString(&StmtStr,
                      "SELECT A.REFERENCED_TABLE_SCHEMA PKTABLE_CAT, NULL PKTABLE_SCHEM, "
                      "A.REFERENCED_TABLE_NAME PKTABLE_NAME, " 
                      "A.REFERENCED_COLUMN_NAME PKCOLUMN_NAME, "
                      "A.TABLE_SCHEMA FKTABLE_CAT, NULL FKTABLE_SCHEM, "
                      "A.TABLE_NAME FKTABLE_NAME, A.COLUMN_NAME FKCOLUMN_NAME, "
                      "A.POSITION_IN_UNIQUE_CONSTRAINT KEY_SEQ, "
                      "CASE update_rule "
                      "  WHEN 'RESTRICT' THEN " XSTR(SQL_RESTRICT)
                    "  WHEN 'NO ACTION' THEN " XSTR(SQL_NO_ACTION)
                      "  WHEN 'CASCADE' THEN " XSTR(SQL_CASCADE)
                      "  WHEN 'SET NULL' THEN " XSTR(SQL_SET_NULL)
                      "  WHEN 'SET DEFAULT' THEN " XSTR(SQL_SET_DEFAULT) " "
                      "END UPDATE_RULE, "
                      "CASE DELETE_RULE" 
                      "  WHEN 'RESTRICT' THEN " XSTR(SQL_RESTRICT)
                      "  WHEN 'NO ACTION' THEN " XSTR(SQL_NO_ACTION)
                      "  WHEN 'CASCADE' THEN " XSTR(SQL_CASCADE)
                      "  WHEN 'SET NULL' THEN " XSTR(SQL_SET_NULL)
                      "  WHEN 'SET DEFAULT' THEN " XSTR(SQL_SET_DEFAULT) " "
                      " END DELETE_RULE,"
                      "A.CONSTRAINT_NAME FK_NAME, "
                      "'PRIMARY' PK_NAME,"
                      XSTR(SQL_NOT_DEFERRABLE) " AS DEFERRABILITY "
                      " FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE A"
                      " JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE B"
                      " ON (B.TABLE_SCHEMA = A.REFERENCED_TABLE_SCHEMA"
                      " AND B.TABLE_NAME = A.REFERENCED_TABLE_NAME"
                      " AND B.COLUMN_NAME = A.REFERENCED_COLUMN_NAME)"
                      " JOIN INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS RC"
                      " ON (RC.CONSTRAINT_NAME = A.CONSTRAINT_NAME"
                      " AND RC.TABLE_NAME = A.TABLE_NAME"
                      " AND RC.CONSTRAINT_SCHEMA = A.TABLE_SCHEMA)"
                      " WHERE B.CONSTRAINT_NAME= 'PRIMARY'",
                      8192, 128);

  if (PKTableName && PKTableName[0])
  {
    MADB_DynstrAppend(&StmtStr, " AND A.REFERENCED_TABLE_SCHEMA "); 

    if (PKCatalogName && PKCatalogName[0])
    {
      MADB_DynstrAppend(&StmtStr, "LIKE '");
      mysql_real_escape_string(Stmt->Connection->mariadb, EscapeBuf, PKCatalogName, MIN(NameLength1, 255));
      MADB_DynstrAppend(&StmtStr, EscapeBuf);
      MADB_DynstrAppend(&StmtStr, "' ");
    }
    else 
      MADB_DynstrAppend(&StmtStr, "= DATABASE()");
    MADB_DynstrAppend(&StmtStr, " AND A.REFERENCED_TABLE_NAME = '");

    mysql_real_escape_string(Stmt->Connection->mariadb, EscapeBuf, PKTableName, MIN(255, NameLength3));
    MADB_DynstrAppend(&StmtStr, EscapeBuf);
    MADB_DynstrAppend(&StmtStr, "' ");
  }

  if (FKTableName && FKTableName[0])
  {
    MADB_DynstrAppend(&StmtStr, " AND A.TABLE_SCHEMA = "); 

    if (FKCatalogName && FKCatalogName[0])
    {
      MADB_DynstrAppend(&StmtStr, "'");
      mysql_real_escape_string(Stmt->Connection->mariadb, EscapeBuf, FKCatalogName, MIN(NameLength4, 255));
      MADB_DynstrAppend(&StmtStr, EscapeBuf);
      MADB_DynstrAppend(&StmtStr, "' ");
    }
    else
      MADB_DynstrAppend(&StmtStr, "DATABASE() ");
    MADB_DynstrAppend(&StmtStr, " AND A.TABLE_NAME = '");

    mysql_real_escape_string(Stmt->Connection->mariadb, EscapeBuf, FKTableName, MIN(255, NameLength6));
    MADB_DynstrAppend(&StmtStr, EscapeBuf);
    MADB_DynstrAppend(&StmtStr, "' ");
  }
  MADB_DynstrAppend(&StmtStr, "ORDER BY FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, KEY_SEQ, PKTABLE_NAME");
  
  ret= Stmt->Methods->ExecDirect(Stmt, StmtStr.str, SQL_NTS);

  MADB_DynstrFree(&StmtStr);

  return ret;
}
/* }}} */

/* {{{ MADB_StmtDescribeCol */
SQLRETURN MADB_StmtDescribeCol(MADB_Stmt *Stmt, SQLUSMALLINT ColumnNumber, void *ColumnName,
                         SQLSMALLINT BufferLength, SQLSMALLINT *NameLengthPtr,
                         SQLSMALLINT *DataTypePtr, SQLULEN *ColumnSizePtr, SQLSMALLINT *DecimalDigitsPtr,
                         SQLSMALLINT *NullablePtr, my_bool isWChar)
{
  MADB_DescRecord *Record;

  MADB_CLEAR_ERROR(&Stmt->Error);

  if (!mysql_stmt_field_count(Stmt->stmt))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07005, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  if (ColumnNumber < 1 || ColumnNumber > mysql_stmt_field_count(Stmt->stmt))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07009, NULL, 0);
    return SQL_ERROR;
  }
  if (!(Record= MADB_DescGetInternalRecord(Stmt->Ird, ColumnNumber - 1, MADB_DESC_WRITE)))
  {
    MADB_CopyError(&Stmt->Error, &Stmt->Ird->Error);
    return Stmt->Error.ReturnValue;
  }
  if (NameLengthPtr)
    *NameLengthPtr= 0;

  /* Don't map types if ansi mode was set */
  if (DataTypePtr)
      *DataTypePtr= (isWChar && !Stmt->Connection->IsAnsi) ? MADB_GetWCharType(Record->ConciseType) : Record->ConciseType;
  /* Columnsize in characters, not bytes! */
  if (ColumnSizePtr)
    *ColumnSizePtr= Record->Length;
     //Record->Precision ? MIN(Record->DisplaySize, Record->Precision) : Record->DisplaySize;
  if (DecimalDigitsPtr)
    *DecimalDigitsPtr= Record->Scale;
  if (NullablePtr)
    *NullablePtr= Record->Nullable;

  if ((ColumnName || BufferLength) && Record->ColumnName)
  {
    size_t Length= MADB_SetString(isWChar ? &Stmt->Connection->Charset : 0, ColumnName, ColumnName ? BufferLength : 0, Record->ColumnName, SQL_NTS, &Stmt->Error); 
    if (NameLengthPtr)
      *NameLengthPtr= (SQLSMALLINT)Length;
    if (!BufferLength)
      MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
  }
  return Stmt->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_SetCursorName */
SQLRETURN MADB_SetCursorName(MADB_Stmt *Stmt, char *Buffer, SQLINTEGER BufferLength)
{
  MADB_List *LStmt, *LStmtNext;
  if (!Buffer)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, NULL, 0);
    return SQL_ERROR;
  }
  if (BufferLength== SQL_NTS)
    BufferLength= (SQLINTEGER)strlen(Buffer);
  if (BufferLength < 0)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY090, NULL, 0);
    return SQL_ERROR;
  }
  if ((BufferLength > 5 && strncmp(Buffer, "SQLCUR", 6) == 0) ||
      (BufferLength > 6 && strncmp(Buffer, "SQL_CUR", 7) == 0))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_34000, NULL, 0);
    return SQL_ERROR;
  }
  /* check if cursor name is unique */
  for (LStmt= Stmt->Connection->Stmts; LStmt; LStmt= LStmtNext)
  {
    MADB_Cursor *Cursor= &((MADB_Stmt *)LStmt->data)->Cursor;
    LStmtNext= LStmt->next;

    if (Stmt != (MADB_Stmt *)LStmt->data &&
        Cursor->Name && strncmp(Cursor->Name, Buffer, BufferLength) == 0)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_3C000, NULL, 0);
      return SQL_ERROR;
    }
  }
  MADB_FREE(Stmt->Cursor.Name);
  Stmt->Cursor.Name= MADB_CALLOC(BufferLength + 1);
  MADB_SetString(0, Stmt->Cursor.Name, BufferLength + 1, Buffer, BufferLength, NULL);
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_GetCursorName */
SQLRETURN MADB_GetCursorName(MADB_Stmt *Stmt, void *CursorName, SQLSMALLINT BufferLength, 
                             SQLSMALLINT *NameLengthPtr, my_bool isWChar)
{
  SQLSMALLINT Length;
  MADB_CLEAR_ERROR(&Stmt->Error);

  if (BufferLength < 0)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY090, NULL, 0);
    return Stmt->Error.ReturnValue;
  }
  if (!Stmt->Cursor.Name)
  {
    Stmt->Cursor.Name= (char *)MADB_CALLOC(MADB_MAX_CURSOR_NAME);
    _snprintf(Stmt->Cursor.Name, MADB_MAX_CURSOR_NAME, "SQL_CUR%d", 
                Stmt->Connection->CursorCount++);
  }
  Length= (SQLSMALLINT)MADB_SetString(isWChar ? &Stmt->Connection->Charset : 0, CursorName,
                                      BufferLength, Stmt->Cursor.Name, SQL_NTS, &Stmt->Error);
  if (NameLengthPtr)
    *NameLengthPtr= (SQLSMALLINT)Length;
  if (!BufferLength)
    MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
   
  return Stmt->Error.ReturnValue;
  
}
/* }}} */

/* {{{ MADB_RefreshRowPtrs */
SQLRETURN MADB_RefreshRowPtrs(MADB_Stmt *Stmt)
{
  return MoveNext(Stmt, 1LL);
}

/* {{{ MADB_RefreshDynamicCursor */
SQLRETURN MADB_RefreshDynamicCursor(MADB_Stmt *Stmt)
{
  SQLRETURN ret;
  SQLLEN    CurrentRow=     Stmt->Cursor.Position;
  long long AffectedRows=   Stmt->AffectedRows;
  SQLLEN    LastRowFetched= Stmt->LastRowFetched;

  ret= Stmt->Methods->Execute(Stmt, FALSE);

  Stmt->Cursor.Position= CurrentRow;
  if (Stmt->Cursor.Position > 0 && (my_ulonglong)Stmt->Cursor.Position >= mysql_stmt_num_rows(Stmt->stmt))
  {
    Stmt->Cursor.Position= (long)mysql_stmt_num_rows(Stmt->stmt) - 1;
  }

  Stmt->LastRowFetched= LastRowFetched;
  Stmt->AffectedRows=   AffectedRows;

  if (Stmt->Cursor.Position < 0)
  {
    Stmt->Cursor.Position= 0;
  }
  return ret;
}
/* }}} */

/* Couple of macsros for this function specifically */
#define MADB_SETPOS_FIRSTROW(agg_result) (agg_result == SQL_INVALID_HANDLE)
#define MADB_SETPOS_AGG_RESULT(agg_result, row_result) if (MADB_SETPOS_FIRSTROW(agg_result)) agg_result= row_result; \
    else if (row_result != agg_result) agg_result= SQL_SUCCESS_WITH_INFO

/* {{{ MADB_SetPos */
SQLRETURN MADB_StmtSetPos(MADB_Stmt *Stmt, SQLSETPOSIROW RowNumber, SQLUSMALLINT Operation,
                      SQLUSMALLINT LockType, int ArrayOffset)
{
  if (!Stmt->result && !Stmt->stmt->fields)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_24000, NULL, 0);
    return Stmt->Error.ReturnValue;
  }
  /* skip this for now, since we don't use unbuffered result sets 
  if (Stmt->Options.CursorType == SQL_CURSOR_FORWARD_ONLY)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_24000, NULL, 0);
    return Stmt->Error.ReturnValue;
  }
  */
  if (LockType != SQL_LOCK_NO_CHANGE)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, NULL, 0);
    return Stmt->Error.ReturnValue;
  }
  switch(Operation) {
  case SQL_POSITION:
    {
      if (RowNumber < 1 || RowNumber > mysql_stmt_num_rows(Stmt->stmt))
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY109, NULL, 0);
        return Stmt->Error.ReturnValue;
      }
      if (Stmt->Options.CursorType == SQL_CURSOR_DYNAMIC)
        if (!SQL_SUCCEEDED(Stmt->Methods->RefreshDynamicCursor(Stmt)))
          return Stmt->Error.ReturnValue;
      EnterCriticalSection(&Stmt->Connection->cs);
      Stmt->Cursor.Position+=(RowNumber - 1);
      MADB_StmtDataSeek(Stmt, Stmt->Cursor.Position);
      LeaveCriticalSection(&Stmt->Connection->cs);
    }
    break;
  case SQL_ADD:
    {
      MADB_DynString DynStmt;
      SQLRETURN      ret;
      char          *TableName=   MADB_GetTableName(Stmt);
      char          *CatalogName= MADB_GetCatalogName(Stmt);
      int            column, param= 0;

      if (Stmt->Options.CursorType == SQL_CURSOR_DYNAMIC)
        if (!SQL_SUCCEEDED(Stmt->Methods->RefreshDynamicCursor(Stmt)))
          return Stmt->Error.ReturnValue;

      Stmt->DaeRowNumber= RowNumber;

      if (Stmt->DataExecutionType != MADB_DAE_ADD)
      {
        Stmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
        MA_SQLAllocHandle(SQL_HANDLE_STMT, Stmt->Connection, (SQLHANDLE *)&Stmt->DaeStmt);

        if (MADB_InitDynamicString(&DynStmt, "INSERT INTO ", 8192, 1024) ||
            MADB_DynStrAppendQuoted(&DynStmt, CatalogName) ||
            MADB_DynstrAppend(&DynStmt, ".") ||
            MADB_DynStrAppendQuoted(&DynStmt, TableName)||
            MADB_DynStrInsertSet(Stmt, &DynStmt))
        {
          MADB_DynstrFree(&DynStmt);
          return Stmt->Error.ReturnValue;
        }

        ResetMetadata(&Stmt->DaeStmt->DefaultsResult, MADB_GetDefaultColumnValues(Stmt, Stmt->stmt->fields));

        Stmt->DataExecutionType= MADB_DAE_ADD;
        ret= Stmt->Methods->Prepare(Stmt->DaeStmt, DynStmt.str, SQL_NTS, FALSE);

        MADB_DynstrFree(&DynStmt);

        if (!SQL_SUCCEEDED(ret))
        {
          MADB_CopyError(&Stmt->Error, &Stmt->DaeStmt->Error);
          Stmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
          return Stmt->Error.ReturnValue;
        }
      }
      
      /* Bind parameters - DaeStmt will process whole array of values, thus we don't need to iterate through the array*/
      for (column= 0; column < MADB_STMT_COLUMN_COUNT(Stmt); ++column)
      {
        MADB_DescRecord *Rec=          MADB_DescGetInternalRecord(Stmt->Ard, column, MADB_DESC_READ),
                        *ApdRec=       NULL;

        if (Rec->inUse && MADB_ColumnIgnoredInAllRows(Stmt->Ard, Rec) == FALSE)
        {
          Stmt->DaeStmt->Methods->BindParam(Stmt->DaeStmt, param + 1, SQL_PARAM_INPUT, Rec->ConciseType, Rec->Type,
            Rec->DisplaySize, Rec->Scale, Rec->DataPtr, Rec->OctetLength, Rec->OctetLengthPtr);
        }
        else
        {
          /*Stmt->DaeStmt->Methods->BindParam(Stmt->DaeStmt, param + 1, SQL_PARAM_INPUT, SQL_CHAR, SQL_C_CHAR, 0, 0,
                            ApdRec->DefaultValue, strlen(ApdRec->DefaultValue), NULL);*/
          continue;
        }
        
        ApdRec= MADB_DescGetInternalRecord(Stmt->DaeStmt->Apd, param, MADB_DESC_READ);
        ApdRec->DefaultValue= MADB_GetDefaultColumnValue(Stmt->DaeStmt->DefaultsResult,
          Stmt->stmt->fields[column].org_name);

        ++param;
      }

      memcpy(&Stmt->DaeStmt->Apd->Header, &Stmt->Ard->Header, sizeof(MADB_Header));
      ret= Stmt->Methods->Execute(Stmt->DaeStmt, FALSE);

      if (!SQL_SUCCEEDED(ret))
      {
        /* We can have SQL_NEED_DATA here, which would not set error (and its ReturnValue) */
        MADB_CopyError(&Stmt->Error, &Stmt->DaeStmt->Error);
        return ret;
      }
      if (Stmt->AffectedRows == -1)
      {
        Stmt->AffectedRows= 0;
      }
      Stmt->AffectedRows+= Stmt->DaeStmt->AffectedRows;

      Stmt->DataExecutionType= MADB_DAE_NORMAL;
      Stmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
      Stmt->DaeStmt= NULL;
    }
    break;
  case SQL_UPDATE:
    {
      char        *TableName= MADB_GetTableName(Stmt);
      my_ulonglong Start=     0, 
                   End=       mysql_stmt_num_rows(Stmt->stmt);
      SQLRETURN    result=    SQL_INVALID_HANDLE; /* Just smth we cannot normally get */   

      if (!TableName)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_IM001, "Updatable Cursors with multiple tables are not supported", 0);
        return Stmt->Error.ReturnValue;
      }
      
      Stmt->AffectedRows= 0;

      if ((SQLLEN)RowNumber > Stmt->LastRowFetched)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_S1107, NULL, 0);
        return Stmt->Error.ReturnValue;
      }

      if (RowNumber < 0 || RowNumber > End)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY109, NULL, 0);
        return Stmt->Error.ReturnValue;
      }

      if (Stmt->Options.CursorType == SQL_CURSOR_DYNAMIC)
        if (!SQL_SUCCEEDED(Stmt->Methods->RefreshDynamicCursor(Stmt)))
          return Stmt->Error.ReturnValue;

      Stmt->DaeRowNumber= MAX(1,RowNumber);
      
      /* Cursor is open, but no row was fetched, so we simulate
         that first row was fetched */
      if (Stmt->Cursor.Position < 0)
        Stmt->Cursor.Position= 1;

      if (RowNumber)
        Start= End= Stmt->Cursor.Position + RowNumber -1;
      else
      {
        Start= Stmt->Cursor.Position;
        /* TODO: if num_rows returns 1, End is 0? Start would be 1, no */
        End= MIN(mysql_stmt_num_rows(Stmt->stmt)-1, Start + Stmt->Ard->Header.ArraySize - 1);
      }
      /* Stmt->ArrayOffset will be incremented in StmtExecute() */
      Start+= Stmt->ArrayOffset;

      /* TODO: SQL_ATTR_ROW_STATUS_PTR should be filled */
      while (Start <= End)
      {
        SQLSMALLINT param= 0, column;
        MADB_StmtDataSeek(Stmt, Start);
        Stmt->Methods->RefreshRowPtrs(Stmt);
        
        /* We don't need to prepare the statement, if SetPos was called
           from SQLParamData() function */
        if (!ArrayOffset)
        {
          if (!SQL_SUCCEEDED(MADB_DaeStmt(Stmt, SQL_UPDATE)))
          {
            MADB_SETPOS_AGG_RESULT(result, Stmt->Error.ReturnValue);
            /* Moving to the next row */
            Stmt->DaeRowNumber++;
            Start++;

            continue;
          }

          for(column= 0; column < MADB_STMT_COLUMN_COUNT(Stmt); ++column)
          {
            SQLLEN          *LengthPtr= NULL;
            my_bool         GetDefault= FALSE;
            MADB_DescRecord *Rec=       MADB_DescGetInternalRecord(Stmt->Ard, column, MADB_DESC_READ);

            /* TODO: shouldn't here be IndicatorPtr? */
            if (Rec->OctetLengthPtr)
              LengthPtr= GetBindOffset(Stmt->Ard, Rec, Rec->OctetLengthPtr, Stmt->DaeRowNumber > 1 ? Stmt->DaeRowNumber - 1 : 0, sizeof(SQLLEN)/*Rec->OctetLength*/);
            if (!Rec->inUse ||
                (LengthPtr && *LengthPtr == SQL_COLUMN_IGNORE))
            {
              GetDefault= TRUE;
              continue;
            }
            
            /* TODO: Looks like this whole thing is not really needed. Not quite clear if !InUse should result in going this way */
            if (GetDefault)
            {
              SQLLEN Length= 0;
              /* set a default value */
              if (Stmt->Methods->GetData(Stmt, column + 1, SQL_C_CHAR, NULL, 0, &Length, TRUE) != SQL_ERROR && Length)
              {
                MADB_FREE(Rec->DefaultValue);
                if (Length > 0) 
                {
                  Rec->DefaultValue= (char *)MADB_CALLOC(Length + 1);
                  Stmt->Methods->GetData(Stmt, column + 1, SQL_C_CHAR, Rec->DefaultValue, Length+1, 0, TRUE);
                }
                Stmt->DaeStmt->Methods->BindParam(Stmt->DaeStmt, param + 1, SQL_PARAM_INPUT, SQL_CHAR, SQL_C_CHAR, 0, 0,
                              Rec->DefaultValue, Length, NULL);
                ++param;
                continue;
              }
            }

            if (!GetDefault)
            {
              Stmt->DaeStmt->Methods->BindParam(Stmt->DaeStmt, param + 1, SQL_PARAM_INPUT, Rec->ConciseType, Rec->Type,
                      Rec->DisplaySize, Rec->Scale,
                      GetBindOffset(Stmt->Ard, Rec, Rec->DataPtr, Stmt->DaeRowNumber > 1 ? Stmt->DaeRowNumber -1 : 0, Rec->OctetLength),
                      Rec->OctetLength, LengthPtr);
            }
            if (PARAM_IS_DAE(LengthPtr) && !DAE_DONE(Stmt->DaeStmt))
            {
              Stmt->Status= SQL_NEED_DATA;
              ++param;
              continue;
            }

            ++param;
          }                             /* End of for(column=0;...) */
          if (Stmt->Status == SQL_NEED_DATA)
            return SQL_NEED_DATA;
        }                               /* End of if (!ArrayOffset) */ 
        
        if (Stmt->DaeStmt->Methods->Execute(Stmt->DaeStmt, FALSE) != SQL_ERROR)
        {
          Stmt->AffectedRows+= Stmt->DaeStmt->AffectedRows;
        }
        else
        {
          MADB_CopyError(&Stmt->Error, &Stmt->DaeStmt->Error);
        }

        MADB_SETPOS_AGG_RESULT(result, Stmt->DaeStmt->Error.ReturnValue);

        Stmt->DaeRowNumber++;
        Start++;
      }                                 /* End of while (Start <= End) */

      Stmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
      Stmt->DaeStmt= NULL;
      Stmt->DataExecutionType= MADB_DAE_NORMAL;

      /* Making sure we do not return initial value */
      return result ==  SQL_INVALID_HANDLE ? SQL_SUCCESS :result;
    }
  case SQL_DELETE:
    {
      MADB_DynString DynamicStmt;
      SQLULEN        SaveArraySize= Stmt->Ard->Header.ArraySize;
      my_ulonglong   Start=         0,
                     End=           mysql_stmt_num_rows(Stmt->stmt);
      char           *TableName=    MADB_GetTableName(Stmt);

      if (!TableName)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_IM001, "Updatable Cursors with multiple tables are not supported", 0);
        return Stmt->Error.ReturnValue;
      }

      Stmt->Ard->Header.ArraySize= 1;
      if (Stmt->Options.CursorType == SQL_CURSOR_DYNAMIC)
        if (!SQL_SUCCEEDED(Stmt->Methods->RefreshDynamicCursor(Stmt)))
          return Stmt->Error.ReturnValue;
      Stmt->AffectedRows= 0;
      if (RowNumber < 0 || RowNumber > End)
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY109, NULL, 0);
        return Stmt->Error.ReturnValue;
      }
      Start= (RowNumber) ? Stmt->Cursor.Position + RowNumber - 1 : Stmt->Cursor.Position;
      if (SaveArraySize && !RowNumber)
        End= MIN(End, Start + SaveArraySize - 1);
      else
        End= Start;

      while (Start <= End)
      {
        MADB_StmtDataSeek(Stmt, Start);
        Stmt->Methods->RefreshRowPtrs(Stmt);
        MADB_InitDynamicString(&DynamicStmt, "DELETE FROM ", 8192, 1024);
        if (MADB_DynStrAppendQuoted(&DynamicStmt, TableName) ||
           MADB_DynStrGetWhere(Stmt, &DynamicStmt, TableName, FALSE))
        {
          MADB_DynstrFree(&DynamicStmt);
          MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);

          return Stmt->Error.ReturnValue;
        }

        LOCK_MARIADB(Stmt->Connection);
        if (mysql_real_query(Stmt->Connection->mariadb, DynamicStmt.str, (unsigned long)DynamicStmt.length))
        {
          MADB_DynstrFree(&DynamicStmt);
          MADB_SetError(&Stmt->Error, MADB_ERR_HY001, mysql_error(Stmt->Connection->mariadb), 
                            mysql_errno(Stmt->Connection->mariadb));

          UNLOCK_MARIADB(Stmt->Connection);

          return Stmt->Error.ReturnValue;
        }
        MADB_DynstrFree(&DynamicStmt);
        Stmt->AffectedRows+= mysql_affected_rows(Stmt->Connection->mariadb);
        Start++;
      }
      UNLOCK_MARIADB(Stmt->Connection);

      Stmt->Ard->Header.ArraySize= SaveArraySize;
      /* if we have a dynamic cursor we need to adjust the rowset size */
      if (Stmt->Options.CursorType == SQL_CURSOR_DYNAMIC)
      {
        Stmt->LastRowFetched-= (unsigned long)Stmt->AffectedRows;
      }
    }
    break;
  case SQL_REFRESH:
    /* todo*/
    break;
  default:
    MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Only SQL_POSITION and SQL_REFRESH Operations are supported", 0);
    return Stmt->Error.ReturnValue;
  }
  return SQL_SUCCESS;
}
/* }}} */
#undef MADB_SETPOS_FIRSTROW
#undef MADB_SETPOS_AGG_RESULT

/* {{{ MADB_StmtFetchScroll */
SQLRETURN MADB_StmtFetchScroll(MADB_Stmt *Stmt, SQLSMALLINT FetchOrientation,
                               SQLLEN FetchOffset)
{
  SQLRETURN ret= SQL_SUCCESS;
  SQLLEN    Position;
  SQLLEN    RowsProcessed;

  RowsProcessed= Stmt->LastRowFetched;
  
  if (Stmt->Options.CursorType == SQL_CURSOR_FORWARD_ONLY &&
      FetchOrientation != SQL_FETCH_NEXT)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY106, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  if (Stmt->Options.CursorType == SQL_CURSOR_DYNAMIC)
  {
    SQLRETURN rc;
    rc= Stmt->Methods->RefreshDynamicCursor(Stmt);
    if (!SQL_SUCCEEDED(rc))
    {
      return Stmt->Error.ReturnValue;
    }
  }

  if (FetchOrientation != SQL_FETCH_NEXT)
  {
    MADB_STMT_FORGET_NEXT_POS(Stmt);
  }

  switch(FetchOrientation) {
  case SQL_FETCH_NEXT:
    Position= Stmt->Cursor.Position < 0 ? 0 : Stmt->Cursor.Position + RowsProcessed;
    break;
  case SQL_FETCH_PRIOR:
    Position= Stmt->Cursor.Position < 0 ? - 1: Stmt->Cursor.Position - MAX(1, Stmt->Ard->Header.ArraySize);
    break;
  case SQL_FETCH_RELATIVE:
    Position= Stmt->Cursor.Position + FetchOffset;
    if (Position < 0 && Stmt->Cursor.Position > 0 &&
        -FetchOffset <= (SQLINTEGER)Stmt->Ard->Header.ArraySize)
      Position= 0;
    break;
  case SQL_FETCH_ABSOLUTE:
    if (FetchOffset < 0)
    {
      if ((long long)mysql_stmt_num_rows(Stmt->stmt) - 1 + FetchOffset < 0 &&
          ((SQLULEN)-FetchOffset <= Stmt->Ard->Header.ArraySize))
        Position= 0;
      else
        Position= (SQLLEN)mysql_stmt_num_rows(Stmt->stmt) + FetchOffset;
    }
    else
      Position= FetchOffset - 1;
    break;
  case SQL_FETCH_FIRST:
    Position= 0;
    break;
  case SQL_FETCH_LAST:
    Position= (SQLLEN)mysql_stmt_num_rows(Stmt->stmt) - MAX(1, Stmt->Ard->Header.ArraySize);
 /*   if (Stmt->Ard->Header.ArraySize > 1)
      Position= MAX(0, Position - Stmt->Ard->Header.ArraySize + 1); */
    break;
  case SQL_FETCH_BOOKMARK:
    if (Stmt->Options.UseBookmarks == SQL_UB_OFF)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY106, NULL, 0);
      return Stmt->Error.ReturnValue;
    }
    if (!Stmt->Options.BookmarkPtr)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY111, NULL, 0);
      return Stmt->Error.ReturnValue;
    }

    Position= *((long *)Stmt->Options.BookmarkPtr);
    if (Stmt->Connection->Environment->OdbcVersion >= SQL_OV_ODBC3)
      Position+= FetchOffset;
   break;
  default:
    MADB_SetError(&Stmt->Error, MADB_ERR_HY106, NULL, 0);
    return Stmt->Error.ReturnValue;
    break;
  }

  if (Position < 0)
  {
    MADB_STMT_RESET_CURSOR(Stmt);
  }
  else
  {
    Stmt->Cursor.Position= (SQLLEN)MIN((my_ulonglong)Position, mysql_stmt_num_rows(Stmt->stmt));
  }
  if (Position < 0 || (my_ulonglong)Position > mysql_stmt_num_rows(Stmt->stmt) - 1)
  {
    /* We need to put cursor before RS start, not only return error */
    if (Position < 0)
    {
      MADB_StmtDataSeek(Stmt, 0);
    }
    return SQL_NO_DATA;
  }

  /* For dynamic cursor we "refresh" resultset eachtime(basically re-executing), and thus the (c/c)cursor is before 1st row at this point,
     and thux we need to restore the last position. For array fetch with not forward_only cursor, the (c/c)cursor is at 1st row of the last
     fetched rowset */
  if (FetchOrientation != SQL_FETCH_NEXT || (RowsProcessed > 1 && Stmt->Options.CursorType != SQL_CURSOR_FORWARD_ONLY) ||
      Stmt->Options.CursorType == SQL_CURSOR_DYNAMIC)
  {
    if (Stmt->Cursor.Next != NULL)
    {
      mysql_stmt_row_seek(Stmt->stmt, Stmt->Cursor.Next);
      ret= SQL_SUCCESS;
    }
    else
    {
      ret= MADB_StmtDataSeek(Stmt, Stmt->Cursor.Position);
    }
  }
  
  /* Assuming, that ret before previous "if" was SQL_SUCCESS */
  if (ret == SQL_SUCCESS)
  {
    ret= Stmt->Methods->Fetch(Stmt);
  }
  if (ret == SQL_NO_DATA_FOUND && Stmt->LastRowFetched > 0)
  {
    ret= SQL_SUCCESS;
  }
  return ret;
}

struct st_ma_stmt_methods MADB_StmtMethods=
{
  MADB_StmtPrepare,
  MADB_StmtExecute,
  MADB_StmtFetch,
  MADB_StmtBindCol,
  MADB_StmtBindParam,
  MADB_StmtExecDirect,
  MADB_StmtGetData,
  MADB_StmtRowCount,
  MADB_StmtParamCount,
  MADB_StmtColumnCount,
  MADB_StmtGetAttr,
  MADB_StmtSetAttr,
  MADB_StmtFree,
  MADB_StmtColAttr,
  MADB_StmtColumnPrivileges,
  MADB_StmtTablePrivileges,
  MADB_StmtTables,
  MADB_StmtStatistics,
  MADB_StmtColumns,
  MADB_StmtProcedureColumns,
  MADB_StmtPrimaryKeys,
  MADB_StmtSpecialColumns,
  MADB_StmtProcedures,
  MADB_StmtForeignKeys,
  MADB_StmtDescribeCol,
  MADB_SetCursorName,
  MADB_GetCursorName,
  MADB_StmtSetPos,
  MADB_StmtFetchScroll,
  MADB_StmtParamData,
  MADB_StmtPutData,
  MADB_StmtBulkOperations,
  MADB_RefreshDynamicCursor,
  MADB_RefreshRowPtrs,
  MADB_GetOutParams
};
