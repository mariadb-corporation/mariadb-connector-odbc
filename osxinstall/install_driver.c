/************************************************************************************
   Copyright (C) 2019 MariaDB Corporation AB
   
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

#define SQL_NOUNICODEMAP

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
# include <windows.h>
# include <Shlwapi.h>
#else

int _snprintf(char *buffer, size_t count, const char *format, ...)
{
  va_list list;
  va_start(list, format);
  int result= vsnprintf(buffer, count, format, list);

  va_end(list);

  /* _snprintf returns negative number if buffer is not big enough */
  if (result > count)
  {
    return count - result - 1;
  }
  return result;
}
#endif

#include <odbcinst.h>



int Usage()
{
  printf("Usage: install_driver <driver_location> [<driver_name>]\n");
  return 1;
}


int main(int argc, char** argv)
{
  unsigned int UsageCount, ErrorCode;
  char DriverDescr[1024], ErrorText[128], OutLocation[512], DriverDir[512];
  const char *DriverLocation, *DriverName, *DriverFileName, *DriverDescription= "MariaDB Connector/ODBC";

  if (argc < 2 || strcmp(argv[1], "--help") == 0)
  {
    return Usage();
  }

  DriverLocation= argv[1];

#ifdef _WIN32
  strcpy(DriverDir, DriverLocation);
  PathRemoveFileSpec(DriverDir);
#else
  {
    char *SlashLocation= strrchr(DriverLocation, '/');
    if (SlashLocation != NULL)
    {
      strncpy(DriverDir, DriverLocation, SlashLocation - DriverLocation);
      DriverDir[SlashLocation - DriverLocation]= '\0';
    }
    else
    {
      strcpy(DriverDir, ".");
    }
  }
#endif

  if (argc > 2)
  {
    DriverName= argv[2];
  }
  else
  {
    DriverName= DriverLocation;
  }

  if (argc > 3)
  {
    DriverDescription= argv[3];
  }

  if (strlen(DriverLocation) > strlen(DriverDir))
  {
    DriverFileName= DriverLocation + strlen(DriverDir) + 1;
  }
  else
  {
    DriverFileName= DriverName;
  }

  /* Our setup library does not have ConfigDriver. Thus there is no sense */
  /*SQLConfigDriver(NULL, ODBC_REMOVE_DRIVER, DriverName, NULL, NULL, 0, NULL);*/
  SQLRemoveDriver(DriverName, FALSE, &UsageCount);
  printf("Installing driver %s in %s as %s\n", DriverFileName, DriverDir, DriverName);
  _snprintf(DriverDescr, sizeof(DriverDescr), "%s%cDriver=%s%cDescription=%s%cThreading=0%c", DriverName, '\0', DriverLocation, '\0', DriverDescription, '\0', '\0');


  if (SQLInstallDriverEx(DriverDescr, DriverDir, OutLocation, sizeof(OutLocation), NULL, ODBC_INSTALL_COMPLETE, NULL) == FALSE)
  {
    SQLInstallerError(1, &ErrorCode, ErrorText, (unsigned short)sizeof(ErrorText), NULL);
    printf("An error occured while registering driver: [%u] %s\n", ErrorCode, ErrorText);

    return 1;
  }

  return 0;
}
