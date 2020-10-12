/****************************************************************************
   Copyright (C) 2012, 2020, MariaDB Corporation.
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc., 
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA

   Part of this code includes code from the PHP project which
   is freely available from http://www.php.net
*****************************************************************************/


#ifndef _WIN32
#include <strings.h>
#include <string.h>
#include <iconv.h>
#else
#include <string.h>
#endif
#include <ma_odbc.h>
#include "ma_global.h"

#define HAVE_ICONV

#ifdef HAVE_ICONV
/* {{{ MADB_MapCharsetName
   Changing charset name into something iconv understands, if necessary.
   Another purpose it to avoid BOMs in result string, adding BE if necessary
   e.g.UTF16 does not work form iconv, while UTF-16 does.
 */
static void MADB_MapCharsetName(const char *cs_name, my_bool target_cs, char *buffer, size_t buff_len)
{
  char digits[3], endianness[3]= "BE";

  if (sscanf(cs_name, "UTF%2[0-9]%2[LBE]", digits, endianness))
  {
    /* We should have at least digits. Endianness we write either default(BE), or what we found in the string */
    snprintf(buffer, buff_len, "UTF-%s%s", digits, endianness);
  }
  else
  {
    /* Not our client - copy as is*/
    strncpy(buffer, cs_name, buff_len - 1);
    buffer[buff_len - 1]= '\0';
  }

  if (target_cs)
  {
    strncat(buffer, "//TRANSLIT", buff_len - strlen(buffer));
  }
}
/* }}} */
#endif

/* {{{ MADB_ConvertString
   Converts string from one charset to another, and writes converted string to given buffer
   @param[in]     from
   @param[in/out] from_len
   @param[in]     from_cs
   @param[out]    to
   @param[in/out] to_len
   @param[in]     to_cs
   @param[out]    errorcode

   @return -1 in case of error, bytes used in the "to" buffer, otherwise
 */
size_t STDCALL MADB_ConvertString(const char *from __attribute__((unused)),
                                   size_t *from_len __attribute__((unused)),
                                   MARIADB_CHARSET_INFO *from_cs __attribute__((unused)),
                                   char *to __attribute__((unused)),
                                   size_t *to_len __attribute__((unused)),
                                   MARIADB_CHARSET_INFO *to_cs __attribute__((unused)), int *errorcode)
{
#ifndef HAVE_ICONV
  *errorcode= ENOTSUP;
  return -1;
#else
  iconv_t conv= 0;
  size_t rc= -1;
  size_t save_len= *to_len;
  char to_encoding[128], from_encoding[128];

  *errorcode= 0;

  /* check if conversion is supported */
  if (!from_cs || !from_cs->encoding || !from_cs->encoding[0] ||
      !to_cs || !to_cs->encoding || !to_cs->encoding[0])
  {
    *errorcode= EINVAL;
    return rc;
  }

  MADB_MapCharsetName(to_cs->encoding, 1, to_encoding, sizeof(to_encoding));
  MADB_MapCharsetName(from_cs->encoding, 0, from_encoding, sizeof(from_encoding));

  if ((conv= iconv_open(to_encoding, from_encoding)) == (iconv_t)-1)
  {
    *errorcode= errno;
    goto error;
  }
  if ((rc= iconv(conv, IF_WIN(,IF_SOLARIS(,(char **)))&from, from_len, &to, to_len)) == (size_t)-1)
  {
    *errorcode= errno;
    goto error;
  }
  rc= save_len - *to_len;
error:
  if (conv != (iconv_t)-1)
    iconv_close(conv);
  return rc;
#endif
}
/* }}} */

