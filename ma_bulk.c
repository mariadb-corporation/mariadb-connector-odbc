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

/* Code allowing to deploy MariaDB bulk operation functionality.
 * i.e. adapting ODBC param arrays to MariaDB arrays */

#include <ma_odbc.h>


char MADB_MapIndicatorValue(SQLLEN OdbcInd)
{
  switch (OdbcInd)
  {
  case SQL_NTS:           return STMT_INDICATOR_NTS;
  case SQL_COLUMN_IGNORE: return STMT_INDICATOR_IGNORE;
  case SQL_NULL_DATA:     return STMT_INDICATOR_NULL;
    /*STMT_INDICATOR_DEFAULT*/
  }
  return '\0';
}


/* Returns number of "active" paramsets, i.e. that do not have SQL_PARAM_IGNORE in status array */
unsigned int MADB_UsedParamSets(MADB_Stmt *Stmt)
{
  SQLULEN i, result= Stmt->Apd->Header.ArraySize;

  if (Stmt->Apd->Header.ArrayStatusPtr != NULL)
  {
    for (i= 0; i < Stmt->Apd->Header.ArraySize; ++i)
    {
      if (Stmt->Apd->Header.ArrayStatusPtr[i] == SQL_PARAM_IGNORE)
      {
        --result;
      }
    }
  }
  return result;
}

BOOL MADB_AppBufferCanBeUsed(SQLSMALLINT CType, SQLSMALLINT SqlType)
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
  case SQL_C_TIMESTAMP:
  case SQL_TYPE_TIMESTAMP:
  case SQL_C_TIME:
  case SQL_TYPE_TIME:
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
  case SQL_C_DATE:
  case SQL_TYPE_DATE:

    return FALSE;
  }
  return TRUE;
}


void MADB_CleanBulkOperData(MADB_Stmt *Stmt, unsigned int ParamOffset)
{
  if (MADB_DOING_BULK_OPER(Stmt))
  {
    MADB_DescRecord *CRec;
    void            *DataPtr= NULL;
    MYSQL_BIND      *MaBind= NULL;
    int             i;

    for (i= ParamOffset; i < MADB_STMT_PARAM_COUNT(Stmt); ++i)
    {
      if (CRec= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ))
      {
        MaBind= &Stmt->params[i - ParamOffset];
        DataPtr= GetBindOffset(Stmt->Apd, CRec, CRec->DataPtr, 0, CRec->OctetLength);

        if (MaBind->buffer != DataPtr)
        {
          switch (CRec->ConciseType)
          {
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
    Stmt->Bulk.Index=     0;
  }
}

void MADB_CopyArrayForBulkOper(MADB_Stmt *Stmt, char *Src, char *Dst, size_t ElementSize)
{
  unsigned int i;
  for (i= 0; i < Stmt->Apd->Header.ArraySize; ++i)
  {
    if (Stmt->Apd->Header.ArrayStatusPtr[i] != SQL_PARAM_IGNORE)
    {
      memcpy(Dst, Src, ElementSize);
      Dst+= ElementSize;
    }
    Src+= ElementSize;
  }
}


SQLRETURN MADB_InitIndicatorArray(MADB_Stmt *Stmt, MYSQL_BIND *MaBind, char InitValue)
{
  MaBind->u.indicator= MADB_ALLOC(Stmt->Bulk.ArraySize);

  if (MaBind->u.indicator == NULL)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
  }
  memset(MaBind->u.indicator, InitValue, Stmt->Bulk.ArraySize);

  return SQL_SUCCESS;
}


SQLRETURN MADB_SetBulkOperLengthArr(MADB_Stmt *Stmt, MADB_DescRecord *CRec, SQLLEN *OctetLengthPtr, SQLLEN *IndicatorPtr,
                                    void *DataPtr, MYSQL_BIND *MaBind)
{
  /* Leaving it so far here commented, but it comlicates things w/out much gains */
  /*if (sizeof(SQLLEN) == sizeof(long) && !MADB_BULK_OPER_HAS2SKIP_ROWS(Stmt) && MADB_AppBufferCanBeUsed())
  {
    if (OctetLengthPtr)
    {
      MaBind->length= OctetLengthPtr;
    }
  }
  else
  {*/
  MaBind->length= MADB_REALLOC(MaBind->length, Stmt->Bulk.ArraySize*sizeof(long));
  if (MaBind->length == NULL)
  {
    return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);;
  }
  unsigned int i;
  Stmt->Bulk.Index= -1;
  for (i= 0; i < Stmt->Apd->Header.ArraySize; ++i, DataPtr= (char*)DataPtr + CRec->OctetLength)
  {
    if (Stmt->Apd->Header.ArrayStatusPtr == NULL || Stmt->Apd->Header.ArrayStatusPtr[i] != SQL_PARAM_IGNORE)
    {
      ++Stmt->Bulk.Index;
      if (OctetLengthPtr != NULL && OctetLengthPtr[i] == SQL_NULL_DATA
        || IndicatorPtr != NULL && IndicatorPtr[i] != SQL_NULL_DATA)
      {
        RETURN_ERROR_OR_CONTINUE(MADB_SetIndicatorValue(Stmt, MaBind, SQL_NULL_DATA));
        continue;
      }
      if (OctetLengthPtr != NULL && OctetLengthPtr[i] == SQL_COLUMN_IGNORE
        || IndicatorPtr != NULL && IndicatorPtr[i] != SQL_COLUMN_IGNORE)
      {
        RETURN_ERROR_OR_CONTINUE(MADB_SetIndicatorValue(Stmt, MaBind, SQL_COLUMN_IGNORE));
        continue;
      }
      MaBind->length[Stmt->Bulk.Index]= MADB_CalculateLength(Stmt, OctetLengthPtr != NULL ? &OctetLengthPtr[i] : NULL, CRec, DataPtr);
    }
  }

  return SQL_SUCCESS;
}
/* {{{ MADB_InitBulkOperBuffers */
/* Allocating data and length arrays, if needed, and initing them in certain cases.
   DataPtr should be ensured to be not NULL */
SQLRETURN MADB_InitBulkOperBuffers(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void *DataPtr, SQLLEN *OctetLengthPtr,
                                   SQLLEN *IndicatorPtr, SQLSMALLINT SqlType, MYSQL_BIND *MaBind)
{
  switch (CRec->ConciseType)
  {
  case CHAR_BINARY_TYPES:
    if (SqlType == SQL_BIT)
    {
      CRec->InternalBuffer= MADB_CALLOC(Stmt->Bulk.ArraySize);
      MaBind->buffer_length= 1;
      break;
    }
  case WCHAR_TYPES:
  case SQL_C_NUMERIC:
    CRec->InternalBuffer= MADB_ALLOC(Stmt->Bulk.ArraySize*sizeof(char*));
    MaBind->buffer_length= sizeof(char*);
    break;
  case SQL_C_TIMESTAMP:
  case SQL_TYPE_TIMESTAMP:
  case SQL_C_TIME:
  case SQL_TYPE_TIME:
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
  case SQL_C_DATE:
  case SQL_TYPE_DATE:
    CRec->InternalBuffer= MADB_ALLOC(Stmt->Bulk.ArraySize*sizeof(MYSQL_TIME));
    MaBind->buffer_length= sizeof(MYSQL_TIME);
    break;
  default:
    MaBind->buffer_type= MADB_GetMaDBTypeAndLength(CRec->ConciseType, &MaBind->is_unsigned, &MaBind->buffer_length);

    if (MaBind->buffer_length == 0)
    {
      MaBind->buffer_length= sizeof(char*);
    }

    if (MADB_BULK_OPER_HAS2SKIP_ROWS(Stmt))
    {

      CRec->InternalBuffer= MADB_ALLOC(Stmt->Bulk.ArraySize*sizeof(MaBind->buffer_length));
      MADB_CopyArrayForBulkOper(Stmt, DataPtr, CRec->InternalBuffer, MaBind->buffer_length);
    }
    else
    {
      MaBind->buffer= DataPtr;
    }
  }
  if (MaBind->buffer != DataPtr)
  {
    MaBind->buffer= CRec->InternalBuffer;
    if (MaBind->buffer == NULL)
    {
      return MADB_SetError(&Stmt->Error, MADB_ERR_HY001, NULL, 0);
    }
    CRec->InternalBuffer= NULL; /* Need to reset this pointer, so the memory won't be freed (accidentally) */
  }

  RETURN_ERROR_OR_CONTINUE(MADB_SetBulkOperLengthArr(Stmt, CRec, OctetLengthPtr, IndicatorPtr, DataPtr, MaBind));

  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_SetIndicatorValue */
SQLRETURN MADB_SetIndicatorValue(MADB_Stmt *Stmt, MYSQL_BIND *MaBind, SQLLEN OdbcIndicator)
{
  if (MaBind->u.indicator == NULL)
  {
    RETURN_ERROR_OR_CONTINUE(MADB_InitIndicatorArray(Stmt, MaBind, STMT_INDICATOR_NONE));
  }

  MaBind->u.indicator[Stmt->Bulk.Index]= MADB_MapIndicatorValue(OdbcIndicator);
  return SQL_SUCCESS;
}
/* }}} */

/* {{{ MADB_ExecuteBulk */
/* Assuming that bulk insert can't go with DAE(and that unlikely ever changes). And that it has been checked before this call,
and we can't have DAE here */
SQLRETURN MADB_ExecuteBulk(MADB_Stmt *Stmt, unsigned int ParamOffset)
{
  unsigned int i;

  for (i= ParamOffset; i < ParamOffset + MADB_STMT_PARAM_COUNT(Stmt); ++i)
  {
    MADB_DescRecord *CRec, *SqlRec;
    SQLLEN          *IndicatorPtr= NULL;
    SQLLEN          *OctetLengthPtr= NULL;
    void            *DataPtr= NULL;
    MYSQL_BIND      *MaBind= &Stmt->params[i - ParamOffset];
    SQLULEN         j, Start=      0;

    if ((CRec= MADB_DescGetInternalRecord(Stmt->Apd, i, MADB_DESC_READ)) &&
      (SqlRec= MADB_DescGetInternalRecord(Stmt->Ipd, i, MADB_DESC_READ)))
    {
      /* check if parameter was bound */
      if (!CRec->inUse)
      {
        return MADB_SetError(&Stmt->Error, MADB_ERR_07002, NULL, 0);
      }

      if (MADB_ConversionSupported(CRec, SqlRec) == FALSE)
      {
        return MADB_SetError(&Stmt->Error, MADB_ERR_07006, NULL, 0);
      }

      Stmt->params[i-ParamOffset].length= NULL;
      IndicatorPtr=   (SQLLEN *)GetBindOffset(Stmt->Apd, CRec, CRec->IndicatorPtr, 0, sizeof(SQLLEN));
      OctetLengthPtr= (SQLLEN *)GetBindOffset(Stmt->Apd, CRec, CRec->OctetLengthPtr, 0, sizeof(SQLLEN));
      DataPtr=        GetBindOffset(Stmt->Apd, CRec, CRec->DataPtr, 0, CRec->OctetLength);

      /* If these are the same pointers, setting indicator to NULL to simplify things a bit */
      if (IndicatorPtr == OctetLengthPtr)
      {
        IndicatorPtr= NULL;
      }
      /* Well, specs kinda say, that both values and lenghts arrays should be set(in instruction to param array operations)
         But there is no error/sqlstate for the case if any of those pointers is not set. Thus we assume that is possible */
      if (DataPtr == NULL)
      {
        /* Special case - DataPtr is not set, we treat it as all values are NULL. Setting indicators and moving on next param */
        RETURN_ERROR_OR_CONTINUE(MADB_InitIndicatorArray(Stmt, MaBind, MADB_MapIndicatorValue(SQL_NULL_DATA)));
        continue;
      }

      RETURN_ERROR_OR_CONTINUE(MADB_InitBulkOperBuffers(Stmt, CRec, DataPtr, OctetLengthPtr, IndicatorPtr, SqlRec->ConciseType, MaBind));

      if (MADB_AppBufferCanBeUsed(CRec->ConciseType, SqlRec->ConciseType))
      {
        /* Everything has been done for such column already */
        continue;
      }

      Start+= Stmt->ArrayOffset;
      Stmt->Bulk.Index= 0;

      /* We either have skipped rows, an*/
      for (j= Start; j < Start + Stmt->Apd->Header.ArraySize; ++j, DataPtr= (char*)DataPtr + CRec->OctetLength)
      {
        void *Buffer= (char*)MaBind->buffer + Stmt->Bulk.Index*MaBind->buffer_length;
        void **BufferPtr= (void**)Buffer; /* For the case when Buffer points to the pointer already */

        if (Stmt->Apd->Header.ArrayStatusPtr != NULL && Stmt->Apd->Header.ArrayStatusPtr[j] == SQL_PARAM_IGNORE)
        {
          continue;
        }
        if (MaBind->u.indicator && MaBind->u.indicator[Stmt->Bulk.Index] > STMT_INDICATOR_NONE)
        {
          ++Stmt->Bulk.Index;
          continue;
        }

        switch (CRec->ConciseType)
        {
        case SQL_C_CHAR:
          if (SqlRec->ConciseType != SQL_BIT)
          {
            break;
          }
        case SQL_C_TIMESTAMP:
        case SQL_TYPE_TIMESTAMP:
        case SQL_C_TIME:
        case SQL_TYPE_TIME:
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
        case SQL_C_DATE:
        case SQL_TYPE_DATE:
          BufferPtr= &Buffer;
        }

        if (!SQL_SUCCEEDED(MADB_ConvertC2Sql(Stmt, CRec, DataPtr, MaBind->length[Stmt->Bulk.Index],
          SqlRec, MaBind, BufferPtr, MaBind->length + Stmt->Bulk.Index)))
        {
          /* Perhaps it's better to move to Clean function */
          CRec->InternalBuffer= NULL;
          return Stmt->Error.ReturnValue;
        }
        CRec->InternalBuffer= NULL;
        ++Stmt->Bulk.Index;
      }
    }
  }

  return MADB_DoExecute(Stmt, FALSE);
}
/* }}} */
