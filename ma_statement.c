/************************************************************************************
   Copyright (C) 2013,2017 MariaDB Corporation AB
   
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

struct st_ma_stmt_methods MADB_StmtMethods; /* declared at the end of file */
static my_bool UpdateMaxLength= 1;


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

  if (!(Stmt->stmt= mysql_stmt_init(Stmt->Connection->mariadb)) ||
    !(Stmt->IApd= MADB_DescInit(Connection, MADB_DESC_APD, FALSE)) ||
    !(Stmt->IArd= MADB_DescInit(Connection, MADB_DESC_ARD, FALSE)) ||
    !(Stmt->IIpd= MADB_DescInit(Connection, MADB_DESC_IPD, FALSE)) ||
    !(Stmt->IIrd= MADB_DescInit(Connection, MADB_DESC_IRD, FALSE)))
  {
    goto error;
  }

  MDBUG_C_PRINT(Stmt->Connection, "-->inited %0x", Stmt->stmt);
  mysql_stmt_attr_set(Stmt->stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &UpdateMaxLength);

  Stmt->PutParam= -1;
  Stmt->Methods= &MADB_StmtMethods;
  /* default behaviour is SQL_CURSOR_STATIC */
  Stmt->Options.CursorType= SQL_CURSOR_STATIC;
  Stmt->Options.UseBookmarks= SQL_UB_OFF;

  Stmt->Apd= Stmt->IApd;
  Stmt->Ard= Stmt->IArd;
  Stmt->Ipd= Stmt->IIpd;
  Stmt->Ird= Stmt->IIrd;

  Stmt->ListItem.data= (void *)Stmt;
  Stmt->Connection->Stmts= list_add(Stmt->Connection->Stmts, &Stmt->ListItem);
  UNLOCK_MARIADB(Connection);
  Stmt->Ard->Header.ArraySize= 1;

  return SQL_SUCCESS;

error:
  if (Stmt && Stmt->stmt)
  {
    mysql_stmt_close(Stmt->stmt);
    UNLOCK_MARIADB(Stmt->Connection);
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
    if(!mysql_real_query(Stmt->Connection->mariadb, StatementText, TextLength))
    {
      ret= SQL_SUCCESS;
      MADB_CLEAR_ERROR(&Stmt->Error);
      Stmt->AffectedRows= mysql_affected_rows(Stmt->Connection->mariadb);
      Stmt->State= MADB_SS_EMULATED;
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
    uint i;
    for (i=0; i < desc->Stmts.elements; ++i)
    {
      MADB_Stmt **refStmt= ((MADB_Stmt **)desc->Stmts.buffer) + i;
      if (Stmt == *refStmt)
      {
        delete_dynamic_element(&desc->Stmts, i);

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
void ResetMetadata(MYSQL_RES** metadata)
{
  if (*metadata != NULL)
  {
    mysql_free_result(*metadata);
    *metadata= NULL;
  }
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
      if (Stmt->State > MADB_SS_PREPARED && !Stmt->MultiStmtCount)
      {
        mysql_stmt_free_result(Stmt->stmt);
        MDBUG_C_PRINT(Stmt->Connection, "-->resetting %0x", Stmt->stmt);
        LOCK_MARIADB(Stmt->Connection);
        mysql_stmt_reset(Stmt->stmt);
        UNLOCK_MARIADB(Stmt->Connection);
      }
      if (Stmt->MultiStmtCount)
      {
        unsigned int i;
        LOCK_MARIADB(Stmt->Connection);
        for (i=0; i < Stmt->MultiStmtCount; ++i)
        {
          MDBUG_C_PRINT(Stmt->Connection, "-->resetting %0x(%u)", Stmt->MultiStmts[i],i);
          mysql_stmt_reset(Stmt->MultiStmts[i]);
        }
        UNLOCK_MARIADB(Stmt->Connection);
      }

      ResetMetadata(&Stmt->metadata);
      ResetMetadata(&Stmt->DefaultsResult);
     
      MADB_FREE(Stmt->result);
      MADB_FREE(Stmt->CharOffset);
      MADB_FREE(Stmt->Lengths);

      RESET_STMT_STATE(Stmt);
      RESET_DAE_STATUS(Stmt);
    }
    break;
  case SQL_UNBIND:
    MADB_FREE(Stmt->result);
    ResetMetadata(&Stmt->metadata);
    MADB_DescFree(Stmt->Ard, TRUE);
    ResetMetadata(&Stmt->DefaultsResult);
    break;
  case SQL_RESET_PARAMS:
    MADB_FREE(Stmt->params);
    ResetMetadata(&Stmt->DefaultsResult);
    MADB_DescFree(Stmt->Apd, TRUE);
    RESET_DAE_STATUS(Stmt);
    break;
  case SQL_DROP:
    MADB_FreeTokens(Stmt->Tokens);
    MADB_FREE(Stmt->params);
    MADB_FREE(Stmt->result);
    MADB_FREE(Stmt->Cursor.Name);
    MADB_FREE(Stmt->StmtString);
    MADB_FREE(Stmt->NativeSql);
    MADB_FREE(Stmt->CatalogName);
    MADB_FREE(Stmt->TableName);
    ResetMetadata(&Stmt->metadata);

    /* For explicit descriptors we only remove reference to the stmt*/
    if (Stmt->Apd->AppType)
    {
      RemoveStmtRefFromDesc(Stmt->Apd, Stmt, TRUE);
      MADB_DescFree(Stmt->IApd, FALSE);
    }
    else
    {
      MADB_DescFree( Stmt->Apd, FALSE);
    }
    if (Stmt->Ard->AppType)
    {
      RemoveStmtRefFromDesc(Stmt->Ard, Stmt, TRUE);
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
    ResetMetadata(&Stmt->DefaultsResult);

    if (Stmt->DaeStmt != NULL)
    {
      Stmt->DaeStmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
      Stmt->DaeStmt= NULL;
    }
    EnterCriticalSection(&Stmt->Connection->cs);
    /* TODO: if multistatement was prepared, but not executed, we would get here Stmt->stmt leaked. Unlikely that is very probable scenario,
             thus leaving this for new version */
    if (Stmt->MultiStmtCount)
    {
      unsigned int i;
      for (i= 0; i < Stmt->MultiStmtCount; ++i)
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
      Stmt->MultiStmtCount= Stmt->MultiStmtNr= 0;
    }
    else if (Stmt->stmt != NULL)
    {
      MDBUG_C_PRINT(Stmt->Connection, "-->closing %0x", Stmt->stmt);
      mysql_stmt_close(Stmt->stmt);
    }

    Stmt->Connection->Stmts= list_delete(Stmt->Connection->Stmts, &Stmt->ListItem);
    LeaveCriticalSection(&Stmt->Connection->cs);
    MADB_FREE(Stmt);
  } /* End of switch (Option) */
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_StmtExecDirect */
SQLRETURN MADB_StmtExecDirect(MADB_Stmt *Stmt, char *StatementText, SQLINTEGER TextLength)
{
  SQLRETURN ret;

  ret= Stmt->Methods->Prepare(Stmt, StatementText, TextLength);
  /* In case statement is not supported, we use mysql_query instead */
  if (!SQL_SUCCEEDED(ret))
  {
    /* This is not quite good - 1064 may simply mean that syntax is wrong. we are screwed then */
    /* Stmt->StmtString is NULL if we had error on one of statements in the batch */
    if ((Stmt->Error.NativeError == 1295 || Stmt->Error.NativeError == 1064) && Stmt->StmtString != NULL)
    {
      Stmt->State= MADB_SS_EMULATED;
    }
    else
    {
      MADB_FREE(Stmt->StmtString);
      MADB_FREE(Stmt->NativeSql);
      return ret;
    }
  }

  return Stmt->Methods->Execute(Stmt);
}
/* }}} */

/* {{{ MADB_FindCursor */
MADB_Stmt *MADB_FindCursor(MADB_Stmt *Stmt, const char *CursorName)
{
  MADB_Dbc *Dbc= Stmt->Connection;
  LIST *LStmt, *LStmtNext;

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
  ResetMetadata(&Stmt->metadata);
  Stmt->metadata= mysql_stmt_result_metadata(Stmt->stmt);

  return Stmt->metadata;
}
/* }}} */

/* {{{ MADB_StmtReset - reseting Stmt handler for new use. Has to be called inside a lock */
void MADB_StmtReset(MADB_Stmt *Stmt)
{
  if (!Stmt->MultiStmtCount)
  {
    if (Stmt->State > MADB_SS_PREPARED)
    {
      MDBUG_C_PRINT(Stmt->Connection, "mysql_stmt_free_result(%0x)", Stmt->stmt);
      mysql_stmt_free_result(Stmt->stmt);
    }

    if (Stmt->State >= MADB_SS_PREPARED)
    {
      MDBUG_C_PRINT(Stmt->Connection, "-->closing %0x", Stmt->stmt);
      mysql_stmt_close(Stmt->stmt);
      Stmt->stmt= mysql_stmt_init(Stmt->Connection->mariadb);
      mysql_stmt_attr_set(Stmt->stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &UpdateMaxLength);

      MDBUG_C_PRINT(Stmt->Connection, "-->inited %0x", Stmt->stmt);
    }
  }
  else
  {
    CloseMultiStatements(Stmt);
    Stmt->stmt= mysql_stmt_init(Stmt->Connection->mariadb);
    mysql_stmt_attr_set(Stmt->stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &UpdateMaxLength);

    MDBUG_C_PRINT(Stmt->Connection, "-->inited %0x", Stmt->stmt);
  }

  switch (Stmt->State)
  {
  case MADB_SS_EXECUTED:

    MADB_FREE(Stmt->result);
    MADB_FREE(Stmt->CharOffset);
    MADB_FREE(Stmt->Lengths);
    RESET_DAE_STATUS(Stmt);

  case MADB_SS_PREPARED:
    ResetMetadata(&Stmt->metadata);
    Stmt->PositionedCursor= NULL;
    Stmt->Ird->Header.Count= 0;

  case MADB_SS_EMULATED:
    MADB_FREE(Stmt->NativeSql);
    MADB_FREE(Stmt->StmtString);
    Stmt->State= MADB_SS_INITED;
    MADB_CLEAR_ERROR(&Stmt->Error);
  }
}
/* }}} */

/* {{{ MADB_StmtPrepare */
SQLRETURN MADB_StmtPrepare(MADB_Stmt *Stmt, char *StatementText, SQLINTEGER TextLength)
{
  char                      *CursorName= NULL;
  char                      *p;
  unsigned int              WhereOffset;
  enum enum_madb_query_type QueryType;

  MDBUG_C_PRINT(Stmt->Connection, "%sMADB_StmtPrepare", "\t->");

  LOCK_MARIADB(Stmt->Connection);
  MADB_StmtReset(Stmt);
  /* After this point we can't have SQL_NTS*/
  ADJUST_LENGTH(StatementText, TextLength);

  /* if we have multiple statements we save single statements in Stmt->StrMultiStmt
     and store the number in Stnt.>MultiStnts */
  if (DSN_OPTION(Stmt->Connection, MADB_OPT_FLAG_MULTI_STATEMENTS)
   && QueryIsPossiblyMultistmt(StatementText))
  {
    int MultiStmts= GetMultiStatements(Stmt, StatementText, TextLength);
    if (!MultiStmts)
      return Stmt->Error.ReturnValue;
    if (MultiStmts > 1)
    {
      /* all statemtens successfully prepared */
      Stmt->StmtString= _strdup(StatementText);

      UNLOCK_MARIADB(Stmt->Connection);
      return SQL_SUCCESS;
    }
  }

  UNLOCK_MARIADB(Stmt->Connection);

  if (!MADB_ValidateStmt(StatementText))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY000, "SQL command SET NAMES is not allowed", 0);
    return Stmt->Error.ReturnValue;
  }

  p= my_strndup(StatementText, TextLength, MYF(0));
  /* FixIsoFormat also trims the string */
  Stmt->StmtString = _strdup(FixIsoFormat(p));
  TextLength= (SQLINTEGER)strlen(Stmt->StmtString);
  MADB_FREE(p);

  MADB_FreeTokens(Stmt->Tokens);
  Stmt->Tokens= MADB_Tokenize(Stmt->StmtString);

  QueryType= MADB_GetQueryType(Stmt);
  /* Transform WHERE CURRENT OF [cursorname]:
     Append WHERE with Parameter Markers
     In StmtExecute we will call SQLSetPos with update or delete:
     */

  if ((CursorName = MADB_ParseCursorName(Stmt, &WhereOffset)))
  {
    DYNAMIC_STRING StmtStr;
    char *TableName;

    /* Make sure we have a delete or update statement
       MADB_QUERY_DELETE and MADB_QUERY_UPDATE defined in the enum to have the same value
       as SQL_UPDATE and SQL_DELETE, respectively */
    if (QueryType == MADB_QUERY_DELETE || QueryType == MADB_QUERY_UPDATE)
    {
      Stmt->PositionedCommand= QueryType;
    }
    else
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_42000, "Invalid SQL Syntax: DELETE or UPDATE expected for positioned update", 0);
      return Stmt->Error.ReturnValue;
    }

    if (!(Stmt->PositionedCursor= MADB_FindCursor(Stmt, CursorName)))
      return Stmt->Error.ReturnValue;

    TableName= MADB_GetTableName(Stmt->PositionedCursor);
    init_dynamic_string(&StmtStr, "", 8192, 1024);
    dynstr_append_mem(&StmtStr, Stmt->StmtString, WhereOffset);
    MADB_DynStrGetWhere(Stmt->PositionedCursor, &StmtStr, TableName, TRUE);
    
    MADB_FREE(Stmt->StmtString);
    Stmt->StmtString= _strdup(StmtStr.str);
    TextLength= (SQLINTEGER)strlen(Stmt->StmtString);
    dynstr_free(&StmtStr);
  }

  if (Stmt->Options.MaxRows)
  {
    char *p;
    Stmt->StmtString= my_realloc((gptr)Stmt->StmtString, strlen(Stmt->StmtString) + 40, MYF(0));
    p= Stmt->StmtString + strlen(Stmt->StmtString);
    my_snprintf(p, 40, " LIMIT %d", Stmt->Options.MaxRows);
    TextLength= (SQLINTEGER)strlen(Stmt->StmtString);
  }

  if (QUERY_DOESNT_RETURN_RESULT(QueryType) && MADB_FindParamPlaceholder(Stmt) == 0
   && !QueryIsPossiblyMultistmt(Stmt->StmtString))
  {
    Stmt->State= MADB_SS_EMULATED;
    return SQL_SUCCESS;
  }

  LOCK_MARIADB(Stmt->Connection);
  if (mysql_stmt_prepare(Stmt->stmt, Stmt->StmtString, TextLength))
  {
    /* Need to save error first */
    MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
    /* We need to close the stmt here, or it becomes unusable like in ODBC-21 */
    mysql_stmt_close(Stmt->stmt);
    Stmt->stmt= mysql_stmt_init(Stmt->Connection->mariadb);
    mysql_stmt_attr_set(Stmt->stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &UpdateMaxLength);

    UNLOCK_MARIADB(Stmt->Connection);

    return Stmt->Error.ReturnValue;
  }
  UNLOCK_MARIADB(Stmt->Connection);

  Stmt->State= MADB_SS_PREPARED;

  /* If we have result returning query - fill descriptor records with metadata */
  if (mysql_stmt_field_count(Stmt->stmt) > 0)
  {
    MADB_DescSetIrdMetadata(Stmt, mysql_fetch_fields(FetchMetadata(Stmt)), mysql_stmt_field_count(Stmt->stmt));
  }

  if ((Stmt->ParamCount= mysql_stmt_param_count(Stmt->stmt)))
  {
    if (Stmt->params)
      MADB_FREE(Stmt->params);
    Stmt->params= (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * Stmt->ParamCount);
    mysql_stmt_bind_param(Stmt->stmt, Stmt->params);
  }

  return SQL_SUCCESS;
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
    ret= Stmt->Methods->Execute(Stmt);
    RESET_DAE_STATUS(Stmt);
    break;
  case MADB_DAE_UPDATE:
    ret= Stmt->Methods->SetPos(Stmt, Stmt->DaeRowNumber, SQL_UPDATE, SQL_LOCK_NO_CHANGE, 1);
    RESET_DAE_STATUS(Stmt);
    break;
  case MADB_DAE_ADD:
    ret= Stmt->DaeStmt->Methods->Execute(Stmt->DaeStmt);
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
    ConvertedDataPtr= MADB_ConvertFromWChar((SQLWCHAR *)DataPtr, (SQLINTEGER)(StrLen_or_Ind/sizeof(SQLWCHAR)), &Length, &Stmt->Connection->charset, NULL);

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

/* TODO: This function doesn't seem to be used anywhere */
SQLULEN GetOffset(MADB_Stmt *Stmt, SQLUINTEGER Count, MADB_DescRecord *Record, SQLUINTEGER Size)
{
  if (Stmt->Ard->Header.BindType)
    return Stmt->Ard->Header.BindType * Count;
  if (Size)
    return Count * Size;
  return Count * Record->OctetLength;
}

/* {{{ MADB_ExecutePositionedUpdate */
SQLRETURN MADB_ExecutePositionedUpdate(MADB_Stmt *Stmt)
{
  int ParamOffset2;
  int j;
  SQLRETURN ret;
  DYNAMIC_ARRAY DynData;
  MADB_Stmt *SaveCursor;

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
  
  ParamOffset2= Stmt->ParamCount - mysql_stmt_field_count(Stmt->PositionedCursor->stmt);
  init_dynamic_array(&DynData, sizeof(char *), 8, 8);

  for (j=ParamOffset2; j < Stmt->ParamCount; j++)
  {
    SQLLEN Length;
    MADB_DescRecord *Rec= MADB_DescGetInternalRecord(Stmt->PositionedCursor->Ard, j - ParamOffset2 + 1, MADB_DESC_READ);
    Length= Rec->OctetLength;
 /*   if (Rec->inUse)
      MA_SQLBindParameter(Stmt, j+1, SQL_PARAM_INPUT, Rec->ConciseType, Rec->Type, Rec->DisplaySize, Rec->Scale, Rec->DataPtr, Length, Rec->OctetLengthPtr);
    else */
    {
      Stmt->Methods->GetData(Stmt->PositionedCursor, j - ParamOffset2 + 1, SQL_CHAR,  NULL, 0, &Length, TRUE);
      p= (char *)MADB_CALLOC(Length + 2);
      insert_dynamic(&DynData, (gptr)&p);
      Stmt->Methods->GetData(Stmt->PositionedCursor, j - ParamOffset2 + 1, SQL_CHAR,  p, Length + 1, NULL, TRUE);
      Stmt->Methods->BindParam(Stmt, j+1, SQL_PARAM_INPUT, SQL_CHAR, SQL_CHAR, 0, 0, p, Length, NULL);
    }
  }

  SaveCursor= Stmt->PositionedCursor;
  Stmt->PositionedCursor= NULL;
  ret= Stmt->Methods->Execute(Stmt);
  Stmt->PositionedCursor= SaveCursor;

  for (j=0; j < (int)DynData.elements; j++)
  {
    get_dynamic(&DynData, (gptr)&p, j);
    MADB_FREE(p);
  }
  delete_dynamic(&DynData);

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
    if (Stmt->PositionedCommand == SQL_DELETE)
      MADB_STMT_RESET_CURSOR(Stmt->PositionedCursor);
      
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
  
  for (i=0; i < (uint)Stmt->ParamCount && ParameterNr < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    MADB_DescRecord *IpdRecord, *ApdRecord;
    if (IpdRecord= MADB_DescGetInternalRecord(Stmt->Ipd, i, MADB_DESC_READ))
    {
      if (IpdRecord->ParameterType == SQL_PARAM_INPUT_OUTPUT ||
          IpdRecord->ParameterType == SQL_PARAM_OUTPUT)
      {
        ApdRecord= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ);
        Bind[ParameterNr].buffer= GetBindOffset(Stmt->Apd, ApdRecord, ApdRecord->DataPtr, CurrentOffset, ApdRecord->OctetLength);
        if (ApdRecord->OctetLengthPtr)
          Bind[ParameterNr].length= (ulong *)GetBindOffset(Stmt->Apd, ApdRecord, ApdRecord->OctetLengthPtr, CurrentOffset, sizeof(SQLLEN));
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

/* {{{ MADB_ConvertCharToBit */
char MADB_ConvertCharToBit(MADB_Stmt *Stmt, char *src)
{
  char *EndPtr= NULL;
  float asNumber= strtof(src, &EndPtr);

  if (asNumber < 0 || asNumber > 1)
  {
    /* 22003 */
  }
  else if (asNumber != 0 && asNumber != 1)
  {
    /* 22001 */
  }
  else if(EndPtr != NULL && *EndPtr != '\0')
  {
    /* 22018. TODO: check if condition is correct */
  }

  return asNumber != 0 ? '\1' : '\0';
}
/* }}} */

BOOL MADB_ConversionSupported(MADB_DescRecord *From, MADB_DescRecord *To)
{
  switch (From->ConciseType)
  {
  case SQL_C_TIMESTAMP:
  case SQL_C_TYPE_TIMESTAMP:
  case SQL_C_TIME:
  case SQL_C_TYPE_TIME:
  case SQL_C_DATE:
  case SQL_C_TYPE_DATE:

    if (To->Type == SQL_INTERVAL)
    {
      return FALSE;
    }

  }
  return TRUE;
}

/* {{{ MADB_StmtExecute */
SQLRETURN MADB_StmtExecute(MADB_Stmt *Stmt)
{
  int          i;
  MYSQL_RES   *DefaultResult= NULL;
  SQLRETURN    ret=           SQL_SUCCESS;
  unsigned int ErrorCount=    0;
  unsigned int StatementNr;
  unsigned int ParamOffset=   0; /* for multi statements */
  unsigned int Iterations=    1;
  SQLLEN       j, Start=      0;

  BOOL         FetchedOutParams= FALSE;

  MDBUG_C_PRINT(Stmt->Connection, "%sMADB_StmtExecute", "\t->");

  MADB_CLEAR_ERROR(&Stmt->Error);

  if (Stmt->State == MADB_SS_EMULATED)
  {
    return MADB_ExecuteQuery(Stmt, Stmt->StmtString, (SQLINTEGER)strlen(Stmt->StmtString));
  }

  if (Stmt->PositionedCommand && Stmt->PositionedCursor)
  {
    return MADB_ExecutePositionedUpdate(Stmt);
  }

  /* Stmt->params was allocated during prepare, but could be cleared
     by SQLResetStmt. In latter case we need to allocate it again */
  if (!Stmt->params &&
      !(Stmt->params = (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * mysql_stmt_param_count(Stmt->stmt))))
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

  if (Stmt->Ard->Header.BindOffsetPtr && Stmt->Ard->Header.BindType)
    Start= *Stmt->Ard->Header.BindOffsetPtr / Stmt->Ard->Header.BindType;

  LOCK_MARIADB(Stmt->Connection);
  Stmt->AffectedRows= 0;
  Start+= Stmt->ArrayOffset;

   if (Stmt->Ipd->Header.RowsProcessedPtr)
      *Stmt->Ipd->Header.RowsProcessedPtr= 0;

  /* calculate Parameter number for multi statements */
  if (Stmt->MultiStmts)
  {
    Iterations= Stmt->MultiStmtCount;    
  }

  for (StatementNr=0; StatementNr < Iterations; StatementNr++)
  {
    if (Stmt->MultiStmts)
    {
      Stmt->stmt= Stmt->MultiStmts[StatementNr];
      Stmt->ParamCount= Stmt->MultiStmts[StatementNr]->param_count;
      Stmt->RebindParams= TRUE;
    }
    /* Convert and bind parameters */
    for (j= Start; j < Start + (int)MAX(Stmt->Apd->Header.ArraySize * test(Stmt->ParamCount), 1); ++j)
    {
      SQLRETURN RowResult= SQL_SUCCESS;

      if (Stmt->Apd->Header.ArrayStatusPtr &&
          Stmt->Apd->Header.ArrayStatusPtr[j-Start] == SQL_PARAM_IGNORE)
      {
        if (Stmt->Ipd->Header.RowsProcessedPtr)
          *Stmt->Ipd->Header.RowsProcessedPtr= *Stmt->Ipd->Header.RowsProcessedPtr + 1;
        if (Stmt->Ipd->Header.ArrayStatusPtr)
          Stmt->Ipd->Header.ArrayStatusPtr[j-Start]= SQL_PARAM_UNUSED;
        continue;
      }

      for (i=ParamOffset;(unsigned int)i < ParamOffset + Stmt->ParamCount; ++i)
      {
        MADB_DescRecord *ApdRecord, *IpdRecord;

        if ((ApdRecord= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ)) &&
            (IpdRecord= MADB_DescGetInternalRecord(Stmt->Ipd, i, MADB_DESC_READ)))
        {
          SQLLEN *IndicatorPtr= NULL;
          SQLLEN *OctetLengthPtr= NULL;
          void   *DataPtr= NULL;
          SQLLEN  Length= 0;

          /* check if parameter was bound */
          if (!ApdRecord->inUse)
          {
            MADB_SetError(&Stmt->Error, MADB_ERR_07002, NULL, 0);
            ret= Stmt->Error.ReturnValue;
            goto end;
          }

          if (ApdRecord->IndicatorPtr)
            IndicatorPtr= (SQLLEN *)GetBindOffset(Stmt->Apd,ApdRecord, ApdRecord->IndicatorPtr, j - Start, sizeof(SQLLEN));
          
          if (ApdRecord->OctetLengthPtr)
          {
            OctetLengthPtr= (SQLLEN *)GetBindOffset(Stmt->Apd, ApdRecord, ApdRecord->OctetLengthPtr, j - Start, sizeof(SQLLEN));
            Length= *OctetLengthPtr;
          }
        
          DataPtr= GetBindOffset(Stmt->Apd, ApdRecord, ApdRecord->DataPtr, j-Start, ApdRecord->OctetLength);

          if (Stmt->stmt->params)
            Stmt->params[i-ParamOffset].long_data_used= Stmt->stmt->params[i-ParamOffset].long_data_used;

          if (PARAM_IS_DAE(OctetLengthPtr))
          {
            if (!DAE_DONE(Stmt))
            {
              ret= SQL_NEED_DATA;
              goto end;
            }
          }

          if (MADB_ConversionSupported(ApdRecord, IpdRecord) == FALSE)
          {
            ret= MADB_SetError(&Stmt->Error, MADB_ERR_07006, NULL, 0);
            goto end;
          }

          /* Long if-else's to fill BIND structures */
          if (IndicatorPtr && *IndicatorPtr == SQL_COLUMN_IGNORE)
          {
            char *Ptr= NULL;
            if (!ApdRecord->DefaultValue)
              Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_NULL;
            else 
            {
              Stmt->params[i-ParamOffset].buffer=       ApdRecord->DefaultValue;
              Stmt->params[i-ParamOffset].length_value= (unsigned long)strlen(ApdRecord->DefaultValue);
              Stmt->params[i-ParamOffset].buffer_type=  MYSQL_TYPE_STRING;
            }
          }
          else if (((IndicatorPtr && *IndicatorPtr == SQL_NULL_DATA) || !DataPtr) &&
              !Stmt->params[i-ParamOffset].long_data_used)
          {
            Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_NULL;
          }
          /* Not DAE in other words. i.e. general case */
          else if (!Stmt->params[i-ParamOffset].long_data_used)
          {
            /* If no OctetLengthPtr was specified, or OctetLengthPtr is SQL_NTS character
                or binary data are null terminated */
            if (!OctetLengthPtr || *OctetLengthPtr == SQL_NTS)
            {
              if (DataPtr)
              {
                if (ApdRecord->ConciseType == SQL_C_WCHAR)
                {
                  Length= SqlwcsLen((SQLWCHAR *)DataPtr) * sizeof(SQLWCHAR);
                }
                else if (ApdRecord->ConciseType == SQL_C_CHAR)
                {
                  Length= strlen((SQLCHAR *)DataPtr);
                }
              }
              if (!OctetLengthPtr && ApdRecord->OctetLength && ApdRecord->OctetLength != SQL_SETPARAM_VALUE_MAX)
              {
                Length= MIN(Length, ApdRecord->OctetLength);
              }
            }
            Stmt->params[i-ParamOffset].length= 0;

            switch (ApdRecord->ConciseType)
            {
            case SQL_C_WCHAR:
              {
                SQLULEN mbLength=0;

                MADB_FREE(ApdRecord->InternalBuffer);

                ApdRecord->InternalBuffer= MADB_ConvertFromWChar((SQLWCHAR *)DataPtr, (SQLINTEGER)(Length / sizeof(SQLWCHAR)), 
                                                                 &mbLength, &Stmt->Connection->charset, NULL);
                ApdRecord->InternalLength=              (unsigned long)mbLength;
                Stmt->params[i-ParamOffset].length=     &ApdRecord->InternalLength;
                Stmt->params[i-ParamOffset].buffer=      ApdRecord->InternalBuffer;
                Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_STRING;
              }
              break;
            case SQL_C_CHAR:
              if (!Stmt->stmt->params[i-ParamOffset].long_data_used)
              {
                if (IpdRecord->ConciseType == SQL_BIT)
                {
                  MADB_FREE(ApdRecord->InternalBuffer);
                  ApdRecord->InternalLength= 1;
                  ApdRecord->InternalBuffer= (char *)MADB_CALLOC(ApdRecord->InternalLength);

                  *ApdRecord->InternalBuffer=  MADB_ConvertCharToBit(Stmt, DataPtr);

                  Stmt->params[i-ParamOffset].buffer=      ApdRecord->InternalBuffer;
                  Stmt->params[i-ParamOffset].length=     &ApdRecord->InternalLength;
                  Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_TINY;
                }
                else
                {
                  ApdRecord->InternalLength= (unsigned long)Length;
                  Stmt->params[i-ParamOffset].length= &ApdRecord->InternalLength;
                  Stmt->params[i-ParamOffset].buffer= DataPtr;
                  Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_STRING;
                }
              } 
              break;
            case SQL_C_NUMERIC:
              {
                SQL_NUMERIC_STRUCT *p;
                int ErrorCode= 0;

                if (Stmt->RebindParams)
                {
                  MADB_FREE(ApdRecord->InternalBuffer);
                  ApdRecord->InternalBuffer= (char *)MADB_CALLOC(80);
                }
                p= (SQL_NUMERIC_STRUCT *)DataPtr;
                p->scale= (SQLSCHAR)IpdRecord->Scale;
                p->precision= (SQLSCHAR)IpdRecord->Precision;
                ApdRecord->InternalLength= (unsigned long)MADB_SqlNumericToChar((SQL_NUMERIC_STRUCT *)p, ApdRecord->InternalBuffer, &ErrorCode);
                if (ErrorCode)
                {
                  /*TODO: I guess this parameters row should be skipped */
                  RowResult= MADB_SetError(&Stmt->Error, ErrorCode, NULL, 0);
                }
                Stmt->params[i-ParamOffset].length= &ApdRecord->InternalLength;
                Stmt->params[i-ParamOffset].buffer= ApdRecord->InternalBuffer;
                Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_STRING;
              }
              break;
            case SQL_C_TIMESTAMP:
            case SQL_TYPE_TIMESTAMP:
              {
                MYSQL_TIME           *tm= NULL;
                SQL_TIMESTAMP_STRUCT *ts= (SQL_TIMESTAMP_STRUCT *)DataPtr;

                MADB_FREE(ApdRecord->InternalBuffer);

                if (IpdRecord->ConciseType != SQL_TYPE_TIME && (ts->year == 0 || ts->month == 0 || ts->day == 0))
                {
                  ret= MADB_SetError(&Stmt->Error, MADB_ERR_22007, NULL, 0);
                  goto end;
                }

                tm= (MYSQL_TIME *)MADB_CALLOC(sizeof(MYSQL_TIME));

                if (tm == NULL)
                {
                  ret= MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
                  goto end;
                }

                /* Default types. Not quite clear if time_type has any effect */
                tm->time_type=                           MYSQL_TIMESTAMP_DATETIME;
                Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_TIMESTAMP;

                switch (IpdRecord->ConciseType) {
                case SQL_TYPE_DATE:

                  if (ts->hour + ts->minute + ts->second + ts->fraction)
                  {
                    ret= MADB_SetError(&Stmt->Error, MADB_ERR_22008, NULL, 0);
                    MADB_FREE(tm);
                    goto end;
                  }
                  Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_DATE;
                  tm->time_type=                           MYSQL_TIMESTAMP_DATE;
                  break;
                case SQL_TYPE_TIME:
                  if (ts->fraction)
                  {
                    ret= MADB_SetError(&Stmt->Error, MADB_ERR_22008, NULL, 0);
                    MADB_FREE(tm);
                    goto end;
                  }
                  Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_TIME;
                  tm->time_type=                           MYSQL_TIMESTAMP_TIME;
                  break;
                }
                tm->year=  ts->year;
                tm->month= ts->month;
                tm->day=   ts->day;

                tm->hour=   ts->hour;
                tm->minute= ts->minute;
                tm->second= ts->second;
                
                tm->second_part= ts->fraction / 1000;

                ApdRecord->InternalBuffer= (void *)tm;
                Stmt->params[i-ParamOffset].buffer= ApdRecord->InternalBuffer;
                Stmt->params[i-ParamOffset].length_value= sizeof(MYSQL_TIME);
              }
              break;
              case SQL_C_TIME:
              case SQL_TYPE_TIME:
              {
                MYSQL_TIME      *tm;
                SQL_TIME_STRUCT *ts= (SQL_TIME_STRUCT *)DataPtr;

                if ((IpdRecord->ConciseType == SQL_TYPE_TIME || IpdRecord->ConciseType == SQL_TYPE_TIMESTAMP ||
                  IpdRecord->ConciseType == SQL_TIME || IpdRecord->ConciseType == SQL_TIMESTAMP || IpdRecord->ConciseType == SQL_DATETIME) &&
                  ts->hour > 23|| ts->minute > 59 || ts->second > 59)
                {
                  ret= MADB_SetError(&Stmt->Error, MADB_ERR_22007, NULL, 0);
                  goto end;
                }

                MADB_FREE(ApdRecord->InternalBuffer);
                tm= (MYSQL_TIME *)MADB_CALLOC(sizeof(MYSQL_TIME));

                tm->year=  1970;
                tm->month= 1;
                tm->day=   1;

                tm->hour=   ts->hour;
                tm->minute= ts->minute;
                tm->second= ts->second;

                tm->second_part= 0;

                tm->time_type= MYSQL_TIMESTAMP_DATETIME;

                ApdRecord->InternalBuffer= (void *)tm;
                Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_DATETIME;
                Stmt->params[i-ParamOffset].buffer= ApdRecord->InternalBuffer;
                Stmt->params[i-ParamOffset].length_value= sizeof(MYSQL_TIME);
              }
              break;
              case SQL_C_INTERVAL_HOUR_TO_MINUTE:
              case SQL_C_INTERVAL_HOUR_TO_SECOND:
              {
                SQL_INTERVAL_STRUCT *is= (SQL_INTERVAL_STRUCT *)DataPtr;
                MYSQL_TIME          *tm= (MYSQL_TIME *)MADB_CALLOC(sizeof(MYSQL_TIME));

                tm->hour=   is->intval.day_second.hour;
                tm->minute= is->intval.day_second.minute;
                tm->second= ApdRecord->ConciseType == SQL_C_INTERVAL_HOUR_TO_SECOND ? is->intval.day_second.second : 0;

                tm->second_part= 0;

                tm->time_type= MYSQL_TIMESTAMP_TIME;
                ApdRecord->InternalBuffer= (void *)tm;
                Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_TIME;
                Stmt->params[i-ParamOffset].buffer= ApdRecord->InternalBuffer;
                Stmt->params[i-ParamOffset].length_value= sizeof(MYSQL_TIME);
              }
              break;

              case SQL_C_DATE:
              case SQL_TYPE_DATE:
              {
                MYSQL_TIME *tm;
                SQL_DATE_STRUCT *ts= (SQL_DATE_STRUCT *)DataPtr;
 
                MADB_FREE(ApdRecord->InternalBuffer);
                tm= (MYSQL_TIME *)MADB_CALLOC(sizeof(MYSQL_TIME));

                tm->year=  ts->year;
                tm->month= ts->month;
                tm->day=   ts->day;

                tm->hour= tm->minute= tm->second= tm->second_part= 0;

                tm->time_type= MYSQL_TIMESTAMP_DATE;

                ApdRecord->InternalBuffer= (void *)tm;
                Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_DATE;
                Stmt->params[i-ParamOffset].buffer= ApdRecord->InternalBuffer;
                Stmt->params[i-ParamOffset].length_value= sizeof(MYSQL_TIME);
              }
              break;
            case SQL_BINARY:
            case SQL_LONGVARBINARY:
              {
                Stmt->params[i-ParamOffset].buffer= (char *)DataPtr;

                if (OctetLengthPtr && *OctetLengthPtr == SQL_NTS)
                {
                  *OctetLengthPtr= strlen((char *)Stmt->params[i-ParamOffset].buffer);
                }

                if (OctetLengthPtr)
                {
                  Stmt->params[i-ParamOffset].length_value= (unsigned long)*OctetLengthPtr;
                }
                else
                {
                  Stmt->params[i-ParamOffset].length_value= (unsigned long)ApdRecord->OctetLength;
                }

                Stmt->params[i-ParamOffset].buffer_type= MYSQL_TYPE_BLOB;
              }
              break;
            default:
              if (!Stmt->params[i-ParamOffset].long_data_used)
              {
                memset(&Stmt->params[i-ParamOffset], 0, sizeof(MYSQL_BIND));
                Stmt->params[i-ParamOffset].buffer_type= MADB_GetTypeAndLength(ApdRecord->ConciseType,
                                      &Stmt->params[i-ParamOffset].is_unsigned, &Stmt->params[i-ParamOffset].buffer_length);
                if (!ApdRecord->OctetLength)
                  ApdRecord->OctetLength= Stmt->params[i-ParamOffset].buffer_length;
                Stmt->params[i-ParamOffset].buffer= DataPtr;
              }
            }
          }
          if (Stmt->params[i-ParamOffset].long_data_used)
            Stmt->params[i-ParamOffset].buffer_type= MADB_GetTypeAndLength(ApdRecord->ConciseType,
                              &Stmt->params[i-ParamOffset].is_unsigned, &Stmt->params[i-ParamOffset].buffer_length);
        }
      }
      if (Stmt->RebindParams && Stmt->ParamCount)
      {
        Stmt->stmt->bind_param_done= 1;
        Stmt->RebindParams= FALSE;
      }
      if (Stmt->ParamCount)
      {
        memcpy(Stmt->stmt->params, Stmt->params, sizeof(MYSQL_BIND) * Stmt->ParamCount);
        Stmt->stmt->send_types_to_server= 1;
      }
      ret= SQL_SUCCESS;

      /**************************** mysql_stmt_execute *************************************/
      if (mysql_stmt_execute(Stmt->stmt))
      {
        MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
        ret= Stmt->Error.ReturnValue;
        ErrorCount++;
        MDBUG_C_PRINT(Stmt->Connection, "mysql_stmt_execute:ERROR%s", "");
      }
      else
      {
        Stmt->State= MADB_SS_EXECUTED;

        if (Stmt->stmt->mysql->server_status & SERVER_PS_OUT_PARAMS)
        {
          FetchedOutParams= TRUE;
          ret= Stmt->Methods->GetOutParams(Stmt, 0);
        }
      }
 
      /* We need to unset InternalLength */
      for (i=ParamOffset; (unsigned int)i < ParamOffset + Stmt->ParamCount; i++)
      {
        MADB_DescRecord *ApdRecord; 
        if ((ApdRecord= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ)))
          ApdRecord->InternalLength= 0;
      }

      /* todo: do we need GetBindOffset here ?! */
      if (Stmt->Ipd->Header.RowsProcessedPtr)
        *Stmt->Ipd->Header.RowsProcessedPtr= *Stmt->Ipd->Header.RowsProcessedPtr + 1;
      if (Stmt->Ipd->Header.ArrayStatusPtr)
        Stmt->Ipd->Header.ArrayStatusPtr[j-Start]= SQL_SUCCEEDED(ret) ? SQL_PARAM_SUCCESS : 
                                                    (j == Stmt->Apd->Header.ArraySize - 1) ? SQL_PARAM_ERROR : SQL_PARAM_DIAG_UNAVAILABLE;
      if (!mysql_stmt_field_count(Stmt->stmt) && SQL_SUCCEEDED(ret) && !Stmt->MultiStmts)
        Stmt->AffectedRows+= mysql_stmt_affected_rows(Stmt->stmt);
      Stmt->ArrayOffset++;
      if (!SQL_SUCCEEDED(ret) && j == Start + Stmt->Apd->Header.ArraySize)
        goto end;
    }
    if (Stmt->MultiStmts)
    {
      ParamOffset+= Stmt->ParamCount;
      if(mysql_stmt_field_count(Stmt->stmt))
        mysql_stmt_store_result(Stmt->stmt);
    }
  }
  
  /* All rows processed, so we can unset ArrayOffset */
  Stmt->ArrayOffset= 0;

  if (Stmt->MultiStmts && !mysql_stmt_field_count(Stmt->stmt))
  {
    Stmt->AffectedRows= mysql_stmt_affected_rows(Stmt->stmt);
    Stmt->MultiStmtNr= 0;
  }

  if (!Stmt->MultiStmts && mysql_stmt_field_count(Stmt->stmt) > 0)
  {
    Stmt->CharOffset= (unsigned long *)my_realloc((gptr)Stmt->CharOffset, 
                                                    sizeof(long) * mysql_stmt_field_count(Stmt->stmt),
                                                    MYF(MY_ZEROFILL) | MYF(MY_ALLOW_ZERO_PTR));
    Stmt->Lengths= (unsigned long *)my_realloc((gptr)Stmt->Lengths, 
                                                   sizeof(long) * mysql_stmt_field_count(Stmt->stmt),
                                                    MYF(MY_ZEROFILL) | MYF(MY_ALLOW_ZERO_PTR));

    /* Todo: for SQL_CURSOR_FORWARD_ONLY we should use cursor and prefetch rows */
    /*************************** mysql_stmt_store_result ******************************/
    /*If we did OUT params already, we should not store */
    if (FetchedOutParams == FALSE && mysql_stmt_store_result(Stmt->stmt) != 0)
    {
      UNLOCK_MARIADB(Stmt->Connection);
      if (DefaultResult)
        mysql_free_result(DefaultResult);

      return MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt);
    }

    MADB_STMT_RESET_CURSOR(Stmt);
    
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
     /*ValueType= Stmt->Connection->IsAnsi ? MADB_GetDefaultType(ParameterType)
                                         : MADB_GetWCharType(MADB_GetDefaultType(ParameterType));*/
   }
   
   if (!(SQL_SUCCEEDED(MADB_DescSetField(Apd, ParameterNumber, SQL_DESC_CONCISE_TYPE, (SQLPOINTER)(SQLLEN)ValueType, SQL_IS_SMALLINT, 0))) ||
       !(SQL_SUCCEEDED(MADB_DescSetField(Apd, ParameterNumber, SQL_DESC_OCTET_LENGTH_PTR, (SQLPOINTER)StrLen_or_IndPtr, SQL_IS_POINTER, 0))) ||
       !(SQL_SUCCEEDED(MADB_DescSetField(Apd, ParameterNumber, SQL_DESC_OCTET_LENGTH, (SQLPOINTER)BufferLength, SQL_IS_INTEGER, 0))) ||
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

  for (i=0; i < MADB_STMT_COLUMN_COUNT(Stmt); ++i)
  {
    ArdRec= MADB_DescGetInternalRecord(Stmt->Ard, i, MADB_DESC_READ);
    if (ArdRec == NULL || !ArdRec->inUse)
    {      
      Stmt->result[i].flags|= MADB_BIND_DUMMY;
      continue;
    }

    DataPtr= (SQLLEN *)GetBindOffset(Stmt->Ard, ArdRec, ArdRec->DataPtr,        RowNumber, ArdRec->OctetLength);

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
      ArdRec->InternalBuffer=       (char *)MADB_CALLOC(MADB_DEFAULT_PRECISION + 1/*-*/ + 1/*.*/);
      Stmt->result[i].buffer=        ArdRec->InternalBuffer;
      Stmt->result[i].buffer_length= MADB_DEFAULT_PRECISION + 2;
      Stmt->result[i].buffer_type=   MYSQL_TYPE_STRING;
      break;
    case SQL_TYPE_TIMESTAMP:
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIME:
    case SQL_C_TIMESTAMP:
    case SQL_C_TIME:
    case SQL_C_DATE:
      MADB_FREE(ArdRec->InternalBuffer);
      ArdRec->InternalBuffer=       (char *)MADB_CALLOC(sizeof(MYSQL_TIME));
      Stmt->result[i].buffer=        ArdRec->InternalBuffer;
      Stmt->result[i].buffer_length= sizeof(MYSQL_TIME);
      Stmt->result[i].buffer_type=   MYSQL_TYPE_TIMESTAMP;
      break;
    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
    case SQL_C_INTERVAL_HOUR_TO_SECOND:
      {
        MYSQL_FIELD *Field= mysql_fetch_field_direct(Stmt->metadata, i);
        MADB_FREE(ArdRec->InternalBuffer);
        ArdRec->InternalBuffer=       (char *)MADB_CALLOC(sizeof(MYSQL_TIME));
        Stmt->result[i].buffer=        ArdRec->InternalBuffer;
        Stmt->result[i].buffer_length= sizeof(MYSQL_TIME);
        Stmt->result[i].buffer_type=   Field && Field->type == MYSQL_TYPE_TIME ? MYSQL_TYPE_TIME : MYSQL_TYPE_TIMESTAMP;
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
      Stmt->result[i].buffer_type=   MADB_GetTypeAndLength(ArdRec->ConciseType,
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
SQLRETURN MADB_FixFetchedValues(MADB_Stmt *Stmt, int RowNumber, MYSQL_ROWS *SaveCursor)
{
  MADB_DescRecord *IrdRec, *ArdRec;
  int             i;
  SQLLEN          *IndicatorPtr= NULL;
  void            *DataPtr=      NULL;
  SQLRETURN       rc= SQL_SUCCESS, FieldRc;

  for (i= 0; i < MADB_STMT_COLUMN_COUNT(Stmt); ++i)
  {
    if ((ArdRec= MADB_DescGetInternalRecord(Stmt->Ard, i, MADB_DESC_READ)) && ArdRec->inUse)
    {
      /* set indicator and dataptr */
      IndicatorPtr= (SQLLEN *)GetBindOffset(Stmt->Ard, ArdRec, ArdRec->OctetLengthPtr, RowNumber, sizeof(SQLLEN));
      DataPtr=      (SQLLEN *)GetBindOffset(Stmt->Ard, ArdRec, ArdRec->DataPtr,        RowNumber, ArdRec->OctetLength);
          
      /* clear IndicatorPtr */
      if (IndicatorPtr && *IndicatorPtr < 0)
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
            Stmt->stmt->result_cursor= SaveCursor;
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
            *p= test(*p != '\0');
        }
        break;
        case SQL_C_TYPE_TIMESTAMP:
        case SQL_C_TYPE_DATE:
        case SQL_C_TYPE_TIME:
        case SQL_C_TIMESTAMP:
        case SQL_C_TIME:
        case SQL_C_DATE:
          FieldRc= MADB_CopyMadbTimestamp(Stmt, (MYSQL_TIME *)ArdRec->InternalBuffer, Stmt->Ard, ArdRec, ArdRec->Type, RowNumber);
          CALC_ALL_FLDS_RC(rc, FieldRc);
          break;
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
        {
          MYSQL_TIME          *tm= (MYSQL_TIME*)ArdRec->InternalBuffer;
          SQL_INTERVAL_STRUCT *ts= (SQL_INTERVAL_STRUCT *)GetBindOffset(Stmt->Ard, ArdRec, ArdRec->DataPtr, RowNumber, ArdRec->OctetLength);;

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
          if (IndicatorPtr)
          {
            *IndicatorPtr= sizeof(SQL_INTERVAL_STRUCT);
          }
        }
        break;
        case SQL_C_NUMERIC:
        {
          int rc= 0;
          MADB_CLEAR_ERROR(&Stmt->Error);
          if (Stmt->result[i].buffer_length < Stmt->stmt->fields[i].max_length)
          {
            MADB_SetError(&Stmt->Error, MADB_ERR_22003, NULL, 0);
            ArdRec->InternalBuffer[Stmt->result[i].buffer_length - 1]= 0;
            return Stmt->Error.ReturnValue;
          }

          if ((rc= MADB_CharToSQLNumeric(ArdRec->InternalBuffer, Stmt->Ard, ArdRec, RowNumber)))
            MADB_SetError(&Stmt->Error, rc, NULL, 0);
          if (Stmt->Ard->Header.ArrayStatusPtr)
            Stmt->Ard->Header.ArrayStatusPtr[RowNumber]= Stmt->Error.ReturnValue;
        }
        break;
        case SQL_C_WCHAR:
        {
          SQLLEN CharLen= MADB_SetString(&Stmt->Connection->charset, DataPtr, ArdRec->OctetLength, (char *)Stmt->result[i].buffer,
            *Stmt->stmt->bind[i].length, &Stmt->Error);


          /*MADB_ConvertAnsi2Unicode(&Stmt->Connection->charset, (char *)Stmt->result[i].buffer, *Stmt->stmt->bind[i].length,
                                   (SQLWCHAR *)DataPtr, ArdRec->OctetLength, &CharLen, 1, &Stmt->Error);*/
          /* Not quite right */
          if (IndicatorPtr)
            *IndicatorPtr= CharLen * sizeof(SQLWCHAR);
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
            break;
          }
          /* else {we are falling through below} */
        default:
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
          // Stmt->result[i].buffer = (char *)Stmt->result[i].buffer + Stmt->result[i].length;
          if (IndicatorPtr)
            *IndicatorPtr= *Stmt->stmt->bind[i].length;
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
  unsigned int j, rc;
  SQLULEN      ArraySize=  Stmt->Ard->Header.ArraySize, Processed, *ProcessedPtr= &Processed;
  MYSQL_ROWS  *SaveCursor= NULL;
  SQLRETURN    Result= SQL_SUCCESS, RowResult;

  MADB_CLEAR_ERROR(&Stmt->Error);

  if (!(MADB_STMT_COLUMN_COUNT(Stmt) > 0))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_24000, NULL, 0);
  }

  if ((Stmt->Options.UseBookmarks == SQL_UB_VARIABLE && Stmt->Options.BookmarkType != SQL_C_VARBOOKMARK) ||
      (Stmt->Options.UseBookmarks != SQL_UB_VARIABLE && Stmt->Options.BookmarkType == SQL_C_VARBOOKMARK))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_07006, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  /* We don't know if any of the ARD parameter changed, so we need to rebind */
  //MADB_FREE(Stmt->result);

  if (Stmt->Ird->Header.ArrayStatusPtr)
  {
    memset(Stmt->Ird->Header.ArrayStatusPtr, SQL_ROW_ERROR, ArraySize);
  }

  Stmt->LastRowFetched= 0;

  if (Stmt->result == NULL && !(Stmt->result= (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * mysql_stmt_field_count(Stmt->stmt))))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  if (Stmt->Ird->Header.RowsProcessedPtr)
  {
    ProcessedPtr= Stmt->Ird->Header.RowsProcessedPtr;
  }

  if (Stmt->Ird->Header.ArrayStatusPtr)
  {
    MADB_InitStatusPtr(Stmt->Ird->Header.ArrayStatusPtr, ArraySize, SQL_ROW_NOROW);
  }

  *ProcessedPtr= 0;


  if (ArraySize > 1 && Stmt->Options.CursorType != SQL_CURSOR_FORWARD_ONLY)
  {
    SaveCursor= Stmt->stmt->result_cursor;
  }

  for (j=0; j < ArraySize; ++j)
  {
    RowResult= SQL_SUCCESS;
    /*************** Setting up BIND structures ********************/
    /* Basically, nothing should happen here, but if happens, then it will happen on each row.
       Thus it's ok to stop */
    RETURN_ERROR_OR_CONTINUE(MADB_PrepareBind(Stmt, j));

    /************************ Bind! ********************************/  
    mysql_stmt_bind_result(Stmt->stmt, Stmt->result);

    if (Stmt->Options.UseBookmarks)
    {
      /* TODO: Bookmark can be not only "unsigned long*", but also "unsigned char*". Can be determined by examining Stmt->Options.BookmarkType */
      long *p= (long *)Stmt->Options.BookmarkPtr;
      p+= j * Stmt->Options.BookmarkLength;
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
        Stmt->Ird->Header.ArrayStatusPtr[j]= MADB_MapToRowStatus(RowResult);
      }

      CALC_ALL_ROWS_RC(Result, RowResult, j);
      
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
      if (j > 0)
      {
        return Result;
      }
      return SQL_NO_DATA;
    }

    ++Stmt->LastRowFetched;
    ++Stmt->PositionedCursor;

    /*Conversion etc. At this point, after fetch we can have RowResult either SQL_SUCCESS or SQL_SUCCESS_WITH_INFO */
    switch (MADB_FixFetchedValues(Stmt, j, SaveCursor))
    {
    case SQL_ERROR:
      RowResult= SQL_ERROR;
      break;
    case SQL_SUCCESS_WITH_INFO:
      RowResult= SQL_SUCCESS_WITH_INFO;
    /* And if result of conversions - success, just leaving that we had before */
    }

    CALC_ALL_ROWS_RC(Result, RowResult, j);

    if (Stmt->Ird->Header.ArrayStatusPtr)
    {
      Stmt->Ird->Header.ArrayStatusPtr[j]= MADB_MapToRowStatus(RowResult);
    }

  }
    
  Stmt->CharOffset= (unsigned long *)my_realloc((gptr)Stmt->CharOffset, 
                                                sizeof(long) * mysql_stmt_field_count(Stmt->stmt),
                                                MYF(MY_ZEROFILL) | MYF(MY_ALLOW_ZERO_PTR));
  memset(Stmt->CharOffset, 0, sizeof(long) * mysql_stmt_field_count(Stmt->stmt));
  Stmt->Lengths= (unsigned long *)my_realloc((gptr)Stmt->Lengths, 
                                                sizeof(long) * mysql_stmt_field_count(Stmt->stmt),
                                                MYF(MY_ZEROFILL) | MYF(MY_ALLOW_ZERO_PTR));
  memset(Stmt->Lengths, 0, sizeof(long) * mysql_stmt_field_count(Stmt->stmt));

  ResetDescIntBuffers(Stmt->Ird);

  if (SaveCursor != NULL)
  {
    Stmt->stmt->result_cursor= SaveCursor;
    //Stmt->stmt->bind
  }

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
    *(SQLINTEGER *)ValuePtr= Stmt->Apd->Header.BindType;
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
    *(SQLINTEGER *)ValuePtr= Stmt->Ard->Header.BindType;
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
        IntStmt = (MADB_Stmt **)alloc_dynamic(&Stmt->Apd->Stmts);
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
        IntStmt = (MADB_Stmt **)alloc_dynamic(&Stmt->Ard->Stmts);
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
    Stmt->Options.MetadataId=(SQLUINTEGER)(SQLULEN)ValuePtr;
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
      *StrLen_or_IndPtr= sizeof(SQLULEN);
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
  unsigned char   *SavePtr;
  MADB_DescRecord *IrdRec= NULL;
  MYSQL_FIELD     *Field= mysql_fetch_field_direct(Stmt->metadata, Offset);

  /* Should not really happen, and is evidence of that something wrong happened in some previous call(SQLFetch?) */
  if (Stmt->stmt->bind == NULL)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY109, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  SavePtr= Stmt->stmt->bind[Offset].row_ptr;

  if (Stmt->stmt->bind[Offset].row_ptr == NULL)
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
      /*OdbcType= Stmt->Connection->IsAnsi || (Col_or_Param_Num == 10 && BufferLength == 2) ?
                IrdRec->ConciseType : MADB_GetWCharType(IrdRec->ConciseType);*/
    }
    break;
  default:
    OdbcType= TargetType;
    break;  
  }
  /* Restoring mariadb/mysql type from odbc type */
  MadbType= MADB_GetTypeAndLength(OdbcType, &Bind.is_unsigned, &Bind.buffer_length);

  /* set global values for Bind */
  Bind.error=   &Error;
  Bind.length=  &Bind.length_value;
  Bind.is_null= &IsNull;

  switch(OdbcType)
  {
  case SQL_DATE:
  case SQL_C_TYPE_DATE:
    {
      MYSQL_TIME tm;
      SQL_DATE_STRUCT *ts= (SQL_DATE_STRUCT *)TargetValuePtr;

      Bind.buffer_length= sizeof(MYSQL_TIME);
      Bind.buffer= (void *)&tm;
      Bind.buffer_type= MYSQL_TYPE_TIMESTAMP;
      mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);
      ts->year= tm.year;
      ts->month= tm.month;
      ts->day= tm.day;
      if (StrLen_or_IndPtr)
        *StrLen_or_IndPtr= sizeof(SQL_DATE_STRUCT);
    }
    break;
  case SQL_TIMESTAMP:
  case SQL_C_TYPE_TIMESTAMP:
    {
      MYSQL_TIME tm;
      SQL_TIMESTAMP_STRUCT *ts= (SQL_TIMESTAMP_STRUCT *)TargetValuePtr;

      Bind.buffer_length= sizeof(MYSQL_TIME);
      Bind.buffer= (void *)&tm;
      Bind.buffer_type= MYSQL_TYPE_TIMESTAMP;
      mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);
      if (!tm.year)
      {
        time_t sec_time;
        struct tm * cur_tm;
        sec_time= time(NULL);
        cur_tm= localtime(&sec_time);
        ts->year= 1900 + cur_tm->tm_year;
        ts->month= cur_tm->tm_mon + 1;
        ts->day= cur_tm->tm_mday;
      }
      else
      {
        ts->year= tm.year;
        ts->month= tm.month;
        ts->day= tm.day;
      }
      ts->hour= tm.hour;
      ts->minute= tm.minute;
      ts->second= tm.second;
      ts->fraction= tm.second_part * 1000;
      if (StrLen_or_IndPtr)
        *StrLen_or_IndPtr= sizeof(SQL_TIMESTAMP_STRUCT);
    }
    break;
  case SQL_TIME:
  case SQL_C_TYPE_TIME:
    {
      MYSQL_TIME tm;
      SQL_TIME_STRUCT *ts= (SQL_TIME_STRUCT *)TargetValuePtr;

      Bind.buffer_length= sizeof(MYSQL_TIME);
      Bind.buffer= (void *)&tm;
      /* c/c is too smart to convert hours to days and days to hours, we don't need that */
      Bind.buffer_type= Field && Field->type == MYSQL_TYPE_TIME ? MYSQL_TYPE_TIME : MYSQL_TYPE_TIMESTAMP;
      mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);
      ts->hour= tm.hour;
      ts->minute= tm.minute;
      ts->second= tm.second;

      if (tm.hour > 23)
      {
        return MADB_SetError(&Stmt->Error, MADB_ERR_22007, NULL, 0);
      }
      if (tm.second_part)
      {
        return MADB_SetError(&Stmt->Error, MADB_ERR_01S07, NULL, 0);
      }
      if (StrLen_or_IndPtr)
        *StrLen_or_IndPtr= sizeof(SQL_TIME_STRUCT);
    }
    break;
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
    {
      MYSQL_TIME tm;
      SQL_INTERVAL_STRUCT *ts= (SQL_INTERVAL_STRUCT *)TargetValuePtr;

      Bind.buffer_length= sizeof(MYSQL_TIME);
      Bind.buffer= (void *)&tm;
      /* c/c is too smart to convert hours to days and days to hours, we don't need that */
      Bind.buffer_type= Field && Field->type == MYSQL_TYPE_TIME ? MYSQL_TYPE_TIME : MYSQL_TYPE_TIMESTAMP;

      mysql_stmt_fetch_column(Stmt->stmt, &Bind, Offset, 0);

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
        *StrLen_or_IndPtr= sizeof(SQL_INTERVAL_STRUCT);
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

        /* reset row_ptr */
        Stmt->stmt->bind[Offset].row_ptr= SavePtr;

        /* check total length: if not enough space, we need to calculate new CharOffset for next fetch */
        if (Stmt->stmt->fields[Offset].max_length)
        {
          size_t ReqBuffOctetLen;
          /* Size in chars */
          CharLength= MbstrCharLen(ClientValue, Stmt->stmt->fields[Offset].max_length - Stmt->CharOffset[Offset],
            Stmt->Connection->charset.cs_info);
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

              CharLength= MADB_SetString(&Stmt->Connection->charset, IrdRec->InternalBuffer, (SQLINTEGER)ReqBuffOctetLen,
                ClientValue, Stmt->stmt->fields[Offset].max_length - Stmt->CharOffset[Offset], &Stmt->Error);
            }
            else
            {
              /* Application's buffer is big enough - writing directly there */
              CharLength= MADB_SetString(&Stmt->Connection->charset, TargetValuePtr, (SQLINTEGER)(BufferLength / sizeof(SQLWCHAR)),
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
            Stmt->Lengths[Offset]= CharLength*sizeof(SQLWCHAR);
          }
        }
      }
      else  /* IrdRec->InternalBuffer == NULL && Stmt->Lengths[Offset] == 0 */
      {
        CharLength= SqlwcsLen((SQLWCHAR*)((char*)IrdRec->InternalBuffer + Stmt->CharOffset[Offset]));
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
        Stmt->CharOffset[Offset]+= BufferLength - sizeof(SQLWCHAR);
        MADB_FREE(ClientValue);

        return MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
      }
      else
      {
        Stmt->CharOffset[Offset]= Stmt->Lengths[Offset];
        MADB_FREE(IrdRec->InternalBuffer);
      }

      MADB_FREE(ClientValue);
      Stmt->stmt->bind[Offset].row_ptr= SavePtr;
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
        *Bind.length= 0;
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
        if (Bind.length_value != 0 && (long)Bind.length_value != -1)
        {
          *StrLen_or_IndPtr= Bind.length_value;
        }
        else if (Bind.buffer_length > 0)
        {
          *StrLen_or_IndPtr= Bind.buffer_length;
        }
        else/* if (TargetType == SQL_C_DEFAULT)*/
        {
          *StrLen_or_IndPtr= IrdRec->OctetLength;
        }
      }
      /* Bind.fetch_result(&Bind, &Stmt->stmt->fields[Offset], &Stmt->stmt->bind[Offset].row_ptr); */
      Stmt->stmt->bind[Offset].row_ptr= SavePtr;
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
#ifdef MAODBC_DEBUG_DEVELOPER
  else if (MDBUG_C_IS_ON(Stmt->Connection) && (TargetType == SQL_C_DEFAULT || TargetType == SQL_ARD_TYPE))
  {
    ma_debug_print(1, "%s(%s)", IrdRec->BaseColumnName, IrdRec->ColumnName);
    ma_print_value(OdbcType, TargetValuePtr, Bind.length_value > 0 ? Bind.length_value : IrdRec->OctetLength);
  }
#endif
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_StmtRowCount */
SQLRETURN MADB_StmtRowCount(MADB_Stmt *Stmt, SQLLEN *RowCountPtr)
{
  if (Stmt->AffectedRows != -1)
    *RowCountPtr= (SQLLEN)Stmt->AffectedRows;
  else if (Stmt->stmt->result.rows && Stmt->stmt && mysql_stmt_field_count(Stmt->stmt))
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
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : NULL,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->BaseColumnName, strlen(Record->BaseColumnName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_BASE_TABLE_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : NULL,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->BaseTableName, strlen(Record->BaseTableName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_CASE_SENSITIVE:
    NumericAttribute= (SQLLEN)Record->CaseSensitive;
    break;
  case SQL_DESC_CATALOG_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->CatalogName, strlen(Record->CatalogName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_SCHEMA_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : 0,
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
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->LiteralPrefix, strlen(Record->LiteralPrefix), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_LITERAL_SUFFIX:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->LiteralSuffix, strlen(Record->LiteralSuffix), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_LOCAL_TYPE_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              "", 0, &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_LABEL:
  case SQL_DESC_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : 0,
                                              CharacterAttributePtr, (IsWchar) ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                              Record->ColumnName, strlen(Record->ColumnName), &Stmt->Error);
    IsNumericAttr= FALSE;
    break;
  case SQL_DESC_TYPE_NAME:
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : 0,
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
    StringLength= (SQLSMALLINT)MADB_SetString(IsWchar ? &Stmt->Connection->charset : 0,
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
    MADB_SetError(&Stmt->Error, MADB_ERR_HY091/*C00*/, NULL, 0);
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
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* TableName is mandatory */
  if (!TableName || !NameLength3)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }

  p= StmtStr;
  p+= my_snprintf(StmtStr, 1024, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL as TABLE_SCHEM, TABLE_NAME,"
                                 "COLUMN_NAME, NULL AS GRANTOR, GRANTEE, PRIVILEGE_TYPE AS PRIVILEGE,"
                                 "IS_GRANTABLE FROM INFORMATION_SCHEMA.COLUMN_PRIVILEGES WHERE ");
  if (CatalogName && CatalogName[0])
    p+= my_snprintf(p, 1024 - strlen(StmtStr), "TABLE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= my_snprintf(p, 1024 - strlen(StmtStr), "TABLE_SCHEMA LIKE DATABASE() ");
  if (TableName && TableName[0])
    p+= my_snprintf(p, 1024 - strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);
  if (ColumnName && ColumnName[0])
    p+= my_snprintf(p, 1024 - strlen(StmtStr), "AND COLUMN_NAME LIKE '%s' ", ColumnName);

   p+= my_snprintf(p, 1024 - strlen(StmtStr), "ORDER BY TABLE_SCHEM, TABLE_NAME, COLUMN_NAME, PRIVILEGE");
  
  ret= Stmt->Methods->Prepare(Stmt, StmtStr, (SQLINTEGER)strlen(StmtStr));
  if (SQL_SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);
  
  return ret;
}
/* }}} */

/* {{{ MADB_StmtTablePrivileges */
SQLRETURN MADB_StmtTablePrivileges(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                                    char *SchemaName, SQLSMALLINT NameLength2,
                                    char *TableName, SQLSMALLINT NameLength3)
{
  SQLRETURN ret;
  char StmtStr[1024],
       *p;

  MADB_CLEAR_ERROR(&Stmt->Error);

  p= StmtStr;
  p += my_snprintf(StmtStr, 1024, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, TABLE_NAME, "
                                  "NULL AS GRANTOR, GRANTEE, PRIVILEGE_TYPE AS PRIVILEGE, IS_GRANTABLE "
                                  "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES WHERE ");
  if (CatalogName)
    p+= my_snprintf(p, 1024 - strlen(StmtStr), "TABLE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= my_snprintf(p, 1024 - strlen(StmtStr), "TABLE_SCHEMA LIKE IF(DATABASE(), DATABASE(), '%%') ");
  if (TableName)
    p+= my_snprintf(p, 1024 - strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);
  
  p+= my_snprintf(p, 1024 - strlen(StmtStr), "ORDER BY TABLE_SCHEM, TABLE_NAME, PRIVILEGE");
  
  ret= Stmt->Methods->Prepare(Stmt, StmtStr, (SQLINTEGER)strlen(StmtStr));
  if (SQL_SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);
  return ret;
}
/* }}} */

/* {{{ MADB_StmtTables */
SQLRETURN MADB_StmtTables(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                          char *SchemaName, SQLSMALLINT NameLength2, char *TableName,
                          SQLSMALLINT NameLength3, char *TableType, SQLSMALLINT NameLength4)
{
  DYNAMIC_STRING StmtStr;
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

  ADJUST_LENGTH(CatalogName, NameLength1);
  ADJUST_LENGTH(TableName, NameLength3);
  ADJUST_LENGTH(TableType, NameLength4);

  if (NameLength1 > 64 || NameLength3 > 64)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY090, "Table and catalog names are limited to 64 chars", 0);
    return Stmt->Error.ReturnValue;
  }

  /* SQL_ALL_CATALOGS 
     If CatalogName is SQL_ALL_CATALOGS and SchemaName and TableName are empty strings, 
     the result set contains a list of valid catalogs for the data source. 
     (All columns except the TABLE_CAT column contain NULLs
  */
  if (CatalogName && NameLength1 && !NameLength3 && !strcmp(CatalogName, SQL_ALL_TABLE_TYPES))
  {
    init_dynamic_string(&StmtStr, "SELECT SCHEMA_NAME AS TABLE_CAT, CONVERT(NULL,CHAR(64)) AS TABLE_SCHEM, "
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
  else if (!NameLength1 && !NameLength3 && TableType && NameLength4 &&
            !strcmp(TableType, SQL_ALL_TABLE_TYPES))
  {
    init_dynamic_string(&StmtStr, "SELECT NULL AS TABLE_CAT, NULL AS TABLE_SCHEM, "
                                  "NULL AS TABLE_NAME, 'TABLE' AS TABLE_TYPE, NULL AS REMARKS "
                                  "FROM DUAL "
                                  "UNION "
                                  "SELECT NULL, NULL, NULL, 'VIEW', NULL FROM DUAL "
                                  "UNION "
                                  "SELECT NULL, NULL, NULL, 'SYSTEM VIEW', NULL FROM DUAL",
                                  8192, 512); 
  }
  else
  {
    init_dynamic_string(&StmtStr, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, TABLE_NAME, "
                                  "if(TABLE_TYPE='BASE TABLE','TABLE',TABLE_TYPE) AS TABLE_TYPE ,"
                                  "TABLE_COMMENT AS REMARKS FROM INFORMATION_SCHEMA.TABLES WHERE 1=1 ",
                                  8192, 512);
    if (Stmt->Options.MetadataId== SQL_TRUE)
      strcpy(Quote, "`");
    else
      strcpy(Quote, "'");

    dynstr_append(&StmtStr, " AND TABLE_SCHEMA ");
    if (CatalogName && NameLength1)
    {
      dynstr_append(&StmtStr, "LIKE ");
      dynstr_append(&StmtStr, Quote);
      dynstr_append(&StmtStr, CatalogName);
      dynstr_append(&StmtStr, Quote);
    }
    else
      dynstr_append(&StmtStr, "= DATABASE() ");

    if (TableName && NameLength3)
    {
      dynstr_append(&StmtStr, " AND TABLE_NAME LIKE ");
      dynstr_append(&StmtStr, Quote);
      dynstr_append(&StmtStr, TableName);
      dynstr_append(&StmtStr, Quote);
    }
    if (TableType && NameLength4 && strcmp(TableType, SQL_ALL_TABLE_TYPES) != 0)
    {
      uint i;
      char *myTypes[3]= {"TABLE", "VIEW", "SYNONYM"};
      dynstr_append(&StmtStr, " AND TABLE_TYPE IN (''");
      for (i= 0; i < 3; i++)
      {
        if (strstr(TableType, myTypes[i]))
        {
          if (strstr(myTypes[i], "TABLE"))
            dynstr_append(&StmtStr, ", 'BASE TABLE'");
          else
          {
            dynstr_append(&StmtStr, ", '");
            dynstr_append(&StmtStr, myTypes[i]);
            dynstr_append(&StmtStr, "'");
          }
        }
      }
      dynstr_append(&StmtStr, ") ");
    }
    dynstr_append(&StmtStr, " ORDER BY TABLE_SCHEMA, TABLE_NAME, TABLE_TYPE");
  }
  MDBUG_C_PRINT(Stmt->Connection, "SQL Statement: %s", StmtStr.str);
  ret= Stmt->Methods->Prepare(Stmt, StmtStr.str, SQL_NTS);
  if (SQL_SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);
  dynstr_free(&StmtStr);
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
 
  my_snprintf(StmtStr, 1024, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, TABLE_NAME, "
                             "NON_UNIQUE, NULL AS INDEX_QUALIFIER, INDEX_NAME, "
                             "%d AS TYPE, "
                             "SEQ_IN_INDEX AS ORDINAL_POSITION, COLUMN_NAME, COLLATION AS ASC_OR_DESC, "
                             "CARDINALITY, NULL AS PAGES, NULL AS FILTER_CONDITION "
                             "FROM INFORMATION_SCHEMA.STATISTICS ",
                             SQL_INDEX_OTHER);
  p+= strlen(StmtStr);
  if (CatalogName && CatalogName[0])
    p+= my_snprintf(p, 1023 - strlen(StmtStr), "WHERE TABLE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= my_snprintf(p, 1023 - strlen(StmtStr), "WHERE TABLE_SCHEMA LIKE IF(DATABASE() IS NOT NULL, DATABASE(), '%%') ");
  
  if (TableName)
    p+= my_snprintf(p, 1023- strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);

  if (Unique == SQL_INDEX_UNIQUE)
    p+= my_snprintf(p, 1023 - strlen(StmtStr), "AND NON_UNIQUE=0 ");

  my_snprintf(p, 1023 - strlen(StmtStr), "ORDER BY TABLE_SCHEM, TABLE_NAME");


  ret= Stmt->Methods->Prepare(Stmt, StmtStr, SQL_NTS);
  if (SQL_SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);

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
  DYNAMIC_STRING StmtStr;
  SQLRETURN ret;

  MDBUG_C_ENTER(Stmt->Connection, "StmtColumns");

  init_dynamic_string(&StmtStr, "", 8192, 1024);
 
  MADB_CLEAR_ERROR(&Stmt->Error);

  if (dynstr_append(&StmtStr, MADB_CATALOG_COLUMNSp1))
    goto dynerror;
  if (dynstr_append(&StmtStr, (Stmt->Connection->Environment->OdbcVersion >= SQL_OV_ODBC3 ? MADB_SQL_DATATYPE_ODBC3 : MADB_SQL_DATATYPE_ODBC2)))
    goto dynerror;
  if (dynstr_append(&StmtStr, MADB_CATALOG_COLUMNSp3))
    goto dynerror;
  if (dynstr_append(&StmtStr, MADB_DEFAULT_COLUMN(Stmt->Connection)))
    goto dynerror;
  if (dynstr_append(&StmtStr, MADB_CATALOG_COLUMNSp4))
    goto dynerror;

  ADJUST_LENGTH(CatalogName, NameLength1);
  ADJUST_LENGTH(SchemaName, NameLength2);
  ADJUST_LENGTH(TableName, NameLength3);
  ADJUST_LENGTH(ColumnName, NameLength4);

  if(dynstr_append(&StmtStr, "TABLE_SCHEMA LIKE "))
    goto dynerror;

  if (CatalogName  && CatalogName[0])
  {
    if (dynstr_append(&StmtStr, "'") ||
        dynstr_append_mem(&StmtStr, CatalogName, NameLength1) ||
        dynstr_append(&StmtStr, "' "))
      goto dynerror;
  }
  else
    if (dynstr_append(&StmtStr, "IF(DATABASE() IS NOT NULL, DATABASE(), '%') "))
      goto dynerror;

  if (TableName && NameLength3)
    if (dynstr_append(&StmtStr, "AND TABLE_NAME LIKE '") ||
        dynstr_append_mem(&StmtStr, TableName, NameLength3) ||
        dynstr_append(&StmtStr, "' "))
    goto dynerror;

  if (ColumnName && NameLength4)
    if (dynstr_append(&StmtStr, "AND COLUMN_NAME LIKE '") ||
        dynstr_append_mem(&StmtStr, ColumnName, NameLength4) ||
        dynstr_append(&StmtStr, "' "))
    goto dynerror;

  if (dynstr_append(&StmtStr, " ORDER BY TABLE_SCHEMA, TABLE_NAME, ORDINAL_POSITION"))
    goto dynerror;

  MDBUG_C_DUMP(Stmt->Connection, StmtStr.str, s);

  ret= Stmt->Methods->Prepare(Stmt, StmtStr.str, SQL_NTS);
  if (SQL_SUCCEEDED(ret))
  {
    ret= Stmt->Methods->Execute(Stmt);
  }
  if (SQL_SUCCEEDED(ret))
  {
    MADB_FixColumnDataTypes(Stmt, SqlColumnsColType);
  }
  dynstr_free(&StmtStr);
  MDBUG_C_DUMP(Stmt->Connection, ret, d);
  return ret;

dynerror:
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

  MADB_CLEAR_ERROR(&Stmt->Error);

  if (!(StmtStr= MADB_CALLOC(Length)))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    return Stmt->Error.ReturnValue;
  }

  p= StmtStr;

  p+= my_snprintf(p, Length, MADB_PROCEDURE_COLUMNS(Stmt));
  
  if (CatalogName && CatalogName[0])
    p+= my_snprintf(p, Length - strlen(StmtStr), "WHERE SPECIFIC_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= my_snprintf(p, Length - strlen(StmtStr), "WHERE SPECIFIC_SCHEMA LIKE DATABASE() ");
  if (ProcName && ProcName[0])
    p+= my_snprintf(p, Length - strlen(StmtStr), "AND SPECIFIC_NAME LIKE '%s' ", ProcName);
  if (ColumnName && ColumnName[0])
    p+= my_snprintf(p, Length- strlen(StmtStr), "AND PARAMETER_NAME LIKE '%s' ", ColumnName);
  p+= my_snprintf(p, Length - strlen(StmtStr), " ORDER BY SPECIFIC_SCHEMA, SPECIFIC_NAME, ORDINAL_POSITION");
  ret= Stmt->Methods->Prepare(Stmt, StmtStr, SQL_NTS);

  MADB_FREE(StmtStr);

  if (SQL_SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);
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
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* TableName is mandatory */
  if (!TableName || !NameLength3)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }

  p= StmtStr;
  p+= my_snprintf(p, 1024, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, "
                           "TABLE_NAME, COLUMN_NAME, ORDINAL_POSITION KEY_SEQ, "
                           "'PRIMARY' PK_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE "
                           "COLUMN_KEY = 'pri' AND ");
  if (CatalogName && CatalogName[0])
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "TABLE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "TABLE_SCHEMA LIKE IF(DATABASE() IS NOT NULL, DATABASE(), '%%') ");
  if (TableName)
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);
   p+= my_snprintf(p, 2048 - strlen(StmtStr), " ORDER BY TABLE_SCHEMA, TABLE_NAME, ORDINAL_POSITION");

  ret= Stmt->Methods->Prepare(Stmt, StmtStr, SQL_NTS);
  if (SQL_SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);

  return ret;
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
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* TableName is mandatory */
  if (!TableName || !NameLength3)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }

  p+= my_snprintf(p, 2048, "SELECT NULL AS SCOPE, COLUMN_NAME, %s,"
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
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "AND TABLE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "AND TABLE_SCHEMA LIKE IF(DATABASE() IS NOT NULL, DATABASE(), '%%') ");
  if (TableName && TableName[0])
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "AND TABLE_NAME LIKE '%s' ", TableName);

  if (Nullable == SQL_NO_NULLS)
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "AND IS_NULLABLE <> 'YES' ");

  if (IdentifierType == SQL_BEST_ROWID)
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "AND COLUMN_KEY IN ('PRI', 'UNI') ");
  else if (IdentifierType == SQL_ROWVER)
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "AND DATA_TYPE='timestamp' AND EXTRA LIKE '%%CURRENT_TIMESTAMP%%' ");
  p+= my_snprintf(p, 2048 - strlen(StmtStr), "ORDER BY TABLE_SCHEMA, TABLE_NAME, COLUMN_KEY");

  ret= Stmt->Methods->Prepare(Stmt, StmtStr, SQL_NTS);
  if (SQL_SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);

  return ret;
}
/* }}} */

/* {{{ MADB_StmtProcedures */
SQLRETURN MADB_StmtProcedures(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                              char *SchemaName, SQLSMALLINT NameLength2, char *ProcName,
                              SQLSMALLINT NameLength3)
{
   char StmtStr[2048],
       *p;
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);

  p= StmtStr;

  p+= my_snprintf(p, 2048, "SELECT ROUTINE_SCHEMA AS PROCEDURE_CAT, NULL AS PROCEDURE_SCHEM, "
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
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "WHERE ROUTINE_SCHEMA LIKE '%s' ", CatalogName);
  else
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "WHERE ROUTINE_SCHEMA LIKE DATABASE() ");
  if (ProcName && ProcName[0])
    p+= my_snprintf(p, 2048 - strlen(StmtStr), "AND SPECIFIC_NAME LIKE '%s' ", ProcName);
  
  p+= my_snprintf(p, 2048 - strlen(StmtStr), " ORDER BY ROUTINE_SCHEMA, SPECIFIC_NAME");

  ret= Stmt->Methods->Prepare(Stmt, StmtStr, SQL_NTS);
  if (SQL_SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);
  
  return ret;
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
  DYNAMIC_STRING StmtStr;
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
  init_dynamic_string(&StmtStr,
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
    dynstr_append(&StmtStr, " AND A.REFERENCED_TABLE_SCHEMA "); 

    if (PKCatalogName && PKCatalogName[0])
    {
      dynstr_append(&StmtStr, "LIKE '");
      mysql_real_escape_string(Stmt->Connection->mariadb, EscapeBuf, PKCatalogName, MIN(NameLength1, 255));
      dynstr_append(&StmtStr, EscapeBuf);
      dynstr_append(&StmtStr, "' ");
    }
    else 
      dynstr_append(&StmtStr, "= DATABASE()");
    dynstr_append(&StmtStr, " AND A.REFERENCED_TABLE_NAME = '");

    mysql_real_escape_string(Stmt->Connection->mariadb, EscapeBuf, PKTableName, MIN(255, NameLength3));
    dynstr_append(&StmtStr, EscapeBuf);
    dynstr_append(&StmtStr, "' ");
  }

  if (FKTableName && FKTableName[0])
  {
    dynstr_append(&StmtStr, " AND A.TABLE_SCHEMA = "); 

    if (FKCatalogName && FKCatalogName[0])
    {
      dynstr_append(&StmtStr, "'");
      mysql_real_escape_string(Stmt->Connection->mariadb, EscapeBuf, FKCatalogName, MIN(NameLength4, 255));
      dynstr_append(&StmtStr, EscapeBuf);
      dynstr_append(&StmtStr, "' ");
    }
    else
      dynstr_append(&StmtStr, "DATABASE() ");
    dynstr_append(&StmtStr, " AND A.TABLE_NAME = '");

    mysql_real_escape_string(Stmt->Connection->mariadb, EscapeBuf, FKTableName, MIN(255, NameLength6));
    dynstr_append(&StmtStr, EscapeBuf);
    dynstr_append(&StmtStr, "' ");
  }
  dynstr_append(&StmtStr, "ORDER BY FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, KEY_SEQ, PKTABLE_NAME");
  
  ret= Stmt->Methods->Prepare(Stmt, StmtStr.str, SQL_NTS);

  dynstr_free(&StmtStr);

  if (SQL_SUCCEEDED(ret))
    ret= Stmt->Methods->Execute(Stmt);
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
    size_t Length= MADB_SetString(isWChar ? &Stmt->Connection->charset : 0, ColumnName, ColumnName ? BufferLength : 0, Record->ColumnName, SQL_NTS, &Stmt->Error); 
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
  LIST *LStmt, *LStmtNext;
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
    my_snprintf(Stmt->Cursor.Name, MADB_MAX_CURSOR_NAME, "SQL_CUR%d", 
                Stmt->Connection->CursorCount++);
  }
  Length= (SQLSMALLINT)MADB_SetString(isWChar ? &Stmt->Connection->charset : 0, CursorName,
                                      BufferLength, Stmt->Cursor.Name, SQL_NTS, &Stmt->Error);
  if (NameLengthPtr)
    *NameLengthPtr= (SQLSMALLINT)Length;
  if (!BufferLength)
    MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
   
  return Stmt->Error.ReturnValue;
  
}
/* }}} */

SQLRETURN MADB_RefreshRowPtrs(MADB_Stmt *Stmt)
{
  unsigned char *row, *null_ptr, bit_offset= 4;
  unsigned int i;

  if (!Stmt->stmt->result_cursor)
    return SQL_ERROR;

  row= (unsigned char *)Stmt->stmt->result_cursor->data;
  row++;
  null_ptr= row;
  /* TODO: to check what are these magical numbers. Must be some bytes skipping base on number of fields.
     Bytes must be null value indicators. Too much of sacred knowledge here */
  row+= (Stmt->stmt->field_count + 9) / 8;

  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    MYSQL_BIND Bind;
    char MiniBuffer[2];
    int MyError;
    unsigned long MyLength;

    if (*null_ptr & bit_offset)
    {
      Stmt->stmt->bind[i].row_ptr= NULL;
      if (Stmt->stmt->bind[i].is_null)
        *Stmt->stmt->bind[i].is_null= TRUE;
    }
    else
    {
      memset(&Bind, 0, sizeof(MYSQL_BIND));
      Bind.buffer_type= MYSQL_TYPE_STRING;
      Bind.buffer= MiniBuffer;
      Bind.buffer_length= 2;
      Bind.error= (my_bool *)&MyError;
      Bind.length= &MyLength;
      Stmt->stmt->bind[i].row_ptr= row;

      /* TODO: Too much of sacred knowledge here as well */
      mysql_ps_fetch_functions[Stmt->stmt->fields[i].type].func(&Bind, &Stmt->stmt->fields[i], &row);
    }
    if (!((bit_offset <<=1) & 255)) {
      bit_offset= 1; /* To next byte */
      null_ptr++;
    }
  }

  return SQL_SUCCESS;
}

/* {{{ MADB_RefreshDynamicCursor */
SQLRETURN MADB_RefreshDynamicCursor(MADB_Stmt *Stmt)
{
  SQLRETURN ret;
  SQLLEN    CurrentRow=     Stmt->Cursor.Position;
  SQLBIGINT AffectedRows=   Stmt->AffectedRows;
  SQLLEN    LastRowFetched= Stmt->LastRowFetched;

  ret= Stmt->Methods->Execute(Stmt);

  Stmt->Cursor.Position= CurrentRow;
  if (Stmt->Cursor.Position > 0 && (my_ulonglong)Stmt->Cursor.Position >= mysql_stmt_num_rows(Stmt->stmt))
  {
    Stmt->Cursor.Position= (long)mysql_stmt_num_rows(Stmt->stmt) - 1;
  }

  Stmt->LastRowFetched= LastRowFetched;
  Stmt->AffectedRows=   AffectedRows;

  MADB_StmtDataSeek(Stmt, Stmt->Cursor.Position);
  if (SQL_SUCCEEDED(ret))
  {
    /* We need to prevent that bound variables will be overwritten
       by fetching data again: For subsequent GetData we need to update
       bind->row_ptr */
    Stmt->Methods->RefreshRowPtrs(Stmt);
 
    MADB_StmtDataSeek(Stmt, Stmt->Cursor.Position);
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
      DYNAMIC_STRING DynStmt;
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

        if (init_dynamic_string(&DynStmt, "INSERT INTO ", 8192, 1024) ||
            MADB_DynStrAppendQuoted(&DynStmt, CatalogName) ||
            dynstr_append(&DynStmt, ".") ||
            MADB_DynStrAppendQuoted(&DynStmt, TableName)||
            MADB_DynStrInsertSet(Stmt, &DynStmt))
        {
          dynstr_free(&DynStmt);
          return Stmt->Error.ReturnValue;
        }

        Stmt->DataExecutionType= MADB_DAE_ADD;
        ret= Stmt->Methods->Prepare(Stmt->DaeStmt, DynStmt.str, SQL_NTS);
        /* Prepare(SQL_CLOSE in fact) currently resets DefaultResult. Not sure why. But this should go after Prepare so far */
        Stmt->DaeStmt->DefaultsResult= MADB_GetDefaultColumnValues(Stmt, Stmt->stmt->fields);
        dynstr_free(&DynStmt);

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
          Stmt->DaeStmt->Methods->BindParam(Stmt->DaeStmt, param + 1, SQL_PARAM_INPUT, Rec->ConciseType, Rec->Type, Rec->DisplaySize, Rec->Scale,
            Rec->DataPtr, Rec->OctetLength, Rec->OctetLengthPtr);
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

      //memcpy(&Stmt->DaeStmt->Apd->Header, &Stmt->Ard->Header, sizeof(MADB_Header));
      Stmt->DaeStmt->Apd->Header.ArraySize= Stmt->Ard->Header.ArraySize;
      Stmt->DaeStmt->Apd->Header.ArrayStatusPtr= Stmt->Ard->Header.ArrayStatusPtr;
      Stmt->DaeStmt->Apd->Header.BindOffsetPtr= Stmt->Ard->Header.BindOffsetPtr;
      Stmt->DaeStmt->Apd->Header.BindType= Stmt->Ard->Header.BindType;
      Stmt->DaeStmt->Apd->Header.RowsProcessedPtr= Stmt->Ard->Header.RowsProcessedPtr;

      ret= Stmt->Methods->Execute(Stmt->DaeStmt);
      if (!SQL_SUCCEEDED(ret))
      {
        /* We can have SQL_NEED_DATA here, which would not set error (and its ReturnValue) */
        MADB_CopyError(&Stmt->Error, &Stmt->DaeStmt->Error);
        return ret;
      }
      if (Stmt->AffectedRows == -1)
        Stmt->AffectedRows= 0;
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
        End= min(mysql_stmt_num_rows(Stmt->stmt)-1, Start + Stmt->Ard->Header.ArraySize - 1);
      }
      /* Stmt->ArrayOffset will be incremented in StmtExecute() */
      Start+= Stmt->ArrayOffset;

      /* TODO: SQL_ATTR_ROW_STATUS_PTR should be filled */
      while (Start <= End)
      {
        SQLSMALLINT param= 0, column;
        MADB_StmtDataSeek(Stmt,Start);
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
              Stmt->DaeStmt->Methods->BindParam(Stmt->DaeStmt, param + 1, SQL_PARAM_INPUT, Rec->ConciseType, Rec->Type, Rec->DisplaySize, Rec->Scale,
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
        
        if (Stmt->DaeStmt->Methods->Execute(Stmt->DaeStmt) != SQL_ERROR)
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
      DYNAMIC_STRING DynamicStmt;
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
        init_dynamic_string(&DynamicStmt, "DELETE FROM ", 8192, 1024);
        if (MADB_DynStrAppendQuoted(&DynamicStmt, TableName) ||
           MADB_DynStrGetWhere(Stmt, &DynamicStmt, TableName, FALSE))
        {
          dynstr_free(&DynamicStmt);
          MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);

          return Stmt->Error.ReturnValue;
        }

        LOCK_MARIADB(Stmt->Connection);
        if (mysql_real_query(Stmt->Connection->mariadb, DynamicStmt.str, (unsigned long)DynamicStmt.length))
        {
          dynstr_free(&DynamicStmt);
          MADB_SetError(&Stmt->Error, MADB_ERR_HY001, mysql_error(Stmt->Connection->mariadb), 
                            mysql_errno(Stmt->Connection->mariadb));

          UNLOCK_MARIADB(Stmt->Connection);

          return Stmt->Error.ReturnValue;
        }
        dynstr_free(&DynamicStmt);
        Stmt->AffectedRows+= mysql_affected_rows(Stmt->Connection->mariadb);
        Start++;
      }
      UNLOCK_MARIADB(Stmt->Connection);

      Stmt->Ard->Header.ArraySize= SaveArraySize;
      /* if we have a dynamic cursor we need to adjust the rowset size */
      if (Stmt->Options.CursorType == SQL_CURSOR_DYNAMIC)
        Stmt->LastRowFetched-= (unsigned long)Stmt->AffectedRows;
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
  SQLRETURN ret;
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
  switch(FetchOrientation) {
  case SQL_FETCH_NEXT:
    Position= Stmt->Cursor.Position < 0 ? 0 : Stmt->Cursor.Position + RowsProcessed;
/*    if (Stmt->Ird->Header.RowsProcessedPtr)
      Position+= MAX(1, *Stmt->Ird->Header.RowsProcessedPtr);
    else
      Position++; */
    break;
  case SQL_FETCH_PRIOR:
    Position= Stmt->Cursor.Position < 0 ? - 1: Stmt->Cursor.Position - MAX(1, Stmt->Ard->Header.ArraySize);
     /* if (Stmt->Ird->Header.RowsProcessedPtr)
        Position-= MAX(1, *Stmt->Ird->Header.RowsProcessedPtr);
    else
      Position--; */
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
      if ((longlong)mysql_stmt_num_rows(Stmt->stmt) - 1 + FetchOffset < 0 &&
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
    return SQL_NO_DATA;
  }
  
  ret= MADB_StmtDataSeek(Stmt, Stmt->Cursor.Position);
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
