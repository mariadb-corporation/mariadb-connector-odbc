/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013 MontyProgram AB

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
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <getopt.h>
#include <time.h>
#include <assert.h>

typedef unsigned int uint;

#include <mysql.h>


int tests_planned= 0;
char *test_status[]= {"fail", "ok", "skip"};

#define OK  1
#define FAIL 0
#define SKIP 2

#define MAX_ROW_DATA_LEN 1000
#define MAX_NAME_LEN 255

#define skip(A) diag((A)); return SKIP;

typedef struct st_ma_odbc_test {
  int (*my_test)();
  char *title;
} MA_ODBC_TESTS;

#define ODBC_TEST(a)\
int a()

#define plan(a)\
  tests_planned= a;\

#define FAIL_IF(expr,message)\
  if (expr)\
  {\
    fprintf(stdout, "%s (File: %s Line: %d)\n", message, __FILE__, __LINE__);\
    return FAIL;\
  }

SQLCHAR *my_dsn= (SQLCHAR *)"test";
SQLCHAR *my_uid= (SQLCHAR *)"root";
SQLCHAR *my_pwd= (SQLCHAR *)"";
SQLCHAR *my_schema= (SQLCHAR *)"odbc_test";
SQLCHAR *my_drivername= (SQLCHAR *)"MariaDB ODBC 1.0 Driver";
SQLCHAR *my_servername= (SQLCHAR *)"localhost";
unsigned long my_options= 67108866;

SQLHANDLE Env, Connection, Stmt;

unsigned int my_port= 3306;

unsigned long myresult(SQLHANDLE Stmt)
{
  unsigned long Rows= 0;
  while (SQLFetch(Stmt) != SQL_NO_DATA)
    Rows++;
  return Rows;
}

void usage()
{
  fprintf(stdout, "Valid options:\n");
  fprintf(stdout, "-d DSN Name\n");
  fprintf(stdout, "-u Username\n");
  fprintf(stdout, "-p Password\n");
  fprintf(stdout, "-s default database (schema)\n");
  fprintf(stdout, "-P Port number");
  fprintf(stdout, "?  Displays this text\n");
}

void get_options(int argc, char **argv)
{
  int c= 0;

  while ((c=getopt(argc,argv, "d:u:p:P:s:?")) >= 0)
  {
    switch(c) {
    case 'd':
      my_dsn= optarg;
      break;
    case 'u':
      my_uid= optarg;
      break;
    case 'p':
      my_pwd= optarg;
      break;
    case 's':
      my_schema= optarg;
      break;
    case 'P':
      my_port= atoi(optarg);
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

#define MAX_COLUMNS 1000

#define mystmt_rows(hstmt,r, row)  \
	do { \
	if (!SQL_SUCCEEDED(r)) \
	  return row; \
	} while (0)

int my_print_non_format_result(SQLHSTMT Stmt)
{
    SQLRETURN   rc;
    SQLUINTEGER nRowCount=0;
    SQLULEN     pcColDef;
    SQLCHAR     szColName[MAX_NAME_LEN+1];
    SQLCHAR     szData[MAX_COLUMNS][MAX_ROW_DATA_LEN]={{0}};
    SQLSMALLINT nIndex,ncol= 0,pfSqlType, pcbScale, pfNullable;
    SQLLEN      ind_strlen;

    rc = SQLNumResultCols(Stmt,&ncol);
    
    mystmt_rows(Stmt,rc,-1);

    for (nIndex = 1; nIndex <= ncol; ++nIndex)
    {
        rc = SQLDescribeCol(Stmt,nIndex,szColName, MAX_NAME_LEN, NULL,
                            &pfSqlType,&pcColDef,&pcbScale,&pfNullable);
        /* Returning in case of an error -nIndex we will see in the log column# */
        mystmt_rows(Stmt,rc,-nIndex);


        fprintf(stdout, "%s\t", szColName);

        rc = SQLBindCol(Stmt,nIndex, SQL_C_CHAR, szData[nIndex-1],
                        MAX_ROW_DATA_LEN+1,&ind_strlen);
        mystmt_rows(Stmt,rc,-nIndex);
    }

    fprintf(stdout,"\n");

    rc = SQLFetch(Stmt);
    while (SQL_SUCCEEDED(rc))
    {
        ++nRowCount;
        for (nIndex=0; nIndex< ncol; ++nIndex)
            fprintf(stdout, "%s\t", szData[nIndex]);

        fprintf(stdout, "\n");
        rc = SQLFetch(Stmt);
    }

    SQLFreeStmt(Stmt,SQL_UNBIND);
    SQLFreeStmt(Stmt,SQL_CLOSE);

    fprintf(stdout, "# Total rows fetched: %d\n", (int)nRowCount);

    return nRowCount;
}

#define OK_SIMPLE_STMT(stmt, stmtstr)\
if (SQLExecDirect((stmt),(stmtstr),strlen(stmtstr)) != SQL_SUCCESS)\
{\
  fprintf(stdout, "Error in %s:%d:\n", __FILE__, __LINE__);\
  odbc_print_error(SQL_HANDLE_STMT, (stmt));\
  return FAIL;\
}

#define OK_SIMPLE_STMTW(stmt, stmtstr)\
if (SQLExecDirectW((stmt),(stmtstr),SQL_NTS) != SQL_SUCCESS)\
{\
  fprintf(stdout, "Error in %s:%d:\n", __FILE__, __LINE__);\
  odbc_print_error(SQL_HANDLE_STMT, (stmt));\
  return FAIL;\
}

#define ERR_SIMPLE_STMT(stmt, stmtstr)\
if (SQLExecDirect((stmt),(stmtstr),SQL_NTS) != SQL_ERROR)\
{\
  fprintf(stdout, "Error expected in %s:%d:\n", __FILE__, __LINE__);\
  return FAIL;\
}

#define ERR_SIMPLE_STMTW(stmt, stmtstr)\
if (SQLExecDirectW((stmt),(stmtstr),SQL_NTS) != SQL_ERROR)\
{\
  fprintf(stdout, "Error expected in %s:%d:\n", __FILE__, __LINE__);\
  return FAIL;\
}

#define CHECK_HANDLE_RC(handletype, handle ,rc)\
if (!(SQL_SUCCEEDED(rc)))\
{\
  fprintf(stdout, "Error (rc=%d) in %s:%d:\n", rc, __FILE__, __LINE__);\
  odbc_print_error((handletype), (handle));\
  return FAIL;\
}

#define CHECK_STMT_RC(stmt,rc) CHECK_HANDLE_RC(SQL_HANDLE_STMT,stmt,rc)
#define CHECK_DBC_RC(dbc,rc) CHECK_HANDLE_RC(SQL_HANDLE_DBC,dbc,rc)
#define CHECK_ENV_RC(env,rc) CHECK_HANDLE_RC(SQL_HANDLE_ENV,env,rc)
#define CHECK_DESC_RC(desc,rc) CHECK_HANDLE_RC(SQL_HANDLE_DESC,desc,rc)

#define is_num(A,B) \
  if ((int)(A) != (int)(B))\
  {\
    diag("%s %d: expected value %d instead of %d", __FILE__, __LINE__, (B), (A));\
    return FAIL;\
  }

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
  }\
} while(0)

#define CHECK_SQLSTATE_EX(A,B,C)\
   {\
    char Buffer[10];\
    SQLRETURN ret= SQLGetDiagField((B),(A),1,SQL_DIAG_SQLSTATE, (SQLPOINTER *)Buffer, 10, NULL);\
    FAIL_IF(strcmp((C),Buffer) != 0, "Wrong error code");\
  }

int my_fetch_int(SQLHANDLE Stmt, unsigned int ColumnNumber)
{
  int value= 0;
  SQLGetData(Stmt, ColumnNumber, SQL_INTEGER, &value, 0, NULL);
  return value;
}

#define check_sqlstate(stmt, sqlstate) \
  check_sqlstate_ex((stmt), SQL_HANDLE_STMT, (sqlstate))


#define is_wstr(a, b, c) \
  if (wcsncmp(a,b,c) != 0)\
  {\
    diag("Comparison failed in %s line %d", __FILE__, __LINE__);\
    return FAIL;\
  }

wchar_t *sqlwchar_to_wchar_t(SQLWCHAR *in)
{
  static wchar_t buff[2048];
  wchar_t *to= buff;

  if (sizeof(wchar_t) == sizeof(SQLWCHAR))
    return (wchar_t *)in;
/*
  for ( ; *in && to < buff + sizeof(buff) - 2; in++)
    to+= utf16toutf32((UTF16 *)in, (UTF32 *)to);

  *to= L'\0';
  */
  return buff;
}

#define W(A) A
#define WL(A,B) A

SQLWCHAR *my_fetch_wstr(SQLHSTMT Stmt, SQLWCHAR *buffer, SQLUSMALLINT icol, SQLLEN Length)
{
  SQLRETURN rc;
  SQLLEN nLen;

  rc= SQLGetData(Stmt, icol, SQL_WCHAR, buffer, Length, &nLen);
  if (!SQL_SUCCEEDED(rc))
    return L"";
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

#define IS_STR(A,B,C) diag("%s %s", (A),(B)); FAIL_IF(strncmp((A), (B), (C)) != 0, "String comparison failed")

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

int using_dm(HDBC hdbc)
{
  SQLCHAR val[20];
  SQLSMALLINT len;

  if (SQLGetInfo(hdbc, SQL_DM_VER, val, sizeof(val), &len) == SQL_ERROR)
    return 0;

  return 1;
}

#define IS(A) if (!(A)) { diag("Error in %s:%d", __FILE__, __LINE__); return FAIL; }

int mydrvconnect(SQLHENV *henv, SQLHDBC *hdbc, SQLHSTMT *hstmt, SQLCHAR *connIn)
{
  SQLCHAR   connOut[MAX_NAME_LEN+1];
  SQLSMALLINT len;

  CHECK_ENV_RC(*henv, SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, henv));

  CHECK_ENV_RC(*henv, SQLSetEnvAttr(*henv, SQL_ATTR_ODBC_VERSION,
                              (SQLPOINTER)SQL_OV_ODBC3, 0));

  CHECK_ENV_RC(*henv, SQLAllocHandle(SQL_HANDLE_DBC, *henv,  hdbc));

  CHECK_DBC_RC(*hdbc, SQLDriverConnect(*hdbc, NULL, connIn, SQL_NTS, connOut,
                                 MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(*hdbc, SQLSetConnectAttr(*hdbc, SQL_ATTR_AUTOCOMMIT,
                                  (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0));

  CHECK_DBC_RC(*hdbc, SQLAllocHandle(SQL_HANDLE_STMT, *hdbc, hstmt));

  return OK;
}


int ODBC_Connect(SQLHANDLE *Env, SQLHANDLE *Connection, SQLHANDLE *Stmt)
{
  SQLRETURN rc;
  char buffer[100];
  char DSNString[1024];
  char DSNOut[1024];
  SQLSMALLINT Length;
  SQLHANDLE Stmt1;

  *Env=         NULL;
  *Connection=  NULL;
  *Stmt=        NULL;

  rc= SQLAllocHandle(SQL_HANDLE_ENV, NULL, Env);
  FAIL_IF(rc != SQL_SUCCESS, "Couldn't allocate environment handle");
  rc= SQLSetEnvAttr(*Env, SQL_ATTR_ODBC_VERSION,
                              (SQLPOINTER)SQL_OV_ODBC3, 0);
  FAIL_IF(rc != SQL_SUCCESS, "Couldn't set ODBC version");

  rc= SQLAllocHandle(SQL_HANDLE_DBC, *Env, Connection);
  FAIL_IF(rc != SQL_SUCCESS, "Couldn't allocate connection handle");

  _snprintf(DSNString, 1024, "DSN=%s;UID=%s;PWD=%s;PORT=%u;DATABASE=%s;OPTION=%ul;", my_dsn, my_uid,
           my_pwd, my_port, my_schema, my_options );
  printf("DSN: %s\n", DSNString);

  rc= SQLDriverConnect(*Connection,NULL, (SQLCHAR *)DSNString, SQL_NTS, (SQLCHAR *)DSNOut, 1024, &Length, SQL_DRIVER_NOPROMPT);
  FAIL_IF(rc != SQL_SUCCESS, "Connection failed");

  rc= SQLAllocHandle(SQL_HANDLE_STMT, *Connection, Stmt);
  FAIL_IF(rc != SQL_SUCCESS, "Couldn't allocate statement handle");

  rc= SQLAllocHandle(SQL_HANDLE_STMT, *Connection, &Stmt1);
  FAIL_IF(rc != SQL_SUCCESS, "Couldn't allocate statement handle");

  strcpy(buffer, "CREATE SCHEMA IF NOT EXISTS ");
  strcat(buffer, (my_schema) ? my_schema : "test");
  rc= SQLExecDirect(Stmt1, (SQLCHAR *)buffer, (SQLINTEGER)strlen(buffer));

  strcpy(buffer, "USE ");
  strcat(buffer, (my_schema) ? my_schema : "test");
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
    SQLFreeHandle(SQL_HANDLE_DBC, Connection);
  }
  if ( Env!= NULL)
  {
    SQLFreeHandle(SQL_HANDLE_ENV, Env);
  }
}

#define IS_WSTR(a, b, c) \
do { \
  wchar_t *val_a= (a), *val_b= (b); \
  int val_len= (int)(c); \
  if (memcmp(val_a, val_b, val_len * sizeof(wchar_t)) != 0) { \
    printf("# %s ('%*ls') != '%*ls' in %s on line %d\n", \
           #a, val_len, val_a, val_len, val_b, __FILE__, __LINE__); \
    return FAIL; \
  } \
} while (0);


/**
 Helper for converting a (char *) to a (SQLWCHAR *)
*/
#define WC(string) dup_char_as_sqlwchar((string))


/**
  Convert a char * to a SQLWCHAR *. New space is allocated and never freed.
  Because this is used in short-lived test programs, this is okay, if not
  ideal.
*/
SQLWCHAR *dup_char_as_sqlwchar(SQLCHAR *from)
{
  SQLWCHAR *to= malloc((strlen((char *)from) + 1) * sizeof(SQLWCHAR));
  SQLWCHAR *out= to;
  while (from && *from)
    *(to++)= (SQLWCHAR)*(from++);
  *to= 0;
  return out;
}

int run_tests(MA_ODBC_TESTS *tests)
{
  int rc, i=1, failed=0;

  if (ODBC_Connect(&Env,&Connection,&Stmt) == FAIL)
  {
    ODBC_Disconnect(Env,Connection,Stmt);
    fprintf(stdout, "HALT! Could not connect to the server\n");
    return 1;
  }
  fprintf(stdout, "1..%d\n", tests_planned);
  while (tests->title)
  {
    rc= tests->my_test();
    if (rc == FAIL)
      failed++;
    fprintf(stdout, "%s %d - %s\n", test_status[rc], i++,tests->title);
    tests++;
    SQLFreeStmt(Stmt, SQL_DROP);
    SQLAllocHandle(SQL_HANDLE_STMT, Connection, &Stmt);
    /* reset Statement */
  }
  ODBC_Disconnect(Env,Connection,Stmt);
  if (failed)
    return 1;
  return 0;
}
#endif
