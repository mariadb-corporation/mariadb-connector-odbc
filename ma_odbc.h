/************************************************************************************
   Copyright (C) 2013,2015 MariaDB Corporation AB
   
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
#ifndef _ma_odbc_h_
#define _ma_odbc_h_

#include <ma_compatibility.h>

#ifdef _WIN32
# include "ma_platform_win32.h"
#else
# include "ma_platform_posix.h"
#endif

#define ODBCVER 0x0351

#include <stdlib.h>

#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>

#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>

#include <errmsg.h>
#include <string.h>

#include <ma_odbc_version.h>

typedef struct st_ma_odbc_connection MADB_Dbc;
typedef struct st_ma_odbc_stmt MADB_Stmt;

typedef struct st_ma_odbc_error
{
  char SqlState[SQL_SQLSTATE_SIZE + 1];
  char SqlStateV2[SQLSTATE_LENGTH + 1];
  char SqlErrorMsg[SQL_MAX_MESSAGE_LENGTH + 1];
  SQLRETURN ReturnValue;
} MADB_ERROR;

typedef struct
{
  char SqlState[SQLSTATE_LENGTH + 1];
  SQLINTEGER NativeError;
  char SqlErrorMsg[SQL_MAX_MESSAGE_LENGTH + 1];
  SQLRETURN ReturnValue;
  MADB_ERROR *ErrRecord;
} MADB_Error;

typedef struct
{
  SQLUINTEGER TargetType;
  SQLPOINTER TargetValuePtr;
  SQLLEN BufferLength;
  SQLLEN Utf8BufferLength;
  SQLLEN *StrLen_or_Ind;
  void *InternalBuffer; /* used for conversion */
} MADB_ColBind;

typedef struct
{
  SQLSMALLINT InputOutputType;
  SQLSMALLINT ValueType;
  SQLSMALLINT ParameterType;
  SQLULEN ColumnSize;
  SQLSMALLINT DecimalDigits; 
  SQLPOINTER ParameterValuePtr;
  SQLLEN BufferLength;
  SQLLEN *StrLen_or_IndPtr;
  void *InternalBuffer; /* used for conversion */
} MADB_ParmBind;

typedef struct
{
  /* Header */
  SQLSMALLINT AllocType;
  SQLULEN ArraySize;
  SQLUSMALLINT *ArrayStatusPtr;
  SQLULEN *BindOffsetPtr;
  SQLINTEGER BindType;
  SQLLEN Count;
  SQLULEN *RowsProcessedPtr;
  /* Header end */
} MADB_Header;

typedef struct
{
	SQLLEN RowsetSize; /* for ODBC3 fetch operation */
	SQLUINTEGER	BindSize;	/* size of each structure if using * Row-wise Binding */
	SQLUSMALLINT	*RowOperationPtr;
	SQLULEN		*RowOffsetPtr;
  MADB_ColBind *ColumnBind;
	MYSQL_BIND *Bind;
	SQLSMALLINT	Allocated;
	SQLLEN		size_of_rowset_odbc2; /* for SQLExtendedFetch */
} MADB_Ard;

typedef struct
{
	SQLLEN ParamsetSize;
	SQLUINTEGER	ParamBindType;
	SQLUSMALLINT *ParamOperationPtr;
	SQLULEN *ParamOffsetPtr;
	MADB_ParmBind *ParamBind;
	MYSQL_BIND *Bind;
	SQLSMALLINT	Allocated;
	SQLLEN Dummy; /* dummy item to fit APD to ARD */
} MADB_Apd;

typedef struct
{
  MADB_Stmt *stmt;
	SQLULEN *RowsFetched;
	SQLUSMALLINT *RowStatusArray;
	SQLUINTEGER FieldCount;
	SQLSMALLINT	Allocated;
	MYSQL_FIELD *Fields;
} MADB_Ird;

typedef struct
{
  MADB_Header Header;
#if (ODBCVER >= 0x0300)
	SQLUINTEGER *ParamProcessedPtr;
#else
	SQLULEN *ParamProcessedPtr; /* SQLParamOptions */
#endif /* ODBCVER */
	SQLUSMALLINT *ParamStatusPtr;
	SQLSMALLINT Allocated;
	MADB_ParmBind *Parameters;
} MADB_Ipd;

typedef struct {
  SQLINTEGER AutoUniqueValue;
  char *BaseCatalogName;
  char *BaseColumnName;
  char *BaseTableName;
  SQLINTEGER CaseSensitive;
  char *CatalogName;
  char *ColumnName;
  SQLSMALLINT ConciseType;
  SQLPOINTER DataPtr;
  SQLSMALLINT DateTimeIntervalCode;
  SQLINTEGER DateTimeIntervalPrecision;
  SQLINTEGER DescLength;
  SQLLEN DisplaySize;
  SQLSMALLINT FixedPrecScale;
  SQLLEN *IndicatorPtr;
  char *Label;
  SQLULEN Length;
  char *LiteralPrefix;
  char *LiteralSuffix;
  char *LocalTypeName;
  SQLSMALLINT Nullable;
  SQLINTEGER NumPrecRadix;
  SQLLEN OctetLength;
  SQLLEN *OctetLengthPtr;
  SQLSMALLINT ParameterType;
  SQLSMALLINT Precision;
  SQLSMALLINT RowVer;
  SQLSMALLINT Scale;
  char *SchemaName;
  SQLSMALLINT Searchable;
  char *TableName;
  SQLSMALLINT Type;
  char *TypeName;
  SQLSMALLINT Unnamed;
  SQLSMALLINT Unsigned;
  SQLSMALLINT Updateable;
  unsigned long InternalLength;
  char *InternalBuffer; /* used for internal conversion */
  char *DefaultValue;
  char *DaeData;
  unsigned long DaeDataLength;
  my_bool PutData;
  my_bool inUse;
  my_bool TruncError;
} MADB_DescRecord;

typedef struct
{
  MADB_Header Header;
  SQLINTEGER DescType;  /* SQL_ATTR_APP_ROW_DESC or SQL_ATTR_APP_PARAM_DESC */
  my_bool AppType;      /* Allocated by Application ? */
  DYNAMIC_ARRAY Records;
  DYNAMIC_ARRAY Stmts;
  MADB_Error Error;
  MADB_Dbc * Dbc;       /* Disconnect must automatically free allocated descriptors. Thus
                           descriptor has to know the connection it is allocated on */
  LIST ListItem;        /* To store in the dbc */
  union {
    MADB_Ard Ard;
    MADB_Apd Apd;
    MADB_Ipd Ipd;
    MADB_Ird Ird;
  } Fields;
} MADB_Desc;

struct st_ma_desc_fldid
{
  SQLSMALLINT FieldIdentifier;
  SQLSMALLINT Access[4];
};

struct st_ma_stmt_methods;

struct st_bind_column
{
  SQLUINTEGER TargetType;
  SQLPOINTER TargetValuePtr;
  SQLLEN BufferLength;
  SQLLEN Utf8BufferLength;
  SQLLEN *StrLen_or_Ind;
  void *InternalBuffer;
};

struct st_bind_param
{
  SQLSMALLINT InputOutputType;
  SQLSMALLINT ValueType;
  SQLSMALLINT ParameterType;
  SQLULEN ColumnSize;
  SQLSMALLINT DecimalDigits; 
  SQLPOINTER ParameterValuePtr;
  SQLLEN BufferLength;
  SQLLEN *StrLen_or_IndPtr;
  void *InternalBuffer;
};

typedef struct 
{
 	SQLLEN MaxRows;
	SQLLEN MaxLength;
	SQLLEN KeysetSize;
  SQLUINTEGER CursorType;
	SQLUINTEGER	ScrollConcurrency;
  SQLUINTEGER RetrieveData;
	SQLUINTEGER UseBookmarks;
	void* BookmarkPtr;
  SQLLEN BookmarkLength;
  SQLSMALLINT BookmarkType;
	SQLUINTEGER	MetadataId;
  SQLULEN SimulateCursor;
} MADB_StmtOptions;

typedef struct
{
  char *Name;
  long Position;
} MADB_Cursor;

enum MADB_DaeType {MADB_DAE_NORMAL=0, MADB_DAE_ADD=1, MADB_DAE_UPDATE=2, MADB_DAE_DELETE=3};

typedef struct {
  DYNAMIC_ARRAY tokens;
} MADB_QUERY;

struct st_ma_odbc_stmt
{
  MADB_Dbc *Connection;
  struct st_ma_stmt_methods *Methods;
  MADB_StmtOptions Options;
  MADB_Error Error;
  MADB_Cursor Cursor;
  MYSQL_STMT *stmt;
  LIST ListItem;
  MADB_QUERY *Tokens;
  SQLINTEGER ColumnCount;
  SQLINTEGER ParamCount;
  my_bool isMultiQuery;
  unsigned int FetchType;
  enum MADB_DaeType DataExecutionType;
  MYSQL_RES *DefaultsResult;
  int ArrayOffset;
  SQLSETPOSIROW DaeRowNumber;
  int Status;
  MADB_DescRecord *PutDataRec;
  MADB_Stmt *DaeStmt;
  char *StmtString;
  char *NativeSql;
  MADB_Stmt *PositionedCursor;
  unsigned int PositionedCommand;
  my_bool EmulatedStmt;
  unsigned int MultiStmtCount;
  MYSQL_STMT **MultiStmts;
  unsigned int MultiStmtNr;
  unsigned int MultiStmtMaxParam;
  unsigned long LastRowFetched;
  struct st_bind_column *bind_columns; /* ARD */
  struct st_bind_param *bind_params;
  MYSQL_BIND *result;
  MYSQL_BIND *params;
  int PutParam;
  my_bool RebindParams;
  my_bool bind_done;
  SQLBIGINT AffectedRows;
  unsigned long *CharOffset;
  unsigned long *Lengths;
  char *TableName;
  char *CatalogName;
  char TmpBuf[1]; /* for null bindings */
  MYSQL_FIELD *BulkFields;
  /* Application Descriptors */
  MADB_Desc *Apd;
  MADB_Desc *Ard;
  MADB_Desc *Ird;
  MADB_Desc *Ipd;
  /* Internal Descriptors */
  MADB_Desc *IApd;
  MADB_Desc *IArd;
  MADB_Desc *IIrd;
  MADB_Desc *IIpd;
};

typedef struct st_ma_odbc_environment {
  MADB_Error Error;
  CRITICAL_SECTION cs;
  LIST *Dbcs;
  SQLUINTEGER Trace;
  SQLWCHAR *TraceFile;
  SQLINTEGER OdbcVersion;
  SQLINTEGER OutputNTS;
} MADB_Env;


enum enum_dsn_item_type {
  DSN_TYPE_STRING,
  DSN_TYPE_INT,
  DSN_TYPE_BOOL,
  DSN_TYPE_COMBO,
  DSN_TYPE_OPTION
};

typedef struct 
{
  char *DsnKey;
  size_t DsnOffset;
  enum enum_dsn_item_type Type;
  unsigned long FlagValue;
  bool IsAlias;
} MADB_DsnKey;

/* Definitions to tell setup library via isPrompt field what should it do */
#define MAODBC_CONFIG           0
#define MAODBC_PROMPT           1
#define MAODBC_PROMPT_REQUIRED  2

typedef struct st_madb_dsn
{
  /*** General ***/
  char *DSNName;
  char *Driver;
  char  *Description;
  /*** Connection parameters ***/
  char *ServerName;
  my_bool IsNamedPipe;
  my_bool IsTcpIp;
  char *UserName;
  char *Password;
  char *Catalog;
  unsigned int Port;
  /* Options */
  unsigned long Options;
  char *CharacterSet;
  char *InitCommand;
  char *TraceFile;
  unsigned int ConnectionTimeout;
  my_bool Reconnect;
  my_bool MultiStatements;
  /* TRUE means "no prompt" */
  my_bool ConnectPrompt;
  /* --- Internal --- */
  int isPrompt;
  MADB_DsnKey *Keys;
  char ErrorMsg[SQL_MAX_MESSAGE_LENGTH];
  my_bool FreeMe;
  /* Callbacke required for prompt to keep all memory de/allocation operations
     on same side of libraries */
  char * (*allocator)(size_t);
  void (*free)(void*);
} MADB_Dsn;

typedef struct st_client_charset
{
  unsigned int CodePage;
  CHARSET_INFO *cs_info;
} Client_Charset;

struct st_ma_odbc_connection
{
  MYSQL *mariadb;                /* handle to a mariadb connection */
  CRITICAL_SECTION cs;           /* mutex */
  MADB_Env *Environment;         /* global environment */
  MADB_Dsn *Dsn;
  struct st_ma_connection_methods *Methods;
  MADB_Error Error;
  Client_Charset charset;
  char *DataBase;
  LIST ListItem;
  LIST *Stmts;
  LIST *Descrs;
  /* Attributes */
  SQLINTEGER AccessMode;
  my_bool IsAnsi;
  SQLINTEGER IsolationLevel;     /* tx_isolation */
  SQLULEN AsyncEnable;
  SQLUINTEGER AutoIpd;
  SQLUINTEGER AutoCommit;
  SQLUINTEGER ConnectionDead;
  SQLUINTEGER ConnectionTimeout;
  unsigned long Options;
  char *CatalogName;
  SQLPOINTER EnlistInDtc;
  SQLUINTEGER LoginTimeout;
  SQLUINTEGER MetadataId;
  SQLULEN OdbcCursors;
  SQLUINTEGER PacketSize;
  HWND QuietMode;
  SQLUINTEGER Trace;
  char *TraceFile;
  char *TranslateLib;
  SQLINTEGER TxnIsolation;
  SQLINTEGER CursorCount;
};

typedef BOOL (*PromptDSN)(HWND hwnd, MADB_Dsn *Dsn);

typedef struct
{
  void     *LibraryHandle;
  PromptDSN Call;
} MADB_Prompt;

SQLRETURN DSNPrompt_Lookup(MADB_Prompt *prompt, const char *SetupLibName, MADB_Dbc *Dbc);
int       DSNPrompt_Free  (MADB_Prompt *prompt);

Client_Charset* GetDefaultOsCharset(Client_Charset *cc);
int InitClientCharset(Client_Charset *cc, const char * name);
void CloseClientCharset(Client_Charset *cc);

#include <ma_error.h>
#include <ma_parse.h>
#include <ma_dsn.h>
#include <ma_info.h>
#include <ma_environment.h>
#include <ma_connection.h>
#include <ma_debug.h>
#include <ma_desc.h>
#include <ma_statement.h>
#include <ma_string.h>
#include <ma_result.h>
#include <ma_driver.h>
#include <ma_helper.h>

#endif /* _ma_odbc_h_ */
