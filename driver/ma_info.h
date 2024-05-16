/************************************************************************************
   Copyright (C) 2013, 2016 MariaDB Corporation AB
   
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
#ifndef _ma_info_h_
#define _ma_info_h_

// In fact atm for variables it's srangely 81.
#define MADB_DECIMAL_MAX_PRECISION 65

typedef struct
{
  bytes TypeName;
  bytes DataType;
  bytes ColumnSize;
  bytes LiteralPrefix;
  bytes LiteralSuffix;
  bytes CreateParams;
  bytes Nullable;
  bytes CaseSensitive;
  bytes Searchable;
  bytes Unsigned;
  bytes FixedPrecScale;
  bytes AutoUniqueValue;
  bytes LocalTypeName;
  bytes MinimumScale;
  bytes MaximumScale;
  bytes SqlDataType1;
  bytes SqlDateTimeSub;
  bytes NumPrecRadix;
  bytes IntervalPrecision;
  bytes SqlDataType;
} MADB_TypeInfo;


SQLRETURN MADB_GetTypeInfo(SQLHSTMT StatementHandle,
                           SQLSMALLINT DataType);

#endif /* _ma_info_h_ */
