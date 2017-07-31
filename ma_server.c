/************************************************************************************
   Copyright (C) 2016, 2017 MariaDB Corporation AB
   
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

/* Server-dependent functionality and helpers to use that functionality */

#include <ma_odbc.h>

unsigned long VersionCapabilityMap[][2]= {{100202, MADB_CAPABLE_EXEC_DIRECT},
                                          {100207, MADB_ENCLOSES_COLUMN_DEF_WITH_QUOTES}};

/* {{{  */
void MADB_SetCapabilities(MADB_Dbc *Dbc, unsigned long ServerVersion)
{
  int i;

  for (i= 0; i < sizeof(VersionCapabilityMap)/sizeof(VersionCapabilityMap[0]); ++i)
  {
    if (ServerVersion >= VersionCapabilityMap[i][0])
    {
      Dbc->ServerCapabilities |= VersionCapabilityMap[i][1];
    }
  }
}

BOOL MADB_ServerSupports(MADB_Dbc *Dbc, char Capability)
{
  return test(Dbc->ServerCapabilities & Capability);
}
/* }}} */

