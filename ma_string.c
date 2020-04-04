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

extern MARIADB_CHARSET_INFO*  DmUnicodeCs;

char *MADB_GetTableName(MADB_Stmt *Stmt)
{
  char *TableName= NULL;
 unsigned  int i= 0;
  if (Stmt->TableName && Stmt->TableName[0])
    return Stmt->TableName;
  if (!mysql_stmt_field_count(Stmt->stmt))
    return NULL;

  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    if (Stmt->stmt->fields[i].org_table)
    {
      if (!TableName)
        TableName= Stmt->stmt->fields[i].org_table;
      if (strcmp(TableName, Stmt->stmt->fields[i].org_table))
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY000, "Couldn't identify unique table name", 0);
        return NULL;
      }
    }
  }
  if (TableName)
    Stmt->TableName= _strdup(TableName);
  return TableName;
}

char *MADB_GetCatalogName(MADB_Stmt *Stmt)
{
  char *CatalogName= NULL;
  unsigned int i= 0;
  if (Stmt->CatalogName && Stmt->CatalogName[0])
    return Stmt->CatalogName;
  if (!mysql_stmt_field_count(Stmt->stmt))
    return NULL;

  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    if (Stmt->stmt->fields[i].org_table)
    {
      if (!CatalogName)
        CatalogName= Stmt->stmt->fields[i].db;
      if (strcmp(CatalogName, Stmt->stmt->fields[i].db))
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY000, "Couldn't identify unique catalog name", 0);
        return NULL;
      }
    }
  }
  if (CatalogName)
    Stmt->CatalogName= _strdup(CatalogName);
  return CatalogName;
}

my_bool MADB_DynStrAppendQuoted(MADB_DynString *DynString, char *String)
{
  if (MADB_DynstrAppend(DynString, "`") ||
      MADB_DynstrAppend(DynString, String) ||
      MADB_DynstrAppend(DynString, "`"))
    return TRUE;
  return FALSE;
}

my_bool MADB_DynStrUpdateSet(MADB_Stmt *Stmt, MADB_DynString *DynString)
{
  int             i, IgnoredColumns= 0;
  MADB_DescRecord *Record;

  if (MADB_DynstrAppend(DynString, " SET "))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    return TRUE;
  }
  // ???? memcpy(&Stmt->Da->Apd->Header, &Stmt->Ard->Header, sizeof(MADB_Header));
  for (i=0; i < MADB_STMT_COLUMN_COUNT(Stmt); i++)
  {
    SQLLEN *IndicatorPtr= NULL;
    Record= MADB_DescGetInternalRecord(Stmt->Ard, i, MADB_DESC_READ);
    if (Record->IndicatorPtr)
      IndicatorPtr= (SQLLEN *)GetBindOffset(Stmt->Ard, Record, Record->IndicatorPtr, Stmt->DaeRowNumber > 1 ? Stmt->DaeRowNumber-1 : 0,
                                            sizeof(SQLLEN)/*Record->OctetLength*/);
    if ((IndicatorPtr && *IndicatorPtr == SQL_COLUMN_IGNORE) || !Record->inUse)
    {
      IgnoredColumns++;
      continue;
    }
    
    if ((i - IgnoredColumns) && MADB_DynstrAppend(DynString, ","))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      return TRUE;
    }
    if (MADB_DynStrAppendQuoted(DynString, Stmt->stmt->fields[i].org_name))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      return TRUE;
    }
    if (MADB_DynstrAppend(DynString, "=?"))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      return TRUE;
    }
  }
  if (IgnoredColumns == mysql_stmt_field_count(Stmt->stmt))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_21S02, NULL, 0);
    return TRUE;
  }
  return FALSE;
}

my_bool MADB_DynStrInsertSet(MADB_Stmt *Stmt, MADB_DynString *DynString)
{
  MADB_DynString  ColVals;
  int             i, NeedComma= 0;
  MADB_DescRecord *Record;

  MADB_InitDynamicString(&ColVals, "VALUES (", 32, 32);
  if (MADB_DynstrAppend(DynString, " ("))
  {
    goto dynerror;
    
    return TRUE;
  }

  /* We use only columns, that have been bound, and are not IGNORED */
  for (i= 0; i < MADB_STMT_COLUMN_COUNT(Stmt); i++)
  {
    Record= MADB_DescGetInternalRecord(Stmt->Ard, i, MADB_DESC_READ);
    if (!Record->inUse || MADB_ColumnIgnoredInAllRows(Stmt->Ard, Record) == TRUE)
    {
      continue;
    }

    if ((NeedComma) && 
        (MADB_DynstrAppend(DynString, ",") || MADB_DynstrAppend(&ColVals, ",")))
      goto dynerror;

    if (MADB_DynStrAppendQuoted(DynString, Stmt->stmt->fields[i].org_name) ||
        MADB_DynstrAppend(&ColVals, "?"))
       goto dynerror;

    NeedComma= 1;
  }
  if (MADB_DynstrAppend(DynString, ") " ) ||
      MADB_DynstrAppend(&ColVals, ")") ||
      MADB_DynstrAppend(DynString, ColVals.str))
    goto dynerror;
  MADB_DynstrFree(&ColVals);
  return FALSE;
dynerror:
  MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  MADB_DynstrFree(&ColVals);
  return TRUE;
}

my_bool MADB_DynStrGetColumns(MADB_Stmt *Stmt, MADB_DynString *DynString)
{
  unsigned int i;
  if (MADB_DynstrAppend(DynString, " ("))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    return TRUE;
  }
  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    if (i && MADB_DynstrAppend(DynString, ", "))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      return TRUE;
    }
    if (MADB_DynStrAppendQuoted(DynString, Stmt->stmt->fields[i].org_name))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      return TRUE;
    }
  }
  if (MADB_DynstrAppend(DynString, " )"))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    return TRUE;
  }
  return FALSE;
}

my_bool MADB_DynStrGetWhere(MADB_Stmt *Stmt, MADB_DynString *DynString, char *TableName, my_bool ParameterMarkers)
{
  int UniqueCount=0, PrimaryCount= 0;
  int i, Flag= 0;
  char *Column= NULL, *Escaped= NULL;
  SQLLEN StrLength;
  unsigned long EscapedLength;

  for (i= 0; i < MADB_STMT_COLUMN_COUNT(Stmt); i++)
  {
    MYSQL_FIELD *field= mysql_fetch_field_direct(FetchMetadata(Stmt), i);
    if (field->flags & PRI_KEY_FLAG)
      PrimaryCount++;
    if (field->flags & UNIQUE_KEY_FLAG)
      UniqueCount++;
  }
  /* We need to use all columns, otherwise it will be difficult to map fields for Positioned Update */
  if (PrimaryCount && PrimaryCount != MADB_KeyTypeCount(Stmt->Connection, TableName, PRI_KEY_FLAG))
    PrimaryCount= 0;
  if (UniqueCount && UniqueCount != MADB_KeyTypeCount(Stmt->Connection, TableName, UNIQUE_KEY_FLAG))
    UniqueCount= 0;
  
  /* if no primary or unique key is in the cursor, the cursor must contain all
     columns from table in TableName */
  if (!PrimaryCount && !UniqueCount)
  {
    char      StmtStr[256];
    MADB_Stmt *CountStmt;
    int       FieldCount= 0;

    MA_SQLAllocHandle(SQL_HANDLE_STMT, Stmt->Connection, (SQLHANDLE*)&CountStmt);
    _snprintf(StmtStr, 256, "SELECT * FROM `%s` LIMIT 0", TableName);
    CountStmt->Methods->ExecDirect(CountStmt, (char *)StmtStr, SQL_NTS);
    FieldCount= mysql_stmt_field_count(((MADB_Stmt *)CountStmt)->stmt);
    CountStmt->Methods->StmtFree(CountStmt, SQL_DROP);

    if (FieldCount != MADB_STMT_COLUMN_COUNT(Stmt))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_S1000, "Can't build index for update/delete", 0);
      return TRUE;
    }
  }
  if (MADB_DynstrAppend(DynString, " WHERE 1"))
    goto memerror;
  for (i= 0; i < MADB_STMT_COLUMN_COUNT(Stmt); i++)
  {
    MYSQL_FIELD *field= mysql_fetch_field_direct(Stmt->metadata, i);
    if (field->flags & Flag || !Flag)
    {
      if (MADB_DynstrAppend(DynString, " AND ") ||
          MADB_DynStrAppendQuoted(DynString, field->org_name))
          goto memerror;
      if (ParameterMarkers)
      {
        if (MADB_DynstrAppend(DynString, "=?"))
          goto memerror;
      }
      else
      { 
        if (!SQL_SUCCEEDED(Stmt->Methods->GetData(Stmt, i+1, SQL_C_CHAR, NULL, 0, &StrLength, TRUE)))
        {
          MADB_FREE(Column);
          return TRUE;
        }
        if (StrLength < 0)
        {
           if (MADB_DynstrAppend(DynString, " IS NULL"))
             goto memerror;
        }
        else
        {
          Column= MADB_CALLOC(StrLength + 1);
          Stmt->Methods->GetData(Stmt,i+1, SQL_C_CHAR, Column, StrLength + 1, &StrLength, TRUE);
          Escaped = MADB_CALLOC(2 * StrLength + 1);
          EscapedLength= mysql_real_escape_string(Stmt->Connection->mariadb, Escaped, Column, (unsigned long)StrLength);

          if (MADB_DynstrAppend(DynString, "= '") ||
            MADB_DynstrAppend(DynString, Escaped) ||//, EscapedLength) ||
            MADB_DynstrAppend(DynString, "'"))
          {
            goto memerror;
          }
          MADB_FREE(Column);
          MADB_FREE(Escaped);
        }
      }
    }
  }
  if (MADB_DynstrAppend(DynString, " LIMIT 1"))
    goto memerror;
  MADB_FREE(Column);

  return FALSE;

memerror:
  MADB_FREE(Column);
  MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);

  return TRUE;
}


my_bool MADB_DynStrGetValues(MADB_Stmt *Stmt, MADB_DynString *DynString)
{
  unsigned int i;
  if (MADB_DynstrAppend(DynString, " VALUES("))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    return TRUE;
  }
  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    if (MADB_DynstrAppend(DynString, (i) ? ",?" : "?"))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      return TRUE;
    }
  }
  if (MADB_DynstrAppend(DynString, ")"))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    return TRUE;
  }
  return FALSE;
}

char *MADB_GetInsertStatement(MADB_Stmt *Stmt)
{
  char *StmtStr;
  size_t Length= 1024;
  char *p;
  char *TableName;
  unsigned int i;

  if (!(StmtStr= MADB_CALLOC(1024)))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY013, NULL, 0);
    return NULL;
  }
  if (!(TableName= MADB_GetTableName(Stmt)))
    goto error;
  p= StmtStr;
  
  p+= _snprintf(StmtStr, 1024, "INSERT INTO `%s` (", TableName);
  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    if (strlen(StmtStr) > Length - NAME_LEN - 4/* comma + 2 ticks + terminating NULL */)
    {
      Length+= 1024;
      if (!(StmtStr= MADB_REALLOC(StmtStr, Length)))
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY013, NULL, 0);
        goto error;
      }
    }
    p+= _snprintf(p, Length - strlen(StmtStr), "%s`%s`", (i==0) ? "" : ",", Stmt->stmt->fields[i].org_name);
  }
  p+= _snprintf(p, Length - strlen(StmtStr), ") VALUES (");

  if (strlen(StmtStr) > Length - mysql_stmt_field_count(Stmt->stmt)*2 - 1)/* , and ? for each column  + (- 1 comma for 1st column + closing ')')
                                                                            + terminating NULL */
  {
    Length= strlen(StmtStr) + mysql_stmt_field_count(Stmt->stmt)*2 + 1;
    if (!(StmtStr= MADB_REALLOC(StmtStr, Length)))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY013, NULL, 0);
      goto error;
    }
  }

  for (i=0; i < mysql_stmt_field_count(Stmt->stmt); i++)
  {
    p+= _snprintf(p, Length - strlen(StmtStr), "%s?", (i==0) ? "" : ",");
  }
  p+= _snprintf(p, Length - strlen(StmtStr), ")");
  return StmtStr;

error:
  if (StmtStr)
    MADB_FREE(StmtStr);
  return NULL;
}


my_bool MADB_ValidateStmt(MADB_QUERY *Query)
{
  return Query->QueryType != MADB_QUERY_SET_NAMES;
}


char *MADB_ToLower(const char *src, char *buff, size_t buff_size)
{
  size_t i= 0;

  if (buff_size > 0)
  {
    while (*src && i < buff_size)
    {
      buff[i++]= tolower(*src++);
    }

    buff[i == buff_size ? i - 1 : i]= '\0';
  }
  return buff;
}


int InitClientCharset(Client_Charset *cc, const char * name)
{
  /* There is no legal charset names longer than 31 chars */
  char lowered[32];
  cc->cs_info= mariadb_get_charset_by_name(MADB_ToLower(name, lowered, sizeof(lowered)));

  if (cc->cs_info == NULL)
  {
    return 1;
  }

  cc->CodePage= cc->cs_info->codepage;

  return 0;
}


void CopyClientCharset(Client_Charset * Src, Client_Charset * Dst)
{
  Dst->CodePage= Src->CodePage;
  Dst->cs_info= Src->cs_info;
}


void CloseClientCharset(Client_Charset *cc)
{
}


/* Hmmm... Length in characters is SQLLEN, octet length SQLINTEGER */
SQLLEN MbstrOctetLen(const char *str, SQLLEN *CharLen, MARIADB_CHARSET_INFO *cs)
{
  SQLLEN result= 0, inChars= *CharLen;

  if (str)
  {
    if (cs->mb_charlen == NULL)
    {
      /* Charset uses no more than a byte per char. Result is strlen or umber of chars */
      if (*CharLen < 0)
      {
        result= (SQLLEN)strlen(str);
        *CharLen= result;
      }
      else
      {
        result= *CharLen;
      }
      return result;
    }
    else
    {
      while (inChars > 0 || (inChars < 0 && *str))
      {
        result+= cs->mb_charlen(0 + *str);
        --inChars;
        str+= cs->mb_charlen(*str);
      }
    }
  }

  if (*CharLen < 0)
  {
    *CharLen-= inChars;
  }
  return result;
}


/* Number of characters in given number of bytes */
SQLLEN MbstrCharLen(const char *str, SQLINTEGER OctetLen, MARIADB_CHARSET_INFO *cs)
{
  SQLLEN       result= 0;
  const char   *ptr= str;
  unsigned int charlen;

  if (str)
  {
    if (cs->mb_charlen == NULL || cs->char_maxlen == 1)
    {
      return OctetLen;
    }
    while (ptr < str + OctetLen)
    {
      charlen= cs->mb_charlen((unsigned char)*ptr);
      if (charlen == 0)
      {
        /* Dirty hack to avoid dead loop - Has to be the error! */
        charlen= 1;
      }

      /* Skipping thru 0 bytes */
      while (charlen > 0 && *ptr == '\0')
      {
          --charlen;
          ++ptr;
      }

      /* Stopping if current character is terminating NULL - charlen == 0 means all bytes of current char was 0 */
      if (charlen == 0)
      {
        return result;
      }
      /* else we increment ptr for number of left bytes */
      ptr+= charlen;
      ++result;
    }
  }

  return result;
}


/* Length of NT SQLWCHAR string in characters */
SQLINTEGER SqlwcsCharLen(SQLWCHAR *str, SQLLEN octets)
{
  SQLINTEGER result= 0;
  SQLWCHAR   *end=   octets != (SQLLEN)-1 ? str + octets/sizeof(SQLWCHAR) : (SQLWCHAR*)octets /*for simplicity - the address to be always bigger */;

  if (str)
  {
    while (str < end && *str)
    {
      str+= (DmUnicodeCs->mb_charlen(*str))/sizeof(SQLWCHAR);

      if (str > end)
      {
        break;
      }
      ++result;
    }
  }
  return result;
}


/* Length in SQLWCHAR units
   @buff_length[in] - size of the str buffer or negative number  */
SQLLEN SqlwcsLen(SQLWCHAR *str, SQLLEN buff_length)
{
  SQLINTEGER result= 0;

  if (str)
  {
    /* If buff_length is negative - we will never hit 1st condition, otherwise we hit it after last character
       of the buffer is processed */
    while ((--buff_length) != -1 && *str)
    {
      ++result;
      ++str;
    }
  }
  return result;
}

/* Length of a string with respect to specified buffer size
@buff_length[in] - size of the str buffer or negative number  */
SQLLEN SafeStrlen(SQLCHAR *str, SQLLEN buff_length)
{
  SQLINTEGER result= 0;

  if (str)
  {
    /* If buff_length is negative - we will never hit 1st condition, otherwise we hit it after last character
    of the buffer is processed */
    while ((--buff_length) != -1 && *str)
    {
      ++result;
      ++str;
    }
  }
  return result;
}