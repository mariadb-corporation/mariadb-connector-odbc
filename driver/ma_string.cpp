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

#include "ResultSetMetaData.h"
#include "interface/ResultSet.h"

extern MARIADB_CHARSET_INFO*  DmUnicodeCs;

char* MADB_GetTableName(MADB_Stmt *Stmt)
{
  char *TableName= nullptr;
  uint32_t  i= 0, colCount= 0;
  const MYSQL_FIELD *Field= nullptr;

  if (Stmt->TableName && Stmt->TableName[0])
  {
    return Stmt->TableName;
  }
  if (!Stmt->rs)
  {
    return nullptr;
  }

  colCount= Stmt->metadata->getColumnCount();
  Field= Stmt->metadata->getFields();
  for (i=0; i < colCount; i++)
  {
    if (Field[i].org_table)
    {
      if (!TableName)
      {
        TableName= Field[i].org_table;
      }
      if (strcmp(TableName, Field[i].org_table))
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY000, "Couldn't identify unique table name", 0);
        return nullptr;
      }
    }
  }
  if (TableName)
  {
    Stmt->TableName= _strdup(TableName);
  }
  return Stmt->TableName;
}

char *MADB_GetCatalogName(MADB_Stmt *Stmt)
{
  char *CatalogName= nullptr;
  uint32_t i= 0, colCount= 0;
  const MYSQL_FIELD* Field= nullptr;

  if (Stmt->CatalogName && Stmt->CatalogName[0])
  {
    return Stmt->CatalogName;
  }
  if (!Stmt->metadata)
  {
    return nullptr;
  }

  colCount= Stmt->metadata->getColumnCount();
  Field= Stmt->metadata->getFields();
  for (i=0; i < colCount; i++)
  {
    if (Field[i].org_table)
    {
      if (!CatalogName)
        CatalogName= Field[i].db;
      if (strcmp(CatalogName, Field[i].db))
      {
        MADB_SetError(&Stmt->Error, MADB_ERR_HY000, "Couldn't identify unique catalog name", 0);
        return nullptr;
      }
    }
  }
  if (CatalogName)
  {
    Stmt->CatalogName= _strdup(CatalogName);
  }
  return Stmt->CatalogName;
}


my_bool MADB_DynStrAppendQuoted(MADB_DynString *DynString, char *String)
{
  if (MADB_DynstrAppendMem(DynString, "`", 1) ||
    MADB_DynstrAppend(DynString, String) ||
    MADB_DynstrAppendMem(DynString, "`", 1))
  {
    return TRUE;
  }
  return FALSE;
}

bool MADB_DynStrUpdateSet(MADB_Stmt* Stmt, SQLString& DynString)
{
  int             i, IgnoredColumns= 0;
  MADB_DescRecord *Record;

  DynString.append(" SET ");

  const MYSQL_FIELD *Field= Stmt->metadata->getFields();
  // ???? memcpy(&Stmt->Da->Apd->Header, &Stmt->Ard->Header, sizeof(MADB_Header));
  for (i=0; i < MADB_STMT_COLUMN_COUNT(Stmt); i++)
  {
    SQLLEN *IndicatorPtr= nullptr;
    Record= MADB_DescGetInternalRecord(Stmt->Ard, i, MADB_DESC_READ);
    if (Record->IndicatorPtr)
      IndicatorPtr= (SQLLEN *)GetBindOffset(Stmt->Ard->Header, Record->IndicatorPtr, Stmt->DaeRowNumber > 1 ? Stmt->DaeRowNumber-1 : 0,
                                            sizeof(SQLLEN)/*Record->OctetLength*/);
    if ((IndicatorPtr && *IndicatorPtr == SQL_COLUMN_IGNORE) || !Record->inUse)
    {
      ++IgnoredColumns;
      continue;
    }
    
    if (i != IgnoredColumns)
    {
      DynString.append(1, ',');
    }
    DynString.append(1, '`').append(Field[i].org_name).append("`=? ");
  }
  if (IgnoredColumns == Stmt->metadata->getColumnCount())
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_21S02, nullptr, 0);
    return true;
  }
  return false;
}

my_bool MADB_DynStrInsertSet(MADB_Stmt *Stmt, MADB_DynString *DynString)
{
  MADB_DynString  ColVals;
  int             i, NeedComma= 0;
  MADB_DescRecord *Record;
  const MYSQL_FIELD *Field;

  MADB_InitDynamicString(&ColVals, "VALUES (", 32, 32);
  if (MADB_DYNAPPENDCONST(DynString, " ("))
  {
    goto dynerror;
    
    return TRUE;
  }

  Field= Stmt->metadata->getFields();
  /* We use only columns, that have been bound, and are not IGNORED
     TODO: we gave to use this column count in most, if not all, places, where it's got via API function */
  for (i= 0; i < MADB_STMT_COLUMN_COUNT(Stmt); i++)
  {
    Record= MADB_DescGetInternalRecord(Stmt->Ard, i, MADB_DESC_READ);
    if (!Record->inUse || MADB_ColumnIgnoredInAllRows(Stmt->Ard, Record) == TRUE)
    {
      continue;
    }

    if ((NeedComma) && 
        (MADB_DYNAPPENDCONST(DynString, ",") || MADB_DYNAPPENDCONST(&ColVals, ",")))
      goto dynerror;

    if (MADB_DynStrAppendQuoted(DynString, Field[i].org_name) ||
        MADB_DYNAPPENDCONST(&ColVals, "?"))
       goto dynerror;

    NeedComma= 1;
  }
  if (MADB_DYNAPPENDCONST(DynString, ") " ) ||
      MADB_DYNAPPENDCONST(&ColVals, ")") ||
      MADB_DynstrAppend(DynString, ColVals.str))
    goto dynerror;
  MADB_DynstrFree(&ColVals);
  return FALSE;
dynerror:
  MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
  MADB_DynstrFree(&ColVals);
  return TRUE;
}

my_bool MADB_DynStrGetColumns(MADB_Stmt *Stmt, MADB_DynString *DynString)
{
  if (MADB_DYNAPPENDCONST(DynString, " ("))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
    return TRUE;
  }

  unsigned int i;
  std::size_t colCount= Stmt->metadata->getColumnCount();
  const MYSQL_FIELD *Field=   Stmt->metadata->getFields();
  for (i=0; i < colCount; i++)
  {
    if (i && MADB_DYNAPPENDCONST(DynString, ", "))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
      return TRUE;
    }
    if (MADB_DynStrAppendQuoted(DynString, Field[i].org_name))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
      return TRUE;
    }
  }
  if (MADB_DYNAPPENDCONST(DynString, " )"))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
    return TRUE;
  }
  return FALSE;
}

bool MADB_DynStrGetWhere(MADB_Stmt *Stmt, SQLString &DynString, char *TableName, bool ParameterMarkers)
{
  int UniqueCount=0, PrimaryCount= 0, TotalPrimaryCount= 0, TotalUniqueCount= 0, TotalTableFieldCount= 0;
  int i, Flag= 0, IndexArrIdx= 0;
  char *Column= nullptr, *Escaped= nullptr;
  SQLLEN StrLength;
  unsigned long EscapedLength;

  if (Stmt->UniqueIndex != nullptr)
  {
    TotalUniqueCount= Stmt->UniqueIndex[0];
    IndexArrIdx= 1;
  }
  else
  {
    for (i= 0; i < MADB_STMT_COLUMN_COUNT(Stmt); i++)
    {
      const MYSQL_FIELD* field= FetchMetadata(Stmt)->getField(i);
      if (field->flags & PRI_KEY_FLAG)
      {
        ++PrimaryCount;
      }
      if (field->flags & UNIQUE_KEY_FLAG)
      {
        ++UniqueCount;
      }
    }

    TotalTableFieldCount= MADB_KeyTypeCount(Stmt->Connection, TableName, &TotalPrimaryCount, &TotalUniqueCount);

    if (TotalTableFieldCount < 0)
    {
      /* Error. Expecting that the called function has set the error */
      return TRUE;
    }
    /* We need to use all columns, otherwise it will be difficult to map fields for Positioned Update */
    if (PrimaryCount != TotalPrimaryCount)
    {
      PrimaryCount= 0;
    }
    if (UniqueCount != TotalUniqueCount)
    {
      UniqueCount= 0;
    }

    /* if no primary or unique key is in the cursor, the cursor must contain all
       columns from table in TableName */
       /* We use unique index if we do not have primary. TODO: If there are more than one unique index - we are in trouble */
    if (PrimaryCount != 0)
    {
      Flag= PRI_KEY_FLAG;
      /* Changing meaning of TotalUniqueCount from field count in unique index to field count in *best* unique index(that can be primary as well) */
      TotalUniqueCount= PrimaryCount;
    }
    else if (UniqueCount != 0)
    {
      Flag= UNIQUE_KEY_FLAG;
      /* TotalUniqueCount is equal UniqueCount */
    }
    else if (TotalTableFieldCount != MADB_STMT_COLUMN_COUNT(Stmt))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_S1000, "Can't build index for update/delete", 0);
      return TRUE;
    }
    else
    {
      TotalUniqueCount= 0;
    }

    if (TotalUniqueCount != 0)
    {
      /* First element gets number of columns in the index */
      Stmt->UniqueIndex= static_cast<unsigned short*>(MADB_ALLOC((TotalUniqueCount + 1) * sizeof(*Stmt->UniqueIndex)));
      if (Stmt->UniqueIndex == nullptr)
      {
        goto memerror;
      }
      Stmt->UniqueIndex[0]= TotalUniqueCount;
    }
  }

  DynString.append(" WHERE 1");

  /* If we already know index columns - we walk through column index values stored in Stmt->UniqueIndex, all columns otherwise */
  for (i= IndexArrIdx == 0 ? 0 : Stmt->UniqueIndex[1];
    IndexArrIdx == 0 ? i < MADB_STMT_COLUMN_COUNT(Stmt) : IndexArrIdx <= Stmt->UniqueIndex[0];
    i= IndexArrIdx == 0 ? i + 1 : (++IndexArrIdx > Stmt->UniqueIndex[0] ? 0 /* Doesn't really matter what we set here - loop won't go further,
                                                                               we just don't want to read past Stmt->UniqueIndex allocated area */
                                                                        : Stmt->UniqueIndex[IndexArrIdx]))
  {
    const MYSQL_FIELD *field= Stmt->metadata->getField(i);

    /* If we have already index columns, or column has right flag(primary or unique) set, or there is no good index */
    if ( IndexArrIdx != 0 || field->flags & Flag || Flag == 0)
    {
      /* Storing index information */
      if (Flag != 0)
      {
        /* Complicated(aka silly) way not to introduce another variable to store arr index for writing to the arr */
        Stmt->UniqueIndex[Stmt->UniqueIndex[0] - (--TotalUniqueCount)]= i;
      }
      DynString.append(" AND ").append(field->org_name);

      if (ParameterMarkers)
      {
        DynString.append("=?");
      }
      else
      { 
        if (!SQL_SUCCEEDED(Stmt->Methods->GetData(Stmt, i+1, SQL_C_CHAR, nullptr, 0, &StrLength, true)))
        {
          MADB_FREE(Column);
          return TRUE;
        }
        if (StrLength < 0)
        {
          DynString.append(" IS NULL");
        }
        else
        {
          Column= static_cast<char*>(MADB_CALLOC(StrLength + 1));
          Stmt->Methods->GetData(Stmt,i+1, SQL_C_CHAR, Column, StrLength + 1, &StrLength, true);
          Escaped= static_cast<char*>(MADB_CALLOC(2 * StrLength + 1));
          EscapedLength= mysql_real_escape_string(Stmt->Connection->mariadb, Escaped, Column, (unsigned long)StrLength);

          DynString.append("= '").append(Escaped).append("'");

          MADB_FREE(Column);
          MADB_FREE(Escaped);
        }
      }
    }
  }

  DynString.append(" LIMIT 1");
  MADB_FREE(Column);

  return false;

memerror:
  MADB_FREE(Column);
  MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);

  return true;
}


my_bool MADB_DynStrGetValues(MADB_Stmt *Stmt, MADB_DynString *DynString)
{
  unsigned int i;
  if (MADB_DYNAPPENDCONST(DynString, " VALUES("))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
    return TRUE;
  }
  for (i=0; i < Stmt->metadata->getColumnCount(); i++)
  {
    if (MADB_DynstrAppend(DynString, (i) ? ",?" : "?"))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
      return TRUE;
    }
  }
  if (MADB_DYNAPPENDCONST(DynString, ")"))
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
    return TRUE;
  }
  return FALSE;
}


bool MADB_ValidateStmt(MADB_QUERY *Query)
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

  if (cc->cs_info == nullptr)
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
    if (cs->mb_charlen == nullptr)
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


/* Number of wchar units in given number of bytes */
SQLLEN MbstrCharLen(const char *str, SQLINTEGER OctetLen, MARIADB_CHARSET_INFO *cs)
{
  SQLLEN       result= 0;
  const char   *ptr= str;
  unsigned int charlen;

  if (str)
  {
    if (cs->mb_charlen == nullptr || cs->char_maxlen == 1)
    {
      return OctetLen;
    }
    while (ptr < str + OctetLen)
    {
      charlen= cs->mb_charlen((unsigned char)*ptr);
      if (charlen == 0)
      {
        /* Dirty hack to avoid dead loop - Has to be the error! but there is no way to set it here */
        charlen= 1;
      }

      /* Skipping thru 0 bytes */
      while (charlen > 0 && *ptr == '\0')
      {
          --charlen;
          ++ptr;
      }

      /* Stopping if current character is terminating nullptr - charlen == 0 means all bytes of current char was 0 */
      /*if (charlen == 0)
      {
        return result;
      }*/
      /* else we increment ptr for number of left bytes */
      ptr+= charlen;

      if (charlen == 4 && sizeof(SQLWCHAR) == 2)
      {
        // Thinking mostly about UTF8 and if it needs 4 bytes to encode the character, then it needs 2
        // sqlwchar units.
        result+= 2;
      }
      else
      {
        ++result;
      }
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

/* Bind is not really needed, but since the caller already allocates it... */
void StreamWstring(MADB_Stmt* Stmt, SQLUSMALLINT Offset, MADB_DescRecord* IrdRec, MYSQL_BIND& Bind,
                   SQLWCHAR* TargetValuePtr, SQLLEN BufferLength, SQLLEN* StrLen_or_IndPtr)
{
  char* ClientValue= NULL;
  size_t CharLength= 0;

  /* Kinda this it not 1st call for this value, and we have it nice and recoded */
  if (IrdRec->InternalBuffer == nullptr/* && Stmt->Lengths[Offset] == 0*/)
  {
    unsigned long FieldBufferLen= 0;
    Bind.length= &FieldBufferLen;
    Bind.buffer_type= MYSQL_TYPE_STRING;
    /* Getting value's length to allocate the buffer */
    if (Stmt->rs->get(&Bind, Offset, Stmt->CharOffset[Offset]))
    {
      MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt.get());
      throw Stmt->Error;
    }
    /* Adding byte for terminating null */
    ++FieldBufferLen;
    if (!(ClientValue= (char*)MADB_CALLOC(FieldBufferLen)))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      throw Stmt->Error;
    }
    Bind.buffer= ClientValue;
    Bind.buffer_length= FieldBufferLen;
    Bind.length= &Bind.length_value;

    if (Stmt->rs->get(&Bind, Offset, Stmt->CharOffset[Offset]))
    {
      MADB_FREE(ClientValue);
      MADB_SetNativeError(&Stmt->Error, SQL_HANDLE_STMT, Stmt->stmt.get());
      throw Stmt->Error;
    }

    /* check total length: if not enough space, we need to calculate new CharOffset for next fetch */
    if (Bind.length_value > 0)
    {
      size_t ReqBuffOctetLen;
      /* Size in -chars- wchar units */
      CharLength= MbstrCharLen(ClientValue, Bind.length_value - Stmt->CharOffset[Offset],
        Stmt->Connection->Charset.cs_info);
      /* MbstrCharLen gave us length in SQLWCHAR units, not in characters. */
      ReqBuffOctetLen= (CharLength + 1)*sizeof(SQLWCHAR);

      if (BufferLength)
      {
        /* Buffer is not big enough. Alocating InternalBuffer.
           MADB_SetString would do that anyway if - allocate buffer fitting the whole wide string,
           and then copied its part to the application's buffer */
        if (ReqBuffOctetLen > (size_t)BufferLength)
        {
          IrdRec->InternalBuffer= (char*)MADB_CALLOC(ReqBuffOctetLen);

          if (IrdRec->InternalBuffer == nullptr)
          {
            MADB_FREE(ClientValue);
            MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
            throw Stmt->Error;
          }

          CharLength= MADB_SetString(&Stmt->Connection->Charset, IrdRec->InternalBuffer, (SQLINTEGER)ReqBuffOctetLen / sizeof(SQLWCHAR),
            ClientValue, Bind.length_value - Stmt->CharOffset[Offset], &Stmt->Error);
        }
        else
        {
          /* Application's buffer is big enough - writing directly there */
          CharLength= MADB_SetString(&Stmt->Connection->Charset, TargetValuePtr, (SQLINTEGER)(BufferLength / sizeof(SQLWCHAR)),
            ClientValue, Bind.length_value - Stmt->CharOffset[Offset], &Stmt->Error);
        }

        if (!SQL_SUCCEEDED(Stmt->Error.ReturnValue))
        {
          MADB_FREE(ClientValue);
          MADB_FREE(IrdRec->InternalBuffer);

          throw Stmt->Error;
        }
      }
      if (!Stmt->CharOffset[Offset])
      {
        Stmt->Lengths[Offset]= (unsigned long)(CharLength * sizeof(SQLWCHAR));
      }
    }
    else if (BufferLength >= sizeof(SQLWCHAR)) //else to if (Bind.length_value > 0)
    {
      // There is nothing left - writing terminating null
      *(SQLWCHAR*)TargetValuePtr= 0;
    }
  }
  else  /* IrdRec->InternalBuffer == NULL */
  {
    // Length and offset are in bytes, but simply dividing by sizeof(SQLWCHAR) doesn't look right
    CharLength= (Stmt->Lengths[Offset] - Stmt->CharOffset[Offset]) / sizeof(SQLWCHAR);
    /* SqlwcsLen((SQLWCHAR*)((char*)IrdRec->InternalBuffer + Stmt->CharOffset[Offset]), -1);*/
  }

  if (StrLen_or_IndPtr)
  {
    *StrLen_or_IndPtr= CharLength * sizeof(SQLWCHAR);
  }

  if (!BufferLength)
  {
    MADB_FREE(ClientValue);
    MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
    throw Stmt->Error;
  }

  if (IrdRec->InternalBuffer)
  {
    /* If we have more place than only for the TN */
    if (BufferLength > sizeof(SQLWCHAR))
    {
      memcpy(TargetValuePtr, (char*)IrdRec->InternalBuffer + Stmt->CharOffset[Offset],
        MIN(BufferLength - sizeof(SQLWCHAR), CharLength * sizeof(SQLWCHAR)));
    }
    /* Terminating Null */
    *(SQLWCHAR*)((char*)TargetValuePtr + MIN(BufferLength - sizeof(SQLWCHAR), CharLength * sizeof(SQLWCHAR)))= 0;
  }

  if (CharLength >= BufferLength / sizeof(SQLWCHAR))
  {
    /* Calculate new offset and substract 1 byte for null termination */
    Stmt->CharOffset[Offset] += (unsigned long)BufferLength - sizeof(SQLWCHAR);
    MADB_FREE(ClientValue);

    MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0);
    throw Stmt->Error;
  }
  else
  {
    Stmt->CharOffset[Offset]= Stmt->Lengths[Offset];
    MADB_FREE(IrdRec->InternalBuffer);
  }
  MADB_FREE(ClientValue);
}


char * ma_strmov(char * dest, const char * src, size_t len)
{
  return strncpy(dest, src, len) + len;
}
