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

extern Client_Charset utf8;
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


SQLWCHAR *MADB_ConvertToWchar(char *Ptr, int PtrLength, Client_Charset* cc)
{
  SQLWCHAR *WStr= NULL;
  int Length;

  if (!Ptr)
    return WStr;

  if (PtrLength == SQL_NTS)
    PtrLength= -1;

  if (!cc || !cc->CodePage)
    cc= &utf8;

  /* TODO: Check this
     In fact MultiByteToWideChar does not copy terminating character by default
     Thus +1 does not make sense
     "If the function succeeds and cchWideChar is 0, the return value is the required size,
      in characters, for the buffer indicated by lpWideCharStr...
      MultiByteToWideChar does not null-terminate an output string if the input string length
      is explicitly specified without a terminating null character. To null-terminate an output
      string for this function, the application should pass in -1 or explicitly count the
      terminating null character for the input string." */
  if ((Length= MultiByteToWideChar(cc->CodePage, 0, Ptr, PtrLength, NULL, 0)))
    if ((WStr= (SQLWCHAR *)MADB_CALLOC(sizeof(SQLWCHAR) * Length + 1)))
      MultiByteToWideChar(cc->CodePage, 0, Ptr, PtrLength, WStr, Length);
  return WStr;
}

/* {{{ MADB_ConvertFromWChar */
char *MADB_ConvertFromWChar(SQLWCHAR *Ptr, SQLINTEGER PtrLength, SQLULEN *Length, Client_Charset *cc, BOOL *Error)
{
  char *AscStr;
  int AscLen, AllocLen;
  
  if (Error)
    *Error= 0;

  if (cc == NULL || cc->CodePage < 1)
  {
    cc= &utf8;
  }

  if (PtrLength == SQL_NTS)
    PtrLength= -1;

  AllocLen= AscLen= WideCharToMultiByte(cc->CodePage, 0, Ptr, PtrLength, NULL, 0, NULL, NULL);
  if (PtrLength != -1)
    ++AllocLen;
  
  if (!(AscStr = (char *)MADB_CALLOC(AllocLen)))
    return NULL;

  AscLen= WideCharToMultiByte(cc->CodePage,  0, Ptr, PtrLength, AscStr, AscLen, NULL, (cc->CodePage != CP_UTF8) ? Error : NULL);
  if (AscLen && PtrLength == -1)
    --AscLen;

  if (Length)
    *Length= (SQLINTEGER)AscLen;
  return AscStr;
}
/* }}} */


int MADB_ConvertAnsi2Unicode(Client_Charset *cc, char *AnsiString, int AnsiLength, 
                             SQLWCHAR *UnicodeString, int UnicodeLength, 
                             SQLLEN *LengthIndicator, BOOL IsNull, MADB_Error *Error)
{
  SQLLEN RequiredLength;
  SQLWCHAR *Tmp= UnicodeString;
  int rc= 0;

  if (LengthIndicator)
    *LengthIndicator= 0;

  if (Error)
    MADB_CLEAR_ERROR(Error);

  if (!AnsiLength || UnicodeLength < 0)
  {
    if (Error)
      MADB_SetError(Error, MADB_ERR_HY090, NULL, 0);
    return 1;
  }

  if (AnsiLength == SQL_NTS || AnsiLength == -1)
    IsNull= 1;

  /* calculate required length */
  RequiredLength= MultiByteToWideChar(cc->CodePage, 0, AnsiString, IsNull ? -IsNull : AnsiLength, NULL, 0);

  /* Set LengthIndicator */
  if (LengthIndicator)
    *LengthIndicator= RequiredLength - IsNull;
  if (!UnicodeLength)
    return 0;

  if (RequiredLength > UnicodeLength)
    Tmp= (SQLWCHAR *)malloc(RequiredLength * sizeof(SQLWCHAR));
  
  RequiredLength= MultiByteToWideChar(cc->CodePage, 0, AnsiString, IsNull ? -IsNull : AnsiLength, Tmp, RequiredLength);
  if (RequiredLength < 1)
  {
    if (Error)
      MADB_SetError(Error, MADB_ERR_HY000, "Ansi to Unicode conversion error occured", GetLastError());
    rc= 1;
    goto end;
  }

  /* Truncation */
  if (Tmp != UnicodeString)
  {
   
    wcsncpy(UnicodeString, L"", 1);
    wcsncat(UnicodeString, Tmp, UnicodeLength- 1);
    if (Error)
      MADB_SetError(Error, MADB_ERR_01004, NULL, 0);
  }
end:
  if (Tmp != UnicodeString)
    free(Tmp);
  return rc;
}

size_t MADB_SetString(Client_Charset* cc, void *Dest, unsigned int DestLength,
                      char *Src, int SrcLength, MADB_Error *Error)
{
  char *p= (char *)Dest;
  SQLLEN Length= 0;

  if (SrcLength == SQL_NTS)
  {
    if (Src != NULL)
    {
      SrcLength= strlen(Src);
    }
    else
    {
      SrcLength= 0;
    }
  }

  /* Not enough space */
  if (!DestLength || !Dest)
  {
    if (Dest)
      MADB_SetError(Error, MADB_ERR_01004, NULL, 0);
    if (!cc || !cc->CodePage)
      return SrcLength;
    else
    {
      Length= MultiByteToWideChar(cc->CodePage, 0, Src, SrcLength, NULL, 0);
      return Length;
    }
  }

  if (!Src || !strlen(Src) || !SrcLength)
  {
    memset(p, 0, cc && cc->CodePage ? sizeof(SQLWCHAR) : sizeof(SQLCHAR));
    return 0;
  }

  if (!cc || !cc->CodePage)
  {
    size_t len= SrcLength;
    strncpy_s((char *)Dest, DestLength, Src ? Src : "", _TRUNCATE);
    if (Error && len >= DestLength)
      MADB_SetError(Error, MADB_ERR_01004, NULL, 0);
    return SrcLength;
  }
  else
  {
    MADB_ConvertAnsi2Unicode(cc, Src, -1, (SQLWCHAR *)Dest, DestLength, &Length, 1, Error);
    return Length;
  }
}
