/************************************************************************************
Copyright (C) 2013,2014 MariaDB Corporation AB

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

#ifndef _ma_odbc_setup_h_
#define _ma_odbc_setup_h_

#define MA_WIN_SET_VALUE(Page, Field, Value) \
 Edit_SetText(GetDlgItem(hwndTab[(Page)],(Field)), (Value));

#define MA_WIN_SET_MAXLEN(Page, Field, Value) \
 Edit_LimitText(GetDlgItem(hwndTab[(Page)],(Field)), (Value));



typedef struct 
{
  MADB_DsnKey *Key;
  int Page;
  int Item;
  int MaxLength;  /* For DSN_TYPE_CBOXGROUP that is the bit it represents */
  my_bool Mandatory;
} MADB_DsnMap;

#endif /* _ma_odbc_setup_h_ */
