/************************************************************************************
   Copyright (C) 2013,2019 MariaDB Corporation AB
   
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
#define _GNU_SOURCE
#include <ma_odbc.h>


#define DSNKEY_OPTIONS_INDEX   3
#define DSNKEY_OPTION_INDEX    4
#define DSNKEY_NAMEDPIPE_INDEX 5
#define DSNKEY_TCPIP_INDEX     6
#define DSNKEY_SERVER_INDEX    7
#define DSNKEY_UID_INDEX       8
#define DSNKEY_PWD_INDEX       9
#define DSNKEY_DATABASE_INDEX 10
#define DSNKEY_FP_INDEX       25
#define DSNKEY_FPLIST_INDEX   26
#define DSNKEY_STREAMRS_INDEX 43


/* If adding new connstring option, one should add them to the end of the array section before aliases, because some arrays defining program logic
   use indexes of an element in this array. In particular, in the setup lib there is array mapping dsn field(i.e. this array element) to parent window
   and resource id of the contorl in the dialog. Thus if you still think it should go somewhere in the middle, mind also change that array and at least
   DsnKeysSwitch below in this file, and index defines in this file, accordingly */
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
  {"TLSPEERFP",      offsetof(MADB_Dsn, TlsPeerFp),         DSN_TYPE_STRING, 0, 0},
  {"TLSPEERFPLIST",  offsetof(MADB_Dsn, TlsPeerFpList),     DSN_TYPE_STRING, 0, 0},
  {"SSLCRL",         offsetof(MADB_Dsn, SslCrl),            DSN_TYPE_STRING, 0, 0},
  {"SSLCRLPATH",     offsetof(MADB_Dsn, SslCrlPath),        DSN_TYPE_STRING, 0, 0},
  {"SOCKET",         offsetof(MADB_Dsn, Socket),            DSN_TYPE_STRING, 0, 0},
  {"SAVEFILE",       offsetof(MADB_Dsn, SaveFile),          DSN_TYPE_STRING, 0, 0}, /* 30 */
  {"USE_MYCNF",      offsetof(MADB_Dsn, ReadMycnf),         DSN_TYPE_OPTION, MADB_OPT_FLAG_USE_CNF, 0},
  {"TLSVERSION",     offsetof(MADB_Dsn, TlsVersion),        DSN_TYPE_CBOXGROUP, 0, 0},
  {"FORCETLS",       offsetof(MADB_Dsn, ForceTls),          DSN_TYPE_BOOL,   0, 0},
  {"SERVERKEY",      offsetof(MADB_Dsn, ServerKey),         DSN_TYPE_STRING, 0, 0},
  {"TLSKEYPWD",      offsetof(MADB_Dsn, TlsKeyPwd),         DSN_TYPE_STRING, 0, 0},
  {"INTERACTIVE",    offsetof(MADB_Dsn, InteractiveClient), DSN_TYPE_BOOL,   0, 0},
  {"FORWARDONLY",    offsetof(MADB_Dsn, ForceForwardOnly),  DSN_TYPE_OPTION, MADB_OPT_FLAG_FORWARD_CURSOR, 0},
  {"SCHEMANOERROR",  offsetof(MADB_Dsn, NeglectSchemaParam),DSN_TYPE_BOOL,   0, 0},
  {"READ_TIMEOUT",   offsetof(MADB_Dsn, ReadTimeout),       DSN_TYPE_INT,    0, 0},
  {"WRITE_TIMEOUT",  offsetof(MADB_Dsn, WriteTimeout),      DSN_TYPE_INT,    0, 0}, /* 40 */
  {"NOLOCALINFILE",  offsetof(MADB_Dsn, DisableLocalInfile),DSN_TYPE_BOOL,   0, 0},
  {"NULLISCURRENT",  offsetof(MADB_Dsn, NullSchemaMeansCurrent),DSN_TYPE_BOOL, 0, 0},
  {"STREAMRS",       offsetof(MADB_Dsn, StreamResult),      DSN_TYPE_OPTION, MADB_OPT_FLAG_NO_CACHE, 0},

  /* Aliases. Here offset is index of aliased key */
  {"SERVERNAME",     DSNKEY_SERVER_INDEX,                   DSN_TYPE_STRING, 0, 1},
  {"USER",           DSNKEY_UID_INDEX,                      DSN_TYPE_STRING, 0, 1},
  {"PASSWORD",       DSNKEY_PWD_INDEX,                      DSN_TYPE_STRING, 0, 1},
  {"DB",             DSNKEY_DATABASE_INDEX,                 DSN_TYPE_COMBO,  0, 1},
  {"SSLFP",          DSNKEY_FP_INDEX,                       DSN_TYPE_STRING, 0, 1},
  {"SSLFPLIST",      DSNKEY_FPLIST_INDEX,                   DSN_TYPE_STRING, 0, 1},
  {"NO_CACHE",       DSNKEY_STREAMRS_INDEX,                 DSN_TYPE_BOOL,   0, 1},

  /* Terminating Null */
  {NULL, 0, DSN_TYPE_BOOL,0,0}
};

#define IS_OPTIONS_BITMAP(key_index) (key_index == DSNKEY_OPTIONS_INDEX)

typedef struct
{
  unsigned int Key;
  unsigned int Dependent;
  BOOL         Same;      /* Should dependent be switched same way, or in reverse */
} MADB_DsnKeyDep;

/* Define pairs of keys that are switches, i.e. setting one should reset the other.
   Transitive dependencies have to be defined as direct dependencies here as well */
const MADB_DsnKeyDep DsnKeysSwitch[]=
{
  {DSNKEY_NAMEDPIPE_INDEX, DSNKEY_TCPIP_INDEX,     0},
  {DSNKEY_TCPIP_INDEX,     DSNKEY_NAMEDPIPE_INDEX, 0}
};

const char TlsVersionName[][8]= {"TLSv1.1", "TLSv1.2", "TLSv1.3"};
const char TlsVersionBits[]=    {MADB_TLSV11, MADB_TLSV12, MADB_TLSV13};

/* {{{ MADB_DSN_SetDefaults() */
void MADB_DSN_SetDefaults(MADB_Dsn *Dsn)
{
  Dsn->IsTcpIp= 1;
}
/* }}} */

/* {{{ MADB_Dsn_Init() */
MADB_Dsn *MADB_DSN_Init()
{
  MADB_Dsn *Dsn;

  if ((Dsn= (MADB_Dsn *)MADB_CALLOC(sizeof(MADB_Dsn))))
  {
    Dsn->Keys= (MADB_DsnKey *)&DsnKeys;
    /*Dsn->NullSchemaMeansCurrent = '\1';*/
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
  MADB_FREE(Dsn->Socket);
  MADB_FREE(Dsn->ConnCPluginsDir);
  MADB_FREE(Dsn->SslKey);
  MADB_FREE(Dsn->SslCert);
  MADB_FREE(Dsn->SslCa);
  MADB_FREE(Dsn->SslCaPath);
  MADB_FREE(Dsn->SslCipher);
  MADB_FREE(Dsn->SslCrl);
  MADB_FREE(Dsn->SslCrlPath);
  MADB_FREE(Dsn->TlsPeerFp);
  MADB_FREE(Dsn->TlsPeerFpList);
  MADB_FREE(Dsn->SaveFile);
  MADB_FREE(Dsn->ServerKey);
  MADB_FREE(Dsn->TlsKeyPwd);
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
      case DSN_TYPE_CBOXGROUP:
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

      if (*p && OverWrite == FALSE)
        break;
      /* For the case of making copy of currently stored values */
       MADB_RESET(*p, Value);
    }
    break;
  case DSN_TYPE_BOOL:
    /* If value is not set or we may overwrite it */
    if (!(*GET_FIELD_PTR(Dsn, DsnKey, my_bool) && OverWrite == FALSE))
    {
      *GET_FIELD_PTR(Dsn, DsnKey, my_bool)= atoi(Value);
    }
    break;
  case DSN_TYPE_CBOXGROUP:
    /* If value is not set or we may overwrite it */
    if (! (*GET_FIELD_PTR(Dsn, DsnKey, char) && OverWrite == FALSE))
    {
      char IntValue= atoi(Value);

      /* Atm we have only one DSN_TYPE_CBOXGROUP!!!, and following works only for it. If sometime another such field is added,
         we will need another data structure array, that will bind DSN field with string values and bits for this field.
         So far we use hardcoded arrays for the singe such field we have atm */
      if (IntValue == '\0')
      {
        unsigned int i;

        IntValue= 0;
        for (i= 0; i < sizeof(TlsVersionBits); ++i)
        {
#ifdef _AIX
          if (strstr(Value, TlsVersionName[i]) != NULL)
#else
          if (strcasestr(Value, TlsVersionName[i]) != NULL)
#endif
          {
            IntValue|= TlsVersionBits[i];
          }
        }
      }
      *GET_FIELD_PTR(Dsn, DsnKey, char)= IntValue;
    }
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
    if ((Value= strchr(KeyValue, '=')) != NULL)
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
      switch (DsnKeys[i].Type) {
      case DSN_TYPE_BOOL:
        ret= SQLWritePrivateProfileString(Dsn->DSNName, DsnKeys[i].DsnKey, 
          *GET_FIELD_PTR(Dsn, &DsnKeys[i], my_bool) ? "1" : "0", "ODBC.INI");
        break;
      case DSN_TYPE_INT:
        {
          _snprintf(Value ,32, "%d", *(int *)((char *)Dsn + DsnKeys[i].DsnOffset));
          ret= SQLWritePrivateProfileString(Dsn->DSNName, DsnKeys[i].DsnKey, Value, "ODBC.INI");
        }
        break;
      case DSN_TYPE_CBOXGROUP:
        {
          _snprintf(Value, 32, "%hu", (short)*GET_FIELD_PTR(Dsn, &DsnKeys[i], char));
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
      default:
        /* To avoid warning with some compilers */
        break;
      }  /* switch */

      if (!ret)
      {
        SQLInstallerError(1,&ErrNum, Dsn->ErrorMsg, SQL_MAX_MESSAGE_LENGTH, NULL);
        return FALSE;
      }
    }
    i++;
  }
  /* Save Options */
  _snprintf(Value ,32, "%d", Dsn->Options);
  if (!(ret= SQLWritePrivateProfileString(Dsn->DSNName, "OPTIONS", Value, "ODBC.INI")))
  {
    SQLInstallerError(1,&ErrNum, Dsn->ErrorMsg, SQL_MAX_MESSAGE_LENGTH, NULL);
    return FALSE;
  }
  return TRUE;
}
/* }}} */


size_t ConnStringLength(const char * String, char Delimiter)
{
  size_t      result= strlen(String);
  const char *p=      String + result + 1;

  if (Delimiter != '\0')
  {
    return result;
  }
  /* else - we have string with null terminated key=value pairs with additional NULL after last pair */
  while (*p)
  {
    p+= strlen(p) + 1;
  }

  return p - String /* Length without ending NULL */;
}

/* {{{ MADB_ParseConnString */
my_bool MADB_ParseConnString(MADB_Dsn *Dsn, const char *String, size_t Length, char Delimiter)
{
  char    *Buffer, *Key, *Value, *ValueBuf;
  my_bool ret= TRUE;

  if (!String)
    return FALSE;

  if (Length == SQL_NTS)
  {
    Length= ConnStringLength(String, Delimiter);
  }

  Buffer= MADB_ALLOC(Length + 1);
  Buffer= memcpy(Buffer, String, Length + 1);
  Key=    Buffer;
  ValueBuf= MADB_ALLOC(Length - 4); /*DSN=<value> - DSN or DRIVER must be in */

  while (Key && Key < ((char *)Buffer + Length))
  {
    int i= 0;

    /* The case of ;; - "empty key/value pair. Probably that shouldn't be allowed. But parser uset to digest this, so leaving this as a feature so far
       TODO: check and maybe remove for the next version */
    if (Delimiter != '\0' && *Key == Delimiter)
    {
      ++Key;
      continue;
    }
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
        char *p= NULL;

        if (DsnKeys[i].IsAlias)
        {
          i= DsnKeys[i].DsnOffset; /* For aliases DsnOffset is index of aliased "main" key */
        }

        Value= ltrim(Value);

        if (Value[0] == '{')
        {
          char *valueBufPtr= ValueBuf;
          char *prev= ++Value;
          *valueBufPtr= '\0';
          while ((p = strchr(prev, '}')) != NULL )
          {
            memcpy(valueBufPtr, prev, p - prev);
            valueBufPtr+= p - prev;
            if (*(p + 1) == '}')
            {
              *(valueBufPtr++)= '}';
              *valueBufPtr= '\0';
              prev= p + 2;
            }
            else
            {
              *valueBufPtr= '\0';
              ++p;
              break;
            }
          }
          Value= ValueBuf;
        }
        else if ((p= strchr(Value, Delimiter)))
        {
          *p= 0;
        }
        /* TODO: 3.2 we should not trim enclosed in braces, I think */
        Value= trim(Value);

        /* Overwriting here - if an option repeated more than once in the string, its last entrance will determine the value */
        if (!MADB_DsnStoreValue(Dsn, i, Value, TRUE))
        {
          ret= FALSE;
          goto end;
        }
        if (IS_OPTIONS_BITMAP(i))
        {
          MADB_DsnUpdateOptionsFields(Dsn);
        }

        if (p)
        {
          Key= p + 1;
        }
        else
        {
          Key= NULL;
        }
        break;
      }
      ++i;
    }
    /* Unknown keyword */
    if (DsnKeys[i].DsnKey == NULL)
    {
      //TODO: shouldn't some error/warning be thrown?
      Key= strchr(Value, Delimiter);
      if (Key != NULL)
      {
        ++Key;
      }
    }
  }

end:
  MADB_FREE(Buffer);
  MADB_FREE(ValueBuf);

  return ret;
}
/* }}} */

/* {{{ MADB_ReadConnString */
/* Like ParseConnString, but expands DSN if needed, preserving connection string values precedence.
   Or in other words - it is combination of ReadDsn and ParseConnString */
BOOL MADB_ReadConnString(MADB_Dsn *Dsn, const char *String, size_t Length, char Delimiter)
{
  /* Basically at this point we need DSN name only */
  if (!MADB_ParseConnString(Dsn, String, Length, Delimiter))
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
    MADB_ParseConnString(Dsn, String, Length, Delimiter);
  }
  return TRUE;
}
/* }}} */

/* {{{ MADB_DsnToString */
SQLULEN MADB_DsnToString(MADB_Dsn *Dsn, char *OutString, SQLULEN OutLength)
{
  int     i=           0;
  SQLULEN TotalLength= 0;
  char    *p=          OutString;
  char    *Value=      NULL;
  char    TmpStr[1024]= { '\0' };
  char    IntVal[12];
  int     CpyLength;

  if (OutLength && OutString)
    OutString[0]= '\0';
  
  while (DsnKeys[i].DsnKey)
  {
    Value= NULL;

    if (!DsnKeys[i].IsAlias)
    {
      switch (DsnKeys[i].Type) {
      case DSN_TYPE_STRING:
      case DSN_TYPE_COMBO:
         Value= *GET_FIELD_PTR(Dsn, &DsnKeys[i], char*);
         if (MADB_IS_EMPTY(Value))
         {
           ++i;
           continue;
         }
         break;
      case DSN_TYPE_INT:
        if (*GET_FIELD_PTR(Dsn, &DsnKeys[i], int))
        {
          _snprintf(IntVal, sizeof(IntVal), "%d", *GET_FIELD_PTR(Dsn, &DsnKeys[i], int));
          Value= IntVal;
        }
        break;
      case DSN_TYPE_BOOL:
        if (*GET_FIELD_PTR(Dsn, &DsnKeys[i], my_bool))
        {
          Value= "1";
        }
        break;
      case DSN_TYPE_CBOXGROUP:
        if (*GET_FIELD_PTR(Dsn, &DsnKeys[i], char))
        {
          _snprintf(IntVal, sizeof(IntVal), "%hu", (short)*GET_FIELD_PTR(Dsn, &DsnKeys[i], char));
          Value= IntVal;
        }
        break;
      default:
        /* To avoid warning with some compilers */
        break;
      }
    }

    if (Value)
    {
      my_bool isSpecial= (strchr(Value, ' ') ||  strchr(Value, ';') || strchr(Value, '@'));
      CpyLength= _snprintf(TmpStr + TotalLength, 1024 - TotalLength, "%s%s=%s%s%s", (TotalLength) ? ";" : "",
                             DsnKeys[i].DsnKey, isSpecial ? "{" : "", Value, isSpecial ? "}" : "");
      TotalLength+= CpyLength;
    }
    ++i;
  }

  if (OutLength && OutString)
  {
    strncpy_s(OutString, OutLength, TmpStr, TotalLength);
  }
  return TotalLength;
}
/* }}} */
