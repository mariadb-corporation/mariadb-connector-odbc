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
#ifndef _ma_dsn_h_
#define _ma_dsn_h_

#include <odbcinst.h>

/* MySQL ODBC compatibility options */
#define MADB_OPT_FLAG_FIELD_LENGTH                      1
#define MADB_OPT_FLAG_FOUND_ROWS                        2
#define MADB_OPT_FLAG_DEBUG                             4
#define MADB_OPT_FLAG_BIG_PACKETS                       8
#define MADB_OPT_FLAG_NO_PROMPT                        16
#define MADB_OPT_FLAG_DYNAMIC_CURSOR                   32
#define MADB_OPT_FLAG_NO_SCHEMA                        64
#define MADB_OPT_FLAG_NO_DEFAULT_CURSOR               128
#define MADB_OPT_FLAG_NO_LOCALE                       256
#define MADB_OPT_FLAG_PAD_SPACE                       512
#define MADB_OPT_FLAG_FULL_COLUMN_NAMES              1024 /*10*/
#define MADB_OPT_FLAG_COMPRESSED_PROTO               2048
#define MADB_OPT_FLAG_IGNORE_SPACE                   4096
#define MADB_OPT_FLAG_NAMED_PIPE                     8192
#define MADB_OPT_FLAG_NO_BIGINT                     16384
#define MADB_OPT_FLAG_NO_CATALOG                    32768
#define MADB_OPT_FLAG_USE_CNF                       65536
#define MADB_OPT_FLAG_SAFE                         131072
#define MADB_OPT_FLAG_NO_TRANSACTIONS              262144
#define MADB_OPT_FLAG_LOG_QUERY                    524288
#define MADB_OPT_FLAG_NO_CACHE                    1048576 /*20*/
#define MADB_OPT_FLAG_FORWARD_CURSOR              2097152
#define MADB_OPT_FLAG_AUTO_RECONNECT              4194304
#define MADB_OPT_FLAG_AUTO_IS_NULL                8388608
#define MADB_OPT_FLAG_ZERO_DATE_TO_MIN           16777216
#define MADB_OPT_FLAG_MIN_DATE_TO_ZERO           33554432
#define MADB_OPT_FLAG_MULTI_STATEMENTS           67108864
#define MADB_OPT_FLAG_COLUMN_SIZE_S32           134217728
#define MADN_OPT_FLAG_NO_BINARY_RESULT          268435456
#define MADN_OPT_FLAG_BIGINT_BIND_STR           536870912
#define MADN_OPT_FLAG_NO_INFORMATION_SCHEMA    1073741824 /*30*/

enum enum_dsn_item_type {
  DSN_TYPE_STRING,
  DSN_TYPE_INT,
  DSN_TYPE_BOOL,
  DSN_TYPE_COMBO,    /* Mainly the same as string, but the field in the dialog is combobox */
  DSN_TYPE_OPTION,   /* Connection string option has correspondent OPTIONS bit */
  DSN_TYPE_CBOXGROUP /* Group of checkboxes each of them represent a bit in the field's value
                        Bitmap size is 1 byte */
};

typedef struct
{
  unsigned int Page;
  unsigned long Item;
  unsigned long value;
} MADB_OptionsMap;

typedef struct 
{
  char                    *DsnKey;
  unsigned int            DsnOffset;
  enum enum_dsn_item_type Type;
  unsigned long           FlagValue;
  my_bool                 IsAlias;
} MADB_DsnKey;

/* Definitions to tell setup library via isPrompt field what should it do */
#define MAODBC_CONFIG           0
#define MAODBC_PROMPT           1
#define MAODBC_PROMPT_REQUIRED  2

/* TLS version bits */
#define MADB_TLSV11 1
#define MADB_TLSV12 2
#define MADB_TLSV13 4

extern const char TlsVersionName[3][8];
extern const char TlsVersionBits[3];

typedef struct st_madb_dsn
{
  /* TODO: Does it really matter to keep this array in the DSN structure? */
  char ErrorMsg[SQL_MAX_MESSAGE_LENGTH];
  /*** General ***/
  char    *DSNName;
  char    *Driver;
  char    *Description;
  /*** Connection parameters ***/
  char    *ServerName;
  char    *UserName;
  char    *Password;
  char    *Catalog;
  char *CharacterSet;
  char *InitCommand;
  char *TraceFile;
  char *Socket;
  char *ConnCPluginsDir;
  /* SSL Settings */
  char *SslKey;
  char *SslCert;
  char *SslCa;
  char *SslCaPath;
  char *SslCipher;
  char *SslCrl;
  char *SslCrlPath;
  char *TlsPeerFp;
  char *TlsPeerFpList;
  char *TlsKeyPwd;
  char* ServerKey;
  char* SaveFile;
  /* --- Internal --- */
  MADB_DsnKey* Keys;
  /* Callbacke required for prompt to keep all memory de/allocation operations
     on same side of libraries */
  char* (*allocator)(size_t);
  void (*free)(void*);
  int isPrompt;
  /* Internal - end */
  unsigned int Port;
  /* Options */
  unsigned int Options;
  unsigned int ConnectionTimeout;
  unsigned int ReadTimeout;
  unsigned int WriteTimeout;
  my_bool StreamResult; /* bool so far, but in future should be changed to uint */
  my_bool Reconnect;
  my_bool MultiStatements;
  /* TRUE means "no prompt" */
  my_bool ConnectPrompt;
  my_bool IsNamedPipe;
  my_bool IsTcpIp;
  my_bool SslVerify;
  char TlsVersion;
  my_bool ForceTls;
  my_bool ReadMycnf;
  my_bool InteractiveClient;
  my_bool ForceForwardOnly;
  my_bool NeglectSchemaParam;
  my_bool DisableLocalInfile;
  my_bool NullSchemaMeansCurrent;
} MADB_Dsn;

/* this structure is used to store and retrieve DSN Information */
extern MADB_DsnKey DsnKeys[];

#define GET_FIELD_PTR(DSN, DSNKEY, TYPE) ((TYPE *)((char*)(DSN) + (DSNKEY)->DsnOffset))


/*** Function prototypes ***/
MADB_Dsn *  MADB_DSN_Init       (void);
void        MADB_DSN_SetDefaults(MADB_Dsn *Dsn);
void        MADB_DSN_Free       (MADB_Dsn *Dsn);
my_bool     MADB_ReadDSN        (MADB_Dsn *Dsn, const char *KeyValue, my_bool OverWrite);
my_bool     MADB_SaveDSN        (MADB_Dsn *Dsn);
my_bool     MADB_DSN_Exists     (const char *DsnName);
my_bool     MADB_ParseConnString(MADB_Dsn *Dsn, const char *String, size_t Length, char Delimiter);
BOOL        MADB_ReadConnString (MADB_Dsn *Dsn, const char *String, size_t Length, char Delimiter);
SQLULEN     MADB_DsnToString    (MADB_Dsn *Dsn, char *OutString, SQLULEN OutLength);
void        MADB_DsnUpdateOptionsFields (MADB_Dsn *Dsn);
BOOL        MADB_DSN_PossibleConnect    (MADB_Dsn *Dsn);

/*** Helper macros ***/
#define DSN_OPTION(_a,_b)\
  ((_a)->Options & _b)

#define MA_ODBC_CURSOR_DYNAMIC(_a)\
  DSN_OPTION((_a), MADB_OPT_FLAG_DYNAMIC_CURSOR)

#define MA_ODBC_CURSOR_FORWARD_ONLY(_a)\
  DSN_OPTION((_a), MADB_OPT_FLAG_FORWARD_CURSOR)

#define MADB_DSN_SET_STR(dsn, item, value, len)\
  if((value) && (len) != 0)\
  {\
    if ((len) == SQL_NTS)\
      (len)=(SQLSMALLINT)strlen((value));\
    MADB_FREE((dsn)->item);\
    (dsn)->item= (char *)calloc(len + 1, sizeof(char));\
    memcpy((dsn)->item, (value),(len));\
  }

#endif /* _ma_dsn_h_ */
