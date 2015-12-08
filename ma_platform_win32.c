/************************************************************************************
   Copyright (C) 2014,2015 MariaDB Corporation AB
   
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


/* MariaDB ODBC driver Win32 specific helper functions */

/* NOTE If you change something in this program, please consider if other platform's version 
        of the function you are changing, needs to be changed accordingly */

#include <ma_odbc.h>

char LogFile[256];


const char* GetDefaultLogDir()
{
  const char *DefaultLogDir= "c:";
  char *tmp= getenv("USERPROFILE");
  if (tmp)
  {
    DefaultLogDir= tmp;
  }

  tmp= getenv("TMP");
  if (tmp)
  {
    DefaultLogDir= tmp;
  }

  _snprintf(LogFile, sizeof(LogFile), "%s\\MAODBC.LOG", DefaultLogDir);
 
  return LogFile;
}


/* Connection is needed to set custom error */
SQLRETURN DSNPrompt_Lookup(MADB_Prompt *prompt, const char * SetupLibName, MADB_Dbc *Dbc)
{
  if (!(prompt->LibraryHandle=(void*) LoadLibrary(SetupLibName)))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_HY000, "Couldn't load setup library", 0);
  }
  if (!(prompt->Call= (PromptDSN)GetProcAddress((HMODULE)prompt->LibraryHandle, "DSNPrompt")))
  {
    MADB_SetError(&Dbc->Error, MADB_ERR_HY000, "Couldn't find DSNPrompt function in setup library", 0);
  }

  return Dbc->Error.ReturnValue;
}


int DSNPrompt_Free(MADB_Prompt *prompt)
{
  if (prompt->LibraryHandle != NULL)
  {
    FreeLibrary((HMODULE)prompt->LibraryHandle);
  }
  prompt->LibraryHandle= NULL;
  prompt->Call= NULL;

  return 0;
}
