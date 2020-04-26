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


/* MariaDB ODBC driver Win32 specific helper functions */

/* NOTE If you change something in this program, please consider if other platform's version 
        of the function you are changing, needs to be changed accordingly */

#include "ma_odbc.h"
#include <pathcch.h>

extern Client_Charset utf8;
char LogFile[256];

char *strndup(const char *s, size_t n)
{
  char *res= NULL;

  if (s != NULL)
  {
    size_t len= MIN(strlen(s), n);
    res= (char*)malloc(len + 1);
    if (res != NULL)
    {
      memcpy(res, s, len);
      res[len]= '\0';
    }
  }

  return res;
}


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
int DSNPrompt_Lookup(MADB_Prompt *prompt, const char * SetupLibName)
{
  if (!(prompt->LibraryHandle=(void*) LoadLibrary(SetupLibName)))
  {
    return MADB_PROMPT_COULDNT_LOAD;
  }
  if (!(prompt->Call= (PromptDSN)GetProcAddress((HMODULE)prompt->LibraryHandle, "DSNPrompt")))
  {
    return MADB_PROMPT_COULDNT_LOAD;
  }

  return SQL_SUCCESS;
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


SQLWCHAR *MADB_ConvertToWchar(const char *Ptr, SQLLEN PtrLength, Client_Charset* cc)
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
  if ((Length= MultiByteToWideChar(cc->CodePage, 0, Ptr, (int)PtrLength, NULL, 0)))
    if ((WStr= (SQLWCHAR *)MADB_CALLOC(sizeof(SQLWCHAR) * Length + 1)))
      MultiByteToWideChar(cc->CodePage, 0, Ptr, (int)PtrLength, WStr, Length);
  return WStr;
}

  /* {{{ MADB_ConvertFromWChar
  Length gets number of written bytes including TN (if WstrCharLen == -1 or SQL_NTS or if WstrCharLen includes
  TN in the Wstr) */
char *MADB_ConvertFromWChar(const SQLWCHAR *Wstr, SQLINTEGER WstrCharLen, SQLULEN *Length/*Bytes*/, Client_Charset *cc, BOOL *Error)
{
  char *AscStr;
  int AscLen, AllocLen;
  
  if (Error)
    *Error= 0;

  if (cc == NULL || cc->CodePage < 1)
  {
    cc= &utf8;
  }

  if (WstrCharLen == SQL_NTS)
    WstrCharLen= -1;

  AllocLen= AscLen= WideCharToMultiByte(cc->CodePage, 0, Wstr, WstrCharLen, NULL, 0, NULL, NULL);
  if (WstrCharLen != -1)
    ++AllocLen;
  
  if (!(AscStr = (char *)MADB_CALLOC(AllocLen)))
    return NULL;

  AscLen= WideCharToMultiByte(cc->CodePage,  0, Wstr, WstrCharLen, AscStr, AscLen, NULL, (cc->CodePage != CP_UTF8) ? Error : NULL);
  if (AscLen && WstrCharLen == -1)
    --AscLen;

  if (Length)
    *Length= (SQLINTEGER)AscLen;
  return AscStr;
}
/* }}} */

/* Required Length without or with TN(if IsNull is TRUE, or AnsiLength == -1 or SQL_NTS) is put to LenghtIndicator*/
int MADB_ConvertAnsi2Unicode(Client_Charset *cc, const char *AnsiString, SQLLEN AnsiLength,
                             SQLWCHAR *UnicodeString, SQLLEN UnicodeLength, 
                             SQLLEN *LengthIndicator, BOOL IsNull, MADB_Error *Error)
{
  SQLLEN    RequiredLength;
  SQLWCHAR *Tmp= UnicodeString;
  int       rc= 0;

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
  RequiredLength= MultiByteToWideChar(cc->CodePage, 0, AnsiString, IsNull ? -1 : (int)AnsiLength, NULL, 0);

  /* Set LengthIndicator */
  if (LengthIndicator)
    *LengthIndicator= RequiredLength - IsNull;
  if (!UnicodeLength)
    return 0;

  if (RequiredLength > UnicodeLength)
    Tmp= (SQLWCHAR *)malloc(RequiredLength * sizeof(SQLWCHAR));
  
  RequiredLength= MultiByteToWideChar(cc->CodePage, 0, AnsiString, IsNull ? -1 : (int)AnsiLength, Tmp, (int)RequiredLength);
  if (RequiredLength < 1)
  {
    if (Error)
      MADB_SetError(Error, MADB_ERR_HY000, "Ansi to Unicode conversion error occurred", GetLastError());
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

/* Returns required length for result string with(if dest and dest length are provided)
   or without terminating NULL(otherwise). If cc is NULL, or not initialized(CodePage is 0),
   then simply SrcLength is returned. 
   If Dest is not NULL, and DestLenth is 0, then error */
SQLLEN MADB_SetString(Client_Charset* cc, void *Dest, SQLULEN DestLength,
                      const char *Src, SQLLEN SrcLength/*bytes*/, MADB_Error *Error)
{
  char  *p= (char *)Dest;
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
      Length= MultiByteToWideChar(cc->CodePage, 0, Src, (int)SrcLength, NULL, 0);
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
    strncpy_s((char *)Dest, DestLength, Src ? Src : "", _TRUNCATE);
    if (Error && (SQLULEN)SrcLength >= DestLength)
      MADB_SetError(Error, MADB_ERR_01004, NULL, 0);
    return SrcLength;
  }
  else
  {
    MADB_ConvertAnsi2Unicode(cc, Src, -1, (SQLWCHAR *)Dest, DestLength, &Length, 1, Error);
    return Length;
  }
}


int GetSourceAnsiCs(Client_Charset *cc)
{
  cc->CodePage= GetACP();

  /* We don't need cs_info for this */
  return cc->CodePage;
}

BOOL MADB_DirectoryExists(const char *Path)
{
  DWORD FileAttributes = GetFileAttributes(Path);

  return (FileAttributes != INVALID_FILE_ATTRIBUTES) && (FileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

char* MADB_GetDefaultPluginsDir(char* Buffer, size_t Size)
{
  HMODULE hModule = GetModuleHandle(MADB_DRIVER_NAME);
  wchar_t wOurLocation[_MAX_PATH];
  const char *PluginsSubDirName= "\\"MADB_DEFAULT_PLUGINS_SUBDIR;
  HRESULT hr;

  memset(Buffer, 0, Size);
  GetModuleFileNameW(hModule, wOurLocation, _MAX_PATH);
  hr= PathCchRemoveFileSpec(wOurLocation, _MAX_PATH);

  WideCharToMultiByte(GetACP(), 0, wOurLocation, -1, Buffer, Size, NULL, NULL);
  if (strlen(Buffer) < Size - strlen(PluginsSubDirName))
  {
    strcpy(Buffer + strlen(Buffer), PluginsSubDirName);

    if (MADB_DirectoryExists(Buffer) != FALSE)
    {
      return Buffer;
    }
  }
  return NULL;
}

/* {{{ MADB_DSN_PossibleConnect() */
BOOL MADB_DSN_PossibleConnect(MADB_Dsn *Dsn)
{
  return Dsn->ServerName && (Dsn->IsNamedPipe || Dsn->Port > 0);
}
/* }}} */

