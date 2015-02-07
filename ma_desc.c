/************************************************************************************
   Copyright (C) 2013,2015 MariaDB Corporation AB
   
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
  {SQL_DESC_PARAMETER_TYPE,{MADB_DESC_NONE, MADB_DESC_NONE, MADB_DESC_RW, MADB_DESC_RW}},
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
MADB_Desc *MADB_DescInit(enum enum_madb_desc_type DescType, my_bool isExternal)
{
  MADB_Desc *Desc;
  
  if (!(Desc= (MADB_Desc *)MADB_CALLOC(sizeof(MADB_Desc))))
    return NULL;

  Desc->DescType= DescType;

  if (my_init_dynamic_array(&Desc->Records, sizeof(MADB_DescRecord), 0, 0))
  {
    MADB_FREE(Desc);
    Desc= NULL;
  }
  if (isExternal && my_init_dynamic_array(&Desc->Stmts, sizeof(MADB_Stmt**), 0, 0))
  {
    MADB_DescFree(Desc, FALSE);
    Desc= NULL;
  }
  if (Desc)
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
  }
  delete_dynamic(&Desc->Records);
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
  delete_dynamic(&Desc->Stmts);
  if (!RecordsOnly)
    MADB_FREE(Desc);
  return SQL_SUCCESS;
}
/* }}} */ 

/* {{{ MADB_DescSetIrdMetadata */
my_bool 
MADB_DescSetIrdMetadata(MADB_Stmt *Stmt, MYSQL_FIELD *Fields, unsigned int NumFields)
{
  MADB_Desc *Desc= Stmt->Ird;
  MADB_DescRecord *Record;
  SQLUINTEGER i;

  for (i=0; i < NumFields; i++)
  {
    if (!(Record= MADB_DescGetInternalRecord(Stmt->Ird, i, MADB_DESC_WRITE)))
      return 1;
    
    Record->CatalogName= Fields[i].catalog ? my_strdup(Fields[i].catalog, MYF(0)) : NULL;
    Record->TableName= Fields[i].table ? my_strdup(Fields[i].table, MYF(0)) : NULL;
    Record->ColumnName= Fields[i].name ? my_strdup(Fields[i].name, MYF(0)) : NULL;
    Record->BaseTableName= Fields[i].org_table ? my_strdup(Fields[i].org_table, MYF(0)) : NULL;
    Record->BaseColumnName= Fields[i].org_name ? my_strdup(Fields[i].org_name, MYF(0)) : NULL;
    Record->AutoUniqueValue= (Fields[i].flags & AUTO_INCREMENT_FLAG) ? SQL_TRUE : SQL_FALSE;
    Record->CaseSensitive= (Fields[i].flags & BINARY_FLAG) ? SQL_TRUE : SQL_FALSE;
    Record->Nullable= ( (Fields[i].flags & NOT_NULL_FLAG) &&
                        !Record->AutoUniqueValue &&
                        Fields[i].type != MYSQL_TYPE_TIMESTAMP) ? SQL_NO_NULLS : SQL_NULLABLE;
    Record->Unsigned= (Fields[i].flags & UNSIGNED_FLAG) ? SQL_TRUE : SQL_FALSE;
    /* We assume it might be updatable if tablename exists */
    Record->Updateable= (Fields[i].table && Fields[i].table[0]) ? SQL_ATTR_READWRITE_UNKNOWN : SQL_ATTR_READONLY;
    /* 
       SEARCHABLE:
       Columns of type SQL_LONGVARCHAR and SQL_LONGVARBINARY usually return SQL_PRED_CHAR.
    */
    Record->Searchable= (Record->ConciseType == SQL_LONGVARCHAR ||
                         Record->ConciseType == SQL_WLONGVARCHAR ||
                         Record->ConciseType == SQL_LONGVARBINARY) ? SQL_PRED_CHAR : SQL_SEARCHABLE;
    /*
       RADIX:
       If the data type in the SQL_DESC_TYPE field is an approximate numeric data type, this SQLINTEGER field 
       contains a value of 2 because the SQL_DESC_PRECISION field contains the number of bits. 
       If the data type in the SQL_DESC_TYPE field is an exact numeric data type, this field contains a value of 10 
       because the SQL_DESC_PRECISION field contains the number of decimal digits. 
       This field is set to 0 for all non-numeric data types.
   */
    switch (Fields[i].type) {
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
      Record->FixedPrecScale= SQL_FALSE;
      Record->NumPrecRadix= 10;
      Record->Precision= (SQLSMALLINT)Fields[i].length - 2;
      Record->Scale= Fields[i].decimals;
      break;
    case MYSQL_TYPE_FLOAT:
      Record->FixedPrecScale= SQL_FALSE;
      Record->NumPrecRadix= 2;
      Record->Precision= (SQLSMALLINT)Fields[i].length - 2;
      //Record->Scale= Fields[i].decimals;
      break;
    case MYSQL_TYPE_BIT:
      if (Fields[i].length > 1)
        Record->Type= SQL_BINARY;
      break;
    case MYSQL_TYPE_DOUBLE:
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_YEAR:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_LONGLONG:
      Record->NumPrecRadix= 10;
      break;
    default:
      Record->NumPrecRadix= 0;
      break;
    }
    Record->ConciseType= MADB_GetODBCType(&Fields[i]);
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
    Record->DisplaySize= MADB_GetDisplaySize(Fields[i]);
    Record->OctetLength= MADB_GetOctetLength(Fields[i], Stmt->Connection->mariadb->charset->char_maxlen);
    Record->Length= MADB_GetDataSize(Record, Fields[i]);
    
    Record->TypeName= my_strdup(MADB_GetTypeName(Fields[i]), MYF(0));
    switch(Fields[i].type) {
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_VAR_STRING:
      if (Fields[i].flags & BINARY_FLAG)
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
  }
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
    Record->FixedPrecScale= SQL_TRUE;
    Record->LocalTypeName= "";
    Record->Nullable= SQL_NULLABLE;
    Record->ParameterType= SQL_PARAM_INPUT;
    Record->TypeName= "VARCHAR";
    Record->Unsigned= SQL_FALSE;
    Record->ColumnName= "";
    break;
  case MADB_DESC_IRD:
    Record->Nullable= SQL_NULLABLE_UNKNOWN;
    Record->FixedPrecScale= SQL_TRUE;
    Record->CaseSensitive= SQL_TRUE;
    Record->ConciseType= SQL_VARCHAR;
    Record->AutoUniqueValue= SQL_FALSE;
    Record->Type= SQL_VARCHAR;
    Record->TypeName= "VARCHAR";
    Record->Unsigned= SQL_FALSE;
    break;
  }
}
/* }}} */

/* {{{ MADB_DescGetInternalRecord */
MADB_DescRecord *MADB_DescGetInternalRecord(MADB_Desc *Desc, SQLINTEGER RecordNumber, SQLSMALLINT Type)
{
  MADB_DescRecord *DescRecord;
  SQLINTEGER Start= Desc->Records.elements;

  if (RecordNumber > (SQLINTEGER)Desc->Records.elements &&
      Type == MADB_DESC_READ)
  {
    MADB_SetError(&Desc->Error, MADB_ERR_07009, NULL, 0);
    return NULL;
  }

  while (RecordNumber >= (SQLINTEGER)Desc->Records.elements)
  {
    if (!(DescRecord= (MADB_DescRecord *)alloc_dynamic(&Desc->Records)))
    {
      MADB_SetError(&Desc->Error, MADB_ERR_HY001, NULL, 0);
      return NULL;
    }
 
    MADB_DescSetRecordDefaults(Desc, DescRecord);
  }

  if (RecordNumber + 1 > Desc->Header.Count)
    Desc->Header.Count= RecordNumber + 1;
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
    *((SQLINTEGER *)ValuePtr)= Desc->Header.BindType;
    break;
  case SQL_DESC_COUNT:
    *(SQLINTEGER *)ValuePtr= Desc->Header.Count;
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
      *StringLengthPtr= Length;
    break;
  case SQL_DESC_BASE_TABLE_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->BaseTableName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= Length;
    break;
  case SQL_DESC_CASE_SENSITIVE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->CaseSensitive;
    break;
  case SQL_DESC_CATALOG_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->BaseCatalogName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= Length;
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
    *((SQLINTEGER *)ValuePtr)= DescRecord->FixedPrecScale;
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
      *StringLengthPtr= Length;
    break;
  case SQL_DESC_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->BaseColumnName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= Length;
    DescRecord->Unnamed= SQL_NAMED;
    break;
  case SQL_DESC_NULLABLE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Nullable;
    break;
  case SQL_DESC_NUM_PREC_RADIX:
    *((SQLINTEGER *)ValuePtr)= DescRecord->NumPrecRadix;
    break;
  case SQL_DESC_OCTET_LENGTH:
    *((SQLINTEGER *)ValuePtr)= DescRecord->OctetLength;
    break;
  case SQL_DESC_OCTET_LENGTH_PTR:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)DescRecord->OctetLengthPtr;
    break;
  case SQL_DESC_PARAMETER_TYPE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->ParameterType;
    break;
  case SQL_DESC_PRECISION:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Precision;
    break;
#if (ODBCVER >= 0x0350)
  case SQL_DESC_ROWVER:
    *((SQLPOINTER *)ValuePtr)= (SQLPOINTER)DescRecord->RowVer;
    break;
#endif
  case SQL_DESC_SCALE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Scale;
    break;
  case SQL_DESC_SCHEMA_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->SchemaName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= Length;
    break;
  case SQL_DESC_SEARCHABLE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Searchable;
    break;
  case SQL_DESC_TABLE_NAME:
    Length= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->TableName, SQL_NTS, &Desc->Error);
    if (StringLengthPtr)
      *StringLengthPtr= Length;
    break;
  case SQL_DESC_TYPE:
    *((SQLINTEGER *)ValuePtr)= DescRecord->Type;
    break;
  case SQL_DESC_TYPE_NAME:
    *StringLengthPtr= MADB_SetString(isWChar ? &utf8 : 0, ValuePtr, BufferLength, DescRecord->TypeName, SQL_NTS, &Desc->Error);
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
  if (FieldIdentifier == SQL_DESC_UNNAMED && (SQLSMALLINT)ValuePtr == SQL_NAMED)
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
    Desc->Header.Count= (SQLLEN)ValuePtr;
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
  
    if (DescRecord)
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
  if (!SrcDesc->Header.Count)
  {
    MADB_SetError(&DestDesc->Error, MADB_ERR_HY007, NULL, 0);
    return SQL_ERROR;
  }
  /* make sure there aren't old records */
  delete_dynamic(&DestDesc->Records);
  if (my_init_dynamic_array(&DestDesc->Records, sizeof(MADB_DescRecord),
                            SrcDesc->Records.elements, SrcDesc->Records.alloc_increment))
  {
    MADB_SetError(&DestDesc->Error, MADB_ERR_HY001, NULL, 0);
    return SQL_ERROR;
  }

  memcpy(&DestDesc->Header, &SrcDesc->Header, sizeof(MADB_Header));
  DestDesc->AppType= SrcDesc->AppType;
  DestDesc->DescType= SrcDesc->DescType;
  memcpy(&DestDesc->Error, &SrcDesc->Error, sizeof(MADB_Error));

  /* Since we never allocate pointers we can just copy content */
  memcpy(DestDesc->Records.buffer, SrcDesc->Records.buffer,
         SrcDesc->Records.size_of_element * SrcDesc->Records.max_element);
  /* todo: internal buffer needs to be clearead or we need to move it outside of
           record structure 
  */
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

