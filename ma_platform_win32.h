/************************************************************************************
   Copyright (C) 2014,2016 MariaDB Corporation AB
   
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


/* MariaDB ODBC driver platform dependent declarations(win32) */

/* NOTE If you change something in this program, please consider if other platform's declaration
        require similar change */

/* Only one "platform" header is supposed to be included */
#ifndef _ma_platform_x_h_

#define _ma_platform_x_h_

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#define _WINSOCKAPI_
#define DONT_DEFINE_VOID
#define HAVE_UNICODE

#include <windows.h>
#include <WinSock2.h>
#include <shlwapi.h>

#if !defined(HAVE_mit_thread) && !defined(HAVE_STRTOK_R)
#define strtok_r(A,B,C) strtok((A),(B))
#endif
#define strcasecmp(A,B) _stricmp((A),(B))

#define MADB_DRIVER_NAME "maodbc.dll"

char *strndup(const char *s, size_t n);
char* strcasestr(const char* HayStack, const char* Needle);

#endif /*_ma_platform_x_h_ */
