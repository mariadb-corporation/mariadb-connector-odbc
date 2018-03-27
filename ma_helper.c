/************************************************************************************
   Copyright (C) 2013, 2017 MariaDB Corporation AB
   
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
#include <stdint.h>

#define MADB_FIELD_IS_BINARY(_field) ((_field).charsetnr == BINARY_CHARSETNR)

void CloseMultiStatements(MADB_Stmt *Stmt)
{
  unsigned int i;

  for (i=0; i < Stmt->MultiStmtCount; ++i)
  {
    MDBUG_C_PRINT(Stmt->Connection, "-->closing %0x", Stmt->MultiStmts[i]);
    mysql_stmt_close(Stmt->MultiStmts[i]);
  }
  MADB_FREE(Stmt->MultiStmts);
  Stmt->MultiStmtCount= 0;
  Stmt->stmt= NULL;
}


/* Required, but not sufficient condition */
BOOL QueryIsPossiblyMultistmt(char *queryStr)
{
  char *semicolon_pos= strchr(queryStr, ';');
  /* String supposed to come here trimmed. Checking that semicolon is not last char in the string */
  if (semicolon_pos != NULL && semicolon_pos < queryStr + strlen(queryStr) - 1)
  {
    /* CREATE PROCEDURE uses semicolons but is not supported in prepared statement
        protocol */
    if (!MADB_IsStatementSupported(queryStr, "CREATE", "PROCEDURE"))
      return FALSE;
    if (!MADB_IsStatementSupported(queryStr, "CREATE", "FUNCTION"))
      return FALSE;
    if (!MADB_IsStatementSupported(queryStr, "CREATE", "DEFINER"))
      return FALSE;
    return TRUE;
  }

  return FALSE;
}

#define STRING_OR_COMMENT() (quote[0] || quote[1] || comment)
unsigned int GetMultiStatements(MADB_Stmt *Stmt, char *StmtStr, SQLINTEGER Length)
{
  char *p, *prev= NULL, wait_char= 0;
  unsigned int statements= 1;
  int quote[2]= {0,0}, comment= 0;
  char *end;
  MYSQL_STMT *stmt;
  char *StmtCopy= NULL;

  /* mysql_stmt_init requires locking as well */
  LOCK_MARIADB(Stmt->Connection);
  stmt= mysql_stmt_init(Stmt->Connection->mariadb);

  /* if the entire stmt string passes, we don't have multistatement */
  if (stmt && !mysql_stmt_prepare(stmt, StmtStr, Length))
  {
    mysql_stmt_close(stmt);
    UNLOCK_MARIADB(Stmt->Connection);
    return 1;
  }
  mysql_stmt_close(stmt);

  /* make sure we don't have trailing whitespace or semicolon */
  if (Length)
  {
    end= StmtStr + Length - 1;
    while (end > StmtStr && (isspace(0x000000ff & *end) || *end == ';'))
    {
      --end;
      --Length;
    }
  }
  /* We can have here not NULL-terminated string as a source, thus we need to allocate, copy meaningful characters and
     add NULL */
  p= StmtCopy= MADB_ALLOC(Length + 1);
  strncpy(StmtCopy, StmtStr, Length);
  StmtCopy[Length]= '\0';

  end= StmtCopy + Length;
  
  while (p < end)
  {
    if (wait_char)
    {
      if (*p == wait_char && *prev != '\\')
      {
        wait_char= 0;
      }
    }
    else
    {
      switch (*p) {
      case '-':
        if (!STRING_OR_COMMENT() && (p < end - 1) && (char)*(p+1) ==  '-')
        {
          wait_char= '\n';
        }
        break;
      case '#':
        if (!STRING_OR_COMMENT())
        {
          wait_char= '\n';
        }
        break;
      case ';':
        if (!STRING_OR_COMMENT())
        {
          statements++;
          *p= '\0';
        }
        break;
      case '/':
        if (!STRING_OR_COMMENT() && (p < end - 1) && (char)*(p+1) ==  '*')
          comment= 1;
        else if (comment && (p > StmtCopy) && (char)*(prev) == '*')
          comment= 0;
        break;
      case '"':
        quote[0] = !STRING_OR_COMMENT();
        break;
      case '\'':
        quote[1] = !STRING_OR_COMMENT();
        break;
      case '\\':
        /* *end is \0, so we are safe here to increment by 2 bytes */
        if ((Stmt->Connection->mariadb->server_status & SERVER_STATUS_NO_BACKSLASH_ESCAPES) == 0  && p < end - 1)
        {
          /* Skipping escaped character and resetting prev */
          p+= 2;
          prev= 0;
          continue;
        }
      default:
        break;
      }
    }

    prev= p;
    ++p;
  }

  if (statements > 1)
  {
    int i=0;
    unsigned int MaxParams= 0;

    p= StmtCopy;
    Stmt->MultiStmtCount= 0;
    Stmt->MultiStmtNr= 0;
    Stmt->MultiStmts= (MYSQL_STMT **)MADB_CALLOC(sizeof(MYSQL_STMT) * statements);

    while (p < StmtCopy + Length)
    {
      /* Need to be incremented before CloseMultiStatements() */
      ++Stmt->MultiStmtCount;
      Stmt->MultiStmts[i]= i == 0 ? Stmt->stmt : mysql_stmt_init(Stmt->Connection->mariadb);
      MDBUG_C_PRINT(Stmt->Connection, "-->inited&preparing %0x(%d,%s)", Stmt->MultiStmts[i], i, p);

      if (mysql_stmt_prepare(Stmt->MultiStmts[i], p, (unsigned long)strlen(p)))
      {
        MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->MultiStmts[i]);
        CloseMultiStatements(Stmt);
        if (StmtCopy)
        {
          free(StmtCopy);
        }
        UNLOCK_MARIADB(Stmt->Connection);

        return 0;
      }
      if (mysql_stmt_param_count(Stmt->MultiStmts[i]) > MaxParams)
        MaxParams= mysql_stmt_param_count(Stmt->MultiStmts[i]);
      p+= strlen(p) + 1;
      ++i;
    }
    UNLOCK_MARIADB(Stmt->Connection);

    if (MaxParams)
      Stmt->params= (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * MaxParams);
  }
  if (StmtCopy)
    free(StmtCopy);

  return statements;
}
#undef STRING_OR_COMMENT

my_bool MADB_CheckPtrLength(SQLINTEGER MaxLength, char *Ptr, SQLINTEGER NameLen)
{
  if(!Ptr)
    return TRUE;
  if ((NameLen == SQL_NTS && strlen(Ptr) >(size_t) MaxLength) || NameLen > MaxLength)
    return FALSE;
  return TRUE;
}

int  MADB_GetWCharType(int Type)
{
  switch (Type) {
  case SQL_CHAR:
    return SQL_WCHAR;
  case SQL_VARCHAR:
    return SQL_WVARCHAR;
  case SQL_LONGVARCHAR:
    return SQL_WLONGVARCHAR;
  default:
    return Type;
  }
}

int MADB_KeyTypeCount(MADB_Dbc *Connection, char *TableName, int KeyFlag)
{
  int          Count= 0;
  unsigned int i;
  char         StmtStr[1024];
  char         *p= StmtStr;
  char         Database[65];
  MADB_Stmt    *Stmt= NULL;
  MADB_Stmt    *KeyStmt;
  
  Connection->Methods->GetAttr(Connection, SQL_ATTR_CURRENT_CATALOG, Database, 65, NULL, FALSE);
  p+= _snprintf(p, 1024, "SELECT * FROM ");
  if (Database)
  {
    p+= _snprintf(p, 1024 - strlen(p), "`%s`.", Database);
  }
  p+= _snprintf(p, 1024 - strlen(p), "%s LIMIT 0", TableName);
  if (MA_SQLAllocHandle(SQL_HANDLE_STMT, (SQLHANDLE)Connection, (SQLHANDLE*)&Stmt) == SQL_ERROR ||
    Stmt->Methods->ExecDirect(Stmt, (SQLCHAR *)StmtStr, SQL_NTS) == SQL_ERROR ||
    Stmt->Methods->Fetch(Stmt) == SQL_ERROR)
  {
    goto end;
  }
  KeyStmt= (MADB_Stmt *)Stmt;
  for (i=0; i < mysql_stmt_field_count(KeyStmt->stmt); i++)
    if (KeyStmt->stmt->fields[i].flags & KeyFlag)
      Count++;
end:
  if (Stmt)
    Stmt->Methods->StmtFree(Stmt, SQL_DROP);
  return Count;
}

/* {{{ MADB_get_single_row */
my_bool MADB_get_single_row(MADB_Dbc *Connection,
    const char *StmtString,
    SQLINTEGER Length,
    unsigned int NumCols,
    char **Buffers,
    size_t *Buffer_Lengths)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  LOCK_MARIADB(Connection);
  if (mysql_real_query(Connection->mariadb, StmtString, Length) ||
      mysql_field_count(Connection->mariadb) < NumCols)
    return 1;

  if ((result= mysql_store_result(Connection->mariadb)) &&
      (row= mysql_fetch_row(result)))
  {
    unsigned int i;

    UNLOCK_MARIADB(Connection);

    for (i=0; i < NumCols; i++)
      strncpy_s(Buffers[i], Buffer_Lengths[i], row[i], Connection->mariadb->fields[i].max_length);
    mysql_free_result(result);
    return 0;
  }
  UNLOCK_MARIADB(Connection);

  return 1;
}
/* }}} */

/* {{{ MADB_CheckODBCType */
BOOL MADB_CheckODBCType(SQLSMALLINT Type)
{
  switch(Type)
  {
  case SQL_C_CHAR:
  case SQL_C_WCHAR:
  case SQL_C_SSHORT:
  case SQL_C_SHORT:
  case SQL_C_USHORT:
  case SQL_C_SLONG:
  case SQL_C_LONG:
  case SQL_C_ULONG:
  case SQL_C_FLOAT:
  case SQL_C_DOUBLE:
  case SQL_C_BIT:
  case SQL_C_STINYINT:
  case SQL_C_TINYINT:
  case SQL_C_UTINYINT:
  case SQL_C_SBIGINT:
  case SQL_C_UBIGINT:
  case SQL_C_BINARY:
  case SQL_C_TYPE_DATE:
  case SQL_C_TYPE_TIME:
  case SQL_C_TYPE_TIMESTAMP:
  case SQL_C_NUMERIC:
#if (ODBCVER>=0x0350)
  case SQL_C_GUID:
#endif
  case SQL_C_DEFAULT:
    return TRUE;
  default:
    return FALSE;
  }
}

/* {{{ MADB_GetTypeFromConciseType */
SQLSMALLINT MADB_GetTypeFromConciseType(SQLSMALLINT ConciseType)
{
  switch (ConciseType)
  {
    /* todo: support for interval. currently we map only date/time types */
  case SQL_C_DATE:
  case SQL_C_TIME:
  case SQL_C_TIMESTAMP:
    return SQL_DATETIME;
  case SQL_C_INTERVAL_YEAR:
  case SQL_C_INTERVAL_YEAR_TO_MONTH:
  case SQL_C_INTERVAL_MONTH:
  case SQL_C_INTERVAL_DAY:
  case SQL_C_INTERVAL_DAY_TO_HOUR:
  case SQL_C_INTERVAL_DAY_TO_MINUTE:
  case SQL_C_INTERVAL_DAY_TO_SECOND:
  case SQL_C_INTERVAL_HOUR:
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
  case SQL_C_INTERVAL_MINUTE:
  case SQL_C_INTERVAL_MINUTE_TO_SECOND:
  case SQL_C_INTERVAL_SECOND:
      return SQL_INTERVAL;
  default:
    return ConciseType;
  }
}
/* }}} */

/* {{{ MADB_GetTypeName */
char *MADB_GetTypeName(MYSQL_FIELD Field)
{
  switch(Field.type) {
  case MYSQL_TYPE_DECIMAL:
  case MYSQL_TYPE_NEWDECIMAL:
    return "decimal";
  case MYSQL_TYPE_NULL:
    return "null";
  case MYSQL_TYPE_TINY:
    return (Field.flags & NUM_FLAG) ? "tinyint" : "char";
  case MYSQL_TYPE_SHORT:
    return "smallint";
  case MYSQL_TYPE_LONG:
    return "integer";
  case MYSQL_TYPE_FLOAT:
    return "float";
  case MYSQL_TYPE_DOUBLE:
    return "double";
  case MYSQL_TYPE_TIMESTAMP:
    return "timestamp";
  case MYSQL_TYPE_LONGLONG:
    return "bigint";
  case MYSQL_TYPE_INT24:
    return "mediumint";
  case MYSQL_TYPE_DATE:
    return "date";
  case MYSQL_TYPE_TIME:
    return "time";
  case MYSQL_TYPE_DATETIME:
    return "datetime";
  case MYSQL_TYPE_YEAR:
    return "year";
  case MYSQL_TYPE_NEWDATE:
    return "date";
  case MYSQL_TYPE_VARCHAR:
  case MYSQL_TYPE_VAR_STRING:
    return MADB_FIELD_IS_BINARY(Field) ? "varbinary" : "varchar";
  case MYSQL_TYPE_BIT:
    return "bit";
  case MYSQL_TYPE_ENUM:
    return "enum";
  case MYSQL_TYPE_SET:
    return "set";
  case MYSQL_TYPE_TINY_BLOB:
    return MADB_FIELD_IS_BINARY(Field) ? "tinyblob" : "tinytext";
  case MYSQL_TYPE_MEDIUM_BLOB:
    return MADB_FIELD_IS_BINARY(Field) ? "mediumblob" : "mediumtext";
  case MYSQL_TYPE_LONG_BLOB:
    return MADB_FIELD_IS_BINARY(Field) ? "longblob" : "longtext";
  case MYSQL_TYPE_BLOB:
    return MADB_FIELD_IS_BINARY(Field) ? "blob" : "text";
  case MYSQL_TYPE_STRING:
    return MADB_FIELD_IS_BINARY(Field) ? "binary" : "char";
  case MYSQL_TYPE_GEOMETRY:
    return "geometry";
  default:
    return "";
  }
}
/* }}} */

MYSQL_RES *MADB_GetDefaultColumnValues(MADB_Stmt *Stmt, MYSQL_FIELD *fields)
{
  MADB_DynString DynStr;
  unsigned int i;
  MYSQL_RES *result= NULL;
  
  MADB_InitDynamicString(&DynStr, "SELECT COLUMN_NAME, COLUMN_DEFAULT FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA='", 512, 512);
  if (MADB_DynstrAppend(&DynStr, fields[0].db) ||
      MADB_DynstrAppend(&DynStr, "' AND TABLE_NAME='") ||
      MADB_DynstrAppend(&DynStr, fields[0].org_table) ||
      MADB_DynstrAppend(&DynStr, "' AND COLUMN_NAME IN ("))
    goto error;

  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    MADB_DescRecord *Rec= MADB_DescGetInternalRecord(Stmt->Ard, i, MADB_DESC_READ);

    if (!Rec->inUse || MADB_ColumnIgnoredInAllRows(Stmt->Ard, Rec) == TRUE)
    {
      continue;
    }
    if (MADB_DynstrAppend(&DynStr, i > 0 ? ",'" : "'") ||
      MADB_DynstrAppend(&DynStr, fields[i].org_name) ||
      MADB_DynstrAppend(&DynStr, "'"))
    {
      goto error;
    }
  }
  if (MADB_DynstrAppend(&DynStr, ") AND COLUMN_DEFAULT IS NOT NULL"))
    goto error;

  LOCK_MARIADB(Stmt->Connection);
  if (mysql_query(Stmt->Connection->mariadb, DynStr.str))
    goto error;
  result= mysql_store_result(Stmt->Connection->mariadb);
  
error:
    UNLOCK_MARIADB(Stmt->Connection);
    MADB_DynstrFree(&DynStr);
    return result;
}

char *MADB_GetDefaultColumnValue(MYSQL_RES *res, const char *Column)
{
  MYSQL_ROW row;

  if (res == NULL || !res->row_count)
    return NULL;
  mysql_data_seek(res, 0);
  while ((row= mysql_fetch_row(res)))
  {
    if (_stricmp(row[0], Column) == 0)
     return _strdup(row[1]);
  }
  return NULL;
}

size_t MADB_GetDataSize(MADB_DescRecord *Record, MYSQL_FIELD Field, MARIADB_CHARSET_INFO *charset)
{
  switch(Record->ConciseType)
  {
  case SQL_BIT:
    return 1;
  case SQL_TINYINT:
    return 3;
  case SQL_SMALLINT:
    return 5;
  case SQL_INTEGER:
    return 10;
  case SQL_BIGINT:
    return 20 - test(Field.flags & UNSIGNED_FLAG);
  case SQL_REAL:
    return 7;
  case SQL_DOUBLE:
  case SQL_FLOAT:
    return 15;
  case SQL_DECIMAL:
  case SQL_NUMERIC:
    return Record->Precision;
  case SQL_TYPE_DATE:
    return 10;
  case SQL_TYPE_TIME:
    return 8 + Field.decimals;
  case SQL_TYPE_TIMESTAMP:
    return Field.length;
  default:
    {
      if (MADB_FIELD_IS_BINARY(Field) || charset == NULL || charset->char_maxlen < 2/*i.e.0||1*/)
      {
        return Field.length;
      }
      else
      {
        return Field.length/charset->char_maxlen;
      }
    }
  }
}

/* {{{ MADB_GetDisplaySize */
size_t MADB_GetDisplaySize(MYSQL_FIELD Field, MARIADB_CHARSET_INFO *charset)
{
  /* Todo: check these values with output from mysql --with-columntype-info */
  switch (Field.type) {
  case MYSQL_TYPE_NULL:
    return 1;
  case MYSQL_TYPE_BIT:
    return (Field.length == 1) ? 1 : (Field.length + 7) / 8 * 2;
  case MYSQL_TYPE_TINY:
    return 4 - test(Field.flags & UNSIGNED_FLAG);
  case MYSQL_TYPE_SHORT:
  case MYSQL_TYPE_YEAR:
    return 6 - test(Field.flags & UNSIGNED_FLAG);
  case MYSQL_TYPE_INT24:
    return 9 - test(Field.flags & UNSIGNED_FLAG);
  case MYSQL_TYPE_LONG:
    return 11 - test(Field.flags & UNSIGNED_FLAG);
  case MYSQL_TYPE_LONGLONG:
    return 20;
  case MYSQL_TYPE_DOUBLE:
    return 15;
  case MYSQL_TYPE_FLOAT:
    return 7;
  case MYSQL_TYPE_DECIMAL:
  case MYSQL_TYPE_NEWDECIMAL:
    return 10;
  case MYSQL_TYPE_DATE:
    return 10; /* YYYY-MM-DD */
  case MYSQL_TYPE_TIME:
    return 8; /* HH:MM:SS */
  case MYSQL_TYPE_NEWDATE:
  case MYSQL_TYPE_TIMESTAMP:
  case MYSQL_TYPE_DATETIME:
    return 19;
  case MYSQL_TYPE_BLOB:
  case MYSQL_TYPE_ENUM:
  case MYSQL_TYPE_GEOMETRY:
  case MYSQL_TYPE_LONG_BLOB:
  case MYSQL_TYPE_MEDIUM_BLOB:
  case MYSQL_TYPE_SET:
  case MYSQL_TYPE_STRING:
  case MYSQL_TYPE_TINY_BLOB:
  case MYSQL_TYPE_VARCHAR:
  case MYSQL_TYPE_VAR_STRING:
  {
    if (MADB_FIELD_IS_BINARY(Field))
    {
      return Field.length*2; /* ODBC specs says we should give 2 characters per byte to display binaray data in hex form */
    }
    else if (charset == NULL || charset->char_maxlen < 2/*i.e.0||1*/)
    {
      return Field.length;
    }
    else
    {
      return Field.length/charset->char_maxlen;
    }
  }
  default:
    return SQL_NO_TOTAL;
  }
}
/* }}} */

/* {{{ MADB_GetOctetLength */
size_t MADB_GetOctetLength(MYSQL_FIELD Field, unsigned short MaxCharLen)
{
  size_t Length= MIN(MADB_INT_MAX32, Field.length);

  switch (Field.type) {
  case MYSQL_TYPE_NULL:
    return 1;
  case MYSQL_TYPE_BIT:
    return (Field.length + 7) / 8;
  case MYSQL_TYPE_TINY:
    return 1;
  case MYSQL_TYPE_YEAR:
  case MYSQL_TYPE_SHORT:
    return 2;
  case MYSQL_TYPE_INT24:
    return 3;
  case MYSQL_TYPE_LONG:
    return 4;
  case MYSQL_TYPE_LONGLONG:
    return 8;
  case MYSQL_TYPE_DOUBLE:
    return 8;
  case MYSQL_TYPE_FLOAT:
    return 4;
  case MYSQL_TYPE_DECIMAL:
  case MYSQL_TYPE_NEWDECIMAL:
    return Field.length;
  case MYSQL_TYPE_DATE:
    return SQL_DATE_LEN;
  case MYSQL_TYPE_TIME:
    return SQL_TIME_LEN;
   case MYSQL_TYPE_NEWDATE:
  case MYSQL_TYPE_TIMESTAMP:
  case MYSQL_TYPE_DATETIME:
    return SQL_TIMESTAMP_LEN;
  case MYSQL_TYPE_BLOB:
  case MYSQL_TYPE_ENUM:
  case MYSQL_TYPE_GEOMETRY:
  case MYSQL_TYPE_LONG_BLOB:
  case MYSQL_TYPE_MEDIUM_BLOB:
  case MYSQL_TYPE_TINY_BLOB:
    return Length;
  case MYSQL_TYPE_SET:
  case MYSQL_TYPE_STRING:
  case MYSQL_TYPE_VARCHAR:
  case MYSQL_TYPE_VAR_STRING:
    /*if (!(Field.flags & BINARY_FLAG))
      Length *= MaxCharLen ? MaxCharLen : 1;*/
    return Length;
  default:
    return SQL_NO_TOTAL;
  }
}
/* }}} */

/* {{{ MADB_GetDefaultType */
int MADB_GetDefaultType(int SQLDataType)
{
  switch(SQLDataType)
  {
  case SQL_BIGINT:
    return SQL_C_SBIGINT;
  case SQL_BINARY:
    return SQL_C_BINARY;
  case SQL_BIT:
    return SQL_C_BIT;
  case SQL_CHAR:
    return SQL_C_CHAR;
  case SQL_DATE:
  case SQL_TYPE_DATE:
    return SQL_C_DATE;
  case SQL_DECIMAL:
    return SQL_C_CHAR;
  case SQL_DOUBLE:
    return SQL_C_DOUBLE; 
  case SQL_FLOAT:
    return SQL_C_FLOAT;
  case SQL_INTEGER:
    return SQL_C_LONG;
  case SQL_LONGVARBINARY:
    return SQL_C_BINARY;
  case SQL_LONGVARCHAR:
    return SQL_C_CHAR;
  case SQL_NUMERIC:
    return SQL_C_CHAR;
  case SQL_REAL:
    return SQL_C_FLOAT;
  case SQL_SMALLINT:
    return SQL_C_SHORT;
  case SQL_TIME:
  case SQL_TYPE_TIME:
    return SQL_C_TIME;
  case SQL_TIMESTAMP:
  case SQL_TYPE_TIMESTAMP:
    return SQL_C_TIMESTAMP;
  case SQL_TINYINT:
    return SQL_C_TINYINT;
  case SQL_VARBINARY:
    return SQL_C_BINARY;
  case SQL_VARCHAR:
    return SQL_C_CHAR;
  default:
    return SQL_C_CHAR;
  }
}
/* }}} */

/* {{{ MADB_GetODBCType */
       /* It's not quite right to mix here C and SQL types, even though constants are sort of equal */
SQLSMALLINT MADB_GetODBCType(MYSQL_FIELD *field)
{
  switch (field->type) {
    case MYSQL_TYPE_BIT:
      if (field->length > 1)
        return SQL_BINARY;
      return SQL_BIT;
    case MYSQL_TYPE_NULL:
      return SQL_VARCHAR;
    case MYSQL_TYPE_TINY:
      return field->flags & NUM_FLAG ? SQL_TINYINT : SQL_CHAR;
    case MYSQL_TYPE_YEAR:
    case MYSQL_TYPE_SHORT:
      return SQL_SMALLINT;
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONG:
      return SQL_INTEGER;
    case MYSQL_TYPE_FLOAT:
      return SQL_REAL;
    case MYSQL_TYPE_DOUBLE:
      return SQL_DOUBLE;
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
      return SQL_TYPE_TIMESTAMP;
    case MYSQL_TYPE_NEWDATE:
    case MYSQL_TYPE_DATE:
      return SQL_TYPE_DATE;
    case MYSQL_TYPE_TIME:
       return SQL_TYPE_TIME;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
      return MADB_FIELD_IS_BINARY(*field) ? SQL_LONGVARBINARY : SQL_LONGVARCHAR;
    case MYSQL_TYPE_LONGLONG:
      return SQL_BIGINT;
    case MYSQL_TYPE_STRING:
      return MADB_FIELD_IS_BINARY(*field) ? SQL_BINARY : SQL_CHAR;
    case MYSQL_TYPE_VAR_STRING:
      return MADB_FIELD_IS_BINARY(*field) ? SQL_VARBINARY : SQL_VARCHAR;
    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_ENUM:
      return SQL_CHAR;
    case MYSQL_TYPE_GEOMETRY:
      return SQL_LONGVARBINARY;
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
      return SQL_DECIMAL;
    default:
      return SQL_UNKNOWN_TYPE;
  }
}

/* }}} */
/* {{{ MADB_GetTypeLength */
size_t MADB_GetTypeLength(SQLINTEGER SqlDataType, size_t Length)
{
  switch(SqlDataType)
  {
  case SQL_C_BIT:
  case SQL_C_TINYINT:
  case SQL_C_STINYINT:
  case SQL_C_UTINYINT:
    return 1;
  case SQL_C_SHORT:
  case SQL_C_SSHORT:
  case SQL_C_USHORT:
    return 2;
  case SQL_C_LONG:
  case SQL_C_SLONG:
  case SQL_C_ULONG:
    return sizeof(SQLINTEGER);
  case SQL_C_UBIGINT:
  case SQL_C_SBIGINT:
    return sizeof(long long);
  case SQL_C_DOUBLE:
    return sizeof(SQLDOUBLE);
  case SQL_C_FLOAT:
    return sizeof(SQLFLOAT);
  case SQL_C_NUMERIC:
    return sizeof(SQL_NUMERIC_STRUCT);
  case SQL_C_TYPE_TIME:
  case SQL_C_TIME:
    return sizeof(SQL_TIME_STRUCT);
  case SQL_C_TYPE_DATE:
  case SQL_C_DATE:
    return sizeof(SQL_DATE_STRUCT);
  case SQL_C_TYPE_TIMESTAMP:
  case SQL_C_TIMESTAMP:
    return sizeof(SQL_TIMESTAMP_STRUCT);
  default:
    return Length;
  }
}
/* }}} */

/* {{{ MADB_GetMaDBTypeAndLength */
int MADB_GetMaDBTypeAndLength(SQLINTEGER SqlDataType, my_bool *Unsigned, unsigned long *Length)
{
  *Unsigned= 0;
  switch(SqlDataType)
  {
  case SQL_C_BIT:
  case SQL_C_TINYINT:
  case SQL_C_STINYINT:
  case SQL_C_UTINYINT:
    *Length= 1;
    *Unsigned= (SqlDataType == SQL_C_UTINYINT);

    return MYSQL_TYPE_TINY;

  case SQL_C_SHORT:
  case SQL_C_SSHORT:
  case SQL_C_USHORT:
    *Length= 2;
    *Unsigned= (SqlDataType == SQL_C_USHORT);

    return MYSQL_TYPE_SHORT;
 
  case SQL_C_LONG:
  case SQL_C_SLONG:
  case SQL_C_ULONG:
    *Length= sizeof(SQLINTEGER);
    *Unsigned= (SqlDataType == SQL_C_ULONG);
    return MYSQL_TYPE_LONG;
  case SQL_C_UBIGINT:
  case SQL_C_SBIGINT:
    *Length= sizeof(long long);
    *Unsigned= (SqlDataType == SQL_C_UBIGINT);
    return MYSQL_TYPE_LONGLONG;
  case SQL_C_DOUBLE:
    *Length= sizeof(SQLDOUBLE);
    return MYSQL_TYPE_DOUBLE;
  case SQL_C_FLOAT:
    *Length =sizeof(SQLFLOAT);
    return MYSQL_TYPE_FLOAT;
  case SQL_C_NUMERIC:
    /**Length= sizeof(SQL_NUMERIC_STRUCT);*/
    return MYSQL_TYPE_DECIMAL;
  case SQL_C_TYPE_TIME:
  case SQL_C_TIME:
    *Length= sizeof(SQL_TIME_STRUCT);
    return MYSQL_TYPE_TIME;
  case SQL_C_TYPE_DATE:
  case SQL_C_DATE:
    *Length= sizeof(SQL_DATE_STRUCT);
    return MYSQL_TYPE_DATE;
  case SQL_C_TYPE_TIMESTAMP:
  case SQL_C_TIMESTAMP:
    *Length= sizeof(SQL_TIMESTAMP_STRUCT);
    return MYSQL_TYPE_TIMESTAMP;
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
    *Length= sizeof(SQL_INTERVAL_STRUCT);
    return MYSQL_TYPE_TIME;
  case SQL_C_CHAR:
    return MYSQL_TYPE_STRING;
  default:
    return MYSQL_TYPE_BLOB;
  }
}
/* }}} */

SQLRETURN MADB_CopyMadbTimestamp(MADB_Stmt *Stmt, MYSQL_TIME *tm, MADB_Desc *Ard, MADB_DescRecord *ArdRecord, int Type, unsigned long RowNumber)
{
  void *DataPtr= GetBindOffset(Ard, ArdRecord, ArdRecord->DataPtr, RowNumber, ArdRecord->OctetLength);

  switch(Type) {
  case SQL_C_TIMESTAMP:
  case SQL_C_TYPE_TIMESTAMP:
    {
      SQL_TIMESTAMP_STRUCT *ts= (SQL_TIMESTAMP_STRUCT *)DataPtr;
      /* if (!tm->year)
      {
        time_t sec_time;
        struct tm * cur_tm;
        sec_time= time(NULL);
        cur_tm= localtime(&sec_time);
        ts->year= 1900 + cur_tm->tm_year;
        ts->month= cur_tm->tm_mon + 1;
        ts->day= cur_tm->tm_mday;
      }
      else */
      {
        ts->year= tm->year;
        ts->month= tm->month;
        ts->day= tm->day;
      }
      ts->hour= tm->hour;
      ts->minute= tm->minute;
      ts->second= tm->second;
      ts->fraction= tm->second_part * 1000;
      if (ts->year + ts->month + ts->day + ts->hour + ts->minute + ts->fraction + ts->second == 0)
        if (ArdRecord->IndicatorPtr)
          *ArdRecord->IndicatorPtr= SQL_NULL_DATA;
    }
    break;
    case SQL_C_TIME:
    case SQL_C_TYPE_TIME:
    {
      SQL_TIME_STRUCT *ts= (SQL_TIME_STRUCT *)DataPtr;

      if (tm->hour > 23 || tm->minute > 59 || tm->second > 59)
      {
        return MADB_SetError(&Stmt->Error, MADB_ERR_22007, NULL, 0);
      }
      ts->hour= tm->hour;
      ts->minute= tm->minute;
      ts->second= tm->second;
    }
    break;
    case SQL_C_DATE:
    case SQL_TYPE_DATE:
    {
      SQL_DATE_STRUCT *ts= (SQL_DATE_STRUCT *)DataPtr;
      ts->year= tm->year;
      ts->month= tm->month;
      ts->day= tm->day;
      if (ts->year + ts->month + ts->day == 0)
        if (ArdRecord->IndicatorPtr)
          *ArdRecord->IndicatorPtr= SQL_NULL_DATA;
    }
    break;
  }

  return SQL_SUCCESS;
}

void *GetBindOffset(MADB_Desc *Desc, MADB_DescRecord *Record, void *Ptr, SQLULEN RowNumber, size_t PtrSize)
{
  size_t BindOffset= 0;

  if (!Ptr)
    return NULL;

  if (Desc->Header.BindOffsetPtr)
    BindOffset= (size_t)*Desc->Header.BindOffsetPtr;
  /* row wise binding */
  if (Desc->Header.BindType == SQL_BIND_BY_COLUMN ||
      Desc->Header.BindType == SQL_PARAM_BIND_BY_COLUMN)
    BindOffset+= PtrSize * RowNumber;
  else
    BindOffset+= Desc->Header.BindType * RowNumber;
  return (char *)Ptr + BindOffset;
}

/* Checking if column ignored in all bound rows. Should hel*/
BOOL MADB_ColumnIgnoredInAllRows(MADB_Desc *Desc, MADB_DescRecord *Rec)
{
  SQLULEN row;
  SQLLEN *IndicatorPtr;

  for (row= 0; row < Desc->Header.ArraySize; ++row)
  {
    IndicatorPtr= (SQLLEN *)GetBindOffset(Desc, Rec, Rec->IndicatorPtr, row, sizeof(SQLLEN));

    if (IndicatorPtr == NULL || *IndicatorPtr != SQL_COLUMN_IGNORE)
    {
      return FALSE;
    }
  }

  return TRUE;
}


void MADB_NumericInit(SQL_NUMERIC_STRUCT *number, MADB_DescRecord *Ard)
{
  if (!number)
    return;
  number->precision= (SQLCHAR)Ard->Precision;
  number->scale= (SQLCHAR)Ard->Scale;
  memset(number->val, 0, sizeof(number->val));
}

/* {{{ MADB_CharToSQLNumeric */
int MADB_CharToSQLNumeric(char *buffer, MADB_Desc *Ard, MADB_DescRecord *ArdRecord, unsigned long RowNumber)
{
  char *p;
  SQL_NUMERIC_STRUCT *number= (SQL_NUMERIC_STRUCT *)GetBindOffset(Ard, ArdRecord, ArdRecord->DataPtr, RowNumber, ArdRecord->OctetLength);
  int ret= 0;

  if (!buffer || !number)
    return ret;

  p= trim(buffer);
  MADB_NumericInit(number, ArdRecord);

  if (!(number->sign= (*p=='-') ? 0 : 1))
    p++;
  if (!*p)
    return FALSE;

  if (number->precision == 0)
  {
    number->precision= MADB_DEFAULT_PRECISION;
  }

  while (*p=='0')
  {
    p++;
  }
  if (*p)
  {
    int i;
    int bit, hval, tv, dig, sta, olen;
    int tmp_digit= 0;
    int leading_zeros= 0;
    char *dot= strchr(p, '.');
    char digits[100];
    short digits_count= 0;

    /* Overflow check */
    if (number->precision > 0 && (dot - p) > number->precision)
      return MADB_ERR_22003;
    
    if (dot && number->scale > 0)
    {
      short digits_total= 0, 
            digits_significant= 0;
      digits_count= (short)(dot - p);
      memcpy(digits, p, digits_count);
      p= dot + 1;
      while (*p)
      {
        /* ignore non numbers */
        if (!isdigit(0x000000ff & *p))
          break;
        digits_total++;
        /* ignore trailing zeros */
        if (*p != '0')
        {
          digits_significant= digits_total;
        }
        p++;
      }

      if (digits_count + number->scale > number->precision)
      {
        int i;
        /* if digits are zero there is no overflow */
        for (i=1; i <= digits_significant; i++)
        {
          p= dot + i;
          if (*p != '0')
            return MADB_ERR_22003;
        }
      }
      
      memcpy(digits + digits_count, dot + 1, digits_significant);
      if (number->scale > digits_significant)
      {
        for (i= digits_count + digits_significant; i < number->precision && i < digits_count +number->scale; ++i)
        {
          digits[i]= '0';
        }
        digits_significant= number->scale;
      }
      digits_count+= digits_significant;
    }
    else 
    {
      char *start= p;
      while (*p && isdigit(0x000000ff & *p))
        p++;
      /* check overflow */
      if (p - start > number->precision)
      {
        return MADB_ERR_22003;
      }
      digits_count= (short)(p - start);
      memcpy(digits, start, digits_count);
      number->scale= ArdRecord->Scale ? ArdRecord->Scale : 0;
    }

    /* Rounding */
    if (number->scale < 0)
    {
      int64_t OldVal, Val;
      int64_t RoundNumber= (int64_t)pow(10.0, -number->scale);

      digits[number->precision]= 0;
      Val= _atoi64(digits);

      OldVal= Val;
      Val= (Val + RoundNumber / 2) / RoundNumber * RoundNumber;
      if (OldVal != Val)
        return MADB_ERR_22003;
      _snprintf(digits, sizeof(digits), "%lld", Val);
      digits_count= (short)strlen(digits);
      if (digits_count > number->precision)
        return MADB_ERR_22003;
    }

    digits_count= MIN(digits_count, MADB_DEFAULT_PRECISION);
    for (hval = 0, bit = 1L, sta = 0, olen = 0; sta < digits_count;)
    {
      for (dig = 0, i = sta; i < digits_count; i++)
      {
        tv = dig * 10 + digits[i] - '0';
        dig = tv % 2;
        digits[i] = tv / 2 + '0';
        if (i == sta && tv < 2)
          sta++;
      }
      if (dig > 0)
        hval |= bit;
      bit <<= 1;
      if (bit >= (1L << 8))
      {
        number->val[olen++] = hval;
        hval = 0;
        bit = 1L;
        if (olen >= SQL_MAX_NUMERIC_LEN - 1)
        {
          //number->scale = sta - number->precision;
          //ret= MADB_ERR_22003;
          break;
        }
      } 
    }
    if (hval && olen < SQL_MAX_NUMERIC_LEN - 1)
    {
      number->val[olen++] = hval;
    }
  } 
  return ret;
}

/* {{{ MADB_GetHexString */
size_t MADB_GetHexString(char *BinaryBuffer, size_t BinaryLength,
                          char *HexBuffer, size_t HexLength)
{
  const char HexDigits[]= "0123456789ABCDEF";
  char *Start= HexBuffer;
  size_t CurrentLength= HexLength;

  if (!HexBuffer || !BinaryBuffer)
    return 0;
   
  while (BinaryLength-- && CurrentLength > 2)
  {
    *HexBuffer++=HexDigits[*BinaryBuffer >> 4];
    *HexBuffer++=HexDigits[*BinaryBuffer & 0x0F];
    BinaryBuffer++;
    CurrentLength-= 2;
  }
  *HexBuffer= 0;
  return (HexBuffer - Start);
}
/* }}} */

unsigned long MADB_StmtDataTell(MADB_Stmt *Stmt)
{
  MYSQL_ROWS *ptr= Stmt->stmt->result.data;
  unsigned long Offset= 0;

  while (ptr && ptr!= Stmt->stmt->result_cursor)
  {
    Offset++;
    ptr= ptr->next;
  }
  return ptr ? Offset : 0;
}

SQLRETURN MADB_DaeStmt(MADB_Stmt *Stmt, SQLUSMALLINT Operation)
{
  char          *TableName=   MADB_GetTableName(Stmt);
  char          *CatalogName= MADB_GetCatalogName(Stmt);
  MADB_DynString DynStmt;

  MADB_CLEAR_ERROR(&Stmt->Error);
  memset(&DynStmt, 0, sizeof(MADB_DynString));

  if (Stmt->DaeStmt)
    Stmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
  Stmt->DaeStmt= NULL;

  if (!SQL_SUCCEEDED(MA_SQLAllocHandle(SQL_HANDLE_STMT, (SQLHANDLE)Stmt->Connection, (SQLHANDLE *)&Stmt->DaeStmt)))
  {
    MADB_CopyError(&Stmt->Error, &Stmt->Connection->Error);
    goto end;
  }

  switch(Operation)
  {
  case SQL_ADD:
    if (MADB_InitDynamicString(&DynStmt, "INSERT INTO ", 1024, 1024) ||
        MADB_DynStrAppendQuoted(&DynStmt, CatalogName) ||
        MADB_DynstrAppend(&DynStmt, ".") ||
        MADB_DynStrAppendQuoted(&DynStmt, TableName)||
        MADB_DynStrUpdateSet(Stmt, &DynStmt))
    {
      MADB_DynstrFree(&DynStmt);
      return Stmt->Error.ReturnValue;
    }
    Stmt->DataExecutionType= MADB_DAE_ADD;
    break;
  case SQL_DELETE:
    if (MADB_InitDynamicString(&DynStmt, "DELETE FROM ", 1024, 1024) ||
        MADB_DynStrAppendQuoted(&DynStmt, CatalogName) ||
        MADB_DynstrAppend(&DynStmt, ".") ||
        MADB_DynStrAppendQuoted(&DynStmt, TableName) ||
        MADB_DynStrGetWhere(Stmt, &DynStmt, TableName, FALSE))
    {
      MADB_DynstrFree(&DynStmt);
      return Stmt->Error.ReturnValue;
    }
    Stmt->DataExecutionType= MADB_DAE_DELETE;
    break;
  case SQL_UPDATE:
    if (MADB_InitDynamicString(&DynStmt, "UPDATE ", 1024, 1024) ||
        MADB_DynStrAppendQuoted(&DynStmt, CatalogName) ||
        MADB_DynstrAppend(&DynStmt, ".") ||
        MADB_DynStrAppendQuoted(&DynStmt, TableName)||
        MADB_DynStrUpdateSet(Stmt, &DynStmt)||
        MADB_DynStrGetWhere(Stmt, &DynStmt, TableName, FALSE))
    {
      MADB_DynstrFree(&DynStmt);
      return Stmt->Error.ReturnValue;
    }
    Stmt->DataExecutionType= MADB_DAE_UPDATE;
    break;
  }
  
  if (!SQL_SUCCEEDED(Stmt->DaeStmt->Methods->Prepare(Stmt->DaeStmt, (SQLCHAR *)DynStmt.str, SQL_NTS, FALSE)))
  {
    MADB_CopyError(&Stmt->Error, &Stmt->DaeStmt->Error);
    Stmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
  }
   
end:
  MADB_DynstrFree(&DynStmt);
  return Stmt->Error.ReturnValue;

}


int MADB_FindNextDaeParam(MADB_Desc *Desc, int InitialParam, SQLSMALLINT RowNumber)
{
  int             i;
  MADB_DescRecord *Record;

  for (i= InitialParam > -1 ? InitialParam + 1 : 0; i < Desc->Header.Count; i++)
  {
    if ((Record= MADB_DescGetInternalRecord(Desc, i, MADB_DESC_READ)))
    {
      if (Record->OctetLengthPtr)
      {
        /* Stmt->DaeRowNumber is 1 based */
        SQLLEN *OctetLength = (SQLLEN *)GetBindOffset(Desc, Record, Record->OctetLengthPtr, RowNumber > 1 ? RowNumber - 1 : 0, sizeof(SQLLEN));
        if (PARAM_IS_DAE(OctetLength))
        {
          return i;
        }
      }
    }
  }

  return MADB_NOPARAM;
}


BOOL MADB_IsNumericType(SQLSMALLINT ConciseType)
{
  switch (ConciseType)
  {
    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
    case SQL_C_UTINYINT:
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
    case SQL_C_USHORT:
    case SQL_C_LONG:
    case SQL_C_SLONG:
    case SQL_C_ULONG:
    case SQL_C_UBIGINT:
    case SQL_C_SBIGINT:
    case SQL_BIGINT:
    case SQL_C_DOUBLE:
    case SQL_C_FLOAT:
    case SQL_DECIMAL:
      return TRUE;
  }
  return FALSE;
}


BOOL MADB_IsIntType(SQLSMALLINT ConciseType)
{
  switch (ConciseType)
  {
  case SQL_C_TINYINT:
  case SQL_C_STINYINT:
  case SQL_C_UTINYINT:
  case SQL_C_SHORT:
  case SQL_C_SSHORT:
  case SQL_C_USHORT:
  case SQL_C_LONG:
  case SQL_C_SLONG:
  case SQL_C_ULONG:
  case SQL_C_UBIGINT:
  case SQL_C_SBIGINT:
  case SQL_BIGINT:
    return TRUE;
  }
  return FALSE;
}


void MADB_InstallStmt(MADB_Stmt *Stmt)
{
  Stmt->stmt= Stmt->MultiStmts[Stmt->MultiStmtNr];

  if (mysql_stmt_field_count(Stmt->stmt) == 0)
  {
    Stmt->AffectedRows= mysql_stmt_affected_rows(Stmt->stmt);
  }
  else
  {
    MADB_StmtResetResultStructures(Stmt);
    MADB_DescSetIrdMetadata(Stmt, mysql_fetch_fields(FetchMetadata(Stmt)), mysql_stmt_field_count(Stmt->stmt));
  }
}