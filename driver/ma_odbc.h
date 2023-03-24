/************************************************************************************
   Copyright (C) 2013,2023 MariaDB Corporation AB
   
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

#include "class/SQLString.h"
#include "template/CArray.h"

#include <memory>
#include "ma_c_stuff.h"
//#include "PreparedStatement.h"

namespace odbc
{
namespace mariadb
{
class PreparedStatement;
class ResultSet;
class ResultSetMetaData;

namespace Unique
{
  typedef std::unique_ptr<odbc::mariadb::PreparedStatement> PreparedStatement;
  typedef std::unique_ptr<odbc::mariadb::ResultSet> ResultSet;
  typedef std::unique_ptr<::MYSQL_RES, decltype(&mysql_free_result)> MYSQL_RES;
  typedef std::unique_ptr<odbc::mariadb::ResultSetMetaData> ResultSetMetaData;
}
}
}

using namespace odbc::mariadb;

struct MADB_Header
{
  /* Header */
  SQLUSMALLINT *ArrayStatusPtr;
  SQLULEN      *BindOffsetPtr;
  /* TODO: In IPD this is SQLUINTEGER* field */
  SQLULEN      *RowsProcessedPtr;
  SQLULEN       ArraySize;
  SQLULEN       BindType;
  SQLSMALLINT   AllocType;
  SQLSMALLINT   Count;
  /* Header end */
} ;

struct MADB_Ard
{
	SQLUSMALLINT *RowOperationPtr;
	SQLULEN		   *RowOffsetPtr;
  /*MADB_ColBind *ColumnBind;*/
	MYSQL_BIND   *Bind;
  SQLLEN      dummy;
  SQLUINTEGER	BindSize;	/* size of each structure if using * Row-wise Binding */
	SQLSMALLINT	Allocated;
} ;

struct MADB_Apd
{
	SQLUSMALLINT  *ParamOperationPtr;
	SQLULEN       *ParamOffsetPtr;
	/*MADB_ParmBind *ParamBind;*/
	MYSQL_BIND    *Bind;
  SQLLEN      ParamsetSize;
  SQLUINTEGER	ParamBindType;
	SQLSMALLINT	Allocated;
};

struct MADB_Stmt;
struct MADB_Dbc;

struct MADB_Ird
{
  MADB_Stmt* stmt;
	SQLULEN* RowsFetched;
	SQLUSMALLINT* RowStatusArray;
  MYSQL_FIELD* Fields;
	SQLUINTEGER FieldCount;
	SQLSMALLINT	Allocated;
};

struct MADB_Ipd
{
  //MADB_Header Header;
#if (ODBCVER >= 0x0300)
	SQLUINTEGER *ParamProcessedPtr;
#else
	SQLULEN *ParamProcessedPtr; /* SQLParamOptions */
#endif /* ODBCVER */
	SQLUSMALLINT *ParamStatusPtr;
  /*MADB_ParmBind* Parameters;*/
	SQLSMALLINT Allocated;
};

typedef struct {
  char        *BaseCatalogName;
  char        *BaseColumnName;
  char        *BaseTableName;
  char        *CatalogName;
  char        *ColumnName;
  SQLPOINTER  DataPtr;
  SQLLEN* OctetLengthPtr;
  SQLLEN* IndicatorPtr;
  char* Label;
  char* SchemaName;
  char* TableName;
  const char* LiteralPrefix;
  const char* LiteralSuffix;
  char* LocalTypeName;
  char* TypeName;
  char* InternalBuffer;  /* used for internal conversion */
  char* DefaultValue;
  char* DaeData;
  SQLLEN      DisplaySize;
  SQLULEN     Length;
  SQLLEN      OctetLength;
  SQLULEN     DaeDataLength;    /* Doesn't seem to be used anywhere */
  unsigned long InternalLength; /* This to be used in the MYSQL_BIND. Thus is the type */
  SQLINTEGER  AutoUniqueValue;
  SQLINTEGER  DateTimeIntervalPrecision;
  SQLINTEGER  DescLength;
  SQLINTEGER  CaseSensitive;
  SQLINTEGER  NumPrecRadix;
  SQLSMALLINT ConciseType;
  SQLSMALLINT DateTimeIntervalCode;
  SQLSMALLINT FixedPrecScale;
  SQLSMALLINT Nullable;
  SQLSMALLINT ParameterType;
  SQLSMALLINT Precision;
  SQLSMALLINT RowVer;
  SQLSMALLINT Scale;
  SQLSMALLINT Searchable;
  SQLSMALLINT Type;
  SQLSMALLINT Unnamed;
  SQLSMALLINT Unsigned;
  SQLSMALLINT Updateable;
  my_bool     PutData;
  my_bool     inUse;
  my_bool     TruncError;
} MADB_DescRecord;

typedef struct
{
  MADB_Header Header;
  MADB_DynArray Records;
  MADB_DynArray Stmts;
  MADB_Error Error;
  MADB_List ListItem;        /* To store in the dbc */
  union {
    MADB_Ard Ard;
    MADB_Apd Apd;
    MADB_Ipd Ipd;
    MADB_Ird Ird;
  } Fields;

  MADB_Dbc* Dbc;       /* Disconnect must automatically free allocated descriptors. Thus
                         descriptor has to know the connection it is allocated on */
  SQLINTEGER DescType;  /* SQL_ATTR_APP_ROW_DESC or SQL_ATTR_APP_PARAM_DESC */
  my_bool AppType;      /* Allocated by Application ? */
} MADB_Desc;

struct st_ma_desc_fldid
{
  SQLSMALLINT FieldIdentifier;
  SQLSMALLINT Access[4];
};

struct st_ma_stmt_methods;

typedef struct 
{
 	SQLLEN MaxRows;
	SQLLEN MaxLength;
	SQLLEN KeysetSize;
  void* BookmarkPtr;
  SQLLEN BookmarkLength;
  SQLULEN	MetadataId;
  SQLULEN SimulateCursor;
  SQLULEN Timeout;
  SQLUINTEGER CursorType;
	SQLUINTEGER	ScrollConcurrency;
  SQLUINTEGER RetrieveData;
	SQLUINTEGER UseBookmarks;
  SQLSMALLINT BookmarkType;
} MADB_StmtOptions;

/* TODO: To check is it 0 or 1 based? not quite clear from its usage */
typedef struct
{
  char   *Name;
  SQLLEN  Position;
  SQLLEN  RowsetSize;
  int64_t Next;
} MADB_Cursor;

enum MADB_DaeType {MADB_DAE_NORMAL=0, MADB_DAE_ADD=1, MADB_DAE_UPDATE=2, MADB_DAE_DELETE=3};

#define RESET_DAE_STATUS(Stmt_Hndl) (Stmt_Hndl)->Status=0; (Stmt_Hndl)->PutParam= -1
#define MARK_DAE_DONE(Stmt_Hndl)    (Stmt_Hndl)->Status=0; (Stmt_Hndl)->PutParam= (Stmt_Hndl)->ParamCount

#define PARAM_IS_DAE(Len_Ptr) ((Len_Ptr) && (*(Len_Ptr) == SQL_DATA_AT_EXEC || *(Len_Ptr) <= SQL_LEN_DATA_AT_EXEC_OFFSET))
#define DAE_DONE(Stmt_Hndl) ((Stmt_Hndl)->PutParam >= (Stmt_Hndl)->ParamCount)

enum MADB_StmtState {MADB_SS_INITED= 0, MADB_SS_PREPARED= 2, MADB_SS_EXECUTED= 3, MADB_SS_OUTPARAMSFETCHED= 4};

#define STMT_WAS_PREPARED(Stmt_Hndl) ((Stmt_Hndl)->State > MADB_SS_INITED)
#define STMT_REALLY_PREPARED(Stmt_Hndl) ((Stmt_Hndl)->State >= MADB_SS_PREPARED)
#define STMT_EXECUTED(Stmt_Hndl) ((Stmt_Hndl)->State == MADB_SS_EXECUTED)
#define RESET_STMT_STATE(Stmt_Hndl) if ((Stmt_Hndl)->State > MADB_SS_PREPARED) (Stmt_Hndl)->State= MADB_SS_PREPARED

/* Struct used to define column type when driver has to fix it (in catalog functions + SQLGetTypeInfo) */
typedef struct
{
  SQLSMALLINT SqlType;
  my_bool     Unsigned;
  SQLSMALLINT Nullable;
  SQLLEN      OctetLength;

} MADB_ShortTypeInfo;

typedef struct
{
  uint32_t  ArraySize;
  bool      HasRowsToSkip;
} MADB_BulkOperationInfo;

/* Stmt struct needs definitions from my_parse.h */
#include "ma_parse.h"
#include "ma_dsn.h"

#define STMT_STRING(STMT) (STMT)->Query.Original

enum MADB_AppType {
  ATypeGeneral= 0,
  ATypeMSAccess= 1
};

typedef struct st_ma_odbc_environment {
  MADB_Error Error;
  CRITICAL_SECTION cs;
  MADB_List* Dbcs;
  SQLWCHAR* TraceFile;
  SQLUINTEGER Trace;
  SQLINTEGER OdbcVersion;
  enum MADB_AppType AppType;
} MADB_Env;

struct MADB_Dbc
{
  MADB_Error Error;
  CRITICAL_SECTION cs;           /* mutex for mariadb handle, i.e. for server communications */
  CRITICAL_SECTION ListsCs;      /*       for operations with lists */
  MADB_List ListItem;
  Client_Charset Charset= {0,nullptr};
  MYSQL* mariadb= nullptr;                /* handle to a mariadb connection */
  MADB_Env* Environment= nullptr;         /* global environment */
  MADB_Dsn* Dsn= nullptr;
  struct st_ma_connection_methods* Methods= nullptr;

  Client_Charset* ConnOrSrcCharset= nullptr; /* "Source" here stands for which charset Windows DM was using as source, when converted to unicode.
                                  We have to use same charset to recode from unicode to get same string as application sent it.
                                  For Unicode application that is the same as "Charset", or in case of ANSI on Windows - defaulst system
                                  codepage */
  char* CurrentSchema= nullptr; /* Used to store current schema if the seesion tracking is possible */
  MADB_List* Stmts= nullptr;
  MADB_List* Descrs= nullptr;
  /* Attributes */
  char*      CatalogName= nullptr;
  HWND       QuietMode= nullptr;
  char*      TraceFile= nullptr;
  MADB_Stmt* Streamer= nullptr;

  SQLULEN    AsyncEnable= 0;
  SQLPOINTER EnlistInDtc= nullptr;
  SQLULEN    OdbcCursors= 0;
  unsigned long Options= 0;
  SQLUINTEGER AutoIpd= 0;
  SQLUINTEGER AutoCommit= 4;
  SQLUINTEGER ConnectionDead= 0;
  SQLUINTEGER ReadTimeout= 0;
  SQLUINTEGER WriteTimeout= 0;
  SQLUINTEGER PacketSize= 0;
  SQLINTEGER  AccessMode= 0;
  SQLINTEGER  IsolationLevel= 0;     /* tx_isolation */
  SQLUINTEGER Trace= 0;
  SQLUINTEGER MetadataId= 0;
  SQLINTEGER  TxnIsolation= 0;
  SQLINTEGER  CursorCount= 0;
  unsigned int LoginTimeout= 0; /* The attribute is SQLUINTEGER, that is unsigned long, that technically can be 8bytes
                                (not sure how does other DM define it) But C/C option is unsigned int */
  char    ServerCapabilities= '\0';
  bool IsAnsi= false;
  bool IsMySQL=false;

  MADB_Dbc(MADB_Env* Env);

};

struct MADB_Stmt
{
  MADB_StmtOptions          Options;
  MADB_Error                Error;
  MADB_Cursor               Cursor;
  MADB_QUERY                Query;
  MADB_List                 ListItem;
  long long                 AffectedRows= 0;
  MADB_Dbc                  *Connection;
  struct st_ma_stmt_methods *Methods;
  Unique::ResultSet         rs;
  Unique::PreparedStatement stmt;
  Unique::ResultSetMetaData metadata;
  Unique::MYSQL_RES         DefaultsResult;
  MADB_DescRecord           *PutDataRec= nullptr;
  MADB_Stmt                 *DaeStmt= nullptr;
  MADB_Stmt                 *PositionedCursor= nullptr;
  SQLLEN                    LastRowFetched= 0;
  MYSQL_BIND                *result= nullptr;
  MYSQL_BIND                *params= nullptr;
  unsigned long             *CharOffset= nullptr;
  unsigned long             *Lengths= nullptr;
  char                      *TableName= nullptr;
  char                      *CatalogName= nullptr;
  MADB_ShortTypeInfo        *ColsTypeFixArr= nullptr;
  /* Application Descriptors */
  MADB_Desc *Apd= nullptr;
  MADB_Desc *Ard= nullptr;
  MADB_Desc *Ird= nullptr;
  MADB_Desc *Ipd= nullptr;
  /* Internal Descriptors */
  MADB_Desc *IApd;
  MADB_Desc *IArd;
  MADB_Desc *IIrd;
  MADB_Desc *IIpd;
  unsigned short            *UniqueIndex= nullptr; /* Insdexes of columns that make best available unique identifier */
  SQLSETPOSIROW             DaeRowNumber= 0;
  int32_t                   ArrayOffset= 0;
  int32_t                   Status= 0;
  uint32_t                  MultiStmtNr= 0;
  uint32_t                  MultiStmtMaxParam= 0;
  int32_t                   PutParam= -1;
  enum MADB_StmtState       State= MADB_SS_INITED;
  enum MADB_DaeType         DataExecutionType= MADB_DAE_NORMAL;
  SQLSMALLINT               ParamCount= 0;
  MADB_BulkOperationInfo    Bulk;
  bool                      PositionedCommand= false;
  bool                      RebindParams= false;
  bool                      bind_done= false;

  MADB_Stmt(MADB_Dbc *Connection);
  SQLRETURN Prepare(char* StatementText, SQLINTEGER TextLength, bool ServerSide= true);
  SQLRETURN GetOutParams(int CurrentOffset);
  SQLRETURN DoExecuteBatch();
  //const SQLString& OriginalQuery() { return Query.Original; }

private:
  MADB_Stmt()= delete;
};

typedef BOOL (__stdcall *PromptDSN)(HWND hwnd, MADB_Dsn *Dsn);

typedef struct
{
  void     *LibraryHandle;
  PromptDSN Call;
} MADB_Prompt;

#define MADB_PROMPT_NOT_SUPPORTED 1
#define MADB_PROMPT_COULDNT_LOAD  2
int DSNPrompt_Lookup(MADB_Prompt *prompt, const char *SetupLibName);

int DSNPrompt_Free  (MADB_Prompt *prompt);

int   InitClientCharset (Client_Charset *cc, const char *name);
void  CopyClientCharset (Client_Charset *Src, Client_Charset *Dst);
void  CloseClientCharset(Client_Charset *cc);

/* Default precision of SQL_NUMERIC */
#define MADB_DEFAULT_PRECISION 38
#define MADB_MAX_SCALE         MADB_DEFAULT_PRECISION
#define BINARY_CHARSETNR       63
/* Inexistent param id */
#define MADB_NOPARAM           -1
/* Macros to guard communications with the server.
   TODO: make it(locking) optional depending on designated connection string option */
#define LOCK_MARIADB(Dbc)   EnterCriticalSection(&(Dbc)->cs)
#define UNLOCK_MARIADB(Dbc) LeaveCriticalSection(&(Dbc)->cs)

/* Enabling tracing */
#define MAODBC_DEBUG 1
/* Macro checks return of the suplied SQLRETURN function call, checks if it is succeeded, and in case of error pushes error up */
#define RETURN_ERROR_OR_CONTINUE(sqlreturn_func_call) {\
  SQLRETURN rc= (sqlreturn_func_call);\
  if (!SQL_SUCCEEDED(rc)) return rc;\
} while(0)

#define iOdbc() (sizeof(SQLWCHAR)==4)

#include <ma_error.h>
#include <ma_info.h>
#include <ma_environment.h>
#include <ma_connection.h>
#include <ma_debug.h>
#include <ma_desc.h>
#include <ma_statement.h>
#include "ma_string.h"
#include <ma_result.h>
#include <ma_driver.h>
#include <ma_helper.h>
#include <ma_server.h>
#include <ma_typeconv.h>
#include <ma_bulk.h>

#include "ma_api_internal.h"

SQLRETURN MADB_GetBookmark(MADB_Stmt  *StatementHandle,
                           SQLSMALLINT TargetType,
                           SQLPOINTER  TargetValuePtr,
                           SQLLEN      BufferLength,
                           SQLLEN     *StrLen_or_IndPtr);

SQLRETURN MADB_StmtColAttr(MADB_Stmt *Stmt, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttributePtr,
             SQLSMALLINT BufferLength, SQLSMALLINT *StringLengthPtr, SQLLEN *NumericAttributePtr, my_bool IsWchar);

SQLRETURN MADB_StmtColAttr(MADB_Stmt *Stmt, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttributePtr,
             SQLSMALLINT BufferLength, SQLSMALLINT *StringLengthPtr, SQLLEN *NumericAttributePtr, my_bool IsWchar);
#endif /* _ma_odbc_h_ */
