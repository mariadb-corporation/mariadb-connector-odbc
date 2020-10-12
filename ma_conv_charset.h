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

#ifndef _MA_CONV_CHARSET_H_
#define _MA_CONV_CHARSET_H_

#include "mariadb_ctype.h"

size_t MADB_ConvertString(const char *from __attribute__((unused)),
                          size_t *from_len __attribute__((unused)),
                          MARIADB_CHARSET_INFO *from_cs __attribute__((unused)),
                          char *to __attribute__((unused)),
                          size_t *to_len __attribute__((unused)),
                          MARIADB_CHARSET_INFO *to_cs __attribute__((unused)), int *errorcode);
#endif
