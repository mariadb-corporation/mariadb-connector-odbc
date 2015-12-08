/************************************************************************************
   Copyright (C) 2014,2015 MariaDB Corporation AB
   
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


/* MariaDB ODBC driver helper functions for platforms other than windows(so far) */

/* NOTE If you change something in this program, please consider if other platform's version 
        of the function you are changing, needs to be changed accordingly */

#include <ma_odbc.h>
#include <stdarg.h>

char LogFile[256];


void InitializeCriticalSection(CRITICAL_SECTION *cs)
{
  pthread_mutexattr_t attr;

  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(cs, &attr);
}

SQLRETURN DSNPrompt_Lookup(MADB_Prompt *prompt, const char * SetupLibName, MADB_Dbc *Dbc)
{
  MADB_SetError(&Dbc->Error, MADB_ERR_HY000, "Prompting is not supported on this platform", 0);
  return Dbc->Error.ReturnValue;
}


int DSNPrompt_Free  (MADB_Prompt *prompt)
{
  prompt->LibraryHandle= NULL;

  return 0;
}


int DSNPrompt_Free  (MADB_Prompt *prompt)
{
  prompt->LibraryHandle= NULL;

  return 0;
}

/* Mimicking of VS' _snprintf */
int _snprintf(char *buffer, size_t count, const char *format, ...)
{
    va_list list;
    va_start(list, format);
    int result= vsnprintf(buffer, count, format, list);

    va_end(list);

    /* _snprintf returns negative number if buffer is not big enough */
    if (result > count)
    {
      return count - result - 1;
    }
    return result;
}


int strcpy_s(char *dest, size_t buffer_size, const char *src)
{
  size_t src_len;

  if (dest == NULL)
  {
    return EINVAL;
  }

  if (src == NULL)
  {
    *dest= '\0';
    return EINVAL;
  }

  src_len= strlen(src);

  if (buffer_size < src_len + 1)
  {
    *dest= 0;
    return ERANGE;
  }

  memcpy((void*)dest, (void*)src, src_len + 1);

  return 0;
}


const char* GetDefaultLogDir()
{
  const char *DefaultLogDir="/tmp";
  char *tmp= getenv("HOME");

  if (tmp)
  {
    DefaultLogDir= tmp;
  }

  _snprintf(LogFile, sizeof(LogFile), "%s/maodbc.log", DefaultLogDir);

  return LogFile;
}

/* Length in SQLWCHAR units*/
SQLINTEGER SqlwcsLen(SQLWCHAR *str)
{
  SQLINTEGER result= 0;

  if (str)
  {
    while (*str)
    {
      ++result;
      /* str+= (utf16->mb_charlen(*str))/sizeof(SQLWCHAR)); */
      ++str;
    }
  }
  return result;
}

/* CharLen < 0 - treat as NTS */
SQLINTEGER SqlwcsOctetLen(SQLWCHAR *str, SQLINTEGER *CharLen)
{
  SQLINTEGER result= 0, inChars= *CharLen;

  if (str)
  {
    while (inChars > 0 || inChars < 0 && *str)
    {
      result+= utf16->mb_charlen(*str);
      --inChars;
      str+= utf16->mb_charlen(*str)/sizeof(SQLWCHAR);
    }
  }

  if (*CharLen < 0)
  {
    *CharLen-= inChars;
  }
  return result;
}

