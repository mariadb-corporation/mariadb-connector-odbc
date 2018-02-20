/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013, 2018 MariaDB Corporation AB

  The MySQL Connector/ODBC is licensed under the terms of the GPLv2
  <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
  MySQL Connectors. There are special exceptions to the terms and
  conditions of the GPLv2 as it is applied to this software, see the
  FLOSS License Exception
  <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; version 2 of the License.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef _tap_h_
#define _tap_h_

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
# define _WINSOCKAPI_
# include <windows.h>

#else

# include <string.h>
# include <errno.h>
# include <wchar.h>

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


int strcpy_s(char *dest, size_t buffer_size, const char *src)
{
  size_t src_len;

  if (dest == NULL)
  {
    return EINVAL;
  }

  if (src == NULL)
  {
    *dest= '\0';
    return EINVAL;
  }

  src_len= strlen(src);

  if (buffer_size < src_len + 1)
  {
    *dest= 0;
    return ERANGE;
  }

  memcpy((void*)dest, (void*)src, src_len + 1);

  return 0;
}


#define Sleep(ms) sleep(ms/1000)

# ifndef TRUE
#  define TRUE 1
# endif
# ifndef FALSE
#  define FALSE 0
# endif

#define _strdup        strdup
#define _stricmp       strcasecmp
#define _strnicmp      strncasecmp
#define _atoi64(str)   strtoll((str), NULL, 10)
#define _i64toa(a,b,c) longlong2str((a),(b),(c))

#endif

#include <sql.h>
#include <sqlext.h>
#include <getopt.h>
#include <time.h>
#include <assert.h>
/* We need mysql for CHARSET_INFO type and conversion routine */
#include <mysql.h>
typedef unsigned int uint;

static SQLCHAR *my_dsn=        (SQLCHAR *)"test";
static SQLCHAR *my_uid=        (SQLCHAR *)"root";
static SQLCHAR *my_pwd=        (SQLCHAR *)"";
static SQLCHAR *my_schema=     (SQLCHAR *)"odbc_test";
static SQLCHAR *my_drivername= (SQLCHAR *)"MariaDB Connector/ODBC 2.0";
static SQLCHAR *my_servername= (SQLCHAR *)"localhost";

static SQLWCHAR *wdsn;
static SQLWCHAR *wuid;
static SQLWCHAR *wpwd;
static SQLWCHAR *wschema;
static SQLWCHAR *wdrivername;
static SQLWCHAR *wservername;
static SQLWCHAR *wstrport;

static unsigned long my_options= 67108866;

static SQLHANDLE     Env, Connection, Stmt;
static SQLINTEGER    OdbcVer=        SQL_OV_ODBC3;

static unsigned int  my_port=        3306;
char          ma_strport[12]= ";PORT=3306";

static int Travis= 0;

/* To use in tests for conversion of strings to (sql)wchar strings */
SQLWCHAR  sqlwchar_buff[8192], sqlwchar_empty[]= {0};
SQLWCHAR *buff_pos= sqlwchar_buff;

static CHARSET_INFO  *utf8= NULL, *utf16= NULL, *utf32= NULL;

int   tests_planned= 0;
char *test_status[]= {"not ok", "ok", "skip"};

#define FAIL          0
#define OK            1
#define SKIP          2

/* Test Typesv*/
/* Normal test - failure is failure, i.e. bad thing */
#define NORMAL        OK
/* Known failure - yes, we need to fix it, but we do not want it to distract us from
   things that are really interesting. And no, we don't know what causes failure */
#define KNOWN_FAILURE FAIL
/* The problem is in test */
#define TEST_PROBLEM  2
/* Test for the known problem waiting for a fix. */
#define TO_FIX        3

const char comments[][2][60]= { {"\t#TODO: not ok - test is known to fail, unknown reason",
                                 "\t#Yay, seems like this problem has been magically fixed"},
                                {"",""},
                                {"\t#TODO: Test is marked as requiring fix",
                                 "\t#TODO: Test is marked as requiring fix"},
                                {"\t#TODO: Test is for known problem that is waiting for fix",
                                 "\t#The problem seems to be fixed. Mark test as normal"}
                              };
#define MAX_ROW_DATA_LEN 1000
#define MAX_NAME_LEN     255

/* We don't test corresponding EnvAttr, but assume that connections are made using heler functions, and they use OdbcVer */
#define IS_ODBC3() (OdbcVer == SQL_OV_ODBC3)

#define skip(A) diag((A)); return SKIP;

typedef struct st_ma_odbc_test {
  int   (*my_test)();
  char  *title;
  int   test_type;
} MA_ODBC_TESTS;

#define ODBC_TEST(a)\
int a()

#define plan(a)\
  tests_planned= a;\

void mark_all_tests_normal(MA_ODBC_TESTS *tests)
{
  while(tests && tests->title !=  NULL)
  {
    tests->test_type= NORMAL;
    ++tests;
  }
}


void usage()
{
  fprintf(stdout, "Valid options:\n");
  fprintf(stdout, "-d DSN Name\n");
  fprintf(stdout, "-u Username\n");
  fprintf(stdout, "-p Password\n");
  fprintf(stdout, "-s default database (schema)\n");
  fprintf(stdout, "-S Server name/address\n");
  fprintf(stdout, "-P Port number\n");
  fprintf(stdout, "?  Displays this text\n");
}


void get_env_defaults()
{
  char *env_val;

  if ((env_val= getenv("TEST_DSN")) != NULL)
  {
    my_dsn= (SQLCHAR*)env_val;
  }
  if ((env_val= getenv("TEST_DRIVER")) != NULL)
  {
    my_drivername= (SQLCHAR*)env_val;
  }
  if ((env_val= getenv("TEST_SERVER")) != NULL)
  {
    my_servername= (SQLCHAR*)env_val;
  }
  if ((env_val= getenv("TEST_UID")) != NULL)
  {
    my_uid= (SQLCHAR*)env_val;
  }
  if ((env_val= getenv("TEST_PASSWORD")) != NULL)
  {
    my_pwd= (SQLCHAR*)env_val;
  }
  if ((env_val= getenv("TEST_SCHEMA")) != NULL)
  {
    my_schema= (SQLCHAR*)env_val;
  }
  if ((env_val= getenv("TEST_PORT")) != NULL)
  {
    int port= atoi(env_val);
    if (port > 0 && port < 0x10000)
    {
      my_port= port;
    }
  }
}


void get_options(int argc, char **argv)
{
  int c= 0;

  get_env_defaults();

  while ((c=getopt(argc,argv, "d:u:p:P:s:S:?")) >= 0)
  {
    switch(c) {
    case 'd':
      my_dsn= (SQLCHAR*)optarg;
      break;
    case 'u':
      my_uid= (SQLCHAR*)optarg;
      break;
    case 'p':
      my_pwd= (SQLCHAR*)optarg;
      break;
    case 's':
      my_schema= (SQLCHAR*)optarg;
      break;
    case 'P':
      my_port= atoi(optarg);
      break;
    case 'S':
      my_servername= (SQLCHAR*)optarg;
      break;
    case '?':
      usage();
      exit(0);
      break;
    default:
      fprintf(stdout, "Unknown option %c\n", c);
      usage();
      exit(0);
    }
  }

  _snprintf(ma_strport, sizeof(ma_strport), ";PORT=%u", my_port);
}


int myrowcount(SQLHSTMT Stmt)
{
  int Rows=0;
  while (SQLFetch(Stmt) != SQL_NO_DATA)
    Rows++;
  return Rows;
}


#define mystmt(hstmt,r)  \
  do { \
    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO) \
      return FAIL; \
  } while (0)


void diag(const char *fstr, ...)
{
  va_list ap;
  va_start(ap, fstr);
  fprintf(stdout, "# ");
  vfprintf(stdout, fstr, ap);
  va_end(ap);
  fprintf(stdout,"\n");
}


void odbc_print_error(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
  SQLCHAR SQLState[6];
  SQLINTEGER NativeError;
  SQLCHAR SQLMessage[SQL_MAX_MESSAGE_LENGTH];
  SQLSMALLINT TextLengthPtr;

  SQLGetDiagRec(HandleType, Handle, 1, SQLState, &NativeError, SQLMessage, SQL_MAX_MESSAGE_LENGTH, &TextLengthPtr);
  fprintf(stdout, "[%s] (%d) %s\n", SQLState, NativeError, SQLMessage);
}


char little_endian()
{
  int   x= 1;
  char *c= (char*)&x;

  return *c;
}


/* More or less copy of the function from MariaDB C/C */
size_t madbtest_convert_string(CHARSET_INFO *from_cs, const char *from, size_t *from_len,
                               CHARSET_INFO *to_cs, char *to, size_t *to_len, int *errorcode)
{
  size_t rc= -1;
  size_t save_len= *to_len;
  int    dummy;

  if (errorcode == NULL)
    errorcode= &dummy;

  *errorcode= 0;

  if ((rc= mariadb_convert_string(from, from_len, from_cs, to, to_len, to_cs, errorcode)) == -1)
  {
    diag("An error occurred while converting from %s to  %s. Error code %d", from_cs->csname, to_cs->name, *errorcode);
  }

  return rc;
}


#define MAX_COLUMNS 1000

#define mystmt_rows(hstmt, r, row)  \
	do { \
	if (!SQL_SUCCEEDED(r)) {\
    odbc_print_error(SQL_HANDLE_STMT, (hstmt));\
	  return row; }\
	} while (0)


int my_print_non_format_result_ex(SQLHSTMT Stmt, BOOL CloseCursor)
{
    SQLRETURN   rc;
    SQLUINTEGER nRowCount=0;
    SQLULEN     pcColDef;
    SQLCHAR     szColName[MAX_NAME_LEN+1];
    SQLCHAR     szData[MAX_COLUMNS][MAX_ROW_DATA_LEN]={{0}};
    SQLSMALLINT nIndex,ncol= 0,pfSqlType, pcbScale, pfNullable;
    SQLLEN      ind_strlen;

    rc = SQLNumResultCols(Stmt, &ncol);
    
    mystmt_rows(Stmt, rc, -1);

    for (nIndex = 1; nIndex <= ncol; ++nIndex)
    {
        rc = SQLDescribeCol(Stmt,nIndex,szColName, MAX_NAME_LEN, NULL,
                            &pfSqlType,&pcColDef,&pcbScale,&pfNullable);
        /* Returning in case of an error -nIndex we will see in the log column# */
        mystmt_rows(Stmt,rc,-nIndex);


        fprintf(stdout, "%s\t", szColName);

        rc = SQLBindCol(Stmt, nIndex, SQL_C_CHAR, szData[nIndex-1],
                        MAX_ROW_DATA_LEN+1, &ind_strlen);
        mystmt_rows(Stmt, rc, -nIndex);
    }

    fprintf(stdout, "\n");

    rc = SQLFetch(Stmt);
    while (SQL_SUCCEEDED(rc))
    {
        ++nRowCount;
        for (nIndex=0; nIndex< ncol; ++nIndex)
            fprintf(stdout, "%s\t", szData[nIndex]);

        fprintf(stdout, "\n");
        rc = SQLFetch(Stmt);
    }

    SQLFreeStmt(Stmt, SQL_UNBIND);

    if (CloseCursor)
    {
      SQLFreeStmt(Stmt, SQL_CLOSE);
    }

    fprintf(stdout, "# Total rows fetched: %d\n", (int)nRowCount);

    return nRowCount;
}


int my_print_non_format_result(SQLHSTMT Stmt)
{
  return my_print_non_format_result_ex(Stmt, TRUE);
}


#define OK_SIMPLE_STMT(stmt, stmtstr)\
if (SQLExecDirect((stmt), (SQLCHAR*)(stmtstr), (SQLINTEGER)strlen(stmtstr)) != SQL_SUCCESS)\
{\
  fprintf(stdout, "Error in %s:%d:\n", __FILE__, __LINE__);\
  odbc_print_error(SQL_HANDLE_STMT, (stmt));\
  return FAIL;\
}

#define OK_SIMPLE_STMTW(stmt, stmtstr)\
if (SQLExecDirectW((stmt), (SQLWCHAR*)(stmtstr), SQL_NTS) != SQL_SUCCESS)\
{\
  fprintf(stdout, "Error in %s:%d:\n", __FILE__, __LINE__);\
  odbc_print_error(SQL_HANDLE_STMT, (stmt));\
  return FAIL;\
}

#define ERR_SIMPLE_STMT(stmt, stmtstr)\
if (SQLExecDirect((stmt), (SQLCHAR*)(stmtstr), SQL_NTS) != SQL_ERROR)\
{\
  fprintf(stdout, "Error expected in %s:%d:\n", __FILE__, __LINE__);\
  return FAIL;\
}

#define ERR_SIMPLE_STMTW(stmt, stmtstr)\
if (SQLExecDirectW((stmt), (SQLWCHAR*)(stmtstr), SQL_NTS) != SQL_ERROR)\
{\
  fprintf(stdout, "Error expected in %s:%d:\n", __FILE__, __LINE__);\
  return FAIL;\
}

#define CHECK_HANDLE_RC(handletype, handle ,rc)\
do {\
  SQLRETURN local_rc= (rc); \
  if (!(SQL_SUCCEEDED(local_rc)))\
  {\
    fprintf(stdout, "Error (rc=%d) in %s:%d:\n", local_rc, __FILE__, __LINE__);\
    odbc_print_error((handletype), (handle));\
    return FAIL;\
  }\
} while(0)

#define CHECK_STMT_RC(stmt,rc) CHECK_HANDLE_RC(SQL_HANDLE_STMT,stmt,rc)
#define CHECK_DBC_RC(dbc,rc) CHECK_HANDLE_RC(SQL_HANDLE_DBC,dbc,rc)
#define CHECK_ENV_RC(env,rc) CHECK_HANDLE_RC(SQL_HANDLE_ENV,env,rc)
#define CHECK_DESC_RC(desc,rc) CHECK_HANDLE_RC(SQL_HANDLE_DESC,desc,rc)


#define FAIL_IF(expr,message)\
  if (expr)\
    {\
    fprintf(stdout, "%s (File: %s Line: %d)\n", message, __FILE__, __LINE__);\
    return FAIL;\
    }

#define IS(A) if (!(A)) { diag("Error in %s:%d", __FILE__, __LINE__); return FAIL; }
#define IS_STR(A,B,C) do {const char *loc_a=(A), *loc_b=(B);\
diag("%s %s", loc_a, loc_b);\
FAIL_IF(loc_a == NULL || loc_b == NULL || strncmp(loc_a, loc_b, (C)) != 0, "String comparison failed"); } while(0)

#define is_num(A,B) \
do {\
  long long local_a= (long long)(A), local_b= (long long)(B);\
  if (local_a != local_b)\
  {\
    diag("%s %d: %s(%lld)!=%s(%lld)", __FILE__, __LINE__, #A, local_a, #B, local_b);\
    return FAIL;\
  }\
} while(0)

#define EXPECT_DBC(Dbc,Function, Expected)\
do {\
  SQLRETURN ret= (Function);\
  if (ret != (Expected))\
  {\
    CHECK_DBC_RC(Dbc, ret);\
  }\
} while(0)

#define EXPECT_STMT(Stmt,Function, Expected)\
do {\
  SQLRETURN ret= (Function);\
  if (ret != (Expected))\
  {\
    CHECK_STMT_RC(Stmt, ret);\
    diag("%s %d: %s returned %d, expected %s(%d)",__FILE__, __LINE__, #Function, ret, #Expected, Expected);\
    return FAIL;\
  }\
} while(0)


int my_fetch_int(SQLHANDLE Stmt, unsigned int ColumnNumber)
{
  int value= 0;
  SQLGetData(Stmt, ColumnNumber, SQL_INTEGER, &value, 0, NULL);
  return value;
}

SQLWCHAR *my_fetch_wstr(SQLHSTMT Stmt, SQLWCHAR *buffer, SQLUSMALLINT icol, SQLLEN Length)
{
  SQLRETURN rc;
  SQLLEN nLen;

  rc= SQLGetData(Stmt, icol, SQL_WCHAR, buffer, Length, &nLen);
  if (!SQL_SUCCEEDED(rc))
    return sqlwchar_empty;
  return buffer;
}

const char *my_fetch_str(SQLHSTMT Stmt, SQLCHAR *szData,SQLUSMALLINT icol)
{
    SQLLEN nLen;

    SQLGetData(Stmt,icol,SQL_CHAR,szData,1000,&nLen);
    /* If Null value - putting down smth meaningful. also that allows caller to
       better/(in more easy way) test the value */
    if (nLen < 0)
    {
      strcpy(szData, "(Null)"); 
    }
    diag(" my_fetch_str: %s(%ld)", szData, nLen);
    return((const char *)szData);
}


int check_sqlstate_ex(SQLHANDLE hnd, SQLSMALLINT hndtype, char *sqlstate)
{
  SQLCHAR     sql_state[6];
  SQLINTEGER  err_code= 0;
  SQLCHAR     err_msg[SQL_MAX_MESSAGE_LENGTH]= {0};
  SQLSMALLINT err_len= 0;

  memset(err_msg, 'C', SQL_MAX_MESSAGE_LENGTH);
  SQLGetDiagRec(hndtype, hnd, 1, sql_state, &err_code, err_msg,
                SQL_MAX_MESSAGE_LENGTH - 1, &err_len);

  IS_STR(sql_state, (SQLCHAR *)sqlstate, 5);

  return OK;
}
#define check_sqlstate(stmt, sqlstate) \
  check_sqlstate_ex((stmt), SQL_HANDLE_STMT, (sqlstate))

#define CHECK_SQLSTATE_EX(A,B,C) FAIL_IF(check_sqlstate_ex(A,B,C) != OK, "Unexpected sqlstate!")
#define CHECK_SQLSTATE(A,C) CHECK_SQLSTATE_EX(A, SQL_HANDLE_STMT, C)


int using_dm(HDBC hdbc)
{
  SQLCHAR val[20];
  SQLSMALLINT len;

  if (SQLGetInfo(hdbc, SQL_DM_VER, val, sizeof(val), &len) == SQL_ERROR)
    return 0;

  return 1;
}


int mydrvconnect(SQLHENV *henv, SQLHDBC *hdbc, SQLHSTMT *hstmt, SQLCHAR *connIn)
{
  SQLCHAR   connOut[MAX_NAME_LEN+1];
  SQLSMALLINT len;

  CHECK_ENV_RC(*henv, SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, henv));

  CHECK_ENV_RC(*henv, SQLSetEnvAttr(*henv, SQL_ATTR_ODBC_VERSION,
                              (SQLPOINTER)(SQLLEN)OdbcVer, 0));

  CHECK_ENV_RC(*henv, SQLAllocHandle(SQL_HANDLE_DBC, *henv,  hdbc));

  CHECK_DBC_RC(*hdbc, SQLDriverConnect(*hdbc, NULL, connIn, SQL_NTS, connOut,
                                 MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(*hdbc, SQLSetConnectAttr(*hdbc, SQL_ATTR_AUTOCOMMIT,
                                  (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0));

  CHECK_DBC_RC(*hdbc, SQLAllocHandle(SQL_HANDLE_STMT, *hdbc, hstmt));

  return OK;
}


int AllocEnvConn(SQLHANDLE *Env, SQLHANDLE *Connection)
{
  if (*Env == NULL)
  {
    FAIL_IF(!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, NULL, Env)), "Couldn't allocate environment handle");

    FAIL_IF(!SQL_SUCCEEDED(SQLSetEnvAttr(*Env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)(SQLLEN)OdbcVer, 0)), "Couldn't set ODBC version");
  }
  FAIL_IF(!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, *Env, Connection)), "Couldn't allocate connection handle");

  return OK;
}


/* Returns STMT handle for newly created connection, or NULL if connection is unsuccessful */
SQLHANDLE DoConnect(SQLHANDLE Connection,
                    const char *dsn, const char *uid, const char *pwd, unsigned int port, const char *schema, unsigned long *options, const char *server,
                    const char *add_parameters)
{
  SQLHANDLE   stmt= NULL;
  char        DSNString[1024];
  char        DSNOut[1024];
  SQLSMALLINT Length;

  /* my_options |= 4; */ /* To enable debug */
  _snprintf(DSNString, 1024, "DSN=%s;UID=%s;PWD={%s};PORT=%u;DATABASE=%s;OPTION=%lu;SERVER=%s;%s", dsn ? dsn : (const char*)my_dsn,
           uid ? uid : (const char*)my_uid, pwd ? pwd : (const char*)my_pwd, port ? port : my_port,
           schema ? schema : (const char*)my_schema, options ? *options : my_options, server ? server : (const char*)my_servername,
           add_parameters ? add_parameters : "");
  diag("DSN=%s;UID=%s;PWD={%s};PORT=%u;DATABASE=%s;OPTION=%lu;SERVER=%s;%s", dsn ? dsn : (const char*)my_dsn,
           uid ? uid : (const char*)my_uid, "********", port ? port : my_port,
           schema ? schema : (const char*)my_schema, options ? *options : my_options, server ? server : (const char*)my_servername,
           add_parameters ? add_parameters : "");
  
  if(!SQL_SUCCEEDED(SQLDriverConnect(Connection, NULL, (SQLCHAR *)DSNString, SQL_NTS, (SQLCHAR *)DSNOut, 1024, &Length, SQL_DRIVER_NOPROMPT)))
  {
    odbc_print_error(SQL_HANDLE_DBC, Connection);
    return NULL;
  }

  if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, Connection, &stmt)))
  {
    diag("Could not create Stmt handle. Connection: %x", Connection);
    return NULL;
  }

  return stmt;
}

int ODBC_Connect(SQLHANDLE *Env, SQLHANDLE *Connection, SQLHANDLE *Stmt)
{
  SQLRETURN   rc;
  char        buffer[100];
  SQLHANDLE   Stmt1;

  *Env=         NULL;
  *Connection=  NULL;

  IS(AllocEnvConn(Env, Connection));

  *Stmt= DoConnect(*Connection, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL);

  if (Stmt == NULL)
  {
    return FAIL;
  }

  rc= SQLAllocHandle(SQL_HANDLE_STMT, *Connection, &Stmt1);
  FAIL_IF(rc != SQL_SUCCESS, "Couldn't allocate statement handle");

  strcpy(buffer, "CREATE SCHEMA IF NOT EXISTS ");
  strcat(buffer, (my_schema != NULL) ? (char*)my_schema : "test");
  rc= SQLExecDirect(Stmt1, (SQLCHAR *)buffer, (SQLINTEGER)strlen(buffer));

  strcpy(buffer, "USE ");
  strcat(buffer, (my_schema != NULL) ? (char*)my_schema : "test");
  OK_SIMPLE_STMT(Stmt1, buffer);
  SQLFreeStmt(Stmt1, SQL_CLOSE);

  return OK;
}


void ODBC_Disconnect(SQLHANDLE Env, SQLHANDLE Connection, SQLHANDLE Stmt)
{
  if (Stmt != NULL)
  {
    SQLFreeHandle(SQL_HANDLE_STMT, Stmt);
  }
  if (Connection != NULL)
  {
    SQLDisconnect(Connection);
    SQLFreeHandle(SQL_HANDLE_DBC, Connection);
  }
  if ( Env!= NULL)
  {
    SQLFreeHandle(SQL_HANDLE_ENV, Env);
  }
}


SQLHANDLE ConnectWithCharset(SQLHANDLE *conn, const char *charset_name, const char *add_parameters)
{
  char charset_clause[64];

  _snprintf(charset_clause, sizeof(charset_clause), "CHARSET=%s;%s", charset_name, add_parameters ? add_parameters : "");

  return DoConnect(*conn, NULL, NULL, NULL, 0, NULL, NULL, NULL, charset_clause);
}


struct st_ma_server_variable
{
  int     global;
  const char *  name;
  int     value;
};

/* We do not expect many variables to be changed in single test */
#define MAX_CHANGED_VARIABLES_PER_TEST 5
static struct st_ma_server_variable changed_server_variable[MAX_CHANGED_VARIABLES_PER_TEST];

const char *locality[]=     {"LOCAL ", "GLOBAL "};
const char *set_template=   "SET %s%s=%d",
           *show_template=  "SHOW %s%s LIKE '%s'";

/* Macros for 1st parameter of set_variable */
#define LOCAL  0
#define GLOBAL 1

int reset_changed_server_variables(void)
{
  int i, error= 0;
  char query[512];

  /* Traversing array in reverse direction - in this way if same variable was changed more than once,
     it will get its original value */
  for (i= MAX_CHANGED_VARIABLES_PER_TEST - 1; i > -1; --i)
  {
    if (changed_server_variable[i].name != NULL)
    {
      int size= _snprintf(query, sizeof(query), set_template, locality[changed_server_variable[i].global],
                          changed_server_variable[i].name, changed_server_variable[i].value);
      if (error == 0 && !SQL_SUCCEEDED(SQLExecDirect(Stmt, query, size)))
      {
        error= 1;
        diag("Following variables were not reset to their original values:");
      }

      if (error != 0)
      {
        diag(" %s%s to %d", locality[changed_server_variable[i].global],
                          changed_server_variable[i].name, changed_server_variable[i].value);
      }
    }
  }

  return error;
}

SQLWCHAR * str2sqlwchar_on_gbuff(const char *str, size_t len, CHARSET_INFO *from_cs, CHARSET_INFO *to_cs);

int run_tests(MA_ODBC_TESTS *tests)
{
  int         rc, i=1, failed=0;
  const char *comment;
  SQLWCHAR   *buff_before_test;
  /*Dirty hack*/
  CHARSET_INFO fakeUtf32le;

  fakeUtf32le.encoding= "UTF32LE";
  fakeUtf32le.csname=   "utf32le";

  utf8=  mysql_get_charset_by_name("utf8");
  utf16= mysql_get_charset_by_name(little_endian() ? "utf16le" : "utf16");
  utf32= little_endian() ? &fakeUtf32le : mysql_get_charset_by_name("utf32");

  if (utf8 == NULL || utf16 == NULL || utf32 == NULL)
  {
    fprintf(stdout, "HALT! Could not load charset info %p %p %p\n", utf8, utf16, utf32);
    return 1;
  }

  wdsn=        str2sqlwchar_on_gbuff(my_dsn,        strlen(my_dsn) + 1,        utf8, utf16);
  wuid=        str2sqlwchar_on_gbuff(my_uid,        strlen(my_uid) + 1,        utf8, utf16);
  wpwd=        str2sqlwchar_on_gbuff(my_pwd,        strlen(my_pwd) + 1,        utf8, utf16);
  wschema=     str2sqlwchar_on_gbuff(my_schema,     strlen(my_schema) + 1,     utf8, utf16);
  wservername= str2sqlwchar_on_gbuff(my_servername, strlen(my_servername) + 1, utf8, utf16);
  wdrivername= str2sqlwchar_on_gbuff(my_drivername, strlen(my_drivername) + 1, utf8, utf16);
  wstrport=    str2sqlwchar_on_gbuff(ma_strport,    strlen(ma_strport) + 1,    utf8, utf16);

  if (getenv("TRAVIS") != NULL)
  {
    Travis= 1;
  }
  if (ODBC_Connect(&Env,&Connection,&Stmt) == FAIL)
  {
    odbc_print_error(SQL_HANDLE_DBC, Connection); 
    ODBC_Disconnect(Env,Connection,Stmt);
    fprintf(stdout, "HALT! Could not connect to the server\n");
    return 1;
  }
  fprintf(stdout, "1..%d\n", tests_planned);
  while (tests->title)
  {
    buff_before_test= buff_pos;

    rc= tests->my_test();
    comment= "";
    if (rc!= SKIP && tests->test_type != NORMAL)
    {
      comment= comments[tests->test_type][rc];
      rc= OK;
    }
    else if (rc == FAIL)
    {
      failed++;
    }

    fprintf(stdout, "%s %d - %s%s\n", test_status[rc], i++,tests->title, comment);
    tests++;

    if (reset_changed_server_variables())
    {
      fprintf(stdout, "HALT! An error occurred while tried to reset server variables changed by the test!\n");
    }

    buff_pos= buff_before_test;
    *buff_pos= 0;
    if (Stmt != NULL)
    {
      SQLFreeStmt(Stmt, SQL_DROP);
    }
    SQLAllocHandle(SQL_HANDLE_STMT, Connection, &Stmt);
    /* reset Statement */
    fflush(stdout);
  }

  ODBC_Disconnect(Env,Connection,Stmt);

  if (failed)
    return 1;
  return 0;
}


int get_show_value(int global, const char * show_type, const char * var_name)
{
  int size, result;
  char query[512];

  size= _snprintf(query, sizeof(query), show_template, global ? locality[GLOBAL] : locality[LOCAL],
            show_type, var_name);

  /* Using automatically allocated (by the framework) STMT handle*/
  if (!SQL_SUCCEEDED(SQLExecDirect(Stmt, query, size))
   || !SQL_SUCCEEDED(SQLFetch(Stmt)))
  {
    /* atm I can't thing about any -1 value that can be interesting for tests.
       otherwise it will have to be changed */
    return -1;
  }

  result= my_fetch_int(Stmt, 2);

  SQLFreeStmt(Stmt, SQL_CLOSE);

  return result;
}


int get_server_status(int global, const char * var_name)
{
  return get_show_value(global, "STATUS", var_name); 
}


int get_server_variable(int global, const char * var_name)
{
  return get_show_value(global, "VARIABLES", var_name);
}

#define GET_SERVER_STATUS(int_result, global, name) int_result= get_server_status(global, name);\
  FAIL_IF(int_result < 0, "Could not get server status");

#define GET_SERVER_VAR(int_result, global, name) int_result= get_server_variable(global, name);\
  FAIL_IF(int_result < 0, "Could not get server variable");


/* Helper function to change server variable's value, but preserve its current
   value in order to set it back after test*/
int set_variable(int global, const char * var_name, int value)
{
  int i, size, cur_value;
  char query[512];

  for (i= 0; changed_server_variable[i].name != NULL && i < MAX_CHANGED_VARIABLES_PER_TEST; ++i);

  if (i == MAX_CHANGED_VARIABLES_PER_TEST)
  {
    FAIL_IF(TRUE, "For developer: the test has reached limit of variable changes. Please make MAX_CHANGED_VARIABLES_PER_TEST bigger")
  }

  GET_SERVER_VAR(cur_value, global, var_name);

  size= _snprintf(query, sizeof(query), set_template, global ? locality[GLOBAL] : locality[LOCAL],
                var_name, value);

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, query, size));

  changed_server_variable[i].global=  global;
  changed_server_variable[i].name=    var_name;
  changed_server_variable[i].value=   cur_value;

  return OK;
}

void printHex(char *str, int len)
{
  int i;
  for (i= 0; i<len; ++i)
  {
    printf("%02hhx", str[i]);
  }
}

#define IS_WSTR(a, b, c) \
do { \
  SQLWCHAR *val_a= (SQLWCHAR*)(a), *val_b= (SQLWCHAR*)(b); \
  int val_len= (int)(c); \
  if (memcmp(val_a, val_b, val_len * sizeof(SQLWCHAR)) != 0) { \
    printf("#%s('", #a); \
    printHex((char*)val_a, val_len*sizeof(SQLWCHAR)); \
    printf("') != %s('", #b); \
    printHex((char*)val_b, val_len*sizeof(SQLWCHAR)); \
    printf("') in %s on line %d", __FILE__, __LINE__); \
    printf("\n"); \
    return FAIL;\
  } \
} while (0);



SQLWCHAR *dup_char_as_sqlwchar(SQLCHAR *from)
{
  SQLWCHAR *to= malloc((strlen((char *)from) + 1) * sizeof(SQLWCHAR));
  SQLWCHAR *out= to;
  while (from && *from)
    *(to++)= (SQLWCHAR)*(from++);
  *to= 0;
  return out;
}


/* It can be utf8 or something, but the function does not recode to utf16 - simply copies each byte to SQLWCHAR */
SQLWCHAR* latin_as_sqlwchar(char *str, SQLWCHAR *buffer)
{
  SQLWCHAR *res= buffer;

  if (str == NULL)
  {
    return NULL;
  }

  while(*str)
  {
    *buffer++= *str++;
  }

  *buffer= 0;

  return res;
}

/**
  @len[in] - length of the source string in bytes, including teminating NULL
 */
SQLWCHAR * str2sqlwchar_on_gbuff(const char *str, size_t len, CHARSET_INFO *from_cs, CHARSET_INFO *to_cs)
{
  SQLWCHAR   *res= buff_pos;
  size_t      rc, buff_size= sqlwchar_buff + sizeof(sqlwchar_buff) - buff_pos;
  int         error;
  const char *src= str;

  if (len > buff_size/sizeof(SQLWCHAR))
  {
    /* That should not normally happen, but happened. Size of buffer has to be adjusted. */
    diag("NOTE: Global buffer for strings conversion is exhausted. The string will be truncated and the test will probably fail because of that");
    diag("Adjust buffer size and re-compile test. Otherwise using of shorter uid/pwd/dsn etc may help");

    return sqlwchar_empty;
  }

  rc= madbtest_convert_string(from_cs, src, &len, to_cs, (char*)buff_pos, &buff_size, &error);

  if (rc != (size_t)(-1))
  {
    buff_pos+= rc/sizeof(SQLWCHAR);
  }
  else
  {
    return sqlwchar_empty;
  }
  return res;
}


SQLINTEGER SqlwcsLen(SQLWCHAR *str)
{
  SQLINTEGER result= 0;

  if (str)
  {
    while (*str)
    {
      ++result;
      ++str;
    }
  }
  return result;
}


wchar_t *sqlwchar_to_wchar_t(SQLWCHAR *in)
{
  static wchar_t buff[2048];
  char *to= (char *)buff;

  if (sizeof(wchar_t) == sizeof(SQLWCHAR))
    return (wchar_t *)in;
  else
  {
    size_t len= (SqlwcsLen(in) + 1)*sizeof(SQLWCHAR);
    int    error;
    size_t buff_size=  sizeof(buff);

    madbtest_convert_string(utf16, (char*)in, &len, utf32, to, &buff_size, &error);
  }
  
  return buff;
}

#define LW(latin_str) latin_as_sqlwchar(latin_str, sqlwchar_buff)
#define WL(A,B) (sizeof(wchar_t) == sizeof(SQLWCHAR) ? (SQLWCHAR*)A : str2sqlwchar_on_gbuff((char*)(A), (B+1)*sizeof(wchar_t), utf32, utf16))
/* Wchar_t(utf32) to sqlWchar */
#define WW(A) WL(L##A,wcslen(L##A))
/* Pretty much the same as WW, but expects that L string */
#define W(A) WL(A,wcslen(A))
/**
 Helper for converting a (char *) to a (SQLWCHAR *)
*/
/*#define WC(string) dup_char_as_sqlwchar((string))*/
/* Char(utf8) to slqWchar */
#define CW(str) str2sqlwchar_on_gbuff(str, strlen(str)+1, utf8, utf16)

/* @n[in] - number of characters to compare. Negative means treating of strings as null-terminated */
int sqlwcharcmp(SQLWCHAR *s1, SQLWCHAR *s2, int n)
{
  if(s1 == NULL || s2 == NULL)
  {
    return s1!=s2;
  }

  while (n != 0 && *s1 && *s2 && *s1==*s2)
  {
    ++s1; ++s2; --n;
  }

  return n != 0 && *s1!=*s2;
}


char* ltrim(char *str)
{
  while (*str && iswspace(*str))
    ++str;

  return str;
}


/* connstr has to be null-terminated */
char* hide_pwd(char *connstr)
{
  char *ptr= connstr;

  while (*ptr)
  {
    BOOL is_pwd=          FALSE;
    char *key_val_border= strchr(ptr, '=');
    char stop_chr=        ';';

    ptr= ltrim(ptr);

    if (_strnicmp(ptr, "PWD", 3) == 0 || _strnicmp(ptr, "PASSWORD", 8) == 0)
    {
      is_pwd= TRUE;
    }
    if (key_val_border != NULL)
    {
      ptr= ltrim(key_val_border + 1);
    }
    if (*ptr == '{')
    {
      stop_chr= '}';
    }
    while (*ptr && *ptr != stop_chr)
    {
      if (is_pwd)
      {
        *ptr= '*';
      }
      ++ptr;
    }
    ++ptr;
  }

  return connstr;
}


BOOL ServerNotOlderThan(SQLHDBC Conn, unsigned int major, unsigned int minor, unsigned int patch)
{
  unsigned int ServerMajor= 0, ServerMinor= 0, ServerPatch= 0;
  SQLCHAR ServerVersion[32];

  SQLGetInfo(Conn, SQL_DBMS_VER, ServerVersion, sizeof(ServerVersion), NULL);

  sscanf(ServerVersion, "%u.%u.%u", &ServerMajor, &ServerMinor, &ServerPatch);

  if (ServerMajor < major || ServerMajor == major && (ServerMinor < minor || ServerMinor == minor && ServerPatch < patch))
  {
    return FALSE;
  }

  return TRUE;
}


BOOL UnixOdbc(HENV Env)
{
#ifdef SQL_ATTR_UNIXODBC_VERSION
  return TRUE;
#endif
  return FALSE;
 }

#endif      /* #ifndef _tap_h_ */
