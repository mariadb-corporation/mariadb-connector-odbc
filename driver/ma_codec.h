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
#ifndef _ma_codec_h_
#define _ma_codec_h_

#include <memory>
#include "interface/PreparedStatement.h"
#include "ma_odbc.h"


namespace mariadb
{

// {'\0', STMT_INDICATOR_NULL, STMT_INDICATOR_NTS, STMT_INDICATOR_IGNORE, STMT_INDICATOR_NULL, STMT_INDICATOR_IGNORE_ROW};

class IgnoreRow : public ParamCodec
{
  SQLUSMALLINT *statusArr;
public:
  IgnoreRow(SQLUSMALLINT* ArrStatusPtr, std::size_t initialOffset= 0) : statusArr(ArrStatusPtr + initialOffset) {}
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override {
    if (statusArr[row_nr] == SQL_PARAM_IGNORE) {
      bind->u.indicator= &paramIndicatorIgnoreRow;
      return false;
    }
    else {
      bind->u.indicator= &paramIndicatorNone;
    }
    return true;
  }
};


class FixedSizeCopyCodec : public ParamCodec
{
  DescArrayIterator it;

public:
  FixedSizeCopyCodec(const DescArrayIterator& cit) : it(cit) {}
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class CopyCodec: public ParamCodec
{
  DescArrayIterator it;

public:
  CopyCodec(const DescArrayIterator& cit) : it(cit) {}
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class WcharCodec: public ParamCodec
{
  DescArrayIterator it;
  SQLLEN bufferLen;

public:
  WcharCodec(const DescArrayIterator& cit)
    : it(cit)
    /* Meaning of Buffer Length is not quite clear in specs. Thus we treat in the way, that does not break
    (old) testcases. i.e. we neglect its value if Length Ptr is specified.
      Assuming, that if OctetLengthPtr is not NULL for first row, it's not null for all following */
    , bufferLen(it.length() ? -1 : it.getDescRec()->OctetLength)
  {}
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class BitCodec: public ParamCodec
{
  DescArrayIterator it;
  char buf= '\0';

public:
  BitCodec(const DescArrayIterator& cit, MYSQL_BIND& bind);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class Str2TimeCodec : public ParamCodec
{
  DescArrayIterator it;
  MYSQL_TIME buf;

public:
  Str2TimeCodec(const DescArrayIterator& cit, MYSQL_BIND& bind);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class Str2DateCodec : public ParamCodec
{
  DescArrayIterator it;
  MYSQL_TIME buf;

public:
  Str2DateCodec(const DescArrayIterator& cit, MYSQL_BIND& bind);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class Str2TimestampCodec: public ParamCodec
{
  DescArrayIterator it;
  MYSQL_TIME buf;

public:
  Str2TimestampCodec(const DescArrayIterator& cit, MYSQL_BIND& bind);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class NumericCodec: public ParamCodec
{
  DescArrayIterator it;
  char buf[MADB_CHARSIZE_FOR_NUMERIC];
  SQLSCHAR scale;
  SQLSCHAR precision;

public:
  NumericCodec(const DescArrayIterator& cit, MYSQL_BIND& bind, MADB_DescRecord* sqlRec);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class Ts2DateCodec: public ParamCodec
{
  DescArrayIterator it;
  MYSQL_TIME buf;

public:
  Ts2DateCodec(const DescArrayIterator& cit, MYSQL_BIND& bind);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class Ts2TimeCodec: public ParamCodec
{
  DescArrayIterator it;
  MYSQL_TIME buf;

public:
  Ts2TimeCodec(const DescArrayIterator& cit, MYSQL_BIND& bind);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class TsCodec: public ParamCodec
{
  DescArrayIterator it;
  MYSQL_TIME buf;

public:
  TsCodec(const DescArrayIterator& cit, MYSQL_BIND& bind);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class Time2TsCodec: public ParamCodec
{
  DescArrayIterator it;
  MYSQL_TIME buf;
  bool checkValidTime= false;

public:
  Time2TsCodec(const DescArrayIterator& cit, MYSQL_BIND& bind, MADB_DescRecord* sqlRec);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class IntrervalHmsCodec: public ParamCodec
{
  DescArrayIterator it;
  MYSQL_TIME buf;
  bool toSeconds;

public:
  IntrervalHmsCodec(const DescArrayIterator& cit, MYSQL_BIND& bind, bool _toSeconds);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


class DateCodec: public ParamCodec
{
  DescArrayIterator it;
  MYSQL_TIME buf;

public:
  DateCodec(const DescArrayIterator& cit, MYSQL_BIND& bind);
  bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr) override;
};


} // namespace mariadb
#endif /* _ma_xxxxxx_h_ */
