/************************************************************************************
   Copyright (C) 2017 MariaDB Corporation AB
   
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

/* ODBC C->SQL and SQL->C type conversion functions */

#include <ma_odbc.h>


/* {{{ MADB_ConversionSupported */
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
  else if (EndPtr != NULL && *EndPtr != '\0')
  {
    /* 22018. TODO: check if condition is correct */
  }

  return asNumber != 0 ? '\1' : '\0';
}
/* }}} */

/* {{{ MADB_ConvertNumericToChar */
size_t MADB_ConvertNumericToChar(SQL_NUMERIC_STRUCT *Numeric, char *Buffer, int *ErrorCode)
{
  long long Numerator= 0;
  long long Denominator= 1;
  unsigned long long Left= 0, Right= 0;
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
  Denominator= (long long)pow(10, Scale);
  Left= Numerator / Denominator;
  //_i64toa_s(Numerator, Buffer, 38, 10);
  if (Numeric->scale > 0)
  {
    char tmp[38];
    _snprintf(tmp, 38, "%%%d.%df", Numeric->precision, Numeric->scale);
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

/* {{{ MADB_ConvertNullValue */
SQLRETURN MADB_ConvertNullValue(MADB_Stmt *Stmt, MYSQL_BIND *MaBind)
{
  MaBind->buffer_type=  MYSQL_TYPE_NULL;
  MaBind->buffer_length= 0;

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_ProcessIndicator */
/* Returns TRUE if indicator contains some special value, and thus no further type conversion is needed */
BOOL MADB_ProcessIndicator(MADB_Stmt *Stmt, SQLLEN Indicator, char * DefaultValue, MYSQL_BIND *MaBind)
{
  switch (Indicator)
  {
  case SQL_COLUMN_IGNORE:
    if (DefaultValue == NULL)
    {
      MADB_ConvertNullValue(Stmt, MaBind);
    }
    else
    {
      MaBind->buffer=       DefaultValue;
      MaBind->buffer_length= (unsigned long)strlen(DefaultValue);
      MaBind->buffer_type=  MYSQL_TYPE_STRING;
    }
    return TRUE;
  case SQL_NULL_DATA:
    MADB_ConvertNullValue(Stmt, MaBind);
    return TRUE;
  }

  return FALSE;
}
/* }}} */

/* {{{ MADB_CalculateLength */
SQLLEN MADB_CalculateLength(MADB_Stmt *Stmt, SQLLEN *OctetLengthPtr, MADB_DescRecord *CRec, void* DataPtr)
{
  /* If no OctetLengthPtr was specified, or OctetLengthPtr is SQL_NTS character
     are considered to be NULL binary data are null terminated */
  if (!OctetLengthPtr || *OctetLengthPtr == SQL_NTS)
  {
    /* Meaning of Buffer Length is not quite clear in specs. Thus we treat in the way, that does not break
        (old) testcases. i.e. we neglect its value if Length Ptr is specified */
    SQLLEN BufferLen= OctetLengthPtr ? -1 : CRec->OctetLength;

    switch (CRec->ConciseType)
    {
    case SQL_C_WCHAR:
      /* CRec->OctetLength eq 0 means not 0-length buffer, but that this value is not specified. Thus -1, for SqlwcsLen
          and SafeStrlen that means buffer len is not specified */
      return SqlwcsLen((SQLWCHAR *)DataPtr, BufferLen/sizeof(SQLWCHAR) - test(BufferLen == 0)) * sizeof(SQLWCHAR);
      break;
    case SQL_C_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:
    case SQL_C_CHAR:
      return SafeStrlen((SQLCHAR *)DataPtr, BufferLen != 0 ? BufferLen : -1);
    }
  }
  else
  {
    return *OctetLengthPtr;
  }

  return CRec->OctetLength;
}
/* }}} */

/* {{{ MADB_GetBufferForSqlValue */
void* MADB_GetBufferForSqlValue(MADB_Stmt *Stmt, MADB_DescRecord *CRec, size_t Size)
{
  if (Stmt->RebindParams || CRec->InternalBuffer == NULL)
  {
    MADB_FREE(CRec->InternalBuffer);
    CRec->InternalBuffer= MADB_CALLOC(Size);
    if (CRec->InternalBuffer == NULL)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
      return NULL;
    }
  }

  return (void *)CRec->InternalBuffer;
}
/* }}} */

/* {{{ MADB_Wchar2Sql */
SQLRETURN MADB_Wchar2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  SQLULEN mbLength=0;

  MADB_FREE(CRec->InternalBuffer);

  CRec->InternalBuffer= MADB_ConvertFromWChar((SQLWCHAR *)DataPtr, (SQLINTEGER)(Length / sizeof(SQLWCHAR)),
    &mbLength, &Stmt->Connection->charset, NULL);

  if (CRec->InternalBuffer == NULL)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }

  *LengthPtr= (unsigned long)mbLength;
  *Buffer= CRec->InternalBuffer;

  MaBind->buffer_type=  MYSQL_TYPE_STRING;

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Char2Sql */
SQLRETURN MADB_Char2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  if (SqlRec->ConciseType == SQL_BIT)
  {
    if (*Buffer == NULL)
    {
      CRec->InternalBuffer= (char *)MADB_GetBufferForSqlValue(Stmt, CRec, MaBind->buffer_length);

      if (CRec->InternalBuffer == NULL)
      {
        return Stmt->Error.ReturnValue;
      }
      *Buffer= CRec->InternalBuffer;
    }
    

    *LengthPtr= 1;
    **(char**)Buffer= MADB_ConvertCharToBit(Stmt, DataPtr);
    MaBind->buffer_type= MYSQL_TYPE_TINY;
  }
  else
  {
    /* Bulk shouldn't get here, thus logic for single paramset execution */
    *LengthPtr= (unsigned long)Length;
    *Buffer= DataPtr;
    MaBind->buffer_type= MYSQL_TYPE_STRING;
  }

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Numeric2Sql */
SQLRETURN MADB_Numeric2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  SQL_NUMERIC_STRUCT *p;
  int ErrorCode= 0;

  /* We might need to preserve this pointer to be able to later release the memory */
  CRec->InternalBuffer= (char *)MADB_GetBufferForSqlValue(Stmt, CRec, MADB_CHARSIZE_FOR_NUMERIC);

  if (CRec->InternalBuffer == NULL)
  {
    return Stmt->Error.ReturnValue;
  }

  p= (SQL_NUMERIC_STRUCT *)DataPtr;
  p->scale= (SQLSCHAR)SqlRec->Scale;
  p->precision= (SQLSCHAR)SqlRec->Precision;

  *LengthPtr= (unsigned long)MADB_ConvertNumericToChar((SQL_NUMERIC_STRUCT *)p, CRec->InternalBuffer, &ErrorCode);;
  *Buffer= CRec->InternalBuffer;

  MaBind->buffer_type= MYSQL_TYPE_STRING;

  if (ErrorCode)
  {
    /*TODO: I guess this parameters row should be skipped */
    return MADB_SetError(&Stmt->Error, ErrorCode, NULL, 0);
  }

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_TsConversionIsPossible */
SQLRETURN MADB_TsConversionIsPossible(SQL_TIMESTAMP_STRUCT *ts, SQLSMALLINT SqlType, MADB_Error *Error)
{
  switch (SqlType)
  {
  case SQL_TYPE_TIME:
    if (ts->fraction)
    {
      return MADB_SetError(Error, MADB_ERR_22008, NULL, 0);
    }
    break;
  case SQL_TYPE_DATE:
    if (ts->hour + ts->minute + ts->second + ts->fraction)
    {
      return MADB_SetError(Error, MADB_ERR_22008, NULL, 0);
    }
  default:
    /* This only would be good for SQL_TYPE_TIME */
    if (ts->year == 0 || ts->month == 0 || ts->day == 0)
    {
      return MADB_SetError(Error, MADB_ERR_22007, NULL, 0);
    }
  }
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Timestamp2Sql */
SQLRETURN MADB_Timestamp2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  MYSQL_TIME           *tm= NULL;
  SQL_TIMESTAMP_STRUCT *ts= (SQL_TIMESTAMP_STRUCT *)DataPtr;

  RETURN_ERROR_OR_CONTINUE(MADB_TsConversionIsPossible(ts, SqlRec->ConciseType, &Stmt->Error));

  if (*Buffer == NULL)
  {
    tm= (MYSQL_TIME*)MADB_GetBufferForSqlValue(Stmt, CRec, sizeof(MYSQL_TIME));
    if (tm == NULL)
    {
      /* Error is set in function responsible for allocation */
      return Stmt->Error.ReturnValue;
    }
    *Buffer= tm;
  }
  else
  {
    tm= *Buffer;
  }
  

  /* Default types. Not quite clear if time_type has any effect */
  tm->time_type=       MYSQL_TIMESTAMP_DATETIME;
  MaBind->buffer_type= MYSQL_TYPE_TIMESTAMP;

  switch (SqlRec->ConciseType) {
  case SQL_TYPE_DATE:
    MaBind->buffer_type= MYSQL_TYPE_DATE;
    tm->time_type=       MYSQL_TIMESTAMP_DATE;
    break;
  case SQL_TYPE_TIME:
    MaBind->buffer_type= MYSQL_TYPE_TIME;
    tm->time_type=       MYSQL_TIMESTAMP_TIME;
    break;
  }

  tm->year=  ts->year;
  tm->month= ts->month;
  tm->day=   ts->day;

  tm->hour=   ts->hour;
  tm->minute= ts->minute;
  tm->second= ts->second;

  tm->second_part= ts->fraction / 1000;

  *LengthPtr= sizeof(MYSQL_TIME);

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Time2Sql */
SQLRETURN MADB_Time2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  MYSQL_TIME      *tm= NULL;
  SQL_TIME_STRUCT *ts= (SQL_TIME_STRUCT *)DataPtr;

  if ((SqlRec->ConciseType == SQL_TYPE_TIME || SqlRec->ConciseType == SQL_TYPE_TIMESTAMP ||
    SqlRec->ConciseType == SQL_TIME || SqlRec->ConciseType == SQL_TIMESTAMP || SqlRec->ConciseType == SQL_DATETIME) &&
    ts->hour > 23|| ts->minute > 59 || ts->second > 59)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_22007, NULL, 0);
  }

  if (*Buffer == NULL)
  {
    tm= (MYSQL_TIME*)MADB_GetBufferForSqlValue(Stmt, CRec, sizeof(MYSQL_TIME));
    if (tm == NULL)
    {
      /* Error is set in function responsible for allocation */
      return Stmt->Error.ReturnValue;
    }
    *Buffer= tm;
  }
  else
  {
    tm= *Buffer;
  }

  tm->year=  1970;
  tm->month= 1;
  tm->day=   1;

  tm->hour=   ts->hour;
  tm->minute= ts->minute;
  tm->second= ts->second;

  tm->second_part= 0;

  tm->time_type= MYSQL_TIMESTAMP_DATETIME;

  MaBind->buffer_type= MYSQL_TYPE_DATETIME;
  *LengthPtr= sizeof(MYSQL_TIME);

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_IntervalHtoMS2Sql */
SQLRETURN MADB_IntervalHtoMS2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  MYSQL_TIME          *tm= NULL;
  SQL_INTERVAL_STRUCT *is= (SQL_INTERVAL_STRUCT *)DataPtr;

  if (*Buffer == NULL)
  {
    tm= (MYSQL_TIME*)MADB_GetBufferForSqlValue(Stmt, CRec, sizeof(MYSQL_TIME));
    if (tm == NULL)
    {
      /* Error is set in function responsible for allocation */
      return Stmt->Error.ReturnValue;
    }
    *Buffer= tm;
  }
  else
  {
    tm= *Buffer;
  }

  tm->hour=   is->intval.day_second.hour;
  tm->minute= is->intval.day_second.minute;
  tm->second= CRec->ConciseType == SQL_C_INTERVAL_HOUR_TO_SECOND ? is->intval.day_second.second : 0;

  tm->second_part= 0;

  tm->time_type= MYSQL_TIMESTAMP_TIME;
  MaBind->buffer_type= MYSQL_TYPE_TIME;
  *LengthPtr= sizeof(MYSQL_TIME);

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_Date2Sql */
SQLRETURN MADB_Date2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
  MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  MYSQL_TIME      *tm= NULL, **BuffPtr= (MYSQL_TIME**)Buffer;
  SQL_DATE_STRUCT *ts= (SQL_DATE_STRUCT *)DataPtr;

  if (*BuffPtr == NULL)
  {
    tm= (MYSQL_TIME*)MADB_GetBufferForSqlValue(Stmt, CRec, sizeof(MYSQL_TIME));
    if (tm == NULL)
    {
      /* Error is set in function responsible for allocation */
      return Stmt->Error.ReturnValue;
    }
    *BuffPtr= tm;
  }
  else
  {
    tm= *BuffPtr;
  }

  tm->year=  ts->year;
  tm->month= ts->month;
  tm->day=   ts->day;

  tm->hour= tm->minute= tm->second= tm->second_part= 0;
  tm->time_type= MYSQL_TIMESTAMP_DATE;

  MaBind->buffer_type= MYSQL_TYPE_DATE;
  *LengthPtr= sizeof(MYSQL_TIME);

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_ConvertC2Sql */
SQLRETURN MADB_ConvertC2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                            MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr)
{
  if (Buffer == NULL)
  {
    MaBind->buffer= NULL;
    Buffer= &MaBind->buffer;
  }
  if (LengthPtr == NULL)
  {
    LengthPtr= &MaBind->buffer_length;
  }
  /* Switch to fill BIND structures based on C and SQL type */
  switch (CRec->ConciseType)
  {
  case WCHAR_TYPES:
    RETURN_ERROR_OR_CONTINUE(MADB_Wchar2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case CHAR_BINARY_TYPES:
    RETURN_ERROR_OR_CONTINUE(MADB_Char2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_NUMERIC:
    RETURN_ERROR_OR_CONTINUE(MADB_Numeric2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_TIMESTAMP:
  case SQL_TYPE_TIMESTAMP:
    RETURN_ERROR_OR_CONTINUE(MADB_Timestamp2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_TIME:
  case SQL_TYPE_TIME:
    RETURN_ERROR_OR_CONTINUE(MADB_Time2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
    RETURN_ERROR_OR_CONTINUE(MADB_IntervalHtoMS2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  case SQL_C_DATE:
  case SQL_TYPE_DATE:
    RETURN_ERROR_OR_CONTINUE(MADB_Date2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, Buffer, LengthPtr));
    break;
  default:
    /* memset(MaBind, 0, sizeof(MYSQL_BIND));
    MaBind->buffer_length= 0; */
    MaBind->buffer_type=   0;
    MaBind->is_unsigned=   0;

    *LengthPtr= Length;
    MaBind->buffer_type= MADB_GetMaDBTypeAndLength(CRec->ConciseType,
      &MaBind->is_unsigned, &MaBind->buffer_length);

    if (!CRec->OctetLength)
    {
      CRec->OctetLength= MaBind->buffer_length;
    }
    *Buffer= DataPtr;
  }           /* End of switch (CRec->ConsiseType) */

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_C2SQL */
/* Main entrance function for C type to SQL type conversion*/
SQLRETURN MADB_C2SQL(MADB_Stmt* Stmt, MADB_DescRecord *CRec, MADB_DescRecord *SqlRec, SQLULEN ParamSetIdx, MYSQL_BIND *MaBind)
{
  SQLLEN *IndicatorPtr= NULL;
  SQLLEN *OctetLengthPtr= NULL;
  void   *DataPtr= NULL;
  SQLLEN  Length= 0;

  IndicatorPtr=   (SQLLEN *)GetBindOffset(Stmt->Apd, CRec, CRec->IndicatorPtr, ParamSetIdx, sizeof(SQLLEN));
  OctetLengthPtr= (SQLLEN *)GetBindOffset(Stmt->Apd, CRec, CRec->OctetLengthPtr, ParamSetIdx, sizeof(SQLLEN));

  if (PARAM_IS_DAE(OctetLengthPtr))
  {
    if (!DAE_DONE(Stmt))
    {
      return SQL_NEED_DATA;
    }
    else
    {
      MaBind->buffer_type= MADB_GetMaDBTypeAndLength(CRec->ConciseType, &MaBind->is_unsigned, &MaBind->buffer_length);
      /* I guess we can leave w/out this. Keeping it so far for safety */
      MaBind->long_data_used= '\1';
      return SQL_SUCCESS;
    }
  }    /* -- End of DAE parameter processing -- */

  if (IndicatorPtr && MADB_ProcessIndicator(Stmt, *IndicatorPtr, CRec->DefaultValue, MaBind))
  {
    return SQL_SUCCESS;
  }

  /* -- Special cases are done, i.e. not a DAE etc, general case -- */
 
  DataPtr= GetBindOffset(Stmt->Apd, CRec, CRec->DataPtr, ParamSetIdx, CRec->OctetLength);

  /* If indicator wasn't NULL_DATA, but data pointer is still NULL, we convert NULL value */
  if (!DataPtr)
  {
    return MADB_ConvertNullValue(Stmt, MaBind);
  }
  
  Length= MADB_CalculateLength(Stmt, OctetLengthPtr, CRec, DataPtr);

  RETURN_ERROR_OR_CONTINUE(MADB_ConvertC2Sql(Stmt, CRec, DataPtr, Length, SqlRec, MaBind, NULL, NULL));

  return SQL_SUCCESS;
}
/* }}} */
