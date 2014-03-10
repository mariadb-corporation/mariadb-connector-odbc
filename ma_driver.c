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
#include <ma_odbc.h>

/* {{{ MADB_DriverInit */
MADB_Drv *MADB_DriverInit(void)
{
  return (MADB_Drv* )MADB_CALLOC(sizeof(MADB_Drv));
}

void MADB_DriverFree(MADB_Drv *Drv)
{
  if (Drv)
  {
    MADB_FREE(Drv->DriverName);
    MADB_FREE(Drv->OdbcLibrary);
    MADB_FREE(Drv->SetupLibrary);
    MADB_FREE(Drv);
  }
}

/* {{{ MADB_DriverGet */
MADB_Drv * MADB_DriverGet(char *DriverName)
{
  MADB_Drv *Drv= NULL;
  char Value[2048];

  if (!DriverName ||
      !SQLGetPrivateProfileString(DriverName, "Driver", "", Value, 2048, "ODBCINST.INI"))
     return NULL;
  Drv= MADB_DriverInit();
  Drv->DriverName= _strdup(DriverName);
  Drv->OdbcLibrary= _strdup(Value);
  if (SQLGetPrivateProfileString(DriverName, "Setup", "", Value, 2048, "ODBCINST.INI"))
    Drv->SetupLibrary= _strdup(Value);
  return Drv;
}
