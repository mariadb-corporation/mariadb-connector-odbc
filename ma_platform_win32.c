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
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY000, "Couldn't load setup library", 0);
  }
  if (!(prompt->Call= (PromptDSN)GetProcAddress((HMODULE)prompt->LibraryHandle, "DSNPrompt")))
  {
    return MADB_SetError(&Dbc->Error, MADB_ERR_HY000, "Couldn't find DSNPrompt function in setup library", 0);
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


SQLWCHAR *MADB_ConvertToWchar(char *Ptr, int PtrLength, unsigned int CodePage)
{
  SQLWCHAR *WStr= NULL;
  int Length;

  if (PtrLength == SQL_NTS)
    PtrLength= -1;

  if (!Ptr)
    return WStr;
  if (!CodePage)
    CodePage= CP_UTF8;

  if ((Length= MultiByteToWideChar(CodePage, 0, Ptr, PtrLength, NULL, 0)))
    if ((WStr= (SQLWCHAR *)MADB_CALLOC(sizeof(WCHAR) * Length + 1)))
      MultiByteToWideChar(CodePage, 0, Ptr, PtrLength, WStr, Length);
  return WStr;
}

/* {{{ MADB_ConvertFromWChar */
char *MADB_ConvertFromWChar(SQLWCHAR *Wstr, SQLINTEGER WstrCharLen, SQLINTEGER *Length/*Bytes*/, CODEPAGE CodePage, BOOL *Error)
{
  char *AscStr;
  int AscLen, AllocLen;
  
  if (Error)
    *Error= 0;

  if (CodePage < 1)
    CodePage= CP_UTF8;
  if (WstrCharLen == SQL_NTS)
    WstrCharLen= -1;

  AllocLen= AscLen= WideCharToMultiByte(CodePage, 0, Wstr, WstrCharLen, NULL, 0, NULL, NULL);
  if (WstrCharLen != -1)
    ++AllocLen;
  
  if (!(AscStr = (char *)MADB_CALLOC(AllocLen)))
    return NULL;

  AscLen= WideCharToMultiByte(CodePage,  0, Wstr, WstrCharLen, AscStr, AscLen, NULL, (CodePage != CP_UTF8) ? Error : NULL);
  if (AscLen && WstrCharLen == -1)
    --AscLen;

  if (Length)
    *Length= (SQLINTEGER)AscLen;
  return AscStr;
}
/* }}} */


int MADB_ConvertAnsi2Unicode(int CodePage, char *AnsiString, int AnsiLength, 
                             SQLWCHAR *UnicodeString, int UnicodeLength, 
                             int *LengthIndicator, MADB_Error *Error)
{
  SQLINTEGER RequiredLength;
  SQLWCHAR  *Tmp=    UnicodeString;
  char       IsNull= 0;
  int        rc=     0;

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
  RequiredLength= MultiByteToWideChar(CodePage, 0, AnsiString, IsNull ? -IsNull : AnsiLength, NULL, 0);

  /* Set LengthIndicator */
  if (LengthIndicator)
    *LengthIndicator= RequiredLength - IsNull;
  if (!UnicodeLength)
    return 0;


  if (RequiredLength > UnicodeLength)
    Tmp= (SQLWCHAR *)malloc(RequiredLength * sizeof(SQLWCHAR));
  
  RequiredLength= MultiByteToWideChar(CodePage, 0, AnsiString, IsNull ? -IsNull : AnsiLength, Tmp, RequiredLength);
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
/* }}} */

/* {{{ MADB_SetString
       @returns number of characters available at Src */
size_t MADB_SetString(unsigned int CodePage, void *Dest, unsigned int DestLength,
                      char *Src, int SrcLength/*bytes*/, MADB_Error *Error)
{
  SQLLEN Length= 0;

  if (SrcLength == SQL_NTS)
  {
    if (Src != NULL)
    {
      /* Thinking about utf8 - Should be probably len in characters */
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
    if (!CodePage)
      return SrcLength;
    else
    {
      Length= MultiByteToWideChar(CodePage, 0, Src, SrcLength, NULL, 0);
      return Length;
    }
  }

  if (!Src || !strlen(Src) || !SrcLength)
  {
    memset((char *)Dest, 0, CodePage ? sizeof(SQLWCHAR) : sizeof(SQLCHAR));
    return 0;
  }

  if (!CodePage)
  {
    strncpy_s((char *)Dest, DestLength, Src ? Src : "", _TRUNCATE);
    /* strncpy does not write null at the end */
    *((char *)Dest + MIN((unsigned int)SrcLength, DestLength - 1))= '\0';

    if (Error && (unsigned int)SrcLength >= DestLength)
      MADB_SetError(Error, MADB_ERR_01004, NULL, 0);
    return SrcLength;
  }
  else
  {
    MADB_ConvertAnsi2Unicode(CodePage, Src, -1, (SQLWCHAR *)Dest, DestLength, &Length, Error);
    return Length;
  }
}
/* }}} */
