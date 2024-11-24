/************************************************************************************
   Copyright (C) 2017,2018 MariaDB Corporation AB
   
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

/* Code allowing to deploy MariaDB bulk operation functionality.
 * i.e. adapting ODBC param arrays to MariaDB arrays */

#include "ma_odbc.h"

#include "class/ResultSetMetaData.h"
#include "class/ClientSidePreparedStatement.h"
#include "class/ServerSidePreparedStatement.h"

#define MAODBC_DATTIME_AS_PTR_ARR 1

static BOOL CanUseStructArrForDatetime(MADB_Stmt *Stmt)
{
#ifdef MAODBC_DATTIME_AS_PTR_ARR
  return FALSE;
#endif
  return TRUE;
}
char MADB_MapIndicatorValue(SQLLEN OdbcInd)
{
  switch (OdbcInd)
  {
  case SQL_NTS:           return STMT_INDICATOR_NTS;
  case SQL_COLUMN_IGNORE: return STMT_INDICATOR_IGNORE;
  case SQL_NULL_DATA:     return STMT_INDICATOR_NULL;
  case SQL_PARAM_IGNORE:  return STMT_INDICATOR_IGNORE_ROW;
    /*STMT_INDICATOR_DEFAULT*/
  }
  return '\0';
}


bool MADB_AppBufferCanBeUsed(SQLSMALLINT CType, SQLSMALLINT SqlType)
{
  switch (CType)
  {
  case CHAR_BINARY_TYPES:
    /*if (SqlType != SQL_BIT)
    {
      break;
    }*/
  case WCHAR_TYPES:
  case SQL_C_NUMERIC:
  case DATETIME_TYPES:

    return false;
  }
  return true;
}


void MADB_CleanBulkOperData(MADB_Stmt *Stmt, unsigned int ParamOffset)
{
  if (MADB_DOING_BULK_OPER(Stmt))
  {
    if (Stmt->Connection->Dsn->ParamCallbacks && Stmt->stmt->isServerSide() && !Stmt->setParamRowCallback(nullptr))
    {
      // We were doing callbacks - there is nothing to do any more
      Stmt->Bulk.ArraySize= 0;
      Stmt->Bulk.HasRowsToSkip= 0;
      return;
    }
    MADB_DescRecord *CRec;
    void            *DataPtr= nullptr;
    MYSQL_BIND      *MaBind= nullptr;
    int             i;

    for (i= ParamOffset; i < MADB_STMT_PARAM_COUNT(Stmt); ++i)
    {
      if ((CRec= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ)) != nullptr)
      {
        MaBind= &Stmt->params[i - ParamOffset];
        DataPtr= GetBindOffset(Stmt->Apd->Header, CRec->DataPtr, 0, CRec->OctetLength);

        if (MaBind->buffer != DataPtr)
        {
          switch (CRec->ConciseType)
          {
          case DATETIME_TYPES:
            if (CanUseStructArrForDatetime(Stmt) == TRUE)
            {
              MADB_FREE(MaBind->buffer);
              break;
            }
            /* Otherwise falling through and do the same as for others */
          case SQL_C_WCHAR:
          case SQL_C_NUMERIC:
          {
            unsigned int i;
            for (i= 0; i < Stmt->Bulk.ArraySize; ++i)
            {
              MADB_FREE(((char**)MaBind->buffer)[i]);
            }
          }
          /* falling through */
          default:
            MADB_FREE(MaBind->buffer);
          }
        }
        MADB_FREE(MaBind->length);
        MADB_FREE(MaBind->u.indicator);
      }
    }
    Stmt->Bulk.ArraySize= 0;
    Stmt->Bulk.HasRowsToSkip= 0;
  }
}


void MADB_InitIndicatorArray(MADB_Stmt *Stmt, MYSQL_BIND *MaBind, char InitValue)
{
  MaBind->u.indicator= static_cast<char*>(MADB_ALLOC(Stmt->Bulk.ArraySize));

  if (MaBind->u.indicator == nullptr)
  {
    MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
    throw Stmt->Error;
  }
  memset(MaBind->u.indicator, InitValue, Stmt->Bulk.ArraySize);
}


void MADB_SetBulkOperLengthArr(MADB_Stmt *Stmt, MADB_DescRecord *CRec, SQLLEN *OctetLengthPtr, SQLLEN *IndicatorPtr,
                                    void *DataPtr, MYSQL_BIND *MaBind, BOOL VariableLengthMadbType)
{
  /* Leaving it so far here commented, but it comlicates things w/out much gains */
  /*if (sizeof(SQLLEN) == sizeof(long) && MADB_AppBufferCanBeUsed())
  {
    if (OctetLengthPtr)
    {
      MaBind->length= OctetLengthPtr;
    }
  }
  else
  {*/
  unsigned int row;

  if (VariableLengthMadbType)
  {
    MaBind->length= static_cast<unsigned long*>(MADB_REALLOC(MaBind->length, Stmt->Bulk.ArraySize*sizeof(long)));
    if (MaBind->length == nullptr)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
      throw Stmt->Error;
    }
  }

  for (row= 0; row < Stmt->Apd->Header.ArraySize; ++row, DataPtr= (char*)DataPtr + CRec->OctetLength)
  {
    if (Stmt->Apd->Header.ArrayStatusPtr != nullptr && Stmt->Apd->Header.ArrayStatusPtr[row] == SQL_PARAM_IGNORE)
    {
      Stmt->Bulk.HasRowsToSkip= 1;
      continue;
    }

    if ((OctetLengthPtr != nullptr && OctetLengthPtr[row] == SQL_NULL_DATA)
      || (IndicatorPtr != nullptr && IndicatorPtr[row] != SQL_NULL_DATA))
    {
      MADB_SetIndicatorValue(Stmt, MaBind, row, SQL_NULL_DATA);
      continue;
    }
    if ((OctetLengthPtr != nullptr && OctetLengthPtr[row] == SQL_COLUMN_IGNORE)
      || (IndicatorPtr != nullptr && IndicatorPtr[row] != SQL_COLUMN_IGNORE))
    {
      MADB_SetIndicatorValue(Stmt, MaBind, row, SQL_COLUMN_IGNORE);
      continue;
    }

    if (VariableLengthMadbType)
    {
      MaBind->length[row]= (unsigned long)MADB_CalculateLength(Stmt, OctetLengthPtr != nullptr ? &OctetLengthPtr[row] : nullptr, CRec, DataPtr);
    }
  }
}
/* {{{ MADB_InitBulkOperBuffers */
/* Allocating data and length arrays, if needed, and initing them in certain cases.
   DataPtr should be ensured to be not nullptr */
void MADB_InitBulkOperBuffers(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void *DataPtr, SQLLEN *OctetLengthPtr,
                                   SQLLEN *IndicatorPtr, SQLSMALLINT SqlType, MYSQL_BIND *MaBind)
{
  BOOL VariableLengthMadbType= TRUE;

  MaBind->buffer_length= 0;
  MaBind->buffer_type= MADB_GetMaDBTypeAndLength(CRec->ConciseType, &MaBind->is_unsigned, &MaBind->buffer_length);
  /*enum enum_field_types preferredType= Stmt->stmt->getPreferredParamType(MaBind->buffer_type);

  if (preferredType != MaBind->buffer_type)
  {
    MaBind->buffer_length= 0;
  }*/

  /* For fixed length types MADB_GetMaDBTypeAndLength has set buffer_length */
  if (MaBind->buffer_length != 0)
  {
    VariableLengthMadbType= FALSE;
  }
  switch (CRec->ConciseType)
  {
  case CHAR_BINARY_TYPES:
    if (SqlType == SQL_BIT)
    {
      CRec->InternalBuffer= static_cast<char*>(MADB_CALLOC(Stmt->Bulk.ArraySize));
      MaBind->buffer_length= 1;
      break;
    }
  case DATETIME_TYPES:
    if (CanUseStructArrForDatetime(Stmt) == TRUE)
    {
      CRec->InternalBuffer= static_cast<char*>(MADB_ALLOC(Stmt->Bulk.ArraySize*sizeof(MYSQL_TIME)));
      MaBind->buffer_length= sizeof(MYSQL_TIME);
      break;
    }
    /* Otherwise falling thru and allocating array of pointers */
  case WCHAR_TYPES:
  case SQL_C_NUMERIC:
    CRec->InternalBuffer= static_cast<char*>(MADB_CALLOC(Stmt->Bulk.ArraySize*sizeof(char*)));
    MaBind->buffer_length= sizeof(char*);
    break;
  default:
    MaBind->buffer= DataPtr;
    if (MaBind->buffer_length == 0)
    {
      MaBind->buffer_length= sizeof(char*);
    }
  }

  if (MaBind->buffer != DataPtr)
  {
    MaBind->buffer= CRec->InternalBuffer;
    if (MaBind->buffer == nullptr)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
      throw Stmt->Error;
    }
    CRec->InternalBuffer= nullptr; /* Need to reset this pointer, so the memory won't be freed (accidentally) */
  }

  MADB_SetBulkOperLengthArr(Stmt, CRec, OctetLengthPtr, IndicatorPtr, DataPtr, MaBind, VariableLengthMadbType);
}
/* }}} */

/* {{{ MADB_SetIndicatorValue */
void MADB_SetIndicatorValue(MADB_Stmt *Stmt, MYSQL_BIND *MaBind, unsigned int row, SQLLEN OdbcIndicator)
{
  if (MaBind->u.indicator == nullptr)
  {
    MADB_InitIndicatorArray(Stmt, MaBind, STMT_INDICATOR_NONE);
  }
  MaBind->u.indicator[row]= MADB_MapIndicatorValue(OdbcIndicator);
}
/* }}} */

/* {{{  */
bool MADB_Stmt::setParamRowCallback(ParamCodec * callback)
{
  //TODO should do all param codecs. and the operation below should be done once when 1st param codec is added, and not "row callback"
  if (paramCodec.capacity() < stmt->getParamCount()) {
    paramCodec.reserve(stmt->getParamCount());
  }
  paramRowCallback.reset(callback);
  return stmt->setParamCallback(paramRowCallback.get());
}
/* }}} */


SQLRETURN MADB_Stmt::doBulkOldWay(uint32_t parNr, MADB_DescRecord* CRec, MADB_DescRecord* SqlRec, SQLLEN* IndicatorPtr, SQLLEN* OctetLengthPtr, void* DataPtr,
  MYSQL_BIND* MaBind, unsigned int& IndIdx, unsigned int ParamOffset)
{
  SQLULEN row, Start= ArrayOffset;
  unsigned long Dummy;
  /* Well, specs kinda say, that both values and lenghts arrays should be set(in instruction to param array operations)
         But there is no error/sqlstate for the case if any of those pointers is not set. Thus we assume that is possible */
  if (DataPtr == nullptr)
  {
    /* Special case - DataPtr is not set, we treat it as all values are nullptr. Setting indicators and moving on next param */
    MADB_InitIndicatorArray(this, MaBind, MADB_MapIndicatorValue(SQL_NULL_DATA));
  }

  /* Sets Stmt->Bulk.HasRowsToSkip if needed, since it traverses and checks status array anyway */
  MADB_InitBulkOperBuffers(this, CRec, DataPtr, OctetLengthPtr, IndicatorPtr, SqlRec->ConciseType, MaBind);

  if (MaBind->u.indicator != nullptr && IndIdx == (unsigned int)-1)
  {
    IndIdx= parNr - ParamOffset;
  }

  if (MADB_AppBufferCanBeUsed(CRec->ConciseType, SqlRec->ConciseType))
  {
    /* Everything has been done for such column already */
    return true;
  }

  /* We either have skipped rows or need to convert parameter values/convert array */
  for (row= Start; row < Start + Apd->Header.ArraySize; ++row, DataPtr= (char*)DataPtr + CRec->OctetLength)
  {
    void *Buffer= (char*)MaBind->buffer + row * MaBind->buffer_length;
    void **BufferPtr= (void**)Buffer; /* For the case when Buffer points to the pointer already */

    if (Apd->Header.ArrayStatusPtr != nullptr && Apd->Header.ArrayStatusPtr[row] == SQL_PARAM_IGNORE)
    {
      continue;
    }
    if (MaBind->u.indicator && MaBind->u.indicator[row] > STMT_INDICATOR_NONE)
    {
      continue;
    }

    switch (CRec->ConciseType)
    {
    case SQL_C_CHAR:
      if (SqlRec->ConciseType != SQL_BIT)
      {
        break;
      }
    case DATETIME_TYPES:
      if (CanUseStructArrForDatetime(this))
      {
        BufferPtr= &Buffer;
      }
    }

    /* Need &Dummy here as a length ptr, since nullptr is not good here.
       It would make MADB_ConvertC2Sql to use MaBind->buffer_length by default */
    if (!SQL_SUCCEEDED(MADB_ConvertC2Sql(this, CRec, DataPtr, MaBind->length != nullptr ? MaBind->length[row] : 0,
      SqlRec, MaBind, BufferPtr, MaBind->length != nullptr ? MaBind->length + row : &Dummy)))
    {
      /* Perhaps it's better to move to Clean function */
      CRec->InternalBuffer= nullptr;
      return Error.ReturnValue;
    }
    CRec->InternalBuffer= nullptr;
  }

  return false;
}


void MADB_Stmt::setupBulkCallbacks(uint32_t parNr, MADB_DescRecord* CRec, MADB_DescRecord* SqlRec, DescArrayIterator& it,
  MYSQL_BIND* MaBind)
{
  /* Well, specs kinda say, that both values and lenghts arrays should be set(in instruction to param array operations)
     But there is no error/sqlstate for the case if any of those pointers is not set. Thus we assume that is possible */
  if (it.value() == nullptr)
  {
    /* Special case - DataPtr is not set, we treat it as all values are nullptr. Setting indicators and moving on next param */
    MaBind->u.indicator= &paramIndicatorNull;
  }

  auto parIt= paramCodec.begin() + parNr;
  paramCodec.insert(parIt, Unique::ParamCodec(nullptr));
  std::size_t dataArrStep= getArrayStep(Apd->Header, CRec->OctetLength);
  /*if (Apd->Header.ArrayStatusPtr != nullptr && Apd->Header.ArrayStatusPtr[row] == SQL_PARAM_IGNORE)
  {
    continue;
  }
  if (MaBind->u.indicator && MaBind->u.indicator[row] > STMT_INDICATOR_NONE)
  {
    continue;
  }*/
  if (MADB_AppBufferCanBeUsed(CRec->ConciseType, SqlRec->ConciseType)) {
    paramCodec[parNr].reset(new FixedSizeCopyCodec(it));
  }
  else {
    switch (CRec->ConciseType)
    {
    case WCHAR_TYPES:
      paramCodec[parNr].reset(new WcharCodec(it));
      break;
    case CHAR_BINARY_TYPES:
      switch (SqlRec->ConciseType)
      {
      case SQL_BIT:
        paramCodec[parNr].reset(new BitCodec(it, *MaBind));
        break;
      case SQL_TIME:
      case SQL_TYPE_TIME:
        paramCodec[parNr].reset(new Str2TimeCodec(it, *MaBind));
        break;
      case SQL_DATE:
      case SQL_TYPE_DATE:
        paramCodec[parNr].reset(new Str2DateCodec(it, *MaBind));
        break;
      case SQL_TYPE_TIMESTAMP:
      {
        paramCodec[parNr].reset(new Str2TimestampCodec(it, *MaBind));
        break;
      }
      default:
        paramCodec[parNr].reset(new CopyCodec(it));
      }
      break;
    case SQL_C_NUMERIC:
      MaBind->buffer_type= MYSQL_TYPE_STRING;
      paramCodec[parNr].reset(new NumericCodec(it, *MaBind, SqlRec));
      break;
    case SQL_C_TIMESTAMP:
    case SQL_TYPE_TIMESTAMP:
      switch (SqlRec->ConciseType) {

      case SQL_TYPE_DATE:
        MaBind->buffer_type= MYSQL_TYPE_DATE;
        //tm->time_type=       MYSQL_TIMESTAMP_DATE;
        paramCodec[parNr].reset(new Ts2DateCodec(it, *MaBind));
        break;
      case SQL_TYPE_TIME:
        MaBind->buffer_type= MYSQL_TYPE_TIME;
        //tm->time_type= MYSQL_TIMESTAMP_TIME;
        paramCodec[parNr].reset(new Ts2TimeCodec(it, *MaBind));
        break;
      default:
        paramCodec[parNr].reset(new TsCodec(it, *MaBind));
      }
      break;
    case SQL_C_TIME:
    case SQL_C_TYPE_TIME:
      paramCodec[parNr].reset(new Time2TsCodec(it, *MaBind, SqlRec));
      break;
    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
      paramCodec[parNr].reset(new IntrervalHmsCodec(it, *MaBind, false));
      break;
    case SQL_C_INTERVAL_HOUR_TO_SECOND:
      paramCodec[parNr].reset(new IntrervalHmsCodec(it, *MaBind, true));
      break;
    case SQL_C_DATE:
    case SQL_TYPE_DATE:
      MaBind->buffer_type= MYSQL_TYPE_DATE;
      paramCodec[parNr].reset(new DateCodec(it, *MaBind));
      break;
    default:
      if (!CRec->OctetLength)
      {
        CRec->OctetLength= MaBind->buffer_length;
      }
      if (!it.length()) {
        // We don't have length arr - must be fixed size
        paramCodec[parNr].reset(new FixedSizeCopyCodec(it));
      }
      else {
        paramCodec[parNr].reset(new CopyCodec(it));
      }
    }
  }

  stmt->setParamCallback(paramCodec[parNr].get(), parNr);
}

/* {{{ MADB_ExecuteBulk */
/* Assuming that bulk insert can't go with DAE(and that unlikely ever changes). And that it has been checked before this call,
and we can't have DAE here */
SQLRETURN MADB_ExecuteBulk(MADB_Stmt *Stmt, unsigned int ParamOffset)
{
  unsigned int  i, IndIdx= -1;
  bool useCallbacks= Stmt->Connection->Dsn->ParamCallbacks;

  if (Stmt->stmt->isServerSide() && !MADB_ServerSupports(Stmt->Connection, MADB_CAPABLE_PARAM_ARRAYS))
  {
    MADB_CXX_RESET(Stmt->stmt, new ClientSidePreparedStatement(Stmt->Connection->guard.get(), STMT_STRING(Stmt), Stmt->Options.CursorType
      , Stmt->Query.NoBackslashEscape));
    // So far
    useCallbacks= false;
  }

  // i.e. if the available C/C does not support callbacks
  if (useCallbacks &&
    (Stmt->setParamRowCallback(nullptr) || Stmt->stmt->setCallbackData(reinterpret_cast<void*>(Stmt)))) {
    useCallbacks= false;
  }

  for (i= ParamOffset; i < ParamOffset + MADB_STMT_PARAM_COUNT(Stmt); ++i)
  {
    MADB_DescRecord *CRec, *SqlRec;
    MYSQL_BIND      *MaBind= &Stmt->params[i - ParamOffset];

    if ((CRec= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ)) &&
      (SqlRec= MADB_DescGetInternalRecord(Stmt->Ipd, i, MADB_DESC_READ)))
    {
      /* check if parameter was bound */
      if (!CRec->inUse)
      {
        return MADB_SetError(&Stmt->Error, MADB_ERR_07002, nullptr, 0);
      }

      if (MADB_ConversionSupported(CRec, SqlRec) == FALSE)
      {
        return MADB_SetError(&Stmt->Error, MADB_ERR_07006, nullptr, 0);
      }

      MaBind->length= nullptr;
      DescArrayIterator cit(Stmt->Apd->Header, *CRec, i);

      MaBind->buffer_type= MADB_GetMaDBTypeAndLength(CRec->ConciseType, &MaBind->is_unsigned, &MaBind->buffer_length);
      
      if (useCallbacks)
      {
        Stmt->setupBulkCallbacks(i, CRec, SqlRec, cit, MaBind);
      }
      else
      {
        Stmt->doBulkOldWay(i, CRec, SqlRec, cit.indicator(), cit.length(), cit.value(), MaBind, IndIdx, ParamOffset);
      }
    }
  }

  /* just to do this once, and to use already allocated indicator array */
  if (Stmt->Bulk.HasRowsToSkip)
  {
    if (useCallbacks)
    {
      Stmt->stmt->setParamCallback(new IgnoreRow(Stmt->Apd->Header.ArrayStatusPtr, Stmt->ArrayOffset));
    }
    else
    {
      SQLULEN row, Start= Stmt->ArrayOffset;
      if (IndIdx == (unsigned int)-1)
      {
        IndIdx= 0;
      }

      for (row= Start; row < Start + Stmt->Apd->Header.ArraySize; ++row)
      {
        if (Stmt->Apd->Header.ArrayStatusPtr[row] == SQL_PARAM_IGNORE)
        {
          MADB_SetIndicatorValue(Stmt, &Stmt->params[IndIdx], (unsigned int)row, SQL_PARAM_IGNORE);
        }
      }
    }
  }
  return Stmt->DoExecuteBatch();
}
/* }}} */
