/************************************************************************************
   Copyright (C) 2013,2023 MariaDB Corporation AB
   
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

#include "ma_odbc.h"

#include "interface/PreparedStatement.h"
#include "interface/ResultSet.h"
#include "ResultSetMetaData.h"

/* {{{ MADB_StmtResetResultStructures */
void MADB_StmtResetResultStructures(MADB_Stmt *Stmt)
{
  uint32_t columnCount= Stmt->metadata ? Stmt->metadata->getColumnCount() : 0;
  Stmt->CharOffset= (unsigned long *)MADB_REALLOC((char *)Stmt->CharOffset,
    sizeof(long) * columnCount);
  memset(Stmt->CharOffset, 0, sizeof(long) * columnCount);
  Stmt->Lengths= (unsigned long *)MADB_REALLOC((char *)Stmt->Lengths,
    sizeof(long) * columnCount);
  memset(Stmt->Lengths, 0, sizeof(long) * columnCount);

  Stmt->LastRowFetched= 0;
  MADB_STMT_RESET_CURSOR(Stmt);
}
/* }}} */

/* {{{ MoveNext - moves C/C cursor forward for Offset positions */
SQLRETURN MoveNext(MADB_Stmt *Stmt, unsigned long long Offset)
{
  SQLRETURN  result= SQL_SUCCESS;

  if (Stmt->result != nullptr)
  {
    unsigned int i;
    char        *SavedFlag;
    std::size_t columnCount= Stmt->metadata->getColumnCount();

    SavedFlag= (char*)MADB_CALLOC(columnCount);

    if (SavedFlag == nullptr)
    {
      return SQL_ERROR;
    }
    // TODO: is this trick is still needed?
    for (i=0; i < columnCount; i++)
    {
      SavedFlag[i]= Stmt->result[i].flags & MADB_BIND_DUMMY;

      Stmt->result[i].flags|= MADB_BIND_DUMMY;
    }
    Stmt->rs->bind(Stmt->result);
    while (Offset--)
    {
      if (!Stmt->rs->next())
      {
        result= SQL_ERROR;
        break;
      }
    }

    for (i=0; i < columnCount; i++)
    {
      Stmt->result[i].flags &= (~MADB_BIND_DUMMY | SavedFlag[i]);
    }
    Stmt->rs->bind(Stmt->result);

    MADB_FREE(SavedFlag);
  }
  return result;
}
/* }}} */

/* {{{ MADB_StmtDataSeek */
SQLRETURN MADB_StmtDataSeek(MADB_Stmt *Stmt, my_ulonglong FetchOffset)
{
  if (!Stmt->rs)
  {
   return SQL_NO_DATA_FOUND;
  }

  Stmt->rs->absolute(static_cast<int64_t>(FetchOffset));

  return SQL_SUCCESS;  
}
/* }}} */

/* {{{  */
void QuickDropAllPendingResults(MYSQL* Mariadb)
{
  int Next= 0;
  do {
    if (Next == 0)
    {
      if (mysql_field_count(Mariadb) > 0)
      {
        MYSQL_RES *Res= mysql_store_result(Mariadb);

        if (Res)
        {
          mysql_free_result(Res);
        }
      }
    }
  } while ((Next= mysql_next_result(Mariadb)) != -1);
}
/* }}} */

/* {{{ HasOutParams */
bool HasOutParams(MADB_Stmt* Stmt)
{
  for (SQLSMALLINT i= 0; i < Stmt->ParamCount; ++i)
  {
    MADB_DescRecord *IpdRecord;
    if ((IpdRecord= MADB_DescGetInternalRecord(Stmt->Ipd, i, MADB_DESC_READ)) != nullptr) {
      if (IpdRecord->ParameterType == SQL_PARAM_INPUT_OUTPUT ||
        IpdRecord->ParameterType == SQL_PARAM_OUTPUT) {
        return true;
      }
    }
  }
  return false;
}
/* }}} */
/* {{{ MADB_StmtMoreResults */
// TODO: need to do it in unified way - create function in ma_api_internal, to some basic paremeters validation and
//       exceptions processing
SQLRETURN MADB_StmtMoreResults(SQLHSTMT StatementHandle)
{
  MADB_Stmt *Stmt= (MADB_Stmt*)StatementHandle;
  SQLRETURN ret= SQL_SUCCESS;

  if (!Stmt->stmt)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_08S01, NULL, 0);
  }

  /* We can't have it in MADB_StmtResetResultStructures, as it breaks dyn_cursor functionality.
     Thus we free-ing bind structs on move to new result only */
  MADB_FREE(Stmt->result);
  Stmt->metadata.reset();
  Stmt->rs.reset();

  try {
    if (Stmt->stmt->getMoreResults())
    {
      unsigned int ServerStatus;
      mariadb_get_infov(Stmt->Connection->mariadb, MARIADB_CONNECTION_SERVER_STATUS, (void*)&ServerStatus);
      Stmt->rs.reset(Stmt->stmt->getResultSet());
      bool itsOutParams= ServerStatus & SERVER_PS_OUT_PARAMS;
      bool haveOutParams= HasOutParams(Stmt);

      if (Stmt->Query.QueryType == MADB_QUERY_CALL && !itsOutParams && Stmt->Connection->IsMySQL &&
        haveOutParams)
      {
        itsOutParams= Stmt->stmt->isOutParams();
      }
      if (itsOutParams && haveOutParams)
      {
        Stmt->State= MADB_SS_OUTPARAMSFETCHED;
        ret= Stmt->GetOutParams(0);
      }
      else
      {
        FetchMetadata(Stmt);
      }
      MADB_DescSetIrdMetadata(Stmt, Stmt->metadata->getFields(), Stmt->metadata->getColumnCount());
      Stmt->AffectedRows= -1;
    }
    else if (Stmt->stmt->getUpdateCount() > -1)
    {
      MADB_DescFree(Stmt->Ird, TRUE);
      Stmt->AffectedRows= Stmt->stmt->getUpdateCount();
    }
    else
    {
      return SQL_NO_DATA;
    }
  }
  catch (SQLException& e) {
    ret= MADB_FromException(Stmt->Error, e);
  }
  catch (int32_t /*rc*/) {
    ret= MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt.get());
  }

  MADB_StmtResetResultStructures(Stmt);

  return ret;
}
/* }}} */

/* {{{ MADB_RecordsToFetch */
SQLULEN MADB_RowsToFetch(MADB_Cursor *Cursor, SQLULEN ArraySize, unsigned long long RowsInResultset)
{
  SQLULEN Position= Cursor->Position > 0 ? Cursor->Position : 0;
  SQLULEN result= ArraySize;

  Cursor->RowsetSize= ArraySize;

  if (Position + ArraySize - 1 > RowsInResultset)
  {
    if (Position > 0 && RowsInResultset >= Position)
    {
      result= (SQLULEN)(RowsInResultset - Position + 1);
    }
    else
    {
      result= 1;
    }
  }

  return result;
}
/* }}} */

