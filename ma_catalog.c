/************************************************************************************
   Copyright (C) 2021 MariaDB Corporation AB

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

/*
 * Group of helper functions to add condition to the query based on SQL_ATTR_METADATA_ID attribute value
 * Pv - pattern value
 * Oa - ordinary argument
 * Id - identifier
 *
 * if bufferLen is -1, them buffer is assumed to be the dynamic string. Plain char buffer otherwise
 */
/* {{{ AddPvCondition */
static int AddPvCondition(MADB_Dbc *dbc, void* buffer, size_t bufferLen, char* value, SQLSMALLINT len)
{
  char escaped[2*NAME_LEN + 1];
  /* We should probably compare to SQL_NTS, and throw error if just < 0, but I think this way it will work just fine */
  if (len < 0)
  {
    len= (SQLSMALLINT)strlen(value);
  }

  len= (SQLSMALLINT)mysql_real_escape_string(dbc->mariadb, escaped, value, len);

  /* If DynString */
  if (bufferLen == (size_t)-1)
  {
    if (MADB_DYNAPPENDCONST((MADB_DynString*)buffer, " LIKE '") ||
      MADB_DynstrAppendMem((MADB_DynString*)buffer, escaped, len) ||
      MADB_DynstrAppendMem((MADB_DynString*)buffer, "' ", 2))
    {
      return 1;
    }
    return 0;
  }

  return _snprintf((char*)buffer, bufferLen, " LIKE '%.*s' ", len, escaped);
}
/* }}} */

/* {{{ Read_lower_case_table_names */
static char Read_lower_case_table_names(MADB_Dbc *Dbc)
{
  if (Dbc->lcTableNamesMode2 < 0)
  {
    MYSQL_RES *res;
    MYSQL_ROW row;

    if (mysql_real_query(Dbc->mariadb, "SELECT @@lower_case_table_names", sizeof("SELECT @@lower_case_table_names") - 1))
    {
      Dbc->lcTableNamesMode2= '\0';
      return Dbc->lcTableNamesMode2;
    }
    res= mysql_store_result(Dbc->mariadb);
    row= mysql_fetch_row(res);
    if (row[0][0] == '2')
    {
      Dbc->lcTableNamesMode2= '\1';
    }
    else
    {
      Dbc->lcTableNamesMode2= '\0';
    }
    mysql_free_result(res);
  }
  return Dbc->lcTableNamesMode2;
}
/* }}} */

/* {{{ AddOaCondition */
static int AddOaCondition(MADB_Dbc *Dbc, void* buffer, size_t bufferLen, char* value, SQLSMALLINT len)
{
  char escaped[2 * NAME_LEN + 1];
  const char *cs= "=BINARY'", *ci= "='", *compare= cs;
  const size_t cs_len= sizeof("=BINARY'") - 1, ci_len= sizeof("='") - 1;
  size_t compare_len= cs_len;
  if (len < 0)
  {
    len= (SQLSMALLINT)strlen(value);
  }
    
  len= (SQLSMALLINT)mysql_real_escape_string(Dbc->mariadb, escaped, value, len);

  if (Read_lower_case_table_names(Dbc))
  {
    compare= ci;
    compare_len= ci_len;
  }

  /* If DynString */
  if (bufferLen == (size_t)-1)
  {
    if (MADB_DynstrAppendMem((MADB_DynString*)buffer, compare, compare_len) ||
      MADB_DynstrAppendMem((MADB_DynString*)buffer, escaped, len) ||
      MADB_DynstrAppendMem((MADB_DynString*)buffer, "' ", 2))
    {
      return 1;
    }
    return 0;
  }

  return _snprintf((char*)buffer, bufferLen, "%s%.*s' ", compare, len, escaped);
}
/* }}} */

/* {{{ AddIdCondition */
static int AddIdCondition(void* buffer, size_t bufferLen, char* value, SQLSMALLINT len)
{
  if (len < 0)
  {
    len= (SQLSMALLINT)strlen(value);
  }

  /* If DynString */
  if (bufferLen == (size_t)-1)
  {
    MADB_DynstrAppendMem((MADB_DynString*)buffer, "=`", 2);
    MADB_DynstrAppendMem((MADB_DynString*)buffer, value, len);
    MADB_DynstrAppendMem((MADB_DynString*)buffer, "` ", 2);

    return 0; /* Doesn't really matter in case of dynamic string */
  }
  return _snprintf((char*)buffer, bufferLen, "=`%.*s` ", (int)len, value);
}
/* }}} */

/* {{{ AddPvOrIdCondition */
static int AddPvOrIdCondition(MADB_Stmt* Stmt, void *buffer, size_t bufferLen, char *value, SQLSMALLINT len)
{
  SQLULEN MetadataId;
  Stmt->Methods->GetAttr(Stmt, SQL_ATTR_METADATA_ID, &MetadataId, 0, (SQLINTEGER*)NULL);

  if (MetadataId == SQL_TRUE)
  {
    return AddIdCondition(buffer, bufferLen, value, len);
  }
  else
  {
    return AddPvCondition(Stmt->Connection, buffer, bufferLen, value, len);
  }
}
/* }}} */

/* {{{ AddOaOrIdCondition */
static int AddOaOrIdCondition(MADB_Stmt* Stmt, void* buffer, size_t bufferLen, char* value, SQLSMALLINT len)
{
  SQLULEN MetadataId;
  Stmt->Methods->GetAttr(Stmt, SQL_ATTR_METADATA_ID, &MetadataId, 0, (SQLINTEGER*)NULL);

  if (MetadataId == SQL_TRUE)
  {
    return AddIdCondition(buffer, bufferLen, value, len);
  }
  else
  {
    return AddOaCondition(Stmt->Connection, buffer, bufferLen, value, len);
  }
}
/* }}} */

#define SCHEMA_PARAMETER_ERRORS_ALLOWED(STMT) ((STMT)->Connection->Dsn->NeglectSchemaParam == 0)

/* {{{ MADB_StmtColumnPrivileges */
SQLRETURN MADB_StmtColumnPrivileges(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                                    char *SchemaName, SQLSMALLINT NameLength2, char *TableName,
                                    SQLSMALLINT NameLength3, char *ColumnName, SQLSMALLINT NameLength4)
{
  char StmtStr[2048] /* This should cover 3 names * NAME_LEN * 2 escaped + query(~300bytes) */;
  char *p;

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* TableName cannot be NULL, but nothing said it can't be empty string */
  if (TableName == NULL)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
  }
  ADJUST_LENGTH(SchemaName, NameLength2);
  if (SchemaName != NULL && *SchemaName != '\0' && SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
  }
  p= StmtStr;
  p+= _snprintf(StmtStr, sizeof(StmtStr), "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL as TABLE_SCHEM, TABLE_NAME,"
                                 "COLUMN_NAME, NULL AS GRANTOR, GRANTEE, PRIVILEGE_TYPE AS PRIVILEGE,"
                                 "IS_GRANTABLE FROM INFORMATION_SCHEMA.COLUMN_PRIVILEGES WHERE ");

  /* Empty schema name means tables w/out schema. We could get here only if it is empty string */
  if (SchemaName != NULL && *SchemaName == '\0')
  {
    p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "0");
  }
  else
  {
    p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "TABLE_SCHEMA");
    if (CatalogName)
    {
      p+= AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), CatalogName, NameLength1);
    }
    else
    {
      p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "=DATABASE() ");
    }
    p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND TABLE_NAME");
    p+= AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), TableName, NameLength3);

    if (ColumnName)
    {
      p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND COLUMN_NAME");
      p+= AddPvOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), ColumnName, NameLength4);
    }

    p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "ORDER BY TABLE_SCHEM, TABLE_NAME, COLUMN_NAME, PRIVILEGE");
  }
  return Stmt->Methods->ExecDirect(Stmt, StmtStr, (SQLINTEGER)strlen(StmtStr));
}
/* }}} */

/* {{{ MADB_StmtTablePrivileges */
SQLRETURN MADB_StmtTablePrivileges(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                                    char *SchemaName, SQLSMALLINT NameLength2,
                                    char *TableName, SQLSMALLINT NameLength3)
{
  char StmtStr[2048], /* This should cover 2 names * NAME_LEN * 2 escaped + query(~300bytes) */
       *p;

  MADB_CLEAR_ERROR(&Stmt->Error);
  ADJUST_LENGTH(SchemaName, NameLength2);
  if (SchemaName != NULL && *SchemaName != '\0' && *SchemaName != '%' && NameLength2 > 1 && SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
  }

  p= StmtStr;
  p += _snprintf(StmtStr, sizeof(StmtStr), "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, TABLE_NAME, "
                                  "NULL AS GRANTOR, GRANTEE, PRIVILEGE_TYPE AS PRIVILEGE, IS_GRANTABLE "
                                  "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES WHERE ");

  /* Empty schema name means tables w/out schema. We could get here only if it is empty string, otherwise the error would have been already thrown */
  if (SchemaName != NULL && *SchemaName == '\0')
  {
    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "0");
  }
  else
  {
    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "TABLE_SCHEMA");
    if (CatalogName)
    {
      p += AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), CatalogName, NameLength1);
    }
    else
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "=DATABASE()");
    }
    if (TableName)
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), " AND TABLE_NAME");
      p += AddPvOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), TableName, NameLength3);
    }
    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "ORDER BY TABLE_SCHEM, TABLE_NAME, PRIVILEGE");
  }
  return Stmt->Methods->ExecDirect(Stmt, StmtStr, (SQLINTEGER)strlen(StmtStr));
}
/* }}} */

/* {{{ MADB_StmtTables */
SQLRETURN MADB_StmtTables(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT CatalogNameLength,
                          char *SchemaName, SQLSMALLINT SchemaNameLength, char *TableName,
                          SQLSMALLINT TableNameLength, char *TableType, SQLSMALLINT TableTypeLength)
{
  MADB_DynString StmtStr;
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

  /* TODO: Here we need compare character length in fact. Comparing with both either NAME_CHAR_LEN or NAME_LEN don't make quite sense, but
     if > NAME_LEN, then it is for sure too big */
  if (CatalogNameLength > NAME_LEN || TableNameLength > NAME_LEN)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY090, "Table and catalog names are limited to 64 chars", 0);
  }
  /* Schemas are not supported. Thus error except special cases of SQLTables use*/

  if (SchemaName != NULL && *SchemaName != '\0' && *SchemaName != '%' && SchemaNameLength > 1
    && (strcmp(SchemaName, SQL_ALL_SCHEMAS) || CatalogName == NULL || CatalogNameLength != 0 && TableName == NULL || TableNameLength != 0)
    && SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
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
  /* Since we treat our databases as catalogs, the only acceptable value for schema is NULL or "%",
     if that is not the special case of call for schemas list or tables w/out schema(empty string in schema name) - empty resultsets then. */
  else if (SchemaName &&
     (!strcmp(SchemaName,SQL_ALL_SCHEMAS) && CatalogName && CatalogNameLength == 0 && TableName && TableNameLength == 0 || *SchemaName == '\0'))
  {
    MADB_InitDynamicString(&StmtStr, "SELECT NULL AS TABLE_CAT, NULL AS TABLE_SCHEM, "
      "NULL AS TABLE_NAME, NULL AS TABLE_TYPE, NULL AS REMARKS "
      "FROM DUAL WHERE 1=0", 8192, 512);
  }
  else
  {
    MADB_InitDynamicString(&StmtStr, "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, TABLE_NAME, "
                                  "if(TABLE_TYPE='BASE TABLE' OR TABLE_TYPE='SYSTEM VERSIONED','TABLE',TABLE_TYPE) AS TABLE_TYPE ,"
                                  "TABLE_COMMENT AS REMARKS FROM INFORMATION_SCHEMA.TABLES WHERE 1=1 ",
                                  8192, 512);


    if (CatalogName != NULL)
    {
      MADB_DYNAPPENDCONST(&StmtStr, " AND TABLE_SCHEMA");
      AddPvOrIdCondition(Stmt, (void*)&StmtStr, (size_t)-1, CatalogName, CatalogNameLength);
    }
    else if (Stmt->Connection->Environment->AppType == ATypeMSAccess || Stmt->Connection->Dsn->NullSchemaMeansCurrent != '\0')
    {
      MADB_DYNAPPENDCONST(&StmtStr, " AND TABLE_SCHEMA=DATABASE()");
    }

    if (TableName && TableNameLength)
    {
      MADB_DYNAPPENDCONST(&StmtStr, " AND TABLE_NAME");
      AddPvOrIdCondition(Stmt, (void*)&StmtStr, (size_t)-1, TableName, TableNameLength);
    }
    if (TableType && TableTypeLength && strcmp(TableType, SQL_ALL_TABLE_TYPES) != 0)
    {
      unsigned int i;
      char *myTypes[3]= {"TABLE", "VIEW", "SYNONYM"};
      MADB_DYNAPPENDCONST(&StmtStr, " AND TABLE_TYPE IN (''");
      for (i= 0; i < 3; i++)
      {
        if (strstr(TableType, myTypes[i]))
        {
          if (strstr(myTypes[i], "TABLE"))
            MADB_DYNAPPENDCONST(&StmtStr, ", 'BASE TABLE', 'SYSTEM VERSIONED'");
          else
          {
            MADB_DYNAPPENDCONST(&StmtStr, ", '");
            MADB_DynstrAppend(&StmtStr, myTypes[i]);
            MADB_DYNAPPENDCONST(&StmtStr, "'");
          }
        }
      }
      MADB_DYNAPPENDCONST(&StmtStr, ") ");
    }
    MADB_DYNAPPENDCONST(&StmtStr, " ORDER BY TABLE_SCHEMA, TABLE_NAME, TABLE_TYPE");
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
  char StmtStr[2048];
  char *p= StmtStr;
  SQLRETURN ret;

  MADB_CLEAR_ERROR(&Stmt->Error);
  /* TableName is mandatory */
  if (TableName == NULL)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }
  ADJUST_LENGTH(SchemaName, NameLength2);
  if (SchemaName != NULL && *SchemaName != '\0' && SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
  }

  p+= _snprintf(StmtStr, sizeof(StmtStr), "SELECT TABLE_SCHEMA AS TABLE_CAT,NULL AS TABLE_SCHEM,TABLE_NAME, "
                             "IF(NON_UNIQUE=0 AND (SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS s2"
                             " WHERE s2.INDEX_NAME=s1.INDEX_NAME AND s2.TABLE_SCHEMA=s1.TABLE_SCHEMA AND NULLABLE='YES') > 0,"
                             "1,NON_UNIQUE) AS NON_UNIQUE,"
                             "NULL AS INDEX_QUALIFIER,INDEX_NAME,%d AS TYPE,"
                             "SEQ_IN_INDEX AS ORDINAL_POSITION,COLUMN_NAME,COLLATION AS ASC_OR_DESC, "
                             "CARDINALITY,NULL AS PAGES,NULL AS FILTER_CONDITION "
                             "FROM INFORMATION_SCHEMA.STATISTICS s1 ",
                             SQL_INDEX_OTHER);
  /* Empty schema name means tables w/out schema. We could get here only if it is empty string, otherwise the error would have been already thrown */
  if (SchemaName != NULL)
  {
    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "WHERE 0");
  }
  else
  {
    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "WHERE TABLE_SCHEMA");
    /* Same comments as for SQLPrimaryKeys including TODOs */
    if (CatalogName)
    {
      p += AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), CatalogName, NameLength1);
    }
    else
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "=DATABASE() ");
    }

    if (TableName)
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND TABLE_NAME");
      p += AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), TableName, NameLength3);
    }

    if (Unique == SQL_INDEX_UNIQUE)
      p += _snprintf(p, 1023 - strlen(StmtStr), "AND NON_UNIQUE=0 ");

    /* To make PRIMARY to appear before all othe unique indexes we could add , INDEX_NAME!='PRIMARY' after NON_UNIQUE. But
      that would break specs, how the rs should be ordered */
    _snprintf(p, 1023 - strlen(StmtStr), "ORDER BY NON_UNIQUE, INDEX_NAME, ORDINAL_POSITION");
  }

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
  char *ColumnsPart= NULL;
  unsigned int OctetsPerChar= Stmt->Connection->Charset.cs_info->char_maxlen > 0 && Stmt->Connection->Charset.cs_info->char_maxlen < 10 ? Stmt->Connection->Charset.cs_info->char_maxlen : 1;

  MDBUG_C_ENTER(Stmt->Connection, "StmtColumns");
  ADJUST_LENGTH(SchemaName, NameLength2);
  if (SchemaName != NULL && *SchemaName != '\0' && *SchemaName != '%' && NameLength2 > 1 && SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
  }

  ColumnsPart = MADB_CALLOC(Length);
  if (ColumnsPart == NULL)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  _snprintf(ColumnsPart, Length, MADB_CATALOG_COLUMNSp3, OctetsPerChar);

  MADB_InitDynamicString(&StmtStr, "", 8192, 1024);
 
  MADB_CLEAR_ERROR(&Stmt->Error);
  if (MADB_DYNAPPENDCONST(&StmtStr, MADB_CATALOG_COLUMNSp1))
    goto dynerror;
  if (MADB_DynstrAppend(&StmtStr, MADB_SQL_DATATYPE(Stmt)))
    goto dynerror;
  if (MADB_DynstrAppend(&StmtStr, ColumnsPart))
    goto dynerror;
  if (MADB_DynstrAppend(&StmtStr, MADB_DEFAULT_COLUMN(Stmt->Connection)))
    goto dynerror;

  if (MADB_DYNAPPENDCONST(&StmtStr, MADB_CATALOG_COLUMNSp4))
    goto dynerror;

  ADJUST_LENGTH(CatalogName, NameLength1);
  ADJUST_LENGTH(TableName, NameLength3);
  ADJUST_LENGTH(ColumnName, NameLength4);

  /* Empty schema name means tables w/out schema. We could get here only if it is empty string, otherwise the error would have been already thrown */
  if (SchemaName != NULL && *SchemaName == '\0')
  {
    if(MADB_DynstrAppend(&StmtStr, "0"))
      goto dynerror;
  }
  else
  {
    if (MADB_DYNAPPENDCONST(&StmtStr, "TABLE_SCHEMA"))
      goto dynerror;

    if (CatalogName)
    {
      if (AddOaOrIdCondition(Stmt, (void*)&StmtStr, (size_t)-1, CatalogName, NameLength1))
      {
        goto dynerror;
      }
    }
    else
      if (MADB_DYNAPPENDCONST(&StmtStr, "=DATABASE()"))
        goto dynerror;

    if (TableName && NameLength3)
    {
      if (MADB_DynstrAppend(&StmtStr, "AND TABLE_NAME") ||
        AddPvOrIdCondition(Stmt, (void*)&StmtStr, (size_t)-1, TableName, NameLength3))
      {
        goto dynerror;
      }
    }

    if (ColumnName && NameLength4)
    {
      if (MADB_DynstrAppend(&StmtStr, "AND COLUMN_NAME") ||
        AddPvOrIdCondition(Stmt, (void*)&StmtStr, (size_t)-1, ColumnName, NameLength4))
      {
        goto dynerror;
      }
    }

    if (MADB_DYNAPPENDCONST(&StmtStr, " ORDER BY TABLE_SCHEMA, TABLE_NAME, ORDINAL_POSITION"))
      goto dynerror;

    MDBUG_C_DUMP(Stmt->Connection, StmtStr.str, s);
  }
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
  MADB_DynstrFree(&StmtStr);
  return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
}
/* }}} */

/* {{{ MADB_StmtProcedureColumns */
SQLRETURN MADB_StmtProcedureColumns(MADB_Stmt *Stmt, char *CatalogName, SQLSMALLINT NameLength1,
                                char *SchemaName, SQLSMALLINT NameLength2, char *ProcName,
                                SQLSMALLINT NameLength3, char *ColumnName, SQLSMALLINT NameLength4)
{
  char *StmtStr,
       *p;
  size_t Length= strlen(MADB_PROCEDURE_COLUMNS(Stmt)) + 2048;
  SQLRETURN ret;
  unsigned int OctetsPerChar= Stmt->Connection->Charset.cs_info->char_maxlen > 0 ? Stmt->Connection->Charset.cs_info->char_maxlen: 1;

  MADB_CLEAR_ERROR(&Stmt->Error);

  ADJUST_LENGTH(SchemaName, NameLength2);
  if (SchemaName != NULL && *SchemaName != '\0' && *SchemaName != '%' && NameLength2 > 1 && SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
  }
  if (!(StmtStr= MADB_CALLOC(Length)))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }

  p= StmtStr;

  p+= _snprintf(p, Length, MADB_PROCEDURE_COLUMNS(Stmt), OctetsPerChar);
  /* Empty schema name means tables w/out schema. We could get here only if it is empty string, otherwise the error would have been already thrown */
  if (SchemaName != NULL && *SchemaName == '\0')
  {
    p += _snprintf(p, Length - strlen(StmtStr), "WHERE 0");
  }
  else
  {
    p += _snprintf(p, Length - strlen(StmtStr), "WHERE SPECIFIC_SCHEMA");
    if (CatalogName)
      p += AddOaOrIdCondition(Stmt, p, Length - strlen(StmtStr), CatalogName, NameLength1);
    else
      p += _snprintf(p, Length - strlen(StmtStr), "=DATABASE() ");
    if (ProcName && ProcName[0])
    {
      p += _snprintf(p, Length - strlen(StmtStr), "AND SPECIFIC_NAME");
      p += AddPvOrIdCondition(Stmt, p, Length - strlen(StmtStr), ProcName, NameLength3);
    }
    if (ColumnName)
    {
      if (ColumnName[0])
      {
        p += _snprintf(p, Length - strlen(StmtStr), "AND PARAMETER_NAME");
        p += AddPvOrIdCondition(Stmt, p, Length - strlen(StmtStr), ColumnName, NameLength4);
      }
      else
      {
        p += _snprintf(p, Length - strlen(StmtStr), "AND PARAMETER_NAME IS NULL ");
      }
    }

    p += _snprintf(p, Length - strlen(StmtStr), " ORDER BY SPECIFIC_SCHEMA, SPECIFIC_NAME, ORDINAL_POSITION");
  }
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
  if (TableName == NULL)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }
  ADJUST_LENGTH(SchemaName, NameLength2);
  if (SchemaName != NULL && *SchemaName != '\0' && SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
  }

  p= StmtStr;
  p+= _snprintf(p, sizeof(StmtStr), "SELECT TABLE_SCHEMA AS TABLE_CAT, NULL AS TABLE_SCHEM, "
                           "TABLE_NAME, COLUMN_NAME, ORDINAL_POSITION KEY_SEQ, "
                           "'PRIMARY' PK_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE "
                           "COLUMN_KEY = 'pri' AND ");
  /* Empty schema name means tables w/out schema. We could get here only if it is empty string, otherwise the error would have been already thrown */
  if (SchemaName != NULL)
  {
    _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "0");
  }
  else
  {
    p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "TABLE_SCHEMA");
    /* Empty catalog name means table without catalog(schema). MariaDB/MySQL do not have such. Thus should be empty resultset.
       TABLE_SCHEMA='' will do the job. TODO: that can be done without sending query to the server.
       Catalog(schema) cannot be a search pattern. Thus = and not LIKE here */
    if (CatalogName != NULL)
    {
      p += AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), CatalogName, NameLength1);
    }
    else
    {
      /* If Catalog is NULL we return for current DB. If no schema(aka catalog here) is selected, then that means
         table without schema. Since we don't have such tables, that means empty resultset.
         TODO: We should be aboe to construct resultset - empty in this case, avoiding sending query to the server */
      p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "=DATABASE() ");
    }
    p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND TABLE_NAME");
    p+= AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), TableName, NameLength3);
    p+= _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), " ORDER BY TABLE_SCHEMA, TABLE_NAME, ORDINAL_POSITION");
  }
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
  if (TableName == NULL)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "Tablename is required", 0);
    return Stmt->Error.ReturnValue;
  }
  ADJUST_LENGTH(SchemaName, NameLength2);
  if (SchemaName != NULL && *SchemaName != '\0' && SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
  }

  p+= _snprintf(p, sizeof(StmtStr), "SELECT NULL AS SCOPE, COLUMN_NAME, %s,"
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
                           "FROM INFORMATION_SCHEMA.COLUMNS c WHERE 1 ", MADB_SQL_DATATYPE(Stmt));

  /* Empty schema name means tables w/out schema. We could get here only if it is empty string, otherwise the error would have been already thrown */
  if (SchemaName != NULL)
  {
    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND 0");
  }
  else
  {
    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND TABLE_SCHEMA");
    if (CatalogName)
    {
      p += AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), CatalogName, NameLength1);
    }
    else
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "=DATABASE() ");
    }
    if (TableName && TableName[0])
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND TABLE_NAME");
      p += AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), TableName, NameLength3);
    }

    if (Nullable == SQL_NO_NULLS)
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND IS_NULLABLE <> 'YES' ");
    }

    if (IdentifierType == SQL_BEST_ROWID)
    {
      /* If some of columns of a unique index can be NULL, this unique key cannot be SQL_BEST_ROWID since it can't
       "allows any row in the specified table to be uniquely identified" */
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr),
        "AND (COLUMN_KEY='PRI' OR COLUMN_KEY= 'UNI' AND IS_NULLABLE<>'YES' AND "
           "(SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS s1 LEFT JOIN INFORMATION_SCHEMA.STATISTICS s2 USING(INDEX_NAME)"
           " WHERE s1.TABLE_SCHEMA=c.TABLE_SCHEMA AND s1.TABLE_NAME=c.TABLE_NAME AND s1.COLUMN_NAME=c.COLUMN_NAME "
               "AND s2.NULLABLE='YES') > 0) ");
    }
    else if (IdentifierType == SQL_ROWVER)
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND DATA_TYPE='timestamp' AND EXTRA LIKE '%%CURRENT_TIMESTAMP%%' ");
    }
    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "ORDER BY TABLE_SCHEMA, TABLE_NAME, COLUMN_KEY");
  }
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
  ADJUST_LENGTH(SchemaName, NameLength2);
  if (SchemaName != NULL && *SchemaName != '\0' && *SchemaName != '%' && NameLength2 > 1 && SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
  }
  p= StmtStr;

  p+= _snprintf(p, sizeof(StmtStr), "SELECT ROUTINE_SCHEMA AS PROCEDURE_CAT, NULL AS PROCEDURE_SCHEM, "
                           "SPECIFIC_NAME PROCEDURE_NAME, NULL NUM_INPUT_PARAMS, "
                           "NULL NUM_OUTPUT_PARAMS, NULL NUM_RESULT_SETS, "
                           "ROUTINE_COMMENT REMARKS, "
                           "CASE ROUTINE_TYPE "
                           "  WHEN 'FUNCTION' THEN " XSTR(SQL_PT_FUNCTION)
                           "  WHEN 'PROCEDURE' THEN " XSTR(SQL_PT_PROCEDURE)
                           "  ELSE " XSTR(SQL_PT_UNKNOWN) " "
                           "END PROCEDURE_TYPE "
                           "FROM INFORMATION_SCHEMA.ROUTINES ");
  /* Empty schema name means tables w/out schema. We could get here only if it is empty string, otherwise the error would have been already thrown */
  if (SchemaName != NULL && *SchemaName == '\0')
  {
    p += _snprintf(p, sizeof(StmtStr)- strlen(StmtStr), "WHERE 0");
  }
  else
  {
    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "WHERE ROUTINE_SCHEMA");
    /* Catalog is ordinary argument, but schema is pattern value. Since we treat is catalog as a schema, using more permissive PV here
       On other hand we do not do this everywhere. Need to be consistent */
    if (CatalogName != NULL)
    {
      p += AddOaOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), CatalogName, NameLength1);
    }
    else
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "=DATABASE() ");
    }
    if (ProcName != NULL)
    {
      p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), "AND SPECIFIC_NAME");
      p += AddPvOrIdCondition(Stmt, p, sizeof(StmtStr) - strlen(StmtStr), ProcName, NameLength3);
    }

    p += _snprintf(p, sizeof(StmtStr) - strlen(StmtStr), " ORDER BY ROUTINE_SCHEMA, SPECIFIC_NAME");
  }
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

  MADB_CLEAR_ERROR(&Stmt->Error);

  /* PKTableName and FKTableName are mandatory. The requirement is only about NULL names */
  if (PKTableName == NULL && FKTableName == NULL)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY009, "PKTableName or FKTableName are required", 0);
    return Stmt->Error.ReturnValue;
  }
  ADJUST_LENGTH(PKSchemaName, NameLength2);
  ADJUST_LENGTH(FKSchemaName, NameLength5);
  if (((PKSchemaName != NULL && *PKSchemaName != '\0') || (FKSchemaName != NULL && *FKSchemaName != '\0')) &&
    SCHEMA_PARAMETER_ERRORS_ALLOWED(Stmt))
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HYC00, "Schemas are not supported. Use CatalogName parameter instead", 0);
  }

  ADJUST_LENGTH(PKCatalogName, NameLength1);
  ADJUST_LENGTH(PKTableName, NameLength3);
  ADJUST_LENGTH(FKCatalogName, NameLength4);

  ADJUST_LENGTH(FKTableName, NameLength6);

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

  /* Empty schema name means tables w/out schema. We could get here only if it is empty string, otherwise error would have been already thrown */
  if (PKSchemaName != NULL || FKSchemaName != NULL)
  {
    MADB_DYNAPPENDCONST(&StmtStr, " AND 0");
  }
  else
  {
    if (PKTableName != NULL)
    {
      MADB_DYNAPPENDCONST(&StmtStr, " AND A.REFERENCED_TABLE_SCHEMA");

      if (PKCatalogName)
      {
        AddOaOrIdCondition(Stmt, &StmtStr, (size_t)-1, PKCatalogName, NameLength1);
      }
      else
      {
        MADB_DYNAPPENDCONST(&StmtStr, "=DATABASE()");
      }
      MADB_DYNAPPENDCONST(&StmtStr, " AND A.REFERENCED_TABLE_NAME");

      AddOaOrIdCondition(Stmt, &StmtStr, (size_t)-1, PKTableName, NameLength3);
    }

    if (FKTableName != NULL)
    {
      MADB_DYNAPPENDCONST(&StmtStr, " AND A.TABLE_SCHEMA");

      if (FKCatalogName != NULL)
      {
        AddOaOrIdCondition(Stmt, &StmtStr, (size_t)-1, FKCatalogName, NameLength4);
      }
      else
      {
        MADB_DYNAPPENDCONST(&StmtStr, "=DATABASE() ");
      }
      MADB_DYNAPPENDCONST(&StmtStr, " AND A.TABLE_NAME");

      AddOaOrIdCondition(Stmt, &StmtStr, (size_t)-1, FKTableName, NameLength6);
    }
    MADB_DYNAPPENDCONST(&StmtStr, "ORDER BY FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, KEY_SEQ, PKTABLE_NAME");
  }
  ret= Stmt->Methods->ExecDirect(Stmt, StmtStr.str, SQL_NTS);

  MADB_DynstrFree(&StmtStr);

  return ret;
}
/* }}} */

