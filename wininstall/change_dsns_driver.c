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

const char OldVersionsDriverName[][32]=  {"MariaDB ODBC 1.0 Driver", "MariaDB ODBC 2.0 Driver", "MariaDB ODBC 3.0 Driver"};
const char *DriverVersionBeingInstalled= "MariaDB ODBC 3.1 Driver";
const char *OldVersionsString=           "MariaDB ODBC 1.0 Driver, MariaDB ODBC 2.0 Driver, MariaDB ODBC 3.0 Driver";

#include <stdio.h>

int Usage()
{
  printf("Usage: change_dsns_driver [--help][--reg-direct][--also-sys][<driver_name>]\n"
         "  <driver_name> - driver to re-register DSN's, that currently use old driver versions.\n"
         "                  By default that is \"%s\"\n"
         "Options:\n"
         "  --help       - shows this message.\n"
         "  --also-sys   - process also System DSN's in addition to User ones\n"
         "  --reg-direct - change values directly in the Registry instead of using ODBC API\n"
         "Driver versions, that will be substituted:\n"
         "  %s"
         "\n",
    DriverVersionBeingInstalled, OldVersionsString);
  return 1;
}

#define SQL_NOUNICODEMAP

#include <string.h>

#ifdef _WIN32
# include "ma_platform_win32.h"
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

/* Globals */
BOOL Interactive= TRUE;


#include <odbcinst.h>
#include <sqlext.h>

typedef char my_bool;
#include "ma_dsn.h"

/*----------------------------------------------------------------------------------------------------*/
/* Stub to shut up the linker. That is easier than to add source with the real function, as that would pull other problems in.
   SHould be safe since we don't use function calling it anyway */
typedef struct st_ma_odbc_error
{
  char SqlState[SQL_SQLSTATE_SIZE + 1];
  char SqlStateV2[SQL_SQLSTATE_SIZE + 1];
  char SqlErrorMsg[SQL_MAX_MESSAGE_LENGTH + 1];
  SQLRETURN ReturnValue;
} MADB_ERROR;
typedef struct
{
  char SqlState[SQL_SQLSTATE_SIZE + 1];
  SQLINTEGER NativeError;
  char SqlErrorMsg[SQL_MAX_MESSAGE_LENGTH + 1];
  size_t PrefixLen;
  SQLRETURN ReturnValue;
  MADB_ERROR *ErrRecord;
  /* Order number of last requested error record */
  unsigned int ErrorNum;
} MADB_Error;
SQLRETURN MADB_SetError(MADB_Error *Error, unsigned int SqlErrorCode, const char *SqlErrorMsg, unsigned int NativeError)
{
  return SQL_SUCCESS;
}
MADB_ERROR MADB_ErrorList[] =
{
  { "00000", "", "", SQL_ERROR }
};
/*---------- The end of shutting up the linker ------------*/

void Message(const char* Text)
{
  if (Interactive != FALSE)
  {
    fprintf(stdout, "-- %s\n", Text);
  }
}

void Error(const char * ErrorText)
{
  if (Interactive != FALSE)
  {
    fprintf(stderr, "An Error occurred: %s\n", ErrorText);
  }
}


void Die(const char * ErrorText)
{
  Error(ErrorText);
  exit(1);
}


unsigned int Ask(const char *Question, const char *Options, unsigned int Default)
{
  unsigned int i= 0;
  int UserInput;
  if (Interactive != FALSE)
  {
    printf(Question);
    printf("(");
  }

  while (Options[i] != 0)
  {
    if (i > 0)
    {
      printf("/");
    }
    printf("%c", i == Default ? toupper(Options[i]) : Options[i]);
    ++i;
  }
  printf(")");
  UserInput= getc(stdin);

  if (UserInput != 0)
  {
    i= 0;
    while (Options[i] != 0)
    {
      if (toupper(Options[i]) == toupper(UserInput))
      {
        return i;
      }
      ++i;
    }
  }

  return Default;
}


int DriverToChange(const char *DriverName)
{
  unsigned int i;

  for (i= 0; i < sizeof(OldVersionsDriverName)/sizeof(OldVersionsDriverName[0]); ++i)
  {
    if (strcmp(OldVersionsDriverName[i], DriverName) == 0)
    {
      return 1;
    }
  }
  return 0;
}


void FreeDsnsList(char** Dsn, DWORD Count)
{
  DWORD i;

  for (i= 0; i < Count; ++i) free(Dsn[i]);
  free(Dsn);
}


const char * RegOpError()
{
  static char err[1024];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, err, sizeof(err), NULL);

  return err;
}


HKEY MA_OpenRegKey(HKEY Key, const char * Path, BOOL Fatal)
{
  HKEY Handle= NULL;
  LSTATUS ls= RegOpenKeyExA(HKEY_CURRENT_USER, Path, 0, KEY_ALL_ACCESS, &Handle);

  if (ls != ERROR_SUCCESS)
  {
    const char *err= RegOpError();

    if (Fatal != FALSE)
    {
      Die(err);
    }
    else
    {
      Error(err);
    }
  }
  return Handle;
}


int DoSingleRegBranch(HKEY Branch, const char *NewDriverName)
{
  char  DsName[SQL_MAX_DSN_LENGTH], DriverName[256];
  char  DsnSubkey[64 /*> sizeof("Software\\ODBC\\ODBC.INI\\ODBC Data Sources") */ + SQL_MAX_DSN_LENGTH],
    **DsnToChange= (char**)NULL;
  HKEY  OdbcIni, OdbcDataSources;
  DWORD DriverLen, NewDriverNameLen= strlen(NewDriverName) + 1/*Should be with TN*/;
  DWORD DsNameLen= sizeof(DsName), DsnCount= 0, i, TargetDsnCount= 0;

  OdbcIni= MA_OpenRegKey(Branch, "Software\\ODBC\\ODBC.INI", TRUE);

  RegQueryInfoKeyA(OdbcIni, NULL, NULL, NULL, &DsnCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

  for (i= 0; i < DsnCount; ++i)
  {
    RegEnumKeyExA(OdbcIni, i, DsName, &DsNameLen, NULL, NULL, NULL, NULL);

    DsNameLen= sizeof(DsName);

    DriverLen= sizeof(DriverName);
    _snprintf(DsnSubkey, sizeof(DsnSubkey), "Software\\ODBC\\ODBC.INI\\%s", DsName);
    if (RegGetValueA(OdbcIni, DsName, "Driver", RRF_RT_REG_SZ, NULL, DriverName, &DriverLen) == ERROR_SUCCESS)
    {
      if (DriverToChange(DriverName))
      {
        if (DsnToChange == NULL)
        {
          DsnToChange= calloc(DsnCount - i, sizeof(char*));
          if (DsnToChange == NULL)
          {
            Die("Could not allocate memory");
          }
        }
        DsnToChange[TargetDsnCount]= _strdup(DsName);
        ++TargetDsnCount;
        Message(DsName);
      }
    }
  }
  RegCloseKey(OdbcIni);

  /* Changing driver values in the separate cycle. Since some people promise all kinds of misery if change registry
  while navigating thru subkeys. It is simpler to do this separately, than read if this case may cause such issues */
  OdbcDataSources= MA_OpenRegKey(Branch, "Software\\ODBC\\ODBC.INI\\ODBC Data Sources", TRUE);

  for (i= 0; i < TargetDsnCount; ++i)
  {
    _snprintf(DsnSubkey, sizeof(DsnSubkey), "Software\\ODBC\\ODBC.INI\\%s", DsnToChange[i]);
    OdbcIni= MA_OpenRegKey(Branch, DsnSubkey, FALSE);
    if (OdbcIni != NULL)
    {
      LSTATUS s= RegSetValueExA(OdbcIni, "Driver", 0, REG_SZ, NewDriverName, NewDriverNameLen);
      if (s != ERROR_SUCCESS)
      {
        Error(RegOpError());
      }
      RegCloseKey(OdbcIni);
      RegSetValueExA(OdbcDataSources, DsnToChange[i], 0, REG_SZ, NewDriverName, NewDriverNameLen);
    }
  }
  RegCloseKey(OdbcDataSources);
  FreeDsnsList(DsnToChange, TargetDsnCount);
  return 0;
}

void DoRegistryDirectly(const char *NewDriverName, BOOL AlsoSystem)
{
  DoSingleRegBranch(HKEY_CURRENT_USER, NewDriverName);
  if (AlsoSystem)
  {
    DoSingleRegBranch(HKEY_LOCAL_MACHINE, NewDriverName);
  }
}


void DoOdbcWay(const char *NewDriverName, BOOL AlsoSystem)
{
  char          DsName[SQL_MAX_DSN_LENGTH], DriverName[256];
  SQLSMALLINT   DriverLen;
  SQLHANDLE     Env;
  SQLUSMALLINT  Direction= AlsoSystem != FALSE ? SQL_FETCH_FIRST : SQL_FETCH_FIRST_USER;
  SQLRETURN     rc;
  SQLSMALLINT   DsnLen;

  SQLAllocHandle(SQL_HANDLE_ENV, NULL, &Env);
  SQLSetEnvAttr(Env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)(SQLLEN)SQL_OV_ODBC2, 0);
  rc= SQLDataSources(Env, Direction, DsName, sizeof(DsName), &DsnLen, DriverName, sizeof(DriverName), &DriverLen);
  while (rc != SQL_ERROR && rc != SQL_NO_DATA)
  {
    if (DriverToChange(DriverName))
    {
      MADB_Dsn *Dsn= MADB_DSN_Init();

      if (Dsn == NULL)
      {
        Die("Could not allocate memory");
      }
      Dsn->DSNName= _strdup(DsName);
      if (!MADB_ReadDSN(Dsn, NULL, FALSE))
      {
        Error("Could not read DSN");
        MADB_DSN_Free(Dsn);

        continue;
      }

      free(Dsn->Driver);
      Dsn->Driver= _strdup(NewDriverName);

      Message(DsName);
      MADB_SaveDSN(Dsn);
      MADB_DSN_Free(Dsn);
    }
    Direction= SQL_FETCH_NEXT;
    rc= SQLDataSources(Env, Direction, DsName, sizeof(DsName), &DsnLen, DriverName, sizeof(DriverName), &DriverLen);
  }

  if (rc != SQL_NO_DATA) /* i.e. There was an error */
  {
    SQLCHAR SQLState[6];
    SQLINTEGER NativeError;
    SQLCHAR SQLMessage[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT TextLengthPtr;

    SQLGetDiagRec(SQL_HANDLE_ENV, Env, 1, SQLState, &NativeError, SQLMessage, SQL_MAX_MESSAGE_LENGTH, &TextLengthPtr);
    fprintf(stderr, "[%s] (%d) %s\n", SQLState, NativeError, SQLMessage);
  }
}


int main(int argc, char** argv)
{
  const char *NewDriverName= DriverVersionBeingInstalled;
  BOOL        RegistryDirect= FALSE, SystemDsns= FALSE;

  if (argc > 1)
  {
    if (strcmp(argv[1], "--help") == 0)
    {
      Usage();
      return 0;
    }
    else
    {
      int i;
      for (i= 1; i < argc; ++i)
      {
        if (strcmp(argv[i], "--reg-direct") == 0)
        {
          RegistryDirect= TRUE;
        }
        else if (strcmp(argv[i], "--also-sys") == 0)
        {
          SystemDsns= TRUE;
        }
        else if (strcmp(argv[i], "--quiet") == 0)
        {
          Interactive= FALSE;
        }
        else if (strncmp(argv[i], "--", 2) == 0)
        {
          Usage();
          return 1;
        }
        else
        {
          NewDriverName= argv[i];
        }
      }  /* for on argv */
    }    /* argv[1] == "--help" */
  }      /* argc > 1*/
  else
  {
    Usage();
    if (Ask("Are you sure you want to proceed with default paremeter values?", "yn", 0))
    {
      return 0;
    }
  }

  if (RegistryDirect != FALSE)
  {
    DoRegistryDirectly(NewDriverName, SystemDsns);
  }
  else
  {
    DoOdbcWay(NewDriverName, SystemDsns);
  }

  return 0;
}
