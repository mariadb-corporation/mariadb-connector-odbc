/************************************************************************************
   Copyright (C) 2013,2019 MariaDB Corporation AB
   
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

/**
 * Things that are staying C at least for transition period. Mainly the stuff that has to be used
 * both from C and C++ code.
 */

#ifdef __cplusplus
extern "C" {
#endif

char* ltrim(char *Str);
char* trim(char* Str);

#ifdef __cplusplus
} /* End of "extern C" */
#endif

#endif /* _ma_dsn_h_ */
