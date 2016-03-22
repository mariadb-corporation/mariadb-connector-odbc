/************************************************************************************
   Copyright (C) 2013,2016 MariaDB Corporation AB
   
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


#define DSNKEY_OPTIONS_INDEX   3
#define DSNKEY_OPTION_INDEX    4
#define DSNKEY_NAMEDPIPE_INDEX 5
#define DSNKEY_TCPIP_INDEX     6
#define DSNKEY_SERVER_INDEX    7
#define DSNKEY_UID_INDEX       8
#define DSNKEY_PWD_INDEX       9
#define DSNKEY_DATABASE_INDEX 10

MADB_DsnKey DsnKeys[]=
{
  {"DSN",            offsetof(MADB_Dsn, DSNName),           DSN_TYPE_STRING, 0, 0}, /* 0 */
  {"DESCRIPTION",    offsetof(MADB_Dsn, Description),       DSN_TYPE_STRING, 0, 0},
  {"DRIVER",         offsetof(MADB_Dsn, Driver),            DSN_TYPE_STRING, 0, 0},
  /* OPTIONS should go above all DSN_TYPE_OPTION. They are not saved in DSN separately, and then DSN is read, corresponding
     properties are filled from OPTIONS. Also putting its alias here - it should not appear on Windows(unless somebody edits
     registry manually), but on *nix we can expect everything. Array index used in some places to decide if the key is OPTIONS */
  {"OPTIONS",        offsetof(MADB_Dsn, Options),           DSN_TYPE_INT,    0, 0}, /* DSNKEY_OPTIONS_INDEX */
  {"OPTION",         DSNKEY_OPTIONS_INDEX,                  DSN_TYPE_INT,    0, 1}, /* DSNKEY_OPTION_INDEX  */

  {"NamedPipe",      offsetof(MADB_Dsn, IsNamedPipe),       DSN_TYPE_OPTION, MADB_OPT_FLAG_NAMED_PIPE, 0}, /* MADB_DSNKEY_NAMEDPIPE_INDEX */
  {"TCPIP",          offsetof(MADB_Dsn, IsTcpIp),           DSN_TYPE_BOOL,   0, 0}, /* DSNKEY_TCPIP_INDEX */
  {"SERVER",         offsetof(MADB_Dsn, ServerName),        DSN_TYPE_STRING, 0, 0}, /* DSNKEY_SERVER_INDEX     */
  {"UID",            offsetof(MADB_Dsn, UserName),          DSN_TYPE_STRING, 0, 0}, /* DSNKEY_UID_INDEX        */
  {"PWD",            offsetof(MADB_Dsn, Password),          DSN_TYPE_STRING, 0, 0}, /* DSNKEY_PWD_INDEX        */
  {"DATABASE",       offsetof(MADB_Dsn, Catalog),           DSN_TYPE_COMBO,  0, 0}, /* 10 DSNKEY_DATABASE_INDEX */
  {"PORT",           offsetof(MADB_Dsn, Port),              DSN_TYPE_INT,    0, 0},
  {"INITSTMT",       offsetof(MADB_Dsn, InitCommand),       DSN_TYPE_STRING, 0, 0},
  {"CONN_TIMEOUT",   offsetof(MADB_Dsn, ConnectionTimeout), DSN_TYPE_INT,    0, 0},
  {"AUTO_RECONNECT", offsetof(MADB_Dsn, Reconnect),         DSN_TYPE_OPTION, MADB_OPT_FLAG_AUTO_RECONNECT,0},
  {"NO_PROMPT",      offsetof(MADB_Dsn, ConnectPrompt),     DSN_TYPE_OPTION, MADB_OPT_FLAG_NO_PROMPT,0},
  {"CHARSET",        offsetof(MADB_Dsn, CharacterSet),      DSN_TYPE_COMBO,  0, 0},
  {"TRACE",          offsetof(MADB_Dsn, TraceFile),         DSN_TYPE_STRING, 0, 0},
  {"PLUGIN_DIR",     offsetof(MADB_Dsn, ConnCPluginsDir),   DSN_TYPE_STRING, 0, 0},
  /* SSL */
  {"SSLKEY",         offsetof(MADB_Dsn, SslKey),            DSN_TYPE_STRING, 0, 0},
  {"SSLCERT",        offsetof(MADB_Dsn, SslCert),           DSN_TYPE_STRING, 0, 0}, /* 20 */
  {"SSLCA",          offsetof(MADB_Dsn, SslCa),             DSN_TYPE_STRING, 0, 0},
  {"SSLCAPATH",      offsetof(MADB_Dsn, SslCaPath),         DSN_TYPE_STRING, 0, 0},
  {"SSLCIPHER",      offsetof(MADB_Dsn, SslCipher),         DSN_TYPE_STRING, 0, 0},
  {"SSLVERIFY",      offsetof(MADB_Dsn, SslVerify),         DSN_TYPE_BOOL,   0, 0},
  {"SSLFP",          offsetof(MADB_Dsn, SslFp),             DSN_TYPE_STRING, 0, 0},
  {"SSLFPLIST",      offsetof(MADB_Dsn, SslFpList),         DSN_TYPE_STRING, 0, 0},
  {"SSLCRL",         offsetof(MADB_Dsn, SslCrl),            DSN_TYPE_STRING, 0, 0},
  {"SSLCRLPATH",     offsetof(MADB_Dsn, SslCrlPath),        DSN_TYPE_STRING, 0, 0},
  {"SOCKET",         offsetof(MADB_Dsn, Socket),            DSN_TYPE_STRING, 0, 0},
  /* Aliases. Here offset is index of aliased key */
  {"SERVERNAME",     DSNKEY_SERVER_INDEX,                   DSN_TYPE_STRING, 0, 1}, /* 30 */
  {"USER",           DSNKEY_UID_INDEX,                      DSN_TYPE_STRING, 0, 1},
  {"PASSWORD",       DSNKEY_PWD_INDEX,                      DSN_TYPE_STRING, 0, 1},
  {"DB",             DSNKEY_DATABASE_INDEX,                 DSN_TYPE_COMBO,  0, 1},

  /* Terminating Null */
  {NULL, 0, DSN_TYPE_BOOL,0,0}
};

#define IS_OPTIONS_BITMAP(key_index) (key_index == DSNKEY_OPTIONS_INDEX || key_index == DSNKEY_OPTIONS_INDEX)

#define GET_FIELD_PTR(DSN, DSNKEY, TYPE) ((TYPE *)((char*)(DSN) + (DSNKEY)->DsnOffset))

typedef struct
{
  unsigned int Key;
  unsigned int Dependent;
  BOOL         Same;      /* Should dependent be switched same way, or in reverse */
} MADB_DsnKeyDep;

/* Define pairs of keys that are switches, i.e. setting one should reset the other.
   Transitive dependencies have to be defined as dierect dependencies here as well */
const MADB_DsnKeyDep DsnKeysSwitch[]=
{
  {DSNKEY_NAMEDPIPE_INDEX, DSNKEY_TCPIP_INDEX,     0},
  {DSNKEY_TCPIP_INDEX,     DSNKEY_NAMEDPIPE_INDEX, 0}
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
  MADB_FREE(Dsn->ConnCPluginsDir);
  MADB_FREE(Dsn->SslKey);
  MADB_FREE(Dsn->SslCert);
  MADB_FREE(Dsn->SslCa);
  MADB_FREE(Dsn->SslCaPath);
  MADB_FREE(Dsn->SslCipher);
  MADB_FREE(Dsn->SslCrl);
  MADB_FREE(Dsn->SslCrlPath);
  MADB_FREE(Dsn->SslFp);
  MADB_FREE(Dsn->SslFpList);

  if (Dsn->FreeMe)
    MADB_FREE(Dsn); 
}
/* }}} */


void MADB_SetOptionValue(MADB_Dsn *Dsn, MADB_DsnKey *DsnKey, my_bool value)
{
  *GET_FIELD_PTR(Dsn, DsnKey, my_bool)= value;
  if (value)
  {
    Dsn->Options |= DsnKey->FlagValue;
  }
  else
  {
    Dsn->Options &= ~DsnKey->FlagValue;
  }
}


my_bool MADB_DsnStoreValue(MADB_Dsn *Dsn, unsigned int DsnKeyIdx, char *Value, my_bool OverWrite);

/* {{{ MADB_DsnSwitchDependents */
   /* If TCPIP selected, we have to reset NAMEDPIPE */
BOOL MADB_DsnSwitchDependents(MADB_Dsn *Dsn, unsigned int Changed)
{
  int i;

  for (i= 0; i < sizeof(DsnKeysSwitch)/sizeof(MADB_DsnKeyDep); ++i)
  {
    if (DsnKeysSwitch[i].Key == Changed)
    {
      my_bool KeySet;

      switch (DsnKeys[Changed].Type)
      {
      case DSN_TYPE_STRING:
      case DSN_TYPE_COMBO:
        {
          char *str= *GET_FIELD_PTR(Dsn, &DsnKeys[Changed], char*);
          KeySet= str && *str;
        }
        break;
      case DSN_TYPE_OPTION:
      case DSN_TYPE_BOOL:
        {
          KeySet= *GET_FIELD_PTR(Dsn, &DsnKeys[Changed], my_bool);
        }
        break;
      case DSN_TYPE_INT:
        {
          KeySet= *GET_FIELD_PTR(Dsn, &DsnKeys[Changed], int) != 0;
        }
      }

      /* No problem to deal with aliases here as well, but let's keep things simple */
      if (DsnKeys[DsnKeysSwitch[i].Dependent].IsAlias != 0)
      {
        return FALSE;
      }

      switch(DsnKeys[DsnKeysSwitch[i].Dependent].Type)
      {
      case DSN_TYPE_BOOL:
        *GET_FIELD_PTR(Dsn, &DsnKeys[DsnKeysSwitch[i].Dependent], my_bool)= DsnKeysSwitch[i].Same == KeySet ? 1 : 0;
        break;
      case DSN_TYPE_OPTION:
         MADB_SetOptionValue(Dsn, &DsnKeys[DsnKeysSwitch[i].Dependent], DsnKeysSwitch[i].Same == KeySet ? 1 : 0);
        break;
      default:
        return FALSE; /* Only boolean fields are supported as dependent atm */ 
      }
    }
  }

  return TRUE;
}
/* }}} */


/* {{{ MADB_DsnStoreValue */
my_bool MADB_DsnStoreValue(MADB_Dsn *Dsn, unsigned int DsnKeyIdx, char *Value, my_bool OverWrite)
{
  MADB_DsnKey *DsnKey= &DsnKeys[DsnKeyIdx];
  if (!Dsn || DsnKey->IsAlias)
    return FALSE;

  switch(DsnKey->Type) {
  case DSN_TYPE_STRING:
  case DSN_TYPE_COMBO:
    {
      char **p= GET_FIELD_PTR(Dsn, DsnKey, char*);
      char *current= *p;

      if (current && OverWrite == FALSE)
        break;
      /* For the case of making copy of currently stored values */
       *p= _strdup(Value);
       MADB_FREE(current);
    }
    break;
  case DSN_TYPE_BOOL:
    if (*GET_FIELD_PTR(Dsn, DsnKey, my_bool) && OverWrite == FALSE)
      break;
    *GET_FIELD_PTR(Dsn, DsnKey, my_bool)= atoi(Value);
    break;
  case DSN_TYPE_INT:
    if (*GET_FIELD_PTR(Dsn, DsnKey, int) && OverWrite == FALSE)
      break;
     *GET_FIELD_PTR(Dsn, DsnKey, int)= strtoul(Value, NULL, 10);
     break;
  case DSN_TYPE_OPTION:
    if (*GET_FIELD_PTR(Dsn, DsnKey, my_bool) && OverWrite == FALSE)
      break;
    MADB_SetOptionValue(Dsn, DsnKey, strtoul(Value, NULL, 10) != 0 ? 1 : 0);

    break;
  }
  return MADB_DsnSwitchDependents(Dsn, DsnKeyIdx);
}
/* }}} */

/* {{{ MADB_DsnUpdateOptionsFields */
void MADB_DsnUpdateOptionsFields(MADB_Dsn *Dsn)
{
  int i= 0;

  while (DsnKeys[i].DsnKey != NULL)
  { 
    if (DsnKeys[i].IsAlias == 0)
    {
      if (DsnKeys[i].Type == DSN_TYPE_OPTION)
      {
        *GET_FIELD_PTR(Dsn, &DsnKeys[i], my_bool)= (my_bool)(DSN_OPTION(Dsn, DsnKeys[i].FlagValue) ? 1 : 0);
        MADB_DsnSwitchDependents(Dsn, i);
      }
    }
    ++i;
  }
}
/* }}} */

/* {{{ MADB_ReadDSN */
my_bool MADB_ReadDSN(MADB_Dsn *Dsn, const char *KeyValue, my_bool OverWrite)
{
  char *Value;
  /* if no key/value pair was specified, we will try to read Dsn->DSNName */
  if (!KeyValue)
  {
    Value= Dsn->DSNName;
  }
  else 
  {
    if (Value= strchr(KeyValue, '='))
    {
      ++Value;
      MADB_RESET(Dsn->DSNName, Value);
    }
  }
  
  if (Value)
  {
    int  i= 1;
    char KeyVal[1024];

    while (DsnKeys[i].DsnKey)
    {
      unsigned int KeyIdx= DsnKeys[i].IsAlias ? DsnKeys[i].DsnOffset : i;

      if (SQLGetPrivateProfileString(Dsn->DSNName, DsnKeys[i].DsnKey, "", KeyVal, 1024, "ODBC.INI") > 0)
      {
        if (!MADB_DsnStoreValue(Dsn, KeyIdx, KeyVal, OverWrite))
          return FALSE;
      }
      else if (DsnKeys[i].Type == DSN_TYPE_OPTION)
      {
        *GET_FIELD_PTR(Dsn, &DsnKeys[KeyIdx], my_bool)= (my_bool)(DSN_OPTION(Dsn, DsnKeys[KeyIdx].FlagValue) ? 1 : 0);
      }
      ++i;
    }
    return TRUE;
  }
  return FALSE;
}
/* }}} */

my_bool MADB_DSN_Exists(const char *DsnName)
{
  my_bool ret;
  char buffer[1024];
  char *p= "";

  if (!DsnName)
    return FALSE;

  ret= (SQLGetPrivateProfileString(DsnName, NULL, p, buffer, 1024, "ODBC.INI") > 0);
  return ret;
}

/* {{{ MADB_SaveDSN */
my_bool MADB_SaveDSN(MADB_Dsn *Dsn)
{
  int     i= 1;
  char    Value[32];
  my_bool ret;
  DWORD   ErrNum;

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
    /* Skipping aliases - options are saved by primary name only */
    if (!DsnKeys[i].IsAlias)
    {
      ret= TRUE;
      /* We do not save DSN_TYPE_OPTION - they are saved as OPTIONS bits */
      switch(DsnKeys[i].Type){
      case DSN_TYPE_BOOL:
          ret= SQLWritePrivateProfileString(Dsn->DSNName, DsnKeys[i].DsnKey, 
          *GET_FIELD_PTR(Dsn, &DsnKeys[i], my_bool) ? "1" : "0", "ODBC.INI");
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
          char *Val= *GET_FIELD_PTR(Dsn, &DsnKeys[i], char*);
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

/* {{{ MADB_ParseConnString */
my_bool MADB_ParseConnString(MADB_Dsn *Dsn, const char *String, size_t Length, char Delimiter)
{
  char    *Buffer, *Key, *Value;
  my_bool ret;

  if (!String)
    return FALSE;

  if (Length == SQL_NTS)
    Length= strlen(String);

  Buffer= _strdup(String);
  Key=    Buffer;

  while (Key && Key < ((char *)Buffer + Length))
  {
    int i= 0;
    if (!(Value= strchr(Key, '=')))
    {
      ret= FALSE;
      break;
    }

    *Value= 0;
    ++Value;
    Key= trim(Key);

    while (DsnKeys[i].DsnKey)
    {
      if (_stricmp(DsnKeys[i].DsnKey, Key) == 0)
      {
        char    *p;
        my_bool special= FALSE;

        if (DsnKeys[i].IsAlias)
        {
          i= DsnKeys[i].DsnOffset; /* For aliases DsnOffset is index of aliased "main" key */
        }

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

        /* Overwriting here - if an option repeated more than once in the string, its last entrance will determine the value */
        if (!MADB_DsnStoreValue(Dsn, i, Value, TRUE))
          return FALSE;
        if (IS_OPTIONS_BITMAP(i))
        {
          MADB_DsnUpdateOptionsFields(Dsn);
        }

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

/* {{{ MADB_ReadConnString */
/* Like ParseConnString, but expands DSN if needed, preserving connection string values precedence.
   Or in other words - it is combination of ReadDsn and ParseConnString */
BOOL MADB_ReadConnString(MADB_Dsn *Dsn, const char *String, size_t Length, char Delimiter)
{
  /* Basically at this point we need DSN name only */
  if (!MADB_ParseConnString(Dsn, String, Length, ';'))
  {
    return FALSE;
  }

  /* "If the connection string contains the DRIVER keyword, the driver cannot retrieve information about the data source
     from the system information." https://msdn.microsoft.com/en-us/library/ms715433%28v=vs.85%29.aspx */
  if (Dsn->DSNName && MADB_IS_EMPTY(Dsn->Driver))
  {
    MADB_ReadDSN(Dsn, NULL, FALSE);
    /* This redundancy is needed to be able to reset options set in the DSN, e.g. if DSN has Reconnect option selected, and
       connection string has AUTO_RECONNECT=0. Connection string should have precedence */
    MADB_ParseConnString(Dsn, String, Length, ';');
  }
  return TRUE;
}
/* }}} */

/* {{{ MADB_DsnToString */
SQLSMALLINT MADB_DsnToString(MADB_Dsn *Dsn, char *OutString, SQLSMALLINT OutLength)
{
  int           i=           0;
  SQLSMALLINT   TotalLength= 0;
  char          *p=          OutString;
  char          *Value=      NULL;
  char          TmpStr[1024];
  char          IntVal[12];
  int           CpyLength;

  if (OutLength && OutString)
    OutString[0]= '\0';
  
  while (DsnKeys[i].DsnKey)
  {
    Value= NULL;

    if (!DsnKeys[i].IsAlias) {
      switch (DsnKeys[i].Type) {
      case DSN_TYPE_STRING:
      case DSN_TYPE_COMBO:
         Value= *GET_FIELD_PTR(Dsn, &DsnKeys[i], char*);
         break;
      case DSN_TYPE_INT:
        if (*GET_FIELD_PTR(Dsn, &DsnKeys[i], int))
        {
          _snprintf(IntVal, sizeof(IntVal), "%d",*(int *)((char *)Dsn + DsnKeys[i].DsnOffset));
          Value= IntVal;
        }
        break;
      case DSN_TYPE_BOOL:
        if (*GET_FIELD_PTR(Dsn, &DsnKeys[i], my_bool))
        {
          Value= "1";
        }
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
