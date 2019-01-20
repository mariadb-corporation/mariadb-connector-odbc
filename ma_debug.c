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
#include <ma_odbc.h>

#ifdef MAODBC_DEBUG
extern char LogFile[];

void ma_debug_print(my_bool ident, char *format, ...)
{
  FILE *fp= fopen(LogFile, "a");
  if (fp)
  {
    va_list va;
    va_start(va, format);
    if (ident)
      fprintf(fp, "\t");
    vfprintf(fp, format, va);
    fprintf(fp, "\n");
    va_end(va);
    fclose(fp);
  }
}

void ma_debug_printw(wchar_t *format, ...)
{
  FILE *fp= fopen(LogFile, "a");
  if (fp)
  {
    va_list va;
    va_start(va, format);
    fwprintf(fp, format, va);
    fwprintf(fp, L"\n");
    va_end(va);
    fclose(fp);
  }
}

void ma_debug_printv(char *format, va_list args)
{
  FILE *fp= fopen(LogFile, "a");
  if (fp)
  {
    vfprintf(fp, format, args);
    fclose(fp);
  }
}


void ma_debug_print_error(MADB_Error *err)
{
 /*TODO: Make it without #ifdefs */
#ifdef _WIN32
  SYSTEMTIME st;

  GetSystemTime(&st);
  ma_debug_print(1, "%d-%02d-%02d %02d:%02d:%02d [%s](%u)%s", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, err->SqlState, err->NativeError, err->SqlErrorMsg);
#else
  time_t t = time(NULL);\
  struct tm st = *gmtime(&t);\
  ma_debug_print(1, "%d-%02d-%02d %02d:%02d:%02d [%s](%u)%s", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, err->SqlState, err->NativeError, err->SqlErrorMsg);
#endif
}


void ma_print_value(SQLSMALLINT OdbcType, SQLPOINTER Value, SQLLEN octets)
{
  if (Value == 0)
  {
    ma_debug_print(1, "NULL ptr");
  }
  if (octets <= 0)
  {
    octets= 1;
  }
  switch (OdbcType)
  {
    case SQL_C_BIT:
    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
    case SQL_C_UTINYINT:
      ma_debug_print(1, "%d", 0 + *((char*)Value));
      break;
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
    case SQL_C_USHORT:
      ma_debug_print(1, "%d", 0 + *((short int*)Value));
      break;
    case SQL_C_LONG:
    case SQL_C_SLONG:
    case SQL_C_ULONG:
      ma_debug_print(1, "%d", 0 + *((int*)Value));
      break;
    case SQL_C_UBIGINT:
    case SQL_C_SBIGINT:
      ma_debug_print(1, "%ll", 0 + *((long long*)Value));
      break;
    case SQL_C_DOUBLE:
      ma_debug_print(1, "%f", 0.0 + *((SQLDOUBLE*)Value));
      break;
    case SQL_C_FLOAT:
      ma_debug_print(1, "%f", 0.0 + *((SQLFLOAT*)Value));
      break;
    case SQL_C_NUMERIC:
      ma_debug_print(1, "%s", "[numeric struct]");
      break;
    case SQL_C_TYPE_TIME:
    case SQL_C_TIME:
      ma_debug_print(1, "%02d:02d:02d", ((SQL_TIME_STRUCT*)Value)->hour, ((SQL_TIME_STRUCT*)Value)->minute, ((SQL_TIME_STRUCT*)Value)->second);
      break;
    case SQL_C_TYPE_DATE:
    case SQL_C_DATE:
      ma_debug_print(1, "%4d-02d-02d", ((SQL_DATE_STRUCT*)Value)->year, ((SQL_DATE_STRUCT*)Value)->month, ((SQL_DATE_STRUCT*)Value)->day);
      break;
    case SQL_C_TYPE_TIMESTAMP:
    case SQL_C_TIMESTAMP:
      ma_debug_print(1, "%4d-02d-02d %02d:02d:02d", ((SQL_TIMESTAMP_STRUCT*)Value)->year, ((SQL_TIMESTAMP_STRUCT*)Value)->month,
        ((SQL_TIMESTAMP_STRUCT*)Value)->day, ((SQL_TIMESTAMP_STRUCT*)Value)->hour, ((SQL_TIMESTAMP_STRUCT*)Value)->minute, ((SQL_TIMESTAMP_STRUCT*)Value)->second);
      break;
    case SQL_C_CHAR:
      ma_debug_print(1, "%*s%s", MIN(10, octets), (char*)Value, octets > 10 ? "..." : "");
      break;
    default:
      ma_debug_print(1, "%*X%s", MIN(10, octets), (char*)Value, octets > 10 ? "..." : "");
      break;
  }
}

/* #ifdef __APPLE__
void TravisTrace(char *format, va_list args)
{
  BOOL Travis= FALSE;
  Travis= getenv("TRAVIS") != NULL;

  if (Travis != FALSE)
  {
    printf("#");
    vprintf(format, args);
  }

}
#endif */
#endif
