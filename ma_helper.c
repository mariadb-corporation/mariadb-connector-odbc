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
#include <stdint.h>

void CloseMultiStatements(MADB_Stmt *Stmt)
{
  unsigned int i;

  for (i=0; i < Stmt->MultiStmtCount; ++i)
    mysql_stmt_close(Stmt->MultiStmts[i]);
  MADB_FREE(Stmt->MultiStmts);
  Stmt->MultiStmtCount= 0;
}


/* Required, but not sufficient condition */
BOOL QueryIsPossiblyMultistmt(char *queryStr)
{
  if (strchr(queryStr, ';'))
  {
    /* CREATE PROCEDURE uses semicolons but is not supported in prepared statement
        protocol */
    if (!MADB_IsStatementSupported(queryStr, "CREATE", "PROCEDURE"))
      return FALSE;
    if (!MADB_IsStatementSupported(queryStr, "CREATE", "DEFINER"))
      return FALSE;
    return TRUE;
  }

  return FALSE;
}

unsigned int GetMultiStatements(MADB_Stmt *Stmt, char *StmtStr, size_t Length)
{
  char *p, *last, *prev= NULL;
  unsigned int statements= 1;
  int quote[2]= {0,0}, comment= 0;
  char *end;
  MYSQL_STMT *stmt;
  char *StmtCopy= NULL;

  stmt= mysql_stmt_init(Stmt->Connection->mariadb);

  /* if the entire stmt string passes, we don't have multistatement */
  if (stmt && !mysql_stmt_prepare(stmt, StmtStr, Length))
  {
    mysql_stmt_close(stmt);
    return 1;
  }
  mysql_stmt_close(stmt);
  /* make sure we don't have trailing whitespace or semicolon */
  if (Length)
  {
    end= StmtStr + Length - 1;
    while (end > StmtStr && (isspace(*end) || *end == ';'))
      end--;
    Length= end - StmtStr;
  }
  p= last= StmtCopy= my_strdup(StmtStr, MYF(0));
  
  while (p < StmtCopy + Length)
  {
    switch (*p) {
    case ';':
      if (!quote[0] && !quote[1] && !comment)
      {
        statements++;
        last= p + 1;
        *p= '\0';
      }
      break;
    case '/':
      if (!comment && (p < StmtCopy + Length + 1) && (char)*(p+1) ==  '*')
        comment= 1;
      else if (comment && (p > StmtCopy) && (char)*(p-1) == '*')
        comment= 0;
      break;
    case '\"':
      if (prev && *prev != '\\')
        quote[0] = !quote[0];
      break;
    case 39:
      if (prev && *prev != '\\')
        quote[1] = !quote[1];
      break;
    default:
      break;
    }
    prev= p;
    p++;
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
      Stmt->MultiStmts[i]= mysql_stmt_init(Stmt->Connection->mariadb);
      if (mysql_stmt_prepare(Stmt->MultiStmts[i], p, strlen(p)))
      {
        MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->MultiStmts[i]);
        CloseMultiStatements(Stmt);
        if (StmtCopy)
          my_free(StmtCopy);
        return 0;
      }
      if (mysql_stmt_param_count(Stmt->MultiStmts[i]) > MaxParams)
        MaxParams= mysql_stmt_param_count(Stmt->MultiStmts[i]);
      p+= strlen(p) + 1;
      ++i;
    }
    if (MaxParams)
      Stmt->params= (MYSQL_BIND *)MADB_CALLOC(sizeof(MYSQL_BIND) * MaxParams);
  }
  if (StmtCopy)
    my_free(StmtCopy);

  return statements;
}

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
  int Count= 0;
  unsigned int i;
  char StmtStr[1024];
  char *p= StmtStr;
  char Database[65];
  SQLHSTMT Stmt= NULL;
  MADB_Stmt *KeyStmt;
  
  SQLGetConnectAttr((SQLHDBC)Connection, SQL_ATTR_CURRENT_CATALOG, Database, 65, NULL);
  p+= my_snprintf(p, 1024, "SELECT * FROM ");
  if (Database)
    p+= my_snprintf(p, 1024 - strlen(p), "`%s`.", Database);
  p+= my_snprintf(p, 1024 - strlen(p), "%s LIMIT 0", TableName);
  if (SQLAllocStmt((SQLHDBC)Connection, &Stmt) == SQL_ERROR ||
      SQLPrepare(Stmt, (SQLCHAR *)StmtStr, SQL_NTS) == SQL_ERROR ||
      SQLExecute(Stmt) == SQL_ERROR ||
      SQLFetch(Stmt) == SQL_ERROR)
      goto end;
  KeyStmt= (MADB_Stmt *)Stmt;
  for (i=0; i < mysql_stmt_field_count(KeyStmt->stmt); i++)
    if (KeyStmt->stmt->fields[i].flags & KeyFlag)
      Count++;
end:
  if (Stmt)
    SQLFreeHandle(SQL_HANDLE_STMT, Stmt);
  return Count;
}

/* {{{ MADB_get_single_row */
my_bool MADB_get_single_row(MADB_Dbc *Connection,
    const char *StmtString,
    size_t Length,
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
bool MADB_CheckODBCType(SQLSMALLINT Type)
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
    return (Field.flags & BINARY_FLAG) ? "varbinary" : "varchar";
  case MYSQL_TYPE_BIT:
    return "bit";
  case MYSQL_TYPE_ENUM:
    return "enum";
  case MYSQL_TYPE_SET:
    return "set";
  case MYSQL_TYPE_TINY_BLOB:
    return (Field.flags & BINARY_FLAG) ? "tinyblob" : "tinytext";
  case MYSQL_TYPE_MEDIUM_BLOB:
    return (Field.flags & BINARY_FLAG) ? "mediumblob" : "mediumtext";
  case MYSQL_TYPE_LONG_BLOB:
    return (Field.flags & BINARY_FLAG) ? "longblob" : "longtext";
  case MYSQL_TYPE_BLOB:
    return (Field.flags & BINARY_FLAG) ? "blob" : "text";
  case MYSQL_TYPE_STRING:
    return (Field.flags & BINARY_FLAG) ? "binary" : "char";
  case MYSQL_TYPE_GEOMETRY:
    return "geometry";
  default:
    return "";
  }
}
/* }}} */

MYSQL_RES *MADB_GetDefaultColumnValues(MADB_Stmt *Stmt, MYSQL_FIELD *fields)
{
  DYNAMIC_STRING DynStr;
  unsigned int i;
  MYSQL_RES *result= NULL;
  
  init_dynamic_string(&DynStr, "SELECT COLUMN_NAME, COLUMN_DEFAULT FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA='", 512, 512);
  if (dynstr_append(&DynStr, fields[0].db) ||
      dynstr_append(&DynStr, "' AND TABLE_NAME='") ||
      dynstr_append(&DynStr, fields[0].org_table) ||
      dynstr_append(&DynStr, "' AND COLUMN_NAME IN ("))
    goto error;

  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    if (dynstr_append(&DynStr, i > 0 ? ",'" : "'") ||
        dynstr_append(&DynStr, fields[i].org_name) ||
        dynstr_append(&DynStr, "'"))
      goto error;
  }
  if (dynstr_append(&DynStr, ") AND COLUMN_DEFAULT IS NOT NULL"))
    goto error;

  LOCK_MARIADB(Stmt->Connection);
  if (mysql_query(Stmt->Connection->mariadb, DynStr.str))
    goto error;
  result= mysql_store_result(Stmt->Connection->mariadb);
  
error:
    UNLOCK_MARIADB(Stmt->Connection);
    dynstr_free(&DynStr);
    return result;
}

char *MADB_GetDefaultColumnValue(MYSQL_RES *res, const char *Column)
{
  MYSQL_ROW row;

  if (!res->row_count)
    return NULL;
  mysql_data_seek(res, 0);
  while ((row= mysql_fetch_row(res)))
  {
    if (_stricmp(row[0], Column) == 0)
     return _strdup(row[1]);
  }
  return NULL;
}

size_t MADB_GetDataSize(MADB_DescRecord *Record, MYSQL_FIELD Field, CHARSET_INFO *charset)
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
      if (Field.flags & BINARY_FLAG || Field.charsetnr == BINARY_CHARSETNR
        || charset == NULL || charset->char_maxlen < 2/*i.e.0||1*/)
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
size_t MADB_GetDisplaySize(MYSQL_FIELD Field, CHARSET_INFO *charset)
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
    if (Field.flags & BINARY_FLAG || Field.charsetnr == BINARY_CHARSETNR)
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
  size_t Length= MIN(INT_MAX32,Field.length);

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
      return field->flags & BINARY_FLAG ? SQL_LONGVARBINARY : SQL_LONGVARCHAR;
    case MYSQL_TYPE_LONGLONG:
      return SQL_BIGINT;
     case MYSQL_TYPE_STRING:
      return field->flags & BINARY_FLAG ? SQL_BINARY : SQL_CHAR;
    case MYSQL_TYPE_VAR_STRING:
      return field->flags & BINARY_FLAG ? SQL_VARBINARY : SQL_VARCHAR;
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
    return sizeof(longlong);
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

/* {{{ MADB_GetTypeAndLength */
int MADB_GetTypeAndLength(SQLINTEGER SqlDataType, my_bool *Unsigned, unsigned long *Length)
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
    *Length= sizeof(longlong);
    *Unsigned= (SqlDataType == SQL_C_UBIGINT);
    return MYSQL_TYPE_LONGLONG;
  case SQL_C_DOUBLE:
    *Length= sizeof(SQLDOUBLE);
    return MYSQL_TYPE_DOUBLE;
  case SQL_C_FLOAT:
    *Length =sizeof(SQLFLOAT);
    return MYSQL_TYPE_FLOAT;
  case SQL_C_NUMERIC:
    *Length= sizeof(SQL_NUMERIC_STRUCT);
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
  case SQL_C_CHAR:
    return MYSQL_TYPE_STRING;
  default:
    return MYSQL_TYPE_BLOB;
  }
}
/* }}} */

void MADB_CopyMadbTimestamp(MYSQL_TIME *tm, MADB_Desc *Ard, MADB_DescRecord *ArdRecord, int Type, unsigned long RowNumber)
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
    case SQL_TYPE_TIME:
    {
      SQL_TIME_STRUCT *ts= (SQL_TIME_STRUCT *)DataPtr;
      ts->hour= tm->hour;
      ts->minute= tm->minute;
      ts->second= tm->second;
       if (ts->hour + ts->minute + ts->second == 0)
        if (ArdRecord->IndicatorPtr)
          *ArdRecord->IndicatorPtr= SQL_NULL_DATA;
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
}

void *GetBindOffset(MADB_Desc *Desc, MADB_DescRecord *Record, void *Ptr, unsigned long RowNumber, size_t PtrSize)
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
    p++;
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
      digits_count= dot - p;
      memcpy(digits, p, digits_count);
      p= dot + 1;
      while (*p)
      {
        /* ignore non numbers */
        if (!isdigit(*p))
          break;
        digits_total++;
        /* ignore trailing zeros */
        if (*p != '0')
          digits_significant= digits_total;
        p++;
      }
      /* check possible overflow */
      digits_significant= MIN(digits_significant,number->scale);
      if (digits_count + digits_significant > number->precision)
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
      digits_count+= digits_significant;

    } else 
    {
      char *start= p;
      while (*p && isdigit(*p))
        p++;
      /* check overflow */
      if (p - start > number->precision)
      {
        return MADB_ERR_22003;
      }
      digits_count= p - start;
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
      digits_count= strlen(digits);
      if (digits_count > number->precision)
        return MADB_ERR_22003;
    }
    digits_count= MIN(digits_count, 38);
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
      number->val[olen++] = hval;
  } 
  return ret;
}

/* {{{ MADB_SqlNumericToChar */
size_t MADB_SqlNumericToChar(SQL_NUMERIC_STRUCT *Numeric, char *Buffer, int *ErrorCode)
{
  longlong Numerator= 0;
  longlong Denominator= 1;
  ulonglong Left= 0, Right= 0;
  int Scale= 0;
  int ppos= 0;
  long ByteDenominator= 1;
  int i;
  char *p;
  my_bool hasDot= FALSE;

  Buffer[0]= 0;
  *ErrorCode= 0;

  Scale+= (Numeric->scale < 0) ? -Numeric->scale : Numeric->scale;
      
  for (i=0; i < SQL_MAX_NUMERIC_LEN; ++i)
  {
    Numerator+= Numeric->val[i] * ByteDenominator;
    ByteDenominator<<= 8;
  }
  if (!Numeric->sign)
    Numerator= -Numerator;
  Denominator= (longlong)pow(10, Scale);
  Left= Numerator / Denominator;
  //_i64toa_s(Numerator, Buffer, 38, 10);
  if (Numeric->scale > 0)
  {
    char tmp[38];
    _snprintf(tmp,38, "%%%d.%df", Numeric->precision, Numeric->scale);
    _snprintf(Buffer, 38, tmp, Numerator / pow(10, Scale));
  }
  else
  {
    _snprintf(Buffer, 38, "%lld", Numerator);
    while (strlen(Buffer) < (size_t)(Numeric->precision - Numeric->scale))
      strcat(Buffer, "0");
  }
  
  
  if (Buffer[0] == '-')
    Buffer++;
 
 /* Truncation checks:
     1st ensure, that the digits before decimal point will fit */
  if ((p= strchr(Buffer, '.')))
  {
    if (p - Buffer - 1 > Numeric->precision)
    {
      *ErrorCode= MADB_ERR_22003;
      Buffer[Numeric->precision]= 0;
      goto end;
    }
    if (Numeric->scale > 0 && Left > 0 && (p - Buffer) + strlen(p) > Numeric->precision)
    {
      *ErrorCode= MADB_ERR_01S07;
      Buffer[Numeric->precision + 1]= 0;
      goto end;
    }
  }
  while (Numeric->scale < 0 && strlen(Buffer) < (size_t)(Numeric->precision - Numeric->scale))
    strcat(Buffer, "0");
    

  if (strlen(Buffer) > (size_t)(Numeric->precision + Scale) && Numeric->scale > 0)
    *ErrorCode= MADB_ERR_01S07;

end:
    /* check if last char is decimal point */
  if (strlen(Buffer) && Buffer[strlen(Buffer)-1] == '.')
    Buffer[strlen(Buffer)-1] = 0;
  if (!Numeric->sign)
    Buffer--;
  return strlen(Buffer);
}
/* }}} */

/* {{{ trim */
char *trim(char *Str)
{
  char *end;
  while (Str && iswspace(Str[0]))
    Str++;
  end= Str + strlen(Str) - 1;
  while (iswspace(*end))
    *end--= 0;
  return Str;
}
/* }}} */

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
  char *TableName= MADB_GetTableName(Stmt);
  char *CatalogName= MADB_GetCatalogName(Stmt);
  DYNAMIC_STRING DynStmt;

  MADB_CLEAR_ERROR(&Stmt->Error);
  memset(&DynStmt, 0, sizeof(DYNAMIC_STRING));

  if (Stmt->DaeStmt)
    Stmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
  Stmt->DaeStmt= NULL;
  if (!SQL_SUCCEEDED(SQLAllocStmt(Stmt->Connection, (SQLHANDLE *)&Stmt->DaeStmt)))
  {
    MADB_CopyError(&Stmt->Error, &Stmt->Connection->Error);
    goto end;
  }

  switch(Operation)
  {
  case SQL_ADD:
    if (init_dynamic_string(&DynStmt, "INSERT INTO ", 1024, 1024) ||
        MADB_DynStrAppendQuoted(&DynStmt, CatalogName) ||
        dynstr_append(&DynStmt, ".") ||
        MADB_DynStrAppendQuoted(&DynStmt, TableName)||
        MADB_DynStrUpdateSet(Stmt, &DynStmt))
    {
      dynstr_free(&DynStmt);
      return Stmt->Error.ReturnValue;
    }
    Stmt->DataExecutionType= MADB_DAE_ADD;
    break;
  case SQL_DELETE:
    if (init_dynamic_string(&DynStmt, "DELETE FROM ", 1024, 1024) ||
        MADB_DynStrAppendQuoted(&DynStmt, CatalogName) ||
        dynstr_append(&DynStmt, ".") ||
        MADB_DynStrAppendQuoted(&DynStmt, TableName) ||
        MADB_DynStrGetWhere(Stmt, &DynStmt, TableName, FALSE))
    {
      dynstr_free(&DynStmt);
      return Stmt->Error.ReturnValue;
    }
    Stmt->DataExecutionType= MADB_DAE_DELETE;
    break;
  case SQL_UPDATE:
    Stmt->Methods->RefreshRowPtrs(Stmt);
    if (init_dynamic_string(&DynStmt, "UPDATE ", 1024, 1024) ||
        MADB_DynStrAppendQuoted(&DynStmt, CatalogName) ||
        dynstr_append(&DynStmt, ".") ||
        MADB_DynStrAppendQuoted(&DynStmt, TableName)||
        MADB_DynStrUpdateSet(Stmt, &DynStmt)||
        MADB_DynStrGetWhere(Stmt, &DynStmt, TableName, FALSE))
    {
      dynstr_free(&DynStmt);
      return Stmt->Error.ReturnValue;
    }
    Stmt->DataExecutionType= MADB_DAE_UPDATE;
    break;
  }
  
  if (!SQL_SUCCEEDED(SQLPrepare(Stmt->DaeStmt, (SQLCHAR *)DynStmt.str, SQL_NTS)))
  {
    MADB_CopyError(&Stmt->Error, &Stmt->DaeStmt->Error);
    Stmt->Methods->StmtFree(Stmt->DaeStmt, SQL_DROP);
  }
   
end:
  dynstr_free(&DynStmt);
  return Stmt->Error.ReturnValue;

}
