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

CHARSET_INFO*  utf16= NULL;
Client_Charset utf8=  {CP_UTF8, NULL};

/* {{{ ltrim */
char* ltrim(char *Str)
{
  /* I am not sure using iswspace, and not isspace makes any sense here. But probably does not hurt either */
  while (Str && iswspace(Str[0]))
    ++Str;
  return Str;
}
/* }}} */

/* {{{ trim */
char* trim(char *Str)
{
  char *end;
  
  Str= ltrim(Str);

  end= Str + strlen(Str) - 1;
  while (iswspace(*end))
    *end--= 0;
  return Str;
}
/* }}} */

