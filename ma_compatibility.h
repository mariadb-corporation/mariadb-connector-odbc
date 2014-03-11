/************************************************************************************
   Copyright (C) 2013 SkySQL AB
   
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
#ifndef _ma_compatibility_h_
#define _ma_compatibility_h_

#ifndef _WIN32
#include <pthread.h>
  #define CRITICAL_SECTION pthread_mutex_t
  #define InitializeCriticalSection(cs) pthread_mutex_init((cs), 0)
  #define EnterCriticalSection(cs) pthread_mutex_lock((cs))
  #define LeaveCriticalSection(cs) pthread_mutex_unlock((cs))
  #define DeleteCriticalSection(cs) pthread_mutex_destroy((cs))

  #define strncpy_s(a,b,c,d) strncpy((a),(c),(b))
#endif

#endif /* _ma_compatibility_h */
