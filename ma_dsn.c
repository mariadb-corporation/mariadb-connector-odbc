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

MADB_DsnKey DsnKeys[]=
{
  {"DSN", offsetof(MADB_Dsn, DSNName), DSN_TYPE_STRING,0,0},
  {"DESCRIPTION", offsetof(MADB_Dsn, Description), DSN_TYPE_STRING, 0,0},
  {"DRIVER", offsetof(MADB_Dsn, Driver), DSN_TYPE_STRING, 0,0},
  {"NamedPipe", offsetof(MADB_Dsn, IsNamedPipe), DSN_TYPE_OPTION,MADB_OPT_FLAG_NAMED_PIPE,1},
  {"TCPIP", offsetof(MADB_Dsn, IsTcpIp), DSN_TYPE_BOOL,0,0},
  {"SERVER", offsetof(MADB_Dsn, ServerName), DSN_TYPE_STRING,0,0},
  {"UID", offsetof(MADB_Dsn, UserName), DSN_TYPE_STRING,0,0},
  {"PWD", offsetof(MADB_Dsn, Password), DSN_TYPE_STRING,0,0},
  {"DATABASE", offsetof(MADB_Dsn, Catalog), DSN_TYPE_COMBO,0,0},
  {"PORT", offsetof(MADB_Dsn, Port), DSN_TYPE_INT,0,0},
  {"INITSTMT", offsetof(MADB_Dsn, InitCommand), DSN_TYPE_STRING,0,0},
  {"CONN_TIMEOUT", offsetof(MADB_Dsn, ConnectionTimeout), DSN_TYPE_INT,0,0},
  {"AUTO_RECONNECT", offsetof(MADB_Dsn, Reconnect), DSN_TYPE_OPTION,MADB_OPT_FLAG_AUTO_RECONNECT,0},
  {"NO_PROMPT", offsetof(MADB_Dsn, ConnectPrompt), DSN_TYPE_OPTION,MADB_OPT_FLAG_NO_PROMPT,0},
  {"CHARSET", offsetof(MADB_Dsn, CharacterSet), DSN_TYPE_COMBO,0,0},
  {"OPTIONS", offsetof(MADB_Dsn, Options), DSN_TYPE_INT, 0,0},
  {"TRACE", offsetof(MADB_Dsn, TraceFile), DSN_TYPE_STRING, 0, 0},
  /* Aliases */
  {"SERVERNAME", offsetof(MADB_Dsn, ServerName), DSN_TYPE_STRING,0,1},
  {"USER", offsetof(MADB_Dsn, UserName), DSN_TYPE_STRING,0,1},
  {"PASSWORD", offsetof(MADB_Dsn, Password), DSN_TYPE_STRING,0,1},
  {"DB", offsetof(MADB_Dsn, Catalog), DSN_TYPE_COMBO,0,1},
  {"OPTION", offsetof(MADB_Dsn, Options), DSN_TYPE_INT, 0,1},
  {NULL, 0, DSN_TYPE_BOOL}
};

/* {{{ MADB_Dsn_Init() */
MADB_Dsn *MADB_DSN_Init()
{
  MADB_Dsn *Dsn;

  if ((Dsn= (MADB_Dsn *)MADB_CALLOC(sizeof(MADB_Dsn))))
  {
    Dsn->FreeMe= TRUE;
    Dsn->Keys= (MADB_DsnKey *)&DsnKeys;
  }
  return Dsn;
}
/* }}} */

/* {{{ MADB_Dsn_Free */
void MADB_DSN_Free(MADB_Dsn *Dsn)
{
  if (!Dsn)
    return;

  MADB_FREE(Dsn->DSNName);
  MADB_FREE(Dsn->Driver);
  MADB_FREE(Dsn->Description);
  MADB_FREE(Dsn->ServerName);
  MADB_FREE(Dsn->UserName);
  MADB_FREE(Dsn->Password);
  MADB_FREE(Dsn->Catalog);
  MADB_FREE(Dsn->CharacterSet);
  MADB_FREE(Dsn->InitCommand);
  MADB_FREE(Dsn->TraceFile);

  if (Dsn->FreeMe)
    MADB_FREE(Dsn); 
}
/* }}} */


/* {{{ MADB_DsnStoreValue */
my_bool MADB_DsnStoreValue(MADB_Dsn *Dsn, size_t Offset, char *Value, int Type, my_bool OverWrite)
{
  if (!Dsn)
    return FALSE;

  switch(Type) {
  case DSN_TYPE_STRING:
  case DSN_TYPE_COMBO:
    {
      char **p= (char **)((char *)Dsn +Offset);
      char *current= *p;

      if (current && OverWrite == FALSE)
        break;
      /* For the case of making copy of currently stored values */
       *p= _strdup(Value);
       MADB_FREE(current);
    }
    break;
  case DSN_TYPE_BOOL:
    if (*(my_bool *)((char *)Dsn +Offset) && OverWrite == FALSE)
      break;
    /* *(my_bool *)((char *)Dsn +Offset)= atoi(Value); */
    Dsn->Options+= atoi(Value);
    break;
  case DSN_TYPE_INT:
    if (*(int *)((char *)Dsn +Offset) && OverWrite == FALSE)
      break;
     *(int *)((char *)Dsn + Offset)= atoi(Value);
     break; 
  }
  return TRUE;
}
/* }}} */


/* {{{ MADB_ReadDSN */
my_bool MADB_ReadDSN(MADB_Dsn *Dsn, char *KeyValue, my_bool OverWrite)
{
  char *Value;
  /* if no key/value pair was specified, we will try to read Dsn->DSNName */

  if (!KeyValue)
  {
    if (!Dsn->DSNName)
      return FALSE;
    Value= Dsn->DSNName;
  }
  else 
  {
    if (Value= strchr(KeyValue, '='))
      ++Value;
  }
  
  if (Value)
  {
    int i= 1;
    char KeyVal[1024];
    Dsn->DSNName= _strdup(Value);
    while (DsnKeys[i].DsnKey)
    {
      if (SQLGetPrivateProfileString(Dsn->DSNName, DsnKeys[i].DsnKey, "", KeyVal, 1024, "ODBC.INI") > 0)
      {
        if (!MADB_DsnStoreValue(Dsn, DsnKeys[i].DsnOffset, KeyVal, DsnKeys[i].Type, OverWrite))
          return FALSE;
      }
      ++i;
    }
    return TRUE;
  }
  return FALSE;
}
/* }}} */

my_bool MADB_DSN_Exists(char *DsnName)
{
  my_bool ret;
  char buffer[1024];
  char *p= "";
  if (!DsnName)
    return FALSE;

  ret= (SQLGetPrivateProfileString(DsnName, "", p, buffer, 1024, "ODBC.INI") > 0);
  return ret;
}

/* {{{ MADB_SaveDSN */
my_bool MADB_SaveDSN(MADB_Dsn *Dsn)
{
  int i= 1;
  char Value[32];
  my_bool ret;
  DWORD ErrNum;

  if (!SQLValidDSN(Dsn->DSNName))
  {
    strcpy_s(Dsn->ErrorMsg, SQL_MAX_MESSAGE_LENGTH, "Invalid Data Source Name");
    return FALSE;
  }

  if (!SQLRemoveDSNFromIni(Dsn->DSNName))
  {
    SQLInstallerError(1,&ErrNum, Dsn->ErrorMsg, SQL_MAX_MESSAGE_LENGTH, NULL);
    return FALSE;
  }
  if (!SQLWriteDSNToIni(Dsn->DSNName, Dsn->Driver))
  {
    SQLInstallerError(1,&ErrNum, Dsn->ErrorMsg, SQL_MAX_MESSAGE_LENGTH, NULL);
    return FALSE;
  }

  while(DsnKeys[i].DsnKey)
  {
    ret= TRUE;
    switch(DsnKeys[i].Type){
    case DSN_TYPE_BOOL:
        ret= SQLWritePrivateProfileString(Dsn->DSNName, DsnKeys[i].DsnKey, 
        *(my_bool *)((char *)Dsn + DsnKeys[i].DsnOffset) ? "1" : "0", "ODBC.INI");
      break;
    case DSN_TYPE_INT:
      {
        my_snprintf(Value ,32, "%d", *(int *)((char *)Dsn + DsnKeys[i].DsnOffset));
        ret= SQLWritePrivateProfileString(Dsn->DSNName, DsnKeys[i].DsnKey, Value, "ODBC.INI");
      }
      break;
    case DSN_TYPE_STRING:
    case DSN_TYPE_COMBO:
      {
        char *Val= *(char **)((char *)Dsn + DsnKeys[i].DsnOffset);
        if (Val && Val[0])
          ret= SQLWritePrivateProfileString(Dsn->DSNName, DsnKeys[i].DsnKey, Val, "ODBC.INI");
      }
      break;
    }
    if (!ret)
    {
      SQLInstallerError(1,&ErrNum, Dsn->ErrorMsg, SQL_MAX_MESSAGE_LENGTH, NULL);
      return FALSE;
    }
    i++;
  }
  /* Save Options */
  my_snprintf(Value ,32, "%d", Dsn->Options);
  if (!(ret= SQLWritePrivateProfileString(Dsn->DSNName, "OPTIONS", Value, "ODBC.INI")))
  {
    SQLInstallerError(1,&ErrNum, Dsn->ErrorMsg, SQL_MAX_MESSAGE_LENGTH, NULL);
    return FALSE;
  }
  return TRUE;
}
/* }}} */

/* {{{ MADB_ParseString */
my_bool MADB_ParseDSNString(MADB_Dsn *Dsn, char *String, size_t Length, char Delimiter)
{
  char *Buffer, *Key, *Value;
  my_bool ret;
  if (!String)
    return FALSE;

  if (Length == SQL_NTS)
    Length= strlen(String);

  Buffer= _strdup(String);
  Key= Buffer;

  while (Key && Key < ((char *)Buffer + Length))
  {
    int i= 0;
    if (!(Value= strchr(Key, '=')))
    {
      ret= FALSE;
      break;
    }
    *Value= 0;
    Value++;
    Key= trim(Key);
    while (DsnKeys[i].DsnKey)
    {
      if (_stricmp(DsnKeys[i].DsnKey, Key) == 0)
      {
        char *p;
        my_bool special= FALSE;
        Value= trim(Value);
        if (Value[0] == '{')
        {
          ++Value;
          if ((p = strchr(Value, '}')))
          {
            *p= 0;
            special= TRUE;
          }
        }
        else if ((p= strchr(Value, ';')))
          *p= 0;
        Value= trim(Value);

        if (!MADB_DsnStoreValue(Dsn, DsnKeys[i].DsnOffset, Value, DsnKeys[i].Type, FALSE))
          return FALSE;
        if (p)
          *p= (special) ? ' ' : ';';
        break;
      }
      ++i;
    }
    if ((Key= strchr(Value, ';')))
      ++Key;
  }
  MADB_FREE(Buffer);
  return TRUE;
}
/* }}} */

/* {{{ MADB_DsnToString */
SQLSMALLINT MADB_DsnToString(MADB_Dsn *Dsn, char *OutString, SQLSMALLINT OutLength)
{
  int i= 0;
  SQLSMALLINT TotalLength= 0;
  char *p= OutString;
  unsigned long Options= 0;
  char *Value= NULL;
  char TmpStr[1024];
  char IntVal[12];
  int CpyLength;

  if (OutLength && OutString)
    OutString[0]= '\0';
  
  while (DsnKeys[i].DsnKey)
  {
    Value= NULL;
    if (!DsnKeys[i].IsAlias) {
      switch (DsnKeys[i].Type) {
      case DSN_TYPE_STRING:
      case DSN_TYPE_COMBO:
         Value= *(char **)((char *)Dsn + DsnKeys[i].DsnOffset);
         break;
      case DSN_TYPE_INT:
        if (*(int *)((char *)Dsn + DsnKeys[i].DsnOffset))
        {
          _snprintf(IntVal, sizeof(IntVal), "%d",*(int *)((char *)Dsn + DsnKeys[i].DsnOffset));
          Value= IntVal;
        }
        break;
      case DSN_TYPE_OPTION:
        if(*(my_bool *)((char *)Dsn + DsnKeys[i].DsnOffset))
          Options+= DsnKeys[i].FlagValue;
        /* we save all boolean values in options */
        Value= NULL; 
        break;
      }
    }

    if (Value)
    {
      my_bool isSpecial= (strchr(Value, ' ') ||  strchr(Value, ';') || strchr(Value, '@'));
      CpyLength= my_snprintf(TmpStr + TotalLength, 1024 - TotalLength, "%s%s=%s%s%s", (TotalLength) ? ";" : "",
                             DsnKeys[i].DsnKey, isSpecial ? "{" : "", Value, isSpecial ? "}" : "");
      TotalLength+= CpyLength;
    }
    ++i;
  }

  if (OutLength && OutString)
    strncpy_s(OutString, OutLength, TmpStr,  TotalLength);
  return TotalLength;
}
/* }}} */
