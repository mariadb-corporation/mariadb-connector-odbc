/************************************************************************************
   Copyright (C) 2016 MariaDB Corporation AB
   
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

/* Common functions used in both connector and setup library.
 * Moved to avoid redundant dependencies */

#include <ma_odbc.h>
#include <wctype.h>


/* {{{ ltrim */
char* ltrim(char *Str)
{
  if (Str)
  {
    while (*Str && (*Str > 0 && isspace(*Str)))
      ++Str;
  }
  return Str;
}
/* }}} */

/* {{{ trim */
char* trim(char *Str)
{
  char *end;
  
  Str= ltrim(Str);

  end= Str + strlen(Str) - 1;
  while (isspace(*end))
    *end--= 0;
  return Str;
}
/* }}} */

/* Windows only common functions */
#ifdef _WIN32

/* {{{ strcasestr() */
char* strcasestr(const char* HayStack, const char* Needle)
{
  return StrStrIA(HayStack, Needle);
}
/* }}} */

#endif
