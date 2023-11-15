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


#include <memory>
#include <list>
#include "ma_c_stuff.h"
#include "ma_legacy_helpers.h"
#include "lru/pscache.h"
#include "class/SQLString.h"
#include "template/CArray.h"
#include "class/pimpls.h"
#include "class/Results.h"

namespace mariadb
{
  namespace Unique
  {
    typedef std::unique_ptr<mariadb::PsCache<mariadb::ServerPrepareResult>> PsCache;
    typedef std::unique_ptr<mariadb::PreparedStatement> PreparedStatement;
    //typedef std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> MYSQL_RES;
    typedef std::unique_ptr<mariadb::Protocol> Protocol;
  }
}

namespace odbc
{
  // So far just to have nice local type name
  typedef mariadb::PsCache<mariadb::ServerPrepareResult> PsCache;
}
using namespace mariadb;

typedef struct st_client_charset
{
  unsigned int CodePage;
  MARIADB_CHARSET_INFO* cs_info;
} Client_Charset;

struct MADB_ERROR
{
  char SqlState[SQL_SQLSTATE_SIZE + 1];
  char SqlStateV2[SQLSTATE_LENGTH + 1];
  char SqlErrorMsg[SQL_MAX_MESSAGE_LENGTH + 1];
  SQLRETURN ReturnValue;
};


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
};

struct MADB_Ard
{
  SQLUSMALLINT *RowOperationPtr;
  SQLULEN      *RowOffsetPtr;
  /* MYSQL_BIND   *Bind;*/
  SQLLEN       dummy;
  SQLUINTEGER  BindSize;	/* size of each structure if using * Row-wise Binding */
  SQLSMALLINT  Allocated;
};

struct MADB_Apd
{
  SQLUSMALLINT *ParamOperationPtr;
  SQLULEN      *ParamOffsetPtr;
  /* MYSQL_BIND   *Bind;*/
  SQLLEN       ParamsetSize;
  SQLUINTEGER  ParamBindType;
  SQLSMALLINT  Allocated;
};

struct MADB_Stmt;
struct MADB_Dbc;

struct MADB_Ird
{
  /*MADB_Stmt* stmt;*/
  SQLULEN* RowsFetched;
  SQLUSMALLINT* RowStatusArray;
  /* MYSQL_FIELD* Fields;*/
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
	SQLSMALLINT Allocated;
};

typedef struct {
  char        *BaseCatalogName;
  char        *BaseColumnName;
  char        *BaseTableName;
  char        *CatalogName;
  const char  *ColumnName;
  SQLPOINTER  DataPtr;
  SQLLEN* OctetLengthPtr;
  SQLLEN* IndicatorPtr;
  char* Label;
  char* SchemaName;
  char* TableName;
  const char* LiteralPrefix;
  const char* LiteralSuffix;
  const char* LocalTypeName;
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

struct MADB_Env {
  typedef /*typename*/std::list<MADB_Dbc*>::iterator ListIterator;
  MADB_Error Error;
  std::list<MADB_Dbc*> Dbcs;
  SQLWCHAR* TraceFile;
  SQLUINTEGER Trace;
  SQLINTEGER OdbcVersion;
  enum MADB_AppType AppType;

  ListIterator addConnection(MADB_Dbc* conn);
  void forgetConnection(MADB_Env::ListIterator& it);

private:
  std::mutex cs;
};

#include "ma_connection.h"

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
  SQLRETURN Prepare(const char* StatementText, SQLINTEGER TextLength, bool ServerSide= true);
  SQLRETURN GetOutParams(int CurrentOffset);
  SQLRETURN DoExecuteBatch();
  void AfterExecute();
  void AfterPrepare();// Should go to private at some point
  
  //const SQLString& OriginalQuery() { return Query.Original; }

private:
  MADB_Stmt()= delete;
  void ProcessRsMetadata();
  
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

/* Enabling tracing */
#define MAODBC_DEBUG 1
/* Macro checks return of the suplied SQLRETURN function call, checks if it is succeeded, and in case of error pushes error up */
#define RETURN_ERROR_OR_CONTINUE(sqlreturn_func_call) {\
  SQLRETURN rc= (sqlreturn_func_call);\
  if (!SQL_SUCCEEDED(rc)) return rc;\
} while(0)

#define iOdbc() (sizeof(SQLWCHAR)==4)

#include "ma_error.h"
#include "ma_info.h"
#include "ma_environment.h"
#include "ma_connection.h"
#include "ma_debug.h"
#include "ma_desc.h"
#include "ma_statement.h"
#include "ma_string.h"
#include "ma_result.h"
#include "ma_driver.h"
#include "ma_helper.h"
#include "ma_server.h"
#include "ma_typeconv.h"
#include "ma_bulk.h"

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
