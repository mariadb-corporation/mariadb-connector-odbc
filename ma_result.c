/************************************************************************************
   Copyright (C) 2013 SkySQL AB
   
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

/* {{{ MADB_StmtDataSeek */
SQLRETURN MADB_StmtDataSeek(MADB_Stmt *Stmt, my_ulonglong FetchOffset)
{
  MYSQL_ROWS *tmp= NULL;

  if (!Stmt->stmt->result.data)
  {
   return SQL_NO_DATA_FOUND;
  }

  mysql_stmt_data_seek(Stmt->stmt, FetchOffset);

  return SQL_SUCCESS;  
}
/* }}} */

/* {{{ MADB_StmtMoreResults */
SQLRETURN MADB_StmtMoreResults(MADB_Stmt *Stmt)
{
  SQLRETURN ret= SQL_SUCCESS;
  if (!Stmt->stmt)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_08S01, NULL, 0);
  }

  if (Stmt->MultiStmts)
  {
    if (Stmt->MultiStmtNr == Stmt->MultiStmtCount - 1)
      return SQL_NO_DATA;

    Stmt->MultiStmtNr++;
    Stmt->stmt= Stmt->MultiStmts[Stmt->MultiStmtNr];
    Stmt->AffectedRows= mysql_stmt_affected_rows(Stmt->stmt);
    return SQL_SUCCESS;
  }

  /* in case we executed a multi statement, it was done via mysql_query */
  if (Stmt->State == MADB_SS_EMULATED)
  {
    if (!mysql_more_results(Stmt->Connection->mariadb))
      return SQL_NO_DATA;
    else
    {
      LOCK_MARIADB(Stmt->Connection);
      mysql_next_result(Stmt->Connection->mariadb);
      UNLOCK_MARIADB(Stmt->Connection);
    }
    return ret;
  }

  if (mysql_stmt_more_results(Stmt->stmt))
    mysql_stmt_free_result(Stmt->stmt);
  else
    return SQL_NO_DATA;
  
  LOCK_MARIADB(Stmt->Connection);
  if (mysql_stmt_next_result(Stmt->stmt) ||
      !Stmt->stmt->field_count)
  {
    UNLOCK_MARIADB(Stmt->Connection);
    return SQL_NO_DATA;
  }

  MADB_DescSetIrdMetadata(Stmt, mysql_fetch_fields(FetchMetadata(Stmt)), mysql_stmt_field_count(Stmt->stmt));

  if (Stmt->Connection->mariadb->server_status & SERVER_PS_OUT_PARAMS)
  {
    ret= Stmt->Methods->GetOutParams(Stmt, 0);
  }
  else
  {
    if (Stmt->Options.CursorType != SQL_CURSOR_FORWARD_ONLY)
      mysql_stmt_store_result(Stmt->stmt);
  }
  UNLOCK_MARIADB(Stmt->Connection);

  if (/* mysql_stmt_field_count(Stmt->stmt) && */
        Stmt->Options.CursorType != SQL_CURSOR_FORWARD_ONLY)
  {
    mysql_stmt_data_seek(Stmt->stmt, 0);
  }

  return ret;
}
/* }}} */

/* {{{ MADB_RecordsToFetch */
SQLULEN MADB_RowsToFetch(MADB_Cursor *Cursor, SQLULEN ArraySize, unsigned long long RowsInResultst)
{
  SQLLEN  Position= Cursor->Position >= 0 ? Cursor->Position : 0;
  SQLULEN result= ArraySize;

  Cursor->RowsetSize= ArraySize;

  if (Position + ArraySize > RowsInResultst)
    result= (SQLULEN)(RowsInResultst - Position);

  return result >= 0 ? result : 1;
}
/* }}} */

