/************************************************************************************
   Copyright (C) 2013, 2019 MariaDB Corporation AB

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

extern Client_Charset utf8;

/* To keep the source code smaller we use the following structure to check if a field
   identifier is valid for a given descriptor type */
struct st_ma_desc_fldid MADB_DESC_FLDID[]=
{
  {SQL_DESC_ALLOC_TYPE, {MADB_DESC_READ, MADB_DESC_READ, MADB_DESC_NONE, MADB_DESC_NONE}},
  {SQL_DESC_ARRAY_SIZE, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_NONE, MADB_DESC_NONE}},
  {SQL_DESC_ARRAY_STATUS_PTR, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW}},
  {SQL_DESC_BIND_OFFSET_PTR, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_NONE, MADB_DESC_NONE}},
  {SQL_DESC_BIND_TYPE, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_NONE, MADB_DESC_NONE}},
  {SQL_DESC_COUNT, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_ROWS_PROCESSED_PTR, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_RW, MADB_DESC_RW}},
  {SQL_DESC_AUTO_UNIQUE_VALUE,  {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_BASE_COLUMN_NAME, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_BASE_TABLE_NAME, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_CASE_SENSITIVE, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_CATALOG_NAME, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_CONCISE_TYPE, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW}},
  {SQL_DESC_DATA_PTR, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_NONE, MADB_DESC_NONE}},
  {SQL_DESC_DATETIME_INTERVAL_CODE, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_READ}},
  {SQL_DESC_DATETIME_INTERVAL_PRECISION, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_READ}},
  {SQL_DESC_DISPLAY_SIZE, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE}},
  {SQL_DESC_FIXED_PREC_SCALE, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_RW, MADB_DESC_RW}},
  {SQL_DESC_INDICATOR_PTR, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_NONE, MADB_DESC_NONE}},
  {SQL_DESC_LABEL, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE}},
  {SQL_DESC_LENGTH, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_READ}},
  {SQL_DESC_LITERAL_PREFIX, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_LITERAL_SUFFIX, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_LOCAL_TYPE_NAME, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ, MADB_DESC_READ}},
  {SQL_DESC_NAME, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_RW, MADB_DESC_READ}},
  {SQL_DESC_NULLABLE, {MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ, MADB_DESC_READ}},
  {SQL_DESC_NUM_PREC_RADIX, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_READ}},
  {SQL_DESC_OCTET_LENGTH, {MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_READ}},
  {SQL_DESC_OCTET_LENGTH_PTR,{MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_NONE, MADB_DESC_NONE}},
  {SQL_DESC_PARAMETER_TYPE,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_RW, MADB_DESC_NONE}},
  {SQL_DESC_PRECISION,{MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_READ}},
#if (ODBCVER >= 0x0350)
  {SQL_DESC_ROWVER,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ, MADB_DESC_READ}},
#endif
  {SQL_DESC_SCALE,{MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_READ}},
  {SQL_DESC_SCHEMA_NAME,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_SEARCHABLE,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_TABLE_NAME,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_TYPE,{MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_RW, MADB_DESC_READ}},
  {SQL_DESC_TYPE_NAME ,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ, MADB_DESC_READ}},
  {SQL_DESC_UNSIGNED ,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ, MADB_DESC_READ}},
  {SQL_DESC_UPDATABLE ,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_READ}},
  {SQL_DESC_UNNAMED ,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_RW, MADB_DESC_READ}},
  {0, {0, 0, 0, 0}}
};

/* {{{ MADB_DescInit */
MADB_Desc *MADB_DescInit(MADB_Dbc *Dbc,enum enum_madb_desc_type DescType, my_bool isExternal)
{
  MADB_Desc *Desc;
  
  if (!(Desc= (MADB_Desc *)MADB_CALLOC(sizeof(MADB_Desc))))
    return NULL;

  Desc->DescType= DescType;
  MADB_PutErrorPrefix(Dbc, &Desc->Error);

  if (MADB_InitDynamicArray(&Desc->Records, sizeof(MADB_DescRecord), 0, MADB_DESC_INIT_REC_NUM))
  {
    MADB_FREE(Desc);
    return NULL;
  }
  if (isExternal)
  {
    if (MADB_InitDynamicArray(&Desc->Stmts, sizeof(MADB_Stmt**), 0, MADB_DESC_INIT_STMT_NUM))
    {
      MADB_DescFree(Desc, FALSE);
      return NULL;
    }
    else
    {
      Desc->Dbc= Dbc;
      /* MADB_DescInit call for explicit descriptor is in critical section */
      Desc->ListItem.data= (void *)Desc;
      Dbc->Descrs= MADB_ListAdd(Dbc->Descrs, &Desc->ListItem);
    }
  }
  Desc->AppType= isExternal;
  Desc->Header.ArraySize= 1;
 
  return Desc;
}
/* }}} */

/* {{{ MADB_DescFree */
SQLRETURN MADB_DescFree(MADB_Desc *Desc, my_bool RecordsOnly)
{
  MADB_DescRecord *Record;
  unsigned int i;

  if (!Desc)
    return SQL_ERROR;

  /* We need to free internal pointers first */
  for (i=0; i < Desc->Records.elements; i++)
  {
    Record= ((MADB_DescRecord *)Desc->Records.buffer) + i;
    MADB_FREE(Record->InternalBuffer);
    MADB_FREE(Record->DefaultValue);
 
    if (Desc->DescType == MADB_DESC_IRD)
    {
      MADB_FREE(Record->CatalogName);
      MADB_FREE(Record->BaseCatalogName);
      MADB_FREE(Record->BaseColumnName);
      MADB_FREE(Record->BaseTableName);
      MADB_FREE(Record->ColumnName);
      MADB_FREE(Record->TableName);
      MADB_FREE(Record->TypeName);
    }
    else if(Desc->DescType == MADB_DESC_IPD)
    {
      MADB_FREE(Record->TypeName);
    }
  }
  MADB_DeleteDynamic(&Desc->Records);

  Desc->Header.Count= 0;
  if (Desc->AppType)
  {
    EnterCriticalSection(&Desc->Dbc->ListsCs);
    for (i=0; i < Desc->Stmts.elements; i++)
    {
      MADB_Stmt **XStmt= ((MADB_Stmt **)Desc->Stmts.buffer) + i;
      MADB_Stmt *Stmt= *XStmt;
      switch(Desc->DescType) {
      case MADB_DESC_ARD:
        Stmt->Ard=Stmt->IArd;
        break;
      case MADB_DESC_APD:
        Stmt->Apd= Stmt->IApd;
        break;
      }
    }
    MADB_DeleteDynamic(&Desc->Stmts);
  
    Desc->Dbc->Descrs= MADB_ListDelete(Desc->Dbc->Descrs, &Desc->ListItem);
    LeaveCriticalSection(&Desc->Dbc->ListsCs);
  }
  
  if (!RecordsOnly)
    MADB_FREE(Desc);
  return SQL_SUCCESS;
}
/* }}} */ 

/* {{{ MADB_SetIrdRecord */
my_bool
MADB_SetIrdRecord(MADB_Stmt *Stmt, MADB_DescRecord *Record, MYSQL_FIELD *Field)
{
  MY_CHARSET_INFO cs;
  MARIADB_CHARSET_INFO *FieldCs;

  if (Record == NULL)
  {
    return 1;
  }

  mariadb_get_infov(Stmt->Connection->mariadb, MARIADB_CONNECTION_MARIADB_CHARSET_INFO, (void*)&cs);

  MADB_RESET(Record->CatalogName,    Field->db);
  MADB_RESET(Record->TableName,      Field->table);
  MADB_RESET(Record->ColumnName,     Field->name);
  MADB_RESET(Record->BaseTableName,  Field->org_table);
  MADB_RESET(Record->BaseColumnName, Field->org_name);
  Record->AutoUniqueValue= (Field->flags & AUTO_INCREMENT_FLAG) ? SQL_TRUE : SQL_FALSE;
  Record->CaseSensitive=   (Field->flags & BINARY_FLAG) ? SQL_TRUE : SQL_FALSE;
  Record->Nullable= ( (Field->flags & NOT_NULL_FLAG) &&
                      !Record->AutoUniqueValue &&
                      Field->type != MYSQL_TYPE_TIMESTAMP) ? SQL_NO_NULLS : SQL_NULLABLE;
  Record->Unsigned= (Field->flags & UNSIGNED_FLAG) ? SQL_TRUE : SQL_FALSE;
  /* We assume it might be updatable if tablename exists */
  Record->Updateable= (Field->table && Field->table[0]) ? SQL_ATTR_READWRITE_UNKNOWN : SQL_ATTR_READONLY;

  /*
      RADIX:
      If the data type in the SQL_DESC_TYPE field is an approximate numeric data type, this SQLINTEGER field 
      contains a value of 2 because the SQL_DESC_PRECISION field contains the number of bits. 
      If the data type in the SQL_DESC_TYPE field is an exact numeric data type, this field contains a value of 10 
      because the SQL_DESC_PRECISION field contains the number of decimal digits. 
      This field is set to 0 for all non-numeric data types.
  */
  switch (Field->type) {
  case MYSQL_TYPE_DECIMAL:
  case MYSQL_TYPE_NEWDECIMAL:
    Record->NumPrecRadix= 10;
    Record->Scale= Field->decimals;
    Record->Precision= (SQLSMALLINT)Field->length - test(Record->Unsigned == SQL_FALSE) - test(Record->Scale > 0);
    if (Record->Precision == 0)
    {
      Record->Precision= Record->Scale;
    }
    break;
  case MYSQL_TYPE_FLOAT:
    Record->NumPrecRadix= 2;
    Record->Precision= (SQLSMALLINT)Field->length - 2;
    //Record->Scale= Field->decimals;
    break;
  case MYSQL_TYPE_DOUBLE:
  case MYSQL_TYPE_TINY:
  case MYSQL_TYPE_SHORT:
  case MYSQL_TYPE_INT24:
  case MYSQL_TYPE_LONG:
  case MYSQL_TYPE_LONGLONG:
  case MYSQL_TYPE_YEAR:
    Record->NumPrecRadix= 10;
    break;
  case MYSQL_TYPE_TIMESTAMP:
  case MYSQL_TYPE_TIME:
  case MYSQL_TYPE_DATETIME:
    Record->Scale= Field->decimals;
  default:
    Record->NumPrecRadix= 0;
    break;
  }

  Record->ConciseType= MapMariadDbToOdbcType(Field);
  /* 
      TYPE:
      For the datetime and interval data types, this field returns the verbose data type: SQL_DATETIME or SQL_INTERVAL.
  */
  Record->Type= (Record->ConciseType ==  SQL_TYPE_DATE || Record->ConciseType ==  SQL_DATE ||
                 Record->ConciseType ==  SQL_TYPE_TIME || Record->ConciseType ==  SQL_TIME ||
                 Record->ConciseType ==  SQL_TYPE_TIMESTAMP || Record->ConciseType == SQL_TIMESTAMP) ?
                    SQL_DATETIME : Record->ConciseType;

  switch(Record->ConciseType) {
  case SQL_TYPE_DATE:
    Record->DateTimeIntervalCode= SQL_CODE_DATE;
    break;
  case SQL_TYPE_TIME:
    Record->DateTimeIntervalCode= SQL_CODE_TIME;
    break;
  case SQL_TYPE_TIMESTAMP:
    Record->DateTimeIntervalCode= SQL_CODE_TIMESTAMP;
    break;
  }
  /* 
      SEARCHABLE:
      Columns of type SQL_LONGVARCHAR and SQL_LONGVARBINARY usually return SQL_PRED_CHAR.
  */
  Record->Searchable= (Record->ConciseType == SQL_LONGVARCHAR ||
                       Record->ConciseType == SQL_WLONGVARCHAR ||
                       Record->ConciseType == SQL_LONGVARBINARY) ? SQL_PRED_CHAR : SQL_SEARCHABLE;

  Record->DisplaySize= MADB_GetDisplaySize(Field, mariadb_get_charset_by_nr(Field->charsetnr));
  Record->OctetLength= MADB_GetOctetLength(Field, cs.mbmaxlen);
  FieldCs= mariadb_get_charset_by_nr(Field->charsetnr);
  Record->Length= MADB_GetDataSize(Record->ConciseType, Field->length, Record->Unsigned == SQL_TRUE,
                                   Record->Precision, Record->Scale, FieldCs!= NULL ? FieldCs->char_maxlen : 1);
    
  MADB_RESET(Record->TypeName, MADB_GetTypeName(Field));

  switch(Field->type) {
  case MYSQL_TYPE_BLOB:
  case MYSQL_TYPE_LONG_BLOB:
  case MYSQL_TYPE_MEDIUM_BLOB:
  case MYSQL_TYPE_STRING:
  case MYSQL_TYPE_TINY_BLOB:
  case MYSQL_TYPE_VAR_STRING:
    if (Field->flags & BINARY_FLAG)
    {
      Record->LiteralPrefix= "0x";
      Record->LiteralSuffix= "";
      break;
    }
    /* else default */
  case MYSQL_TYPE_DATE:
  case MYSQL_TYPE_DATETIME:
  case MYSQL_TYPE_NEWDATE:
  case MYSQL_TYPE_TIME:
  case MYSQL_TYPE_TIMESTAMP:
  case MYSQL_TYPE_YEAR:
    Record->LiteralPrefix="'";
    Record->LiteralSuffix= "'";
    break;
  default:
    Record->LiteralPrefix= "";
    Record->LiteralSuffix= "";
    break;
  }

  return 0;
}
/* }}} */

/* {{{ MADB_DescSetIrdMetadata */
my_bool 
MADB_DescSetIrdMetadata(MADB_Stmt *Stmt, MYSQL_FIELD *Fields, unsigned int NumFields)
{
  SQLSMALLINT i;

  /* Perhaps we should call routine that does SQL_CLOSE here */
  Stmt->Ird->Header.Count= 0;

  for (i= 0; i < (SQLSMALLINT)NumFields; i++)
  {
    if (MADB_SetIrdRecord(Stmt, MADB_DescGetInternalRecord(Stmt->Ird, i, MADB_DESC_WRITE), &Fields[i]))
    {
      return 1;
    }
  }
  return 0;
}
/* }}} */

/* {{{ MADB_FixOctetLength */
void MADB_FixOctetLength(MADB_DescRecord *Record)
{
  switch (Record->ConciseType) {
  case SQL_BIT:
  case SQL_TINYINT:
    Record->OctetLength= 1;
    break;
  case SQL_SMALLINT:
    Record->OctetLength= 2;
    break;
  case SQL_INTEGER:
  case SQL_REAL:
    Record->OctetLength= 4;
    break;
  case SQL_BIGINT:
  case SQL_DOUBLE:
    Record->OctetLength= 8;
    break;
  case SQL_TYPE_DATE:
    Record->OctetLength= sizeof(SQL_DATE_STRUCT);
    break;
  case SQL_TYPE_TIME:
    Record->OctetLength= sizeof(SQL_TIME_STRUCT);
    break;
  case SQL_TYPE_TIMESTAMP:
    Record->OctetLength= sizeof(SQL_TIMESTAMP_STRUCT);
    break;
  default:
    Record->OctetLength= MIN(MADB_INT_MAX32, Record->OctetLength);
  }
}
/* }}} */

/* {{{ MADB_FixDisplayLength */
void MADB_FixDisplaySize(MADB_DescRecord *Record, const MY_CHARSET_INFO *charset)
{
  switch (Record->ConciseType) {
  case SQL_BIT:
    Record->DisplaySize= 1;
    break;
  case SQL_TINYINT:
    Record->DisplaySize= 4 - test(Record->Unsigned == SQL_TRUE);
    break;
  case SQL_SMALLINT:
    Record->DisplaySize= 6 - test(Record->Unsigned == SQL_TRUE);
    break;
  case SQL_INTEGER:
    Record->DisplaySize= 11 - test(Record->Unsigned == SQL_TRUE);
    break;
  case SQL_REAL:
    Record->DisplaySize= 14;
    break;
  case SQL_BIGINT:
    Record->DisplaySize= 20;
    break;
  case SQL_FLOAT:
  case SQL_DOUBLE:
    Record->DisplaySize= 24;
    break;
  case SQL_DECIMAL:
  case SQL_NUMERIC:
    Record->DisplaySize= Record->Precision + 2;
    break;
  case SQL_TYPE_DATE:
    Record->DisplaySize= SQL_DATE_LEN;
    break;
  case SQL_TYPE_TIME:
    Record->DisplaySize= SQL_TIME_LEN + MADB_FRACTIONAL_PART(Record->Scale);
    break;
  case SQL_TYPE_TIMESTAMP:
    Record->DisplaySize= SQL_TIMESTAMP_LEN + MADB_FRACTIONAL_PART(Record->Scale);
    break;
  case SQL_BINARY:
  case SQL_VARBINARY:
  case SQL_LONGVARBINARY:
    Record->DisplaySize=Record->OctetLength*2; /* For display in hex */
    break;
  case SQL_GUID:
    Record->DisplaySize= 36;
    break;
  default:
    if (charset == NULL || charset->mbmaxlen < 2/*i.e.0||1*/)
    {
      Record->DisplaySize=Record->OctetLength;
    }
    else
    {
      Record->DisplaySize=Record->OctetLength/charset->mbmaxlen;
    }
  }
}
/* }}} */

/* {{{ MADB_FixDataSize - aka Column size */
void MADB_FixDataSize(MADB_DescRecord *Record, const MY_CHARSET_INFO *charset)
{
  Record->Length= MADB_GetDataSize(Record->ConciseType, Record->OctetLength, Record->Unsigned == TRUE, Record->Precision, Record->Scale, charset->mbmaxlen );
}
/* }}} */

/* {{{ MADB_FixIrdRecord */
my_bool
MADB_FixIrdRecord(MADB_Stmt *Stmt, MADB_DescRecord *Record)
{
  MY_CHARSET_INFO cs;

  if (Record == NULL)
  {
    return 1;
  }

  MADB_FixOctetLength(Record);
  /*
      RADIX:
      If the data type in the SQL_DESC_TYPE field is an approximate numeric data type, this SQLINTEGER field 
      contains a value of 2 because the SQL_DESC_PRECISION field contains the number of bits. 
      If the data type in the SQL_DESC_TYPE field is an exact numeric data type, this field contains a value of 10 
      because the SQL_DESC_PRECISION field contains the number of decimal digits. 
      This field is set to 0 for all non-numeric data types.
  */
  switch (Record->ConciseType) {
  case SQL_DECIMAL:
    Record->NumPrecRadix= 10;
    Record->Precision= (SQLSMALLINT)Record->OctetLength - 2;
    /*Record->Scale= Fields[i].decimals;*/
    break;
  case SQL_REAL:
    /* Float*/
    Record->NumPrecRadix= 2;
    Record->Precision= (SQLSMALLINT)Record->OctetLength - 2;
    break;
  case SQL_DOUBLE:
  case SQL_TINYINT:
  case SQL_SMALLINT:
  case SQL_INTEGER:
  case SQL_BIGINT:
    Record->NumPrecRadix= 10;
    break;
  default:
    Record->NumPrecRadix= 0;
    break;
  }
  /* 
      TYPE:
      For the datetime and interval data types, this field returns the verbose data type: SQL_DATETIME or SQL_INTERVAL.
  */
  Record->Type= (Record->ConciseType ==  SQL_TYPE_DATE || Record->ConciseType ==  SQL_DATE ||
                  Record->ConciseType ==  SQL_TYPE_TIME || Record->ConciseType ==  SQL_TIME ||
                  Record->ConciseType ==  SQL_TYPE_TIMESTAMP || Record->ConciseType == SQL_TIMESTAMP) ?
                    SQL_DATETIME : Record->ConciseType;

  switch(Record->ConciseType) {
  case SQL_TYPE_DATE:
    Record->DateTimeIntervalCode= SQL_CODE_DATE;
    break;
  case SQL_TYPE_TIME:
    Record->DateTimeIntervalCode= SQL_CODE_TIME;
    break;
  case SQL_TYPE_TIMESTAMP:
    Record->DateTimeIntervalCode= SQL_CODE_TIMESTAMP;
    break;
  }
  /* 
      SEARCHABLE:
      Columns of type SQL_LONGVARCHAR and SQL_LONGVARBINARY usually return SQL_PRED_CHAR.
  */
  Record->Searchable= (Record->ConciseType == SQL_LONGVARCHAR ||
                        Record->ConciseType == SQL_WLONGVARCHAR ||
                        Record->ConciseType == SQL_LONGVARBINARY) ? SQL_PRED_CHAR : SQL_SEARCHABLE;

  mariadb_get_infov(Stmt->Connection->mariadb, MARIADB_CONNECTION_MARIADB_CHARSET_INFO, (void*)&cs);

  MADB_FixDisplaySize(Record, &cs);
  MADB_FixDataSize(Record, &cs);
    
  /*Record->TypeName= strdup(MADB_GetTypeName(Fields[i]));*/

  switch(Record->ConciseType) {
  case SQL_BINARY:
  case SQL_VARBINARY:
  case SQL_LONGVARBINARY:
      Record->LiteralPrefix= "0x";
      Record->LiteralSuffix= "";
      break;
  /* else default */
  case SQL_TYPE_DATE:
  case SQL_TYPE_TIME:
  case SQL_TYPE_TIMESTAMP:
    Record->LiteralPrefix="'";
    Record->LiteralSuffix= "'";
    break;
  default:
    Record->LiteralPrefix= "";
    Record->LiteralSuffix= "";
    break;
  }

  return 0;
}
/* }}} */

/* {{{ MADB_FixColumnDataTypes */
my_bool
MADB_FixColumnDataTypes(MADB_Stmt *Stmt, MADB_ShortTypeInfo *ColTypesArr)
{
  SQLSMALLINT     i;
  MADB_DescRecord *Record= NULL;

  if (ColTypesArr == NULL)
  {
    return 1;
  }
  for (i=0; i < Stmt->Ird->Header.Count; ++i)
  {
    if (ColTypesArr[i].SqlType != 0)
    {
      Record= MADB_DescGetInternalRecord(Stmt->Ird, i, MADB_DESC_READ);

      if (Record == NULL)
      {
        return 1;
      }
      Record->ConciseType= ColTypesArr[i].SqlType;
      Record->Nullable= ColTypesArr[i].Nullable;

      Record->Unsigned= ColTypesArr[i].Unsigned != 0 ? SQL_TRUE : SQL_FALSE;

      if (ColTypesArr[i].OctetLength > 0)
      {
        Record->OctetLength= ColTypesArr[i].OctetLength;
      }
      if (MADB_FixIrdRecord(Stmt, Record))
      {
        return 1;
      }
    }
  }

  /* If the stmt is re-executed, we should be able to fix columns again */
  Stmt->ColsTypeFixArr= ColTypesArr;
  return 0;
}
/* }}} */

void MADB_DescSetRecordDefaults(MADB_Desc *Desc, MADB_DescRecord *Record)
{
  memset(Record, 0, sizeof(MADB_DescRecord));

  switch (Desc->DescType) {
  case MADB_DESC_APD:
    Record->ConciseType= Record->Type= SQL_C_DEFAULT;
    break;
  case MADB_DESC_ARD:
    Record->ConciseType= Record->Type= SQL_C_DEFAULT;
    break;
  case MADB_DESC_IPD:
    Record->FixedPrecScale= SQL_FALSE;
    Record->LocalTypeName= "";
    Record->Nullable= SQL_NULLABLE;
    Record->ParameterType= SQL_PARAM_INPUT;
    MADB_RESET(Record->TypeName, "VARCHAR");
    Record->Unsigned= SQL_FALSE;
    Record->ColumnName= "";
    break;
  case MADB_DESC_IRD:
    Record->Nullable= SQL_NULLABLE_UNKNOWN;
    Record->FixedPrecScale= SQL_FALSE;
    Record->CaseSensitive= SQL_TRUE;
    Record->ConciseType= SQL_VARCHAR;
    Record->AutoUniqueValue= SQL_FALSE;
    Record->Type= SQL_VARCHAR;
    MADB_RESET(Record->TypeName, "VARCHAR");
    Record->Unsigned= SQL_FALSE;
    break;
  }
}
/* }}} */

/* {{{ MADB_DescGetInternalRecord */
MADB_DescRecord *MADB_DescGetInternalRecord(MADB_Desc *Desc, SQLSMALLINT RecordNumber, SQLSMALLINT Type)
{
  MADB_DescRecord *DescRecord;

  if (RecordNumber > (SQLINTEGER)Desc->Records.elements &&
      Type == MADB_DESC_READ)
  {
    MADB_SetError(&Desc->Error, MADB_ERR_07009, NULL, 0);
    return NULL;
  }

  while (RecordNumber >= (SQLINTEGER)Desc->Records.elements)
  {
    if (!(DescRecord= (MADB_DescRecord *)MADB_AllocDynamic(&Desc->Records)))
    {
      MADB_SetError(&Desc->Error, MADB_ERR_HY001, NULL, 0);
      return NULL;
    }
 
    MADB_DescSetRecordDefaults(Desc, DescRecord);
  }

  if (RecordNumber + 1 > Desc->Header.Count)
    Desc->Header.Count= (SQLSMALLINT)(RecordNumber + 1);

  DescRecord= ((MADB_DescRecord *)Desc->Records.buffer) + RecordNumber;

  return DescRecord;
}
/* }}} */

/* {{{ MADB_DescCheckFldId */
SQLRETURN MADB_DeskCheckFldId(MADB_Desc *Desc, SQLSMALLINT FieldIdentifier, SQLSMALLINT mode)
{
  int i= 0;
  
  while (MADB_DESC_FLDID[i].FieldIdentifier &&
         MADB_DESC_FLDID[i].FieldIdentifier != FieldIdentifier)
    ++i;

  /* End of list = invalid FieldIdentifier */
  if (!MADB_DESC_FLDID[i].FieldIdentifier ||
      !(MADB_DESC_FLDID[i].Access[Desc->DescType] & mode)) {
    MADB_SetError(&Desc->Error, MADB_ERR_HY091, NULL, 0);
    return SQL_ERROR;
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_DescGetField */
SQLRETURN MADB_DescGetField(SQLHDESC DescriptorHandle,
                            SQLSMALLINT RecNumber,
                            SQLSMALLINT FieldIdentifier,
                            SQLPOINTER ValuePtr,
                            SQLINTEGER BufferLength,
                            SQLINTEGER *StringLengthPtr,
                            my_bool isWChar)
{
  MADB_Desc *Desc= (MADB_Desc *)DescriptorHandle;
  MADB_DescRecord *DescRecord= NULL;
  SQLRETURN ret;
  size_t Length;

  /* Bookmark */
  if (RecNumber < 1)
  {
    /* todo */

  }

  ret= MADB_DeskCheckFldId(Desc, FieldIdentifier, MADB_DESC_READ);
  if (!SQL_SUCCEEDED(ret))
    return ret;

  MADB_CLEAR_ERROR(&Desc->Error);

  if (RecNumber)
    if (!(DescRecord= MADB_DescGetInternalRecord(Desc, RecNumber - 1, MADB_DESC_READ)))
      return SQL_ERROR;

  switch (FieldIdentifier) {
  case SQL_DESC_ALLOC_TYPE:
    *((SQLINTEGER *)ValuePtr)= Desc->Header.AllocType;
    break;
  case SQL_DESC_ARRAY_SIZE:
    *((SQLULEN *)ValuePtr)= Desc->Header.ArraySize;
    break;
  case SQL_DESC_ARRAY_STATUS_PTR:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)Desc->Header.ArrayStatusPtr;
    break;
  case SQL_DESC_BIND_OFFSET_PTR:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)Desc->Header.BindOffsetPtr;
    break;
  case SQL_DESC_BIND_TYPE:
    *((SQLULEN *)ValuePtr)= Desc->Header.BindType;
    break;
  case SQL_DESC_COUNT:
    *(SQLSMALLINT *)ValuePtr= Desc->Header.Count;
    break;
  case SQL_DESC_ROWS_PROCESSED_PTR:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)Desc->Header.RowsProcessedPtr;
    break;
  case SQL_DESC_AUTO_UNIQUE_VALUE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->AutoUniqueValue;
    break;
  case SQL_DESC_BASE_COLUMN_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->BaseColumnName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLINTEGER)Length;
    break;
  case SQL_DESC_BASE_TABLE_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->BaseTableName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLINTEGER)Length;
    break;
  case SQL_DESC_CASE_SENSITIVE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->CaseSensitive;
    break;
  case SQL_DESC_CATALOG_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->BaseCatalogName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLINTEGER)Length;
    break;
  case SQL_DESC_CONCISE_TYPE:
    *((SQLSMALLINT *)ValuePtr)= DescRecord->ConciseType;
    break;
  case SQL_DESC_DATA_PTR:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)DescRecord->DataPtr;
    break;
  case SQL_DESC_DATETIME_INTERVAL_CODE:
    *((SQLSMALLINT *)ValuePtr)= DescRecord->DateTimeIntervalCode;
    break;
  case SQL_DESC_DATETIME_INTERVAL_PRECISION:
    *((SQLINTEGER *)ValuePtr)= DescRecord->DateTimeIntervalPrecision;
    break;
  case SQL_DESC_FIXED_PREC_SCALE:
    *((SQLSMALLINT *)ValuePtr)= DescRecord->FixedPrecScale;
    break;
  case SQL_DESC_INDICATOR_PTR:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)DescRecord->IndicatorPtr;
    break;
  case SQL_DESC_LENGTH:
   *((SQLINTEGER *)ValuePtr)= DescRecord->DescLength;
    break;
  case SQL_DESC_LITERAL_PREFIX:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)DescRecord->LiteralPrefix;
    break;
  case SQL_DESC_LITERAL_SUFFIX:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)DescRecord->LiteralSuffix;
    break;
  case SQL_DESC_LOCAL_TYPE_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->LocalTypeName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLINTEGER)Length;
    break;
  case SQL_DESC_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->BaseColumnName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLINTEGER)Length;
    DescRecord->Unnamed= SQL_NAMED;
    break;
  case SQL_DESC_NULLABLE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Nullable;
    break;
  case SQL_DESC_NUM_PREC_RADIX:
    *((SQLINTEGER *)ValuePtr)= DescRecord->NumPrecRadix;
    break;
  case SQL_DESC_OCTET_LENGTH:
    *((SQLLEN *)ValuePtr)= DescRecord->OctetLength;
    break;
  case SQL_DESC_OCTET_LENGTH_PTR:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)DescRecord->OctetLengthPtr;
    break;
  case SQL_DESC_PARAMETER_TYPE:
    *((SQLSMALLINT *)ValuePtr)= DescRecord->ParameterType;
    break;
  case SQL_DESC_PRECISION:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Precision;
    break;
#if (ODBCVER >= 0x0350)
  case SQL_DESC_ROWVER:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)(SQLULEN)DescRecord->RowVer;
    break;
#endif
  case SQL_DESC_SCALE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Scale;
    break;
  case SQL_DESC_SCHEMA_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->SchemaName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLINTEGER)Length;
    break;
  case SQL_DESC_SEARCHABLE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Searchable;
    break;
  case SQL_DESC_TABLE_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->TableName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLINTEGER)Length;
    break;
  case SQL_DESC_TYPE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Type;
    break;
  case SQL_DESC_TYPE_NAME:
    *StringLengthPtr= (SQLINTEGER)MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->TypeName, SQL_NTS, &Desc->Error);
     break;
  case SQL_DESC_UNSIGNED:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Unsigned;
    break;
  case SQL_DESC_UPDATABLE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Updateable;
    break;
  }
  return ret;
}

/* {{{ MADB_DescSetField */
SQLRETURN MADB_DescSetField(SQLHDESC DescriptorHandle,
                            SQLSMALLINT RecNumber,
                            SQLSMALLINT FieldIdentifier,
                            SQLPOINTER ValuePtr,
                            SQLINTEGER BufferLength,
                            my_bool isWChar)
{
  MADB_Desc *Desc= (MADB_Desc *)DescriptorHandle;
  MADB_DescRecord *DescRecord= NULL;
  SQLRETURN ret;
  SQL_UNNAMED;
  ret= MADB_DeskCheckFldId(Desc, FieldIdentifier, MADB_DESC_WRITE);

  /* Application may set IPD's field SQL_DESC_UNNAMED to SQL_UNNAMED only */
  if (FieldIdentifier == SQL_DESC_UNNAMED && (SQLSMALLINT)(SQLULEN)ValuePtr == SQL_NAMED)
  {
    MADB_SetError(&Desc->Error, MADB_ERR_HY092, NULL, 0);
    ret= Desc->Error.ReturnValue;
  }

  if (!SQL_SUCCEEDED(ret))
    return ret;

  MADB_CLEAR_ERROR(&Desc->Error);
  switch (FieldIdentifier) {
  case SQL_DESC_ARRAY_SIZE:
    Desc->Header.ArraySize= (SQLULEN)ValuePtr;
    return SQL_SUCCESS;
  case SQL_DESC_ARRAY_STATUS_PTR:
    Desc->Header.ArrayStatusPtr= (SQLUSMALLINT *)ValuePtr;
    return SQL_SUCCESS;
  case SQL_DESC_BIND_OFFSET_PTR:
    Desc->Header.BindOffsetPtr= (SQLULEN *)ValuePtr;
    return SQL_SUCCESS;
  case SQL_DESC_BIND_TYPE:
    Desc->Header.BindType= (SQLINTEGER)(SQLLEN)ValuePtr;
    return SQL_SUCCESS;
  case SQL_DESC_COUNT:
    Desc->Header.Count= (SQLSMALLINT)(SQLLEN)ValuePtr;
    return SQL_SUCCESS;
  case SQL_DESC_ROWS_PROCESSED_PTR:
    Desc->Header.RowsProcessedPtr= (SQLULEN *)ValuePtr;
    return SQL_SUCCESS;
  }

  if (RecNumber > 0)
  {
    if (!(DescRecord= MADB_DescGetInternalRecord(Desc, RecNumber - 1, MADB_DESC_WRITE)))
      return SQL_ERROR;

    switch (FieldIdentifier) {
    case SQL_DESC_CONCISE_TYPE:
      DescRecord->ConciseType= (SQLSMALLINT)(SQLLEN)ValuePtr;
      DescRecord->Type= MADB_GetTypeFromConciseType(DescRecord->ConciseType);
      if (DescRecord->Type == SQL_INTERVAL)
      {
        DescRecord->DateTimeIntervalCode= DescRecord->ConciseType - 100;
      }
      break;
    case SQL_DESC_DATA_PTR:
      DescRecord->DataPtr= ValuePtr;
      break;
    case SQL_DESC_DATETIME_INTERVAL_CODE:
      DescRecord->DateTimeIntervalCode= (SQLSMALLINT)(SQLLEN)ValuePtr;
      break;
    case SQL_DESC_DATETIME_INTERVAL_PRECISION:
      DescRecord->DateTimeIntervalPrecision= (SQLINTEGER)(SQLLEN)ValuePtr;
      break;
    case SQL_DESC_FIXED_PREC_SCALE:
      DescRecord->FixedPrecScale= (SQLSMALLINT)(SQLLEN)ValuePtr;
      break;
    case SQL_DESC_INDICATOR_PTR:
      DescRecord->IndicatorPtr= (SQLLEN *)ValuePtr;
      break;
    case SQL_DESC_LENGTH:
      DescRecord->DescLength= (SQLINTEGER)(SQLLEN)ValuePtr;
      break;
    case SQL_DESC_NUM_PREC_RADIX:
      DescRecord->NumPrecRadix= (SQLINTEGER)(SQLLEN)ValuePtr;
      break;
    case SQL_DESC_OCTET_LENGTH:
      DescRecord->OctetLength= (SQLLEN)ValuePtr;
      break;
    case SQL_DESC_OCTET_LENGTH_PTR:
      DescRecord->OctetLengthPtr= (SQLLEN *)ValuePtr;
      break;
    case SQL_DESC_PARAMETER_TYPE:
      DescRecord->ParameterType= (SQLSMALLINT)(SQLLEN)ValuePtr;
      break;
    case SQL_DESC_PRECISION:
      DescRecord->Precision= (SQLSMALLINT)(SQLLEN)ValuePtr;
      break;
    case SQL_DESC_SCALE:
      DescRecord->Scale= (SQLSMALLINT)(SQLLEN)ValuePtr;
      break;
    case SQL_DESC_TYPE:
      DescRecord->Type= (SQLSMALLINT)(SQLLEN)ValuePtr;
      DescRecord->ConciseType= DescRecord->Type;
      break;
    }
    /* bug41018 (ma_desc.c):
     We need to unbind in case parameter doesn't set a buffer or header field */
    switch (FieldIdentifier)
    {
    case SQL_DESC_DATA_PTR:
    case SQL_DESC_OCTET_LENGTH_PTR:
    case SQL_DESC_INDICATOR_PTR:
      break;
    default:
      if (Desc->DescType== MADB_DESC_ARD && DescRecord && DescRecord->DataPtr)
        DescRecord->DataPtr= NULL;
      break;
    }
  
    /* inUse is only used to check if column/parameter was bound or not. Thus we do not set it for each field, but only for those,
       that make column/parameter "bound" */
    if (DescRecord && (DescRecord->DataPtr != NULL || DescRecord->OctetLengthPtr != NULL || DescRecord->IndicatorPtr != NULL))
      DescRecord->inUse= 1;
  }
  return ret;
}
/* }}} */

/* {{{ MADB_DescCopyDesc */
SQLRETURN MADB_DescCopyDesc(MADB_Desc *SrcDesc, MADB_Desc *DestDesc)
{
  if (!SrcDesc)
    return SQL_INVALID_HANDLE;
    
  if (DestDesc->DescType == MADB_DESC_IRD)
  {
    MADB_SetError(&DestDesc->Error, MADB_ERR_HY016, NULL, 0);
    return SQL_ERROR;
  }
  if (SrcDesc->DescType == MADB_DESC_IRD && !SrcDesc->Header.Count)
  {
    MADB_SetError(&DestDesc->Error, MADB_ERR_HY007, NULL, 0);
    return SQL_ERROR;
  }
  /* make sure there aren't old records */
  MADB_DeleteDynamic(&DestDesc->Records);
  if (MADB_InitDynamicArray(&DestDesc->Records, sizeof(MADB_DescRecord),
                            SrcDesc->Records.max_element, SrcDesc->Records.alloc_increment))
  {
    MADB_SetError(&DestDesc->Error, MADB_ERR_HY001, NULL, 0);
    return SQL_ERROR;
  }

  memcpy(&DestDesc->Header, &SrcDesc->Header, sizeof(MADB_Header));
  
  /* We don't copy AppType from Src to Dest. If we copy internal descriptor to the explicit/external, it stays explicit/external */

  DestDesc->DescType= SrcDesc->DescType;
  memcpy(&DestDesc->Error, &SrcDesc->Error, sizeof(MADB_Error));

  /* Since we never allocate pointers we can just copy content */
  memcpy(DestDesc->Records.buffer, SrcDesc->Records.buffer,
         SrcDesc->Records.size_of_element * SrcDesc->Records.max_element);
  DestDesc->Records.elements= SrcDesc->Records.elements;

  /* internal buffer needs to be clearead or we will get it freed twice with all nice subsequences */
  {
    unsigned int i;

    for (i= 0; i < DestDesc->Records.elements; ++i)
    {
      MADB_DescRecord *Rec= MADB_DescGetInternalRecord(DestDesc, i, MADB_DESC_READ);

      if (Rec != NULL)
      {
        Rec->InternalBuffer= NULL;
      }
    }
  }

  return SQL_SUCCESS;
}
/* }}} */

SQLRETURN MADB_DescGetRec(MADB_Desc *Desc,
    SQLSMALLINT RecNumber,
    SQLCHAR *Name,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLSMALLINT *TypePtr,
    SQLSMALLINT *SubTypePtr,
    SQLLEN *LengthPtr,
    SQLSMALLINT *PrecisionPtr,
    SQLSMALLINT *ScalePtr,
    SQLSMALLINT *NullablePtr,
    BOOL isWChar)
{
  MADB_DescRecord *Record;
  SQLLEN Length;

  MADB_CLEAR_ERROR(&Desc->Error);

  if (!(Record= MADB_DescGetInternalRecord(Desc, RecNumber, MADB_DESC_READ)))
  {
    MADB_SetError(&Desc->Error, MADB_ERR_07009, NULL, 0);
    return Desc->Error.ReturnValue;
  }
  
  /* SQL_DESC_NAME */
  Length= MADB_SetString(isWChar ? &utf8 : 0, Name, BufferLength, Record->BaseColumnName, SQL_NTS, &Desc->Error);
  if (StringLengthPtr)
    *StringLengthPtr= (SQLSMALLINT)Length;
  Record->Unnamed= SQL_NAMED;

  /* SQL_DESC_TYPE */
  *(SQLSMALLINT *)TypePtr= (SQLSMALLINT)Record->Type;

  /* SQL_DESC_DATETIME_INTERVAL_CODE */
  *(SQLSMALLINT *)SubTypePtr= Record->DateTimeIntervalCode;

  /* SQL_DESC_OCTET_LENGTH */
  *(SQLLEN *)LengthPtr= (SQLLEN)Record->OctetLength;

  /* SQL_DESC_PRECISION */
  *(SQLSMALLINT *)PrecisionPtr= (SQLSMALLINT)Record->Precision;

  /* SQL_DESC_SCALE */
  *(SQLSMALLINT *)ScalePtr= (SQLSMALLINT)Record->Scale;

  /* SQL_DESC_NULLABLE */
  *(SQLSMALLINT *)NullablePtr= (SQLSMALLINT)Record->Nullable;

  return SQL_SUCCESS;
}

