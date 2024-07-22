/************************************************************************************
   Copyright (C) 2013,2023 MariaDB Corporation plc
   
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
#include "ma_odbc.h"

Client_Charset utf8= { CP_UTF8, NULL };
MARIADB_CHARSET_INFO* DmUnicodeCs= NULL;;
Client_Charset SourceAnsiCs= {0, 0}; /* Basically it should be initialized with 0 anyway */
const char* DefaultPluginLocation= NULL;
#ifndef _MAX_PATH
# define _MAX_PATH 260
#endif
static char PluginLocationBuf[_MAX_PATH];

// iOdbc does not have any support of 3.80 and does not define SQL_OV_ODBC3_80(at least in what we have in MacOS)
#ifdef SQL_OV_ODBC3_80
# define MADB_SUPPORTED_OV SQL_OV_ODBC3_80
#else
# define MADB_SUPPORTED_OV SQL_OV_ODBC3
#endif // 


static unsigned int ValidChar(const char* start, const char* end)
{
  return 4;
}
static unsigned int CharOctetLen(unsigned int utf32)
{
  return 4;
}
MARIADB_CHARSET_INFO dummyUtf32le= { 0, 1, "utf32le", "utf32_general_ci", "", 0, "UTF-32LE", 4, 4, CharOctetLen, ValidChar };
MARIADB_CHARSET_INFO * mysql_find_charset_name(const char *name);

#ifdef _WIN32
# pragma comment(lib, "ws2_32.lib")
#endif


static my_bool little_endian()
{
  int x= 1;
  char *c= (char*)&x;

  return *c;
}


/* {{{ MADB_EnvFree */
SQLRETURN MADB_EnvFree(MADB_Env *Env)
{
  if (!Env)
    return SQL_ERROR;
  delete Env;

#ifdef _WIN32
  WSACleanup();
#endif

  return SQL_SUCCESS;
}
/* }}} */

const char* GetDefaultLogDir();
int         GetSourceAnsiCs(Client_Charset *cc);

/* {{{ MADB_EnvInit */
static void DetectAppType(MADB_Env* Env)
{
  Env->AppType= ATypeGeneral;
#ifdef _WIN32
  if (GetModuleHandle("msaccess.exe") != NULL)
  {
    Env->AppType= ATypeMSAccess;
  }
#endif
}
/* }}} */

/* {{{ MADB_EnvInit */
MADB_Env *MADB_EnvInit()
{
  MADB_Env *Env= nullptr;

  #ifdef _WIN32
  /* Since we can't determine if WSAStartup has been called, we need
     to call it again 
     */
  WORD VersionRequested;
  int err;
  WSADATA WsaData;
  /* if possible use latest supported version (2.2) */
  const unsigned int MajorVersion=2,
        MinorVersion=2;
  VersionRequested= MAKEWORD(MajorVersion, MinorVersion);
  /* Load WinSock library */
  if ((err= WSAStartup(VersionRequested, &WsaData)))
  {
    /* todo: optional debug output */
    return Env;
  }
  /* make sure 2.2 or higher is supported */
  if ((LOBYTE(WsaData.wVersion) * 10 + HIBYTE(WsaData.wVersion)) < 22)
  {
    /* todo: optional debug output */
    goto cleanup;
  }
#endif
  mysql_library_init(0, NULL, NULL);
  if (!(Env= new MADB_Env()))
  {
    /* todo: optional debug output */
    goto cleanup;
  }

  MADB_PutErrorPrefix(NULL, &Env->Error);

  Env->OdbcVersion= MADB_SUPPORTED_OV;

  /* This is probably is better todo with thread_once */
  if (DmUnicodeCs == NULL)
  {
    if (sizeof(SQLWCHAR) == 2)
    {
      DmUnicodeCs= mariadb_get_charset_by_name(little_endian() ? "utf16le" : "utf16");
    }
    else
    {
      DmUnicodeCs= little_endian() ? &dummyUtf32le : mariadb_get_charset_by_name("utf32");
    }
  }
  utf8.cs_info= mariadb_get_charset_by_name("utf8mb4");
  GetDefaultLogDir();
  GetSourceAnsiCs(&SourceAnsiCs);
  /* If we have something in the buffer - then we've already tried to get default location w/out much success */
  if (DefaultPluginLocation == NULL && strlen(PluginLocationBuf) == 0)
  {
    DefaultPluginLocation= MADB_GetDefaultPluginsDir(PluginLocationBuf, sizeof(PluginLocationBuf));
  }

  DetectAppType(Env);
cleanup:
#ifdef _WIN32  
  if (!Env)
    WSACleanup();
#endif

  return Env;
}
/* }}} */

/* {{{ MADB_EnvSetAttr */
SQLRETURN MADB_EnvSetAttr(MADB_Env* Env, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{
  SQLINTEGER valueAsInt= static_cast<SQLINTEGER>((SQLLEN)ValuePtr);

  switch (Attribute) {
   case SQL_ATTR_ODBC_VERSION:
    if (!Env->Dbcs.empty())
    {
      return MADB_SetError(&Env->Error, MADB_ERR_HYC00, NULL, 0);
    }
    
    if (valueAsInt != SQL_OV_ODBC2 && valueAsInt != SQL_OV_ODBC3 && valueAsInt != MADB_SUPPORTED_OV)
    {
      return MADB_SetError(&Env->Error, MADB_ERR_HY024, NULL, 0);
    }
    Env->OdbcVersion= valueAsInt;
    break;
  case SQL_ATTR_OUTPUT_NTS:
    if (valueAsInt != SQL_TRUE)
      MADB_SetError(&Env->Error, MADB_ERR_S1C00, NULL, 0);
    break;
  default:
    MADB_SetError(&Env->Error, MADB_ERR_HYC00, NULL, 0);
    break;
  }
  return Env->Error.ReturnValue;
}
/* }}} */

/* {{{ MADB_EnvGetAttr */
SQLRETURN MADB_EnvGetAttr(MADB_Env *Env, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
                          SQLINTEGER *StringLengthPtr)
{
  MADB_CLEAR_ERROR(&Env->Error);
  switch (Attribute) {
  case SQL_ATTR_CONNECTION_POOLING:
    *(SQLUINTEGER *)ValuePtr= SQL_CP_OFF;
    break;
  case SQL_ATTR_ODBC_VERSION:
    *(SQLINTEGER *)ValuePtr= Env->OdbcVersion;
    break;
  case SQL_ATTR_OUTPUT_NTS:
    *(SQLINTEGER *)ValuePtr= SQL_TRUE;
    break;
  default:
    MADB_SetError(&Env->Error, MADB_ERR_HYC00, NULL, 0);
    break;
  }
  return Env->Error.ReturnValue;
}
 /* }}} */

MADB_Env::ListIterator MADB_Env::addConnection(MADB_Dbc* conn)
{
  std::lock_guard<std::mutex> localScopeLock(cs);
  return Dbcs.insert(Dbcs.begin(), conn);
}


void MADB_Env::forgetConnection(MADB_Env::ListIterator& it)
{
  std::lock_guard<std::mutex> localScopeLock(cs);
  Dbcs.erase(it);
}
