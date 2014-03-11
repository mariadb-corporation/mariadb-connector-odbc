/************************************************************************************
   Copyright (C) 2013 SkySQL AB
   
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
#ifndef _na_dsn_h_
#define _ma_dsn_h_

/* MySQL ODBC compatibility options */
#define MADB_OPT_FLAG_FIELD_LENGTH                      1
#define MADB_OPT_FLAG_FOUND_ROWS                        2
#define MADB_OPT_FLAG_DEBUG                             4
#define MADB_OPT_FLAG_BIG_PACKETS                       8
#define MADB_OPT_FLAG_NO_PROMPT                        16
#define MADB_OPT_FLAG_DYNAMIC_CURSOR                   32
#define MADB_OPT_FLAG_NO_SCHEMA                        64
#define MADB_OPT_FLAG_NO_DEFAULT_CURSOR               128
#define MADB_OPT_FLAG_NO_LOCALE                       256
#define MADB_OPT_FLAG_PAD_SPACE                       512
#define MADB_OPT_FLAG_FULL_COLUMN_NAMES              1024
#define MADB_OPT_FLAG_COMPRESSED_PROTO               2048
#define MADB_OPT_FLAG_IGNORE_SPACE                   4096
#define MADB_OPT_FLAG_NAMED_PIPE                     8192
#define MADB_OPT_FLAG_NO_BIGINT                     16384
#define MADB_OPT_FLAG_NO_CATALOG                    32768
#define MADB_OPT_FLAG_USE_CNF                       65536
#define MADB_OPT_FLAG_SAFE                         131072
#define MADB_OPT_FLAG_NO_TRANSACTIONS              262144
#define MADB_OPT_FLAG_LOG_QUERY                    524288
#define MADB_OPT_FLAG_NO_CACHE                    1048576
#define MADB_OPT_FLAG_FORWARD_CURSOR              2097152
#define MADB_OPT_FLAG_AUTO_RECONNECT              4194304
#define MADB_OPT_FLAG_AUTO_IS_NULL                8388608
#define MADB_OPT_FLAG_ZERO_DATE_TO_MIN           16777216
#define MADB_OPT_FLAG_MIN_DATE_TO_ZERO           33554432
#define MADB_OPT_FLAG_MULTI_STATEMENTS           67108864
#define MADB_OPT_FLAG_COLUMN_SIZE_S32           134217728
#define MADN_OPT_FLAG_NO_BINARY_RESULT          268435456
#define MADN_OPT_FLAG_BIGINT_BIND_STR           536870912
#define MADN_OPT_FLAG_NO_INFORMATION_SCHEMA    1073741824

typedef struct
{
  unsigned int Page;
  unsigned long DialogOption;
  unsigned long value;
} MADB_OptionsMap;

/* this structure is used to store and retrieve DSN Information */


MADB_DsnKey DsnKeys[];



/*** Function prototypes ***/
MADB_Dsn *MADB_DSN_Init(void);
void MADB_DSN_Free(MADB_Dsn *Dsn);
my_bool MADB_ReadDSN(MADB_Dsn *Dsn, char *KeyValue, my_bool OverWrite);
my_bool MADB_SaveDSN(MADB_Dsn *Dsn);
my_bool MADB_DSN_Exists(char *DsnName);
my_bool MADB_ParseDSNString(MADB_Dsn *Dsn, char *String, size_t Length, char Delimiter);
SQLSMALLINT MADB_DsnToString(MADB_Dsn *Dsn, char *OutString, SQLSMALLINT OutLength);

/*** Helper macros ***/
#define DSN_OPTION(a,b)\
  ((a)->Options & b)

#define MA_ODBC_CURSOR_DYNAMIC(a)\
  DSN_OPTION((a), MADB_OPT_FLAG_DYNAMIC_CURSOR)

#define MA_ODBC_CURSOR_FORWARD_ONLY(a)\
  DSN_OPTION((a), MADB_OPT_FLAG_FORWARD_CURSOR)

#define MADB_DSN_SET_STR(dsn, item, value, len)\
  if((value) && (len) != 0)\
  {\
    if ((len) == SQL_NTS)\
      (len)=strlen((value));\
    (dsn)->item= (char *)my_malloc(len + 1, MYF(MY_ZEROFILL));\
    memcpy((dsn)->item, (value),(len));\
  }

#define MADB_DSN_GET_UTF8(dsn, item, len) \
  ((dsn)->item) ? MADB_ConvertFromWChar((dsn)->item, wcslen((dsn)->item)+1, (len)) : NULL


#endif /* _ma_dsn_h_ */
