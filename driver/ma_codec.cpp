/************************************************************************************
   Copyright (C) 2024 MariaDB Corporation plc
   
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

#include <cstdio>
#include "ma_codec.h"
#include "class/ResultSetMetaData.h"
#include "ma_string.h"


namespace mariadb
{
  unsigned long getLength(SQLLEN len, void *value) {
    if (len == SQL_NTS) {
      return static_cast<unsigned long>(std::strlen(reinterpret_cast<char*>(value)));
    }
    else {
      return static_cast<unsigned long>(len);
    }
  }


  bool FixedSizeCopyCodec::operator()(void * data, MYSQL_BIND * bind, uint32_t col_nr, uint32_t row_nr)
  {
    bind->buffer= it.value();
    it.next();
    return false;
  }


  bool CopyCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    bind->buffer= it.value();
    bind->buffer_length= getLength(*it.length(), bind->buffer);
    it.next();
    return false;
  }


  bool WcharCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    SQLLEN length;
    SQLULEN mbLength=0;
    MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);

    if (!it.length() || *it.length() == SQL_NTS)
    {
      /* CRec->OctetLength eq 0 means not 0-length buffer, but that this value is not specified. Thus -1, for SqlwcsLen
          and SafeStrlen that means buffer len is not specified */
      length= SqlwcsLen((SQLWCHAR *)it.value(), bufferLen / sizeof(SQLWCHAR) - MADBTEST(bufferLen == 0));
    }
    else
    {
      length= *it.length();
    }

    MADB_FREE(it.getDescRec()->InternalBuffer);
    it.getDescRec()->InternalBuffer= MADB_ConvertFromWChar((SQLWCHAR *)it.value(), (SQLINTEGER)length, &mbLength, &Stmt->Connection->Charset, nullptr);

    if (it.getDescRec()->InternalBuffer == nullptr)
    {
      char error[64];
      std::snprintf(error, sizeof(error), "Error of allocation of the buffer of size %u", (uint32_t)mbLength);
      MADB_SetError(&Stmt->Error, MADB_ERR_HY001, nullptr, 0);
      return true;
    }

    bind->buffer_length= (unsigned long)mbLength;
    bind->buffer= it.getDescRec()->InternalBuffer;
    it.next();
    return false;
  }


  BitCodec::BitCodec(const DescArrayIterator & cit, MYSQL_BIND& bind)
    : it(cit)
  {
    bind.buffer= &buf;
  }


  bool BitCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    bind->buffer= &buf;
    buf= MADB_ConvertCharToBit(reinterpret_cast<MADB_Stmt*>(data), static_cast<char*>(it.value()));
    it.next();
    return false;
  }


  Str2TimeCodec::Str2TimeCodec(const DescArrayIterator & cit, MYSQL_BIND& bind)
    : it(cit)
  {
    bind.buffer= &buf;
  }


  bool Str2TimeCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);
    bool isTime;
    MADB_Str2Ts(static_cast<char*>(it.value()), it.length() ? static_cast<std::size_t>(*it.length()) : 0, &buf,
      false, &Stmt->Error, &isTime);
    if (buf.second_part != 0)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_22008, nullptr, 0);
      return true;
    }
    it.next();
    return false;
  }


  Str2DateCodec::Str2DateCodec(const DescArrayIterator & cit, MYSQL_BIND& bind)
    : it(cit)
  {
    bind.buffer= &buf;
  }


  bool Str2DateCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);
    bool isTime;
    MADB_Str2Ts(static_cast<char*>(it.value()), it.length() ? static_cast<std::size_t>(*it.length()) : 0, &buf,
      false, &Stmt->Error, &isTime);
    if (buf.hour || buf.minute || buf.second || buf.second_part)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_22008, nullptr, 0);
      return true;
    }
    it.next();
    return false;
  }


  Str2TimestampCodec::Str2TimestampCodec(const DescArrayIterator & cit, MYSQL_BIND& bind)
    : it(cit)
  {
    bind.buffer= &buf;
  }


  bool Str2TimestampCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);
    bool isTime= false;
    MADB_Str2Ts(static_cast<char*>(it.value()), it.length() ? static_cast<std::size_t>(*it.length()) : 0, &buf,
      false, &Stmt->Error, &isTime);
    if ((!isTime && buf.year == 0) || buf.month == 0 || buf.day == 0)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_22018, nullptr, 0);
      return true;
    }
    it.next();
    return false;
  }


  NumericCodec::NumericCodec(const DescArrayIterator & cit, MYSQL_BIND & bind, MADB_DescRecord* sqlRec)
    : it(cit)
    , scale(static_cast<SQLSCHAR>(sqlRec->Scale))
    , precision(static_cast<SQLSCHAR>(sqlRec->Precision))
  {
    bind.buffer= buf;
  }

  bool NumericCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    int32_t    errorCode= 0;
    SQL_NUMERIC_STRUCT *p;

    p= reinterpret_cast<SQL_NUMERIC_STRUCT*>(it.value());
    p->scale= scale;
    p->precision= precision;
    bind->buffer_length= static_cast<unsigned long>(MADB_ConvertNumericToChar(p, buf, &errorCode));

    if (errorCode) {
      MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);
      MADB_SetError(&Stmt->Error, errorCode, nullptr, 0);
      return true;
    }
    it.next();
    return false;
  }


  Ts2DateCodec::Ts2DateCodec(const DescArrayIterator& cit, MYSQL_BIND& bind)
    : it(cit)
  {
    buf.time_type= MYSQL_TIMESTAMP_DATE;
    bind.buffer_type= MYSQL_TYPE_DATE;
    bind.buffer= &buf;
  }


  bool Ts2DateCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    SQL_TIMESTAMP_STRUCT *ts= reinterpret_cast<SQL_TIMESTAMP_STRUCT*>(it.value());
    if (ts->hour || ts->minute || ts->second || ts->fraction)
    {
      MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);
      MADB_SetError(&Stmt->Error, MADB_ERR_22008, "Time fields are nonzero", 0);
      return true;
    }
    buf.year=  ts->year;
    buf.month= ts->month;
    buf.day=   ts->day;
    it.next();
    return false;
  }


  Ts2TimeCodec::Ts2TimeCodec(const DescArrayIterator& cit, MYSQL_BIND& bind)
    : it(cit)
  {
    buf.time_type= MYSQL_TIMESTAMP_TIME;
    bind.buffer_type= MYSQL_TYPE_TIME;
    bind.buffer= &buf;
  }


  bool Ts2TimeCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    SQL_TIMESTAMP_STRUCT *ts= reinterpret_cast<SQL_TIMESTAMP_STRUCT*>(it.value());
    MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);

    if (ts->fraction)
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_22008, "Fractional seconds fields are nonzero", 0);
      return true;
    }
    if (!VALID_TIME(ts))
    {
      MADB_SetError(&Stmt->Error, MADB_ERR_22007, "Invalid time", 0);
      return true;
    }
    buf.hour=   ts->hour;
    buf.minute= ts->minute;
    buf.second= ts->second;
    it.next();
    return false;
  }


  TsCodec::TsCodec(const DescArrayIterator& cit, MYSQL_BIND& bind)
    : it(cit)
  {
    buf.time_type= MYSQL_TIMESTAMP_DATETIME;
    bind.buffer_type= MYSQL_TYPE_DATETIME;
    bind.buffer= &buf;
  }


  bool TsCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    SQL_TIMESTAMP_STRUCT *ts= reinterpret_cast<SQL_TIMESTAMP_STRUCT*>(it.value());

    MADB_CopyOdbcTsToMadbTime(ts, &buf);
    it.next();
    return false;
  }


  Time2TsCodec::Time2TsCodec(const DescArrayIterator& cit, MYSQL_BIND& bind, MADB_DescRecord* sqlRec)
    : it(cit)
  {
    if (sqlRec->ConciseType == SQL_TYPE_TIMESTAMP ||
      sqlRec->ConciseType == SQL_TIMESTAMP || sqlRec->ConciseType == SQL_DATETIME) {
      time_t sec_time;
      struct tm *cur_tm;

      checkValidTime= true;
      buf.time_type= MYSQL_TIMESTAMP_DATETIME;
      bind.buffer_type= MYSQL_TYPE_DATETIME;

      // Should do it in more C++ish way at some point
      sec_time= time(nullptr);
      cur_tm= localtime(&sec_time);

      buf.year= 1900 + cur_tm->tm_year;
      buf.month= cur_tm->tm_mon + 1;
      buf.day= cur_tm->tm_mday;
      buf.second_part= 0;
    }
    else {
      if (sqlRec->ConciseType == SQL_TYPE_TIME || sqlRec->ConciseType == SQL_TIME) {
        checkValidTime= true;
      }
      buf.time_type= MYSQL_TIMESTAMP_TIME;
      bind.buffer_type= MYSQL_TYPE_TIME;
      buf.year=  0;
      buf.month= 0;
      buf.day=   0;
    }
    bind.buffer= &buf;
    buf.second_part= 0;
  }


  bool Time2TsCodec::operator()(void *data, MYSQL_BIND * bind, uint32_t col_nr, uint32_t row_nr)
  {
    SQL_TIME_STRUCT *ts= reinterpret_cast<SQL_TIME_STRUCT*>(it.value());
    if (checkValidTime && !VALID_TIME(ts)) {
      MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);
      MADB_SetError(&Stmt->Error, MADB_ERR_22007, nullptr, 0);
      return true;
    }
    buf.hour=   ts->hour;
    buf.minute= ts->minute;
    buf.second= ts->second;
    it.next();
    return false;
  }


  IntrervalHmsCodec::IntrervalHmsCodec(const DescArrayIterator& cit, MYSQL_BIND& bind, bool _toSeconds)
    : it(cit)
    , toSeconds(_toSeconds)
  {
    buf.time_type= MYSQL_TIMESTAMP_TIME;
    bind.buffer_type= MYSQL_TYPE_TIME;
    bind.buffer= &buf;
    if (!toSeconds) {
      buf.second= 0;
    }
  }


  bool IntrervalHmsCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    SQL_INTERVAL_STRUCT *is= reinterpret_cast<SQL_INTERVAL_STRUCT*>(it.value());
    buf.hour=   is->intval.day_second.hour;
    buf.minute= is->intval.day_second.minute;
    if (toSeconds) {
      buf.second= is->intval.day_second.second;
    }

    buf.second_part= 0;
    it.next();
    return false;
  }


  DateCodec::DateCodec(const DescArrayIterator& cit, MYSQL_BIND& bind)
    : it(cit)
  {
    buf.time_type= MYSQL_TIMESTAMP_DATE;
    bind.buffer_type= MYSQL_TYPE_DATE;
    bind.buffer= &buf;
    std::memset(&buf, 0, sizeof(MYSQL_TIME));
  }

  /* {{{ DateCodec::operator() */
  bool DateCodec::operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    SQL_DATE_STRUCT *ts= reinterpret_cast<SQL_DATE_STRUCT*>(it.value());
    buf.year=  ts->year;
    buf.month= ts->month;
    buf.day=   ts->day;
    it.next();
    return true;
  }
  /* }}} */

//--------------- End of Parameter Codecs -> Begin of Result Codecs -----------------

  void NullRCodec::operator()(void *data, uint32_t col_nr, unsigned char* row, unsigned long length)
  {
    MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);
    *Stmt->result[col_nr].is_null= '\1';
  }


  void WcharRCodec::operator()(void * data, uint32_t column, unsigned char* row, unsigned long length)
  {
    MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);
    SQLLEN buffCharLen= it.getDescRec()->OctetLength/sizeof(SQLWCHAR), charLen;
    
    if (length == NULL_LENGTH) {
      if (it.indicator() != nullptr) {
        *it.indicator()= SQL_NULL_DATA;
      }  
      else {
        *it.length()= SQL_NULL_DATA;
      }
      return;
    }
    if (MADB_ConvertAnsi2Unicode(&Stmt->Connection->Charset, (char *)row, (SQLLEN)length, (SQLWCHAR *)it.value(),
      buffCharLen, &charLen, 0, &Stmt->Error)) {
      //CALC_ALL_FLDS_RC(Stmt->aggRc, Stmt->Error.ReturnValue);
    }

    if ((charLen == 0 || charLen > buffCharLen) && length != 0 && it.value() != nullptr &&
      *(char*)row != '\0' && Stmt->Error.ReturnValue != SQL_SUCCESS)
    {
      CALC_ALL_FLDS_RC(Stmt->aggRc, Stmt->Error.ReturnValue);
    }
    else if (charLen > 0 && charLen <= buffCharLen && ((SQLWCHAR*)it.value())[charLen - 1] != 0) {
      if (charLen == buffCharLen) {
        CALC_ALL_FLDS_RC(Stmt->aggRc, MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0));
        ((SQLWCHAR*)it.value())[charLen - 1]= 0;
      }
      else {
        // Length does not include terminating NULL
        ((SQLWCHAR*)it.value())[charLen]= 0;
      }
    }

    if (it.length() != 0) {
      /* If application didn't give data buffer and only want to know the length of data to fetch */
      if (charLen == 0 && it.value() == nullptr) {
        charLen= length;
      }
      /* Not quite right */
      *it.length()= charLen * sizeof(SQLWCHAR);
    }
  }


  void StringRCodec::operator()(void * data, uint32_t column, unsigned char* row, unsigned long length)
  {
    MADB_Stmt *Stmt= reinterpret_cast<MADB_Stmt*>(data);

    if (length == NULL_LENGTH) {
      if (it.indicator() != nullptr) {
        *it.indicator()= SQL_NULL_DATA;
      }
      else {
        *it.length()= SQL_NULL_DATA;
      }
      return;
    }

    uint32_t need2terminate= length == 0 || row[length - 1] != '\0' ? 1 : 0;

    if (it.value() && it.getDescRec()->OctetLength > 0) {
      if (it.getDescRec()->OctetLength < length + need2terminate) {
        CALC_ALL_FLDS_RC(Stmt->aggRc, MADB_SetError(&Stmt->Error, MADB_ERR_01004, NULL, 0));
        std::memcpy(it.value(), row, static_cast<std::size_t>(it.getDescRec()->OctetLength - 1));
        ((char*)it.value())[static_cast<std::size_t>(it.getDescRec()->OctetLength - 1)]= '\0';
      }
      else if (length > 0) {
        std::memcpy(it.value(), row, length);
        if (need2terminate) {
          ((char*)it.value())[length]= '\0';
        }
      }
    }

    if (it.length() != 0) {
      /* If application didn't give data buffer and only want to know the length of data to fetch */
      *it.length()= length;
    }
  }
}

