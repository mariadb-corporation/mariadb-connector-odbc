/************************************************************************************
   Copyright (C) 2013, 2015 MariaDB Corporation AB
   
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
#ifndef _ma_debug_h_
#define _ma_debug_h_

#ifndef MAODBC_DEBUG

void ma_debug_print(my_bool ident, char *format, ...);

#define MDBUG_C_IS_ON(C) (0)
#define MDBUG_C_ENTER(C,A) {}
#define MDBUG_C_RETURN(C,A,E) return (A)
#define MDBUG_C_PRINT(C, format, args) {}
#define MDBUG_C_VOID_RETURN(C) {}
#define MDBUG_C_DUMP(C,A,B) {}

#else

#define MA_DEBUG_FLAG 4

#ifndef WIN32
#include <time.h>
#endif

void ma_debug_print(my_bool ident, char *format, ...);
void ma_debug_print_error(MADB_Error *err);

/* Debug is on for connection */
#define MDBUG_C_IS_ON(C) ((C) && (((MADB_Dbc*)(C))->Options & MA_DEBUG_FLAG))

#ifdef WIN32
#define MDBUG_C_ENTER(C,A)\
  if (MDBUG_C_IS_ON(C))\
    {\
    SYSTEMTIME st;\
    GetSystemTime(&st);\
    ma_debug_print(0, ">>> %d-%02d-%02d %02d:%02d:%02d --- %s (thread: %lu) ---", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, A, ((MADB_Dbc*)(C))->mariadb ? mysql_thread_id(((MADB_Dbc*)(C))->mariadb) : 0);\
    }
#else
#define MDBUG_C_ENTER(C,A)\
  if ((C) && (((MADB_Dbc*)(C))->Options & MA_DEBUG_FLAG))\
    {\
    time_t t = time(NULL);\
    struct tm st= *gmtime(&t);\
    ma_debug_print(0, ">>> %d-%02d-%02d %02d:%02d:%02d --- %s (thread: %d) ---", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec,  A, ((MADB_Dbc*)(C))->mariadb ? mysql_thread_id(((MADB_Dbc*)(C))->mariadb) : 0);\
    }
#endif

#define MDBUG_C_RETURN(C,A,E)\
  if (MDBUG_C_IS_ON(C))\
  {\
    SQLRETURN _ret= (A);\
    if (_ret && (E)->ReturnValue)\
      ma_debug_print_error(E); \
    ma_debug_print(0, "<<< --- end of function, returning %d ---", _ret); \
    return _ret;\
  }\
  return (A);

#define MDBUG_C_PRINT(C, format, ...)\
  if (MDBUG_C_IS_ON(C))\
    ma_debug_print(1, format, __VA_ARGS__);

#define MDBUG_C_VOID_RETURN(C)\
  if (MDBUG_C_IS_ON(C))\
    ma_debug_print(0, "<<< --- end of function ---");\
  return;

#define MDBUG_C_DUMP(C,A,B)\
  if (MDBUG_C_IS_ON(C))\
  ma_debug_print(1, #A ":\t%" #B, A);

#endif /* MAODBC_DEBUG */

/* These macros will be used to force debug output 
   for functions without a DBC handle */
#ifndef MA_ODBC_DEBUG_ALL

#define MDBUG_ENTER(A) {}
#define MDBUG_RETURN(A) return (A)
#define MDBUG_PRINT(format, args) {}
#define MDBUG_VOID_RETURN() {}
#define MDBUG_DUMP(B,C) {}

#else

#define MDBUG_ENTER(A)\
  ma_debug_print(0, ">>> --- %s ---", A);

#define MDBUG_RETURN(A)\
  ma_debug_print(0, "<<< .. end of function ---");\
  return (A);

#define MDBUG_PRINT(format, ...)\
  ma_debug_print(1, format, __VA_ARGS__);

#define MDBUG_VOID_RETURN()\
  ma_debug_print(0, "<<< --- end of function ---");\
  return;

#define MDBUG_DUMP(A,B)\
  ma_debug_print(1, #A ":\t%" #B, A);

#endif /* MA_ODBC_DEBUG_ALL */

#endif /* _ma_debug_h_ */
