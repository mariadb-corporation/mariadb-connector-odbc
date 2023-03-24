/************************************************************************************
   Copyright (C) 2019,2023 MariaDB Corporation AB
   
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
#ifndef _ma_c_stuff_h_
#define _ma_c_stuff_h_

#include "ma_odbc_version.h"

#ifdef _WIN32
# include "ma_platform_win32.h"
#else
# include "ma_platform_posix.h"
#endif

#include <stdlib.h>

#include <ma_legacy_helpers.h>

#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>

#include <errmsg.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include <assert.h>
#include <time.h>
#include <sqlext.h>
#include <mysql.h>
#define MADB_FREE(a) do { \
  free((void*)(a));\
  (a)= NULL; \
} while(0)

#define MADB_ALLOC(a) malloc((a))
#define MADB_CALLOC(a) calloc((a) > 0 ? (a) : 1, sizeof(char))
#define MADB_REALLOC(a,b) realloc((a),(b))

/* If required to free old memory pointed by current ptr, and set new value */
#define MADB_RESET(ptr, newptr) do {\
  const char *local_new_ptr= (newptr);\
  if (local_new_ptr != ptr) {\
    free((char*)(ptr));\
    if (local_new_ptr != NULL)\
      (ptr)= _strdup(local_new_ptr);\
    else\
      (ptr)= NULL;\
  }\
} while(0)
#define MADB_IS_EMPTY(STR) ((STR)==NULL || *(STR)=='\0')

typedef struct st_client_charset
{
  unsigned int CodePage;
  MARIADB_CHARSET_INFO* cs_info;
} Client_Charset;

struct MADB_ERROR
{
  char SqlState[SQL_SQLSTATE_SIZE + 1];
  char SqlStateV2[SQLSTATE_LENGTH + 1];
  char SqlErrorMsg[SQL_MAX_MESSAGE_LENGTH + 1];
  SQLRETURN ReturnValue;
};

struct MADB_Error
{
  size_t PrefixLen;
  struct MADB_ERROR* ErrRecord;
  SQLINTEGER NativeError;
  /* Order number of last requested error record */
  unsigned int ErrorNum;
  char SqlErrorMsg[SQL_MAX_MESSAGE_LENGTH + 1];
  char SqlState[SQLSTATE_LENGTH + 1];
  SQLRETURN ReturnValue;
};

/**
 * Things that are staying C at least for transition period. Mainly the stuff that has to be used
 * both from C and C++ code.
 */

#ifdef __cplusplus
extern "C" {
#endif

const char* ltrim(const char *Str);
char* trim(char* Str);

#ifdef __cplusplus
} /* End of "extern C" */
#endif

#endif /* _ma_c_stuff_h_ */
