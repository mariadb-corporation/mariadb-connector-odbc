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


/* MariaDB ODBC driver helper functions for platforms other than windows(so far) */

/* NOTE If you change something in this program, please consider if other platform's version 
        of the function you are changing, needs to be changed accordingly */

#include <ma_odbc.h>
#include <stdarg.h>


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


/* Mimicking of VS' _snprintf */
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


/* Length in SQLWCHAR units*/
SQLINTEGER SqlwcsLen(SQLWCHAR *str)
{
  SQLINTEGER result= 0;

  if (str)
  {
    while (*str)
    {
      ++result;
      /* str+= (utf16->mb_charlen(*str))/sizeof(SQLWCHAR)); */
      ++str;
    }
  }
  return result;
}


/* CharLen < 0 - treat as NTS */
SQLINTEGER SqlwcsOctetlen(SQLWCHAR *str, SQLINTEGER *CharLen)
{
  SQLINTEGER result= 0, inChars= *CharLen;

  if (str)
  {
    while (inChars > 0 || inChars < 0 && *str)
    {
      result+= utf16->mb_charlen(*str);
      --inChars;
      str+= utf16->mb_charlen(*str)/sizeof(SQLWCHAR);
    }
  }

  if (*CharLen < 0)
  {
    *CharLen-= inChars
  }
  return result;
}


SQLINTEGER MbstrOctetLen(char *str, SQLINTEGER *CharLen, CHARSET_INFO *cs)
{
  SQLINTEGER result= 0, inChars= *CharLen;

  if (str)
  {
    if (cs->mb_charlen == NULL)
    {
      result= strlen(str);
      if (*CharLen < 0)
      {
        *CharLen-= inChars
      }
      return result;
    }
    else
    {
      while (inChars > 0 || inChars < 0 && *str)
      {
        result+= cs->mb_charlen(*str);
        --inChars;
        str+= cs->mb_charlen(*str);
      }
    }
  }

  if (*CharLen < 0)
  {
    *CharLen-= inChars
  }
  return result;
}


SQLINTEGER MbstrCharLen(char *str, SQLINTEGER OctetLen, CHARSET_INFO *cs)
{
  SQLINTEGER result= 0;
  char *ptr= str;

  if (str)
  {
    if (cs->mb_charlen == NULL || cs->char_maxlen == 1)
    {
      return OctetLen;
    }
    while (ptr < str + OctetLen)
    {
      ptr+= cs->mb_charlen(*ptr);
      ++result;
    }
  }

  return result;
}


SQLWCHAR *MADB_ConvertToWchar(char *Ptr, int PtrLength, Client_Charset* cc)
{
  SQLWCHAR *WStr= NULL;
  int Length= 0;

  if (!Ptr)
    return WStr;

  if (PtrLength == SQL_NTS)
  {
    PtrLength= -1;
    /* To copy terminating null as well */
    length= 1;
  }

  if (!cc || !cc->CodePage)
    cc= &utf8;

  Length+= MbstrOctetLen(Ptr, *PtrLength, cc->cs-info);

  if ((WStr= (SQLWCHAR *)MADB_CALLOC(sizeof(SQLWCHAR) * (PtrLength + 1))))
  {
    /* TODO: Need error processing. i.e. if mariadb_convert_string returns -1 */
    mariadb_convert_string(Ptr, Length, cc->cs_info, WStr, sizeof(SQLWCHAR) * (PtrLength + 1), utf16, NULL);
  }

  return WStr;
}


/* {{{ MADB_ConvertFromWChar */
char *MADB_ConvertFromWChar(SQLWCHAR *Ptr, SQLINTEGER PtrLength, SQLULEN *Length, Client_Charset *cc,
                            BOOL *Error)
{
  char *AscStr;
  size_t AscLen= PtrLength, PtrOctetLen;
  
  if (Error)
    *Error= 0;

  if (cc == NULL || cc->CodePage < 1)
  {
    cc= &utf8;
  }

  if (PtrLength == SQL_NTS)
  {
    /*-1 - to calculate length as of nts */
    SQLINTEGER InCharLen= -1;
    PtrOctetLen= SqlwcsOctetLen(Ptr, &InCharLen);
    /* Allocating +1 character for terminating symbol */
    AscLen= (inCharlen+1)*cc->cs_info->char_maxlen;
  }
  else
  {
    /* PtrLength is in characters. mariadb_convert_string(iconv) needs bytes */
    PtrOctetLen= SqlwcsOctetLen(Ptr, &PtrLength);
    AscLen= PtrLength*cc->cs_info->char_maxlen;
  }

  if (!(AscStr = (char *)MADB_CALLOC(AllocLen)))
    return NULL;

  AscLen= mariadb_convert_string(Ptr, PtrOctetLen, utf16, AscStr, AscLen, cc->cs_info, Error);

  if (AscLen > 0)
  {
    if (PtrLength == -1)
    {
      --AscLen;
    }
  }
  else
  {
    MADB_FREE(AscStr);
    AscLen= 0;
  }
  if (Length)
    *Length= (SQLINTEGER)AscLen;

  return AscStr;
}
/* }}} */


int MADB_ConvertAnsi2Unicode(Client_Charset *cc, char *AnsiString, int AnsiLength, 
                             SQLWCHAR *UnicodeString, int UnicodeLength, 
                             int *LengthIndicator, MADB_Error *Error)
{
  SQLINTEGER RequiredLength;
  SQLWCHAR *Tmp= UnicodeString;
  char IsNull= 0;
  int rc= 0, error;

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
  {
    IsNull= 1;
    AnsiLength= strlen(AnsiString);
  }

  /* calculate required length */
  RequiredLength= MbstrCharLen(AnsiString, AnsiLength, cc->cs-info) + isNull;

  /* Set LengthIndicator */
  if (LengthIndicator)
    *LengthIndicator= RequiredLength - isNull;
  if (!UnicodeLength)
    return 0;

  if (RequiredLength > UnicodeLength)
    Tmp= (SQLWCHAR *)malloc(RequiredLength * sizeof(SQLWCHAR));
  
  RequiredLength= mariadb_convert_string(AnsiString, AnsiLength + isNull, cc->cs_info, 
                                        Tmp, sizeof(SQLWCHAR) * RequiredLength, utf16, &error);
  if (RequiredLength < 1)
  {
    if (Error)
      MADB_SetError(Error, MADB_ERR_HY000, "Ansi to Unicode conversion error occured", error);
    rc= 1;
    goto end;
  }

  /* Truncation */
  if (Tmp != UnicodeString)
  {
    memcpy((void*)UnicodeString, (void*)Tmp, (UnicodeLength-1)*sizeof(SQLWCHAR));
    *(UnicodeString+UnicodeLength)= 0;

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
  int Length= 0;

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
    if (!cc)
      return SrcLength;
    else
    {
      Length= MbstrCharLen(Src, SrcLength, cc);
      return Length;
    }
  }

  if (!Src || !strlen(Src) || !SrcLength)
  {
    memset(p, 0, cc && cc ? sizeof(SQLWCHAR) : sizeof(SQLCHAR));
    return 0;
  }

  if (!cc)
  {
    size_t len= SrcLength;
    strncpy_s((char *)Dest, DestLength, Src ? Src : "", _TRUNCATE);
    if (Error && len >= DestLength)
      MADB_SetError(Error, MADB_ERR_01004, NULL, 0);
    return SrcLength;
  }
  else
  {
    MADB_ConvertAnsi2Unicode(cc, Src, -1, (SQLWCHAR *)Dest, DestLength, &Length, Error);
    return Length;
  }
}

