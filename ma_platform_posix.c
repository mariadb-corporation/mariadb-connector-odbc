/************************************************************************************
   Copyright (C) 2014 SkySQL AB
   
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
