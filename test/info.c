/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013, 2022 MariaDB Corporation AB

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

#include "tap.h"

ODBC_TEST(t_gettypeinfo)
{
  SQLSMALLINT pccol;

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_ALL_TYPES));

  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &pccol));
  is_num(pccol, 19);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


ODBC_TEST(sqlgetinfo)
{
  SQLCHAR   rgbValue[100];
  SQLSMALLINT pcbInfo;

  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_DRIVER_ODBC_VER, rgbValue,
                          sizeof(rgbValue), &pcbInfo));

  is_num(pcbInfo, 5);
  IS_STR(rgbValue, "03.51", 5);

  return OK;
}


ODBC_TEST(t_stmt_attr_status)
{
  SQLUSMALLINT rowStatusPtr[3];
  SQLULEN      rowsFetchedPtr= 0;


  if (ForwardOnly == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY option are selected");
  }
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_stmtstatus");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_stmtstatus (id INT, name CHAR(20))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_stmtstatus VALUES (10,'data1'),(20,'data2')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_SCROLLABLE,
                                (SQLPOINTER)SQL_NONSCROLLABLE, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_stmtstatus");

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR,
                                &rowsFetchedPtr, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_STATUS_PTR,
                                rowStatusPtr, 0));

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, 2) != SQL_ERROR, "Error expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_SCROLLABLE,
                                (SQLPOINTER)SQL_SCROLLABLE, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_stmtstatus");

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, 2));

  is_num(rowsFetchedPtr, 1);
  is_num(rowStatusPtr[0], 0);
 /* above error is a result from bind_dummy */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, NULL, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_STATUS_PTR, NULL, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_stmtstatus");

  return OK;
}


ODBC_TEST(t_msdev_bug)
{
  SQLCHAR    catalog[SQL_MAX_OPTION_STRING_LENGTH];
  SQLINTEGER len;
#pragma warning( push )
#pragma warning( disable: 4996)
  CHECK_DBC_RC(Connection, SQLGetConnectOption(Connection, SQL_CURRENT_QUALIFIER, catalog));
#pragma warning( pop )
  IS_STR(catalog, my_schema, strlen(my_schema));

  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, catalog,
                                 sizeof(catalog), &len));
  is_num(len, strlen(my_schema));
  IS_STR(catalog, my_schema, strlen(my_schema));

  return OK;
}


/**
  Bug #28657: ODBC Connector returns FALSE on SQLGetTypeInfo with DATETIME (wxWindows latest)
*/
ODBC_TEST(t_bug28657)
{
#ifdef _WIN32
  /*
   The Microsoft Windows ODBC driver manager automatically maps a request
   for SQL_DATETIME to SQL_TYPE_DATE, which means our little workaround to
   get all of the SQL_DATETIME types at once does not work on there.
  */
  skip("test doesn't work with Microsoft Windows ODBC driver manager");
#else
  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_DATETIME));

  FAIL_IF(myrowcount(Stmt) <= 1, "Expected > 1");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
#endif
}


ODBC_TEST(t_bug14639)
{
  SQLINTEGER connection_id;
  SQLUINTEGER is_dead;
  char buf[100];
  SQLHENV henv2;
  SQLHANDLE  Connection2;
  SQLHANDLE Stmt2;

  /* Create a new connection that we deliberately will kill */
  ODBC_Connect(&henv2, &Connection2, &Stmt2);
  OK_SIMPLE_STMT(Stmt2, "SELECT connection_id()");
  CHECK_STMT_RC(Stmt2, SQLFetch(Stmt2));
  connection_id= my_fetch_int(Stmt2, 1);
  CHECK_STMT_RC(Stmt2, SQLFreeStmt(Stmt2, SQL_CLOSE));

  /* Check that connection is alive */
  CHECK_DBC_RC(Connection2, SQLGetConnectAttr(Connection2, SQL_ATTR_CONNECTION_DEAD, &is_dead,
                                 sizeof(is_dead), 0));
  is_num(is_dead, SQL_CD_FALSE);

  /* From another connection, kill the connection created above */
  sprintf(buf, "KILL %d", connection_id);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, (SQLCHAR *)buf, SQL_NTS));

  /* Now check that the connection killed returns the right state */
  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection2, SQL_ATTR_CONNECTION_DEAD, &is_dead,
                                 sizeof(is_dead), 0));
  is_num(is_dead, SQL_CD_TRUE);

  return OK;
}


/**
 Bug #31055: Uninitiated memory returned by SQLGetFunctions() with
 SQL_API_ODBC3_ALL_FUNCTION
*/
ODBC_TEST(t_bug31055)
{
  SQLUSMALLINT funcs[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];

  /*
     The DM will presumably return true for all functions that it
     can satisfy in place of the driver. This test will only work
     when linked directly to the driver.
  */
  if (using_dm(Connection))
    return OK;

  memset(funcs, 0xff, sizeof(SQLUSMALLINT) * SQL_API_ODBC3_ALL_FUNCTIONS_SIZE);

  CHECK_DBC_RC(Connection, SQLGetFunctions(Connection, SQL_API_ODBC3_ALL_FUNCTIONS, funcs));

  is_num(SQL_FUNC_EXISTS(funcs, SQL_API_SQLALLOCHANDLESTD), 0);

  return OK;
}


/*
   Bug 3780, reading or setting ADODB.Connection.DefaultDatabase 
   is not supported
*/
ODBC_TEST(t_bug3780)
{
  SQLHANDLE Connection1;
  SQLHANDLE Stmt1;
  SQLCHAR   conn[512], conn_out[512];
  SQLSMALLINT conn_out_len;
  SQLCHAR   rgbValue[MAX_NAME_LEN];
  SQLSMALLINT pcbInfo;
  SQLINTEGER attrlen;

  /* The connection string must not include DATABASE. */
  sprintf((char *)conn, "DRIVER=%s;SERVER=%s;UID=%s;PASSWORD=%s;%s;%s",
          my_drivername, my_servername, my_uid, my_pwd, ma_strport, add_connstr);
  diag(conn);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &Connection1));

  CHECK_DBC_RC(Connection1, SQLDriverConnect(Connection1, NULL, conn, (SQLSMALLINT)strlen(conn), conn_out,
                                 (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(Connection1, SQLAllocStmt(Connection1, &Stmt1));

  CHECK_DBC_RC(Connection1, SQLGetInfo(Connection1, SQL_DATABASE_NAME, rgbValue, 
                           MAX_NAME_LEN, &pcbInfo));

  is_num(pcbInfo, 4);
  IS_STR(rgbValue, "null", pcbInfo);

  CHECK_DBC_RC(Connection1, SQLGetConnectAttr(Connection1, SQL_ATTR_CURRENT_CATALOG,
                                  rgbValue, MAX_NAME_LEN, &attrlen));

  is_num(attrlen, 4);
  IS_STR(rgbValue, "null", attrlen);

  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_DROP));
  CHECK_DBC_RC(Connection1, SQLDisconnect(Connection1));
  CHECK_DBC_RC(Connection1, SQLFreeHandle(SQL_HANDLE_DBC, Connection1));

  return OK;
}


/*
  Bug#16653
  MyODBC 3 / truncated UID when performing Data Import in MS Excel
*/
ODBC_TEST(t_bug16653)
{
  SQLHANDLE Connection1;
  SQLCHAR buf[50];

  /*
    Driver managers handle SQLGetConnectAttr before connection in
    inconsistent ways.
  */
  if (using_dm(Connection))
    return OK;

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &Connection1));

  /* this would cause a crash if we arent connected */
  FAIL_IF(SQLGetConnectAttr(Connection1, SQL_ATTR_CURRENT_CATALOG,
                                  buf, 50, NULL) != SQL_ERROR, "Error expected");
  
  CHECK_DBC_RC(Connection1, SQLFreeHandle(SQL_HANDLE_DBC, Connection1));

  return OK;
}


/*
  Bug #30626 - No result record for SQLGetTypeInfo for TIMESTAMP
*/
ODBC_TEST(t_bug30626)
{
  SQLHANDLE henv1;
  SQLHANDLE Connection1;
  SQLHANDLE Stmt1;
  SQLCHAR conn[512];
  
  /* odbc 3 */
  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_TYPE_TIMESTAMP));
  is_num(myrowcount(Stmt), 2);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_TYPE_TIME));
  is_num(myrowcount(Stmt), 1);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_TYPE_DATE));
  is_num(myrowcount(Stmt), 1);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* odbc 2 */
  sprintf((char *)conn, "DRIVER=%s;SERVER=%s;UID=%s;PASSWORD=%s;%s;%s",
          my_drivername, my_servername, my_uid, my_pwd, ma_strport, add_connstr);
  
  CHECK_ENV_RC(henv1, SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv1));
  CHECK_ENV_RC(henv1, SQLSetEnvAttr(henv1, SQL_ATTR_ODBC_VERSION,
			      (SQLPOINTER) SQL_OV_ODBC2, SQL_IS_INTEGER));
  CHECK_ENV_RC(henv1, SQLAllocHandle(SQL_HANDLE_DBC, henv1, &Connection1));
  CHECK_DBC_RC(Connection1, SQLDriverConnect(Connection1, NULL, conn, (SQLSMALLINT)strlen((const char*)conn), NULL, 0,
				 NULL, SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(Connection1, SQLAllocHandle(SQL_HANDLE_STMT, Connection1, &Stmt1));
  
  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_TIMESTAMP));
  is_num(myrowcount(Stmt1), 2);
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_TIME));
  is_num(myrowcount(Stmt1), 1);
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_DATE));
  is_num(myrowcount(Stmt1), 1);
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLFreeHandle(SQL_HANDLE_STMT, Stmt1));
  CHECK_DBC_RC(Connection1, SQLDisconnect(Connection1));
  CHECK_DBC_RC(Connection1, SQLFreeHandle(SQL_HANDLE_DBC, Connection1));
  CHECK_ENV_RC(henv1, SQLFreeHandle(SQL_HANDLE_ENV, henv1));

  return OK;
}


/*
  Bug #43855 - conversion flags not complete
*/
ODBC_TEST(t_bug43855)
{
  SQLUINTEGER convFlags;
  SQLSMALLINT pcbInfo;
  int check;
  /* 
    TODO: add other convert checks, we are only interested in CHAR now 
  */
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_CONVERT_CHAR, &convFlags,
                          sizeof(convFlags), &pcbInfo));

  is_num(pcbInfo, 4);
  check= ((convFlags & SQL_CVT_CHAR) && (convFlags & SQL_CVT_NUMERIC) &&
     (convFlags & SQL_CVT_DECIMAL) && (convFlags & SQL_CVT_INTEGER) &&
     (convFlags & SQL_CVT_SMALLINT) && (convFlags & SQL_CVT_FLOAT) &&
     (convFlags & SQL_CVT_REAL) && (convFlags & SQL_CVT_DOUBLE) &&
     (convFlags & SQL_CVT_VARCHAR) && (convFlags & SQL_CVT_LONGVARCHAR) &&
     (convFlags & SQL_CVT_BIT) && (convFlags & SQL_CVT_TINYINT) &&
     (convFlags & SQL_CVT_BIGINT) && (convFlags & SQL_CVT_DATE) &&
     (convFlags & SQL_CVT_TIME) && (convFlags & SQL_CVT_TIMESTAMP) &&
     (convFlags & SQL_CVT_WCHAR) && (convFlags &SQL_CVT_WVARCHAR) &&
     (convFlags & SQL_CVT_WLONGVARCHAR));
  FAIL_IF(!check, "Flag check failed");
  return OK;
}


/*
Bug#46910
MyODBC 5 - calling SQLGetConnectAttr before getting all results of "CALL ..." statement
*/
ODBC_TEST(t_bug46910)
{
	SQLCHAR     catalog[64];
	SQLINTEGER  len;
  SQLLEN      i;
  SQLSMALLINT col_count;

	SQLCHAR * initStmt[]= {"DROP PROCEDURE IF EXISTS `spbug46910_1`",
		"CREATE PROCEDURE `spbug46910_1`()\
		BEGIN\
		SELECT 1 AS ret;\
		END"};

	SQLCHAR * cleanupStmt= "DROP PROCEDURE IF EXISTS `spbug46910_1`;";

	for (i= 0; i < 2; ++i)
		CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, initStmt[i], SQL_NTS));

	SQLExecDirect(Stmt, "CALL spbug46910_1()", SQL_NTS);
	
	CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &i));
  if (ForwardOnly != TRUE || NoCache != TRUE)
  {
    /* With streaming we can't get this */
    is_num(i, 1);
  }
	CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &col_count));
  is_num(col_count, 1);
  is_num(SQLMoreResults(Stmt), SQL_SUCCESS);
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &i));
  is_num(i, 0);
  FAIL_IF(SQLMoreResults(Stmt)!= SQL_NO_DATA, "No more results expected");

	SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, catalog,
		sizeof(catalog), &len);
	is_num(len, strlen(my_schema));
	IS_STR(catalog, my_schema, strlen(my_schema));

  	/*CHECK_DBC_RC(Connection, */
	SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, catalog,
		sizeof(catalog), &len);

	CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, cleanupStmt, SQL_NTS));

	return OK;
}


/*
  Bug#11749093: SQLDESCRIBECOL RETURNS EXCESSIVLY LONG COLUMN NAMES
  Maximum size of column length returned by SQLDescribeCol should be 
  same as what SQLGetInfo() for SQL_MAX_COLUMN_NAME_LEN returns.
*/
ODBC_TEST(t_bug11749093)
{
  char        colName[512];
  SQLSMALLINT colNameLen;
  SQLSMALLINT maxColLen;

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, 
              "SELECT 1234567890+2234567890+3234567890"
              "+4234567890+5234567890+6234567890+7234567890+"
              "+8234567890+9234567890+1034567890+1234567890+"
              "+1334567890+1434567890+1534567890+1634567890+"
              "+1734567890+1834567890+1934567890+2034567890+"
              "+2134567890+2234567890+2334567890+2434567890+"
              "+2534567890+2634567890+2734567890+2834567890"
              , SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLGetInfo(Connection, SQL_MAX_COLUMN_NAME_LEN, &maxColLen, 255, NULL));

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 1, colName, sizeof(colName), &colNameLen,
                    NULL, NULL, NULL, NULL));

  /* Checking that name is trimmed to SQL_MAX_COLUMN_NAME_LEN including teminating NULL. */
  is_num(colNameLen, maxColLen);
  is_num(colNameLen, strlen(colName));
  IS_STR(colName, "1234567890+2234567890+3234567890"
              "+4234567890+5234567890+6234567890+7234567890+"
              "+8234567890+9234567890+1034567890+1234567890+"
              "+1334567890+1434567890+1534567890+1634567890+"
              "+1734567890+1834567890+1934567890+2034567890+"
              "+2134567890+2234567890+2334567890+2434567890+"
              "+2534567890+2634567890+2734567890+2834567890", colNameLen);
  
  return OK;
}


/* https://mariadb.atlassian.net/browse/ODBC-15
   MariaDB ODBC connector did not support SQL_ODBC_API_CONFORMANCE info type
   Also the testcase checks if correct value returned for SQL_ODBC_SQL_CONFORMANCE */
ODBC_TEST(bug_odbc15)
{
  SQLSMALLINT info= 0xef;

  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_ODBC_API_CONFORMANCE, &info,
                          0, NULL));
  is_num(info, SQL_OAC_LEVEL1);

  info= 0xef;
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_ODBC_SQL_CONFORMANCE , &info,
                          0, NULL));
  is_num(info, SQL_OSC_CORE);

  return OK;
}


/* SQL_NEED_LONG_DATA_LEN has to be "N". Driver is sensitive to its change(DAE functionality)
   Test's purpose is to signalize about its change. */
ODBC_TEST(test_need_long_data_len)
{
  SQLCHAR     NeedLongDataLen[2];
  SQLSMALLINT Len;

   CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_NEED_LONG_DATA_LEN, NeedLongDataLen,
                          sizeof(NeedLongDataLen), &Len));
   is_num(Len, 1);
   IS_STR(NeedLongDataLen, "N", 2);

   return OK;
}


/* https://jira.mariadb.org/browse/ODBC-61
Request of SQL_FILE_USAGE info crashes connector */
ODBC_TEST(odbc61)
{
  SQLUSMALLINT info= 0xef;

  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_FILE_USAGE, &info,
    0, NULL));

  return OK;
}

/*
Bug ODBC-84 and ODBC-62. For ODBC-84 we only tested, that SQLGetTypeInfo returns something for WCHAR types
For ODBC-62 we need to check CREATE_PARAMS
*/
ODBC_TEST(odbc84_62)
{
  SQLHANDLE henv1;
  SQLHANDLE Connection1;
  SQLHANDLE Stmt1;
  SQLCHAR conn[512], params[64];
  SQLLEN ind;
  /* odbc 3 */
  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_WCHAR));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, params, 6), "length", sizeof("length"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_WVARCHAR));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, params, 6), "length", sizeof("length"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_WLONGVARCHAR));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 6, SQL_C_CHAR, params, sizeof(params), &ind));
  is_num(ind, SQL_NULL_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_INTEGER));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 6, SQL_C_CHAR, params, sizeof(params), &ind));
  is_num(ind, SQL_NULL_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_CHAR));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, params, 6), "length", sizeof("length"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_VARCHAR));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, params, 6), "length", sizeof("length"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_LONGVARCHAR));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 6, SQL_C_CHAR, params, sizeof(params), &ind));
  is_num(ind, SQL_NULL_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_DECIMAL));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, params, 6), "precision,scale", sizeof("precision,scale"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_FLOAT));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, params, 6), "precision,scale", sizeof("precision,scale"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_DOUBLE));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, params, 6), "precision,scale", sizeof("precision,scale"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* odbc 2 */
  sprintf((char *)conn, "DRIVER=%s;SERVER=%s;UID=%s;PASSWORD=%s;%s;%s",
    my_drivername, my_servername, my_uid, my_pwd, ma_strport, add_connstr);

  CHECK_ENV_RC(henv1, SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv1));
  CHECK_ENV_RC(henv1, SQLSetEnvAttr(henv1, SQL_ATTR_ODBC_VERSION,
    (SQLPOINTER)SQL_OV_ODBC2, SQL_IS_INTEGER));
  CHECK_ENV_RC(henv1, SQLAllocHandle(SQL_HANDLE_DBC, henv1, &Connection1));
  CHECK_DBC_RC(Connection1, SQLDriverConnect(Connection1, NULL, conn, (SQLSMALLINT)strlen((const char*)conn), NULL, 0,
    NULL, SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(Connection1, SQLAllocHandle(SQL_HANDLE_STMT, Connection1, &Stmt1));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_WCHAR));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  IS_STR(my_fetch_str(Stmt1, params, 6), "length", sizeof("length"));
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_WVARCHAR));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  IS_STR(my_fetch_str(Stmt1, params, 6), "length", sizeof("length"));
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_WLONGVARCHAR));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  CHECK_STMT_RC(Stmt1, SQLGetData(Stmt1, 6, SQL_C_CHAR, params, sizeof(params), &ind));
  is_num(ind, SQL_NULL_DATA);
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_INTEGER));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  CHECK_STMT_RC(Stmt1, SQLGetData(Stmt1, 6, SQL_C_CHAR, params, sizeof(params), &ind));
  is_num(ind, SQL_NULL_DATA);
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_CHAR));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  IS_STR(my_fetch_str(Stmt1, params, 6), "length", sizeof("length"));
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_VARCHAR));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  IS_STR(my_fetch_str(Stmt1, params, 6), "length", sizeof("length"));
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_LONGVARCHAR));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  CHECK_STMT_RC(Stmt1, SQLGetData(Stmt1, 6, SQL_C_CHAR, params, sizeof(params), &ind));
  is_num(ind, SQL_NULL_DATA);
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_DECIMAL));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  IS_STR(my_fetch_str(Stmt1, params, 6), "precision,scale", sizeof("precision,scale"));
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_FLOAT));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  IS_STR(my_fetch_str(Stmt1, params, 6), "precision,scale", sizeof("precision,scale"));
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLGetTypeInfo(Stmt1, SQL_DOUBLE));
  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  IS_STR(my_fetch_str(Stmt1, params, 6), "precision,scale", sizeof("precision,scale"));
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLFreeHandle(SQL_HANDLE_STMT, Stmt1));
  CHECK_DBC_RC(Connection1, SQLDisconnect(Connection1));
  CHECK_DBC_RC(Connection1, SQLFreeHandle(SQL_HANDLE_DBC, Connection1));
  CHECK_ENV_RC(henv1, SQLFreeHandle(SQL_HANDLE_ENV, henv1));

  return OK;
}

/* Test for part of problems causing ODBC-71. Other part is tested in desc.c:t_set_explicit_copy*/
ODBC_TEST(odbc71)
{
  SQLINTEGER Info;

  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_POS_OPERATIONS, &Info, 0, NULL));
  is_num(Info, SQL_POS_POSITION | SQL_POS_REFRESH | SQL_POS_UPDATE | SQL_POS_DELETE | SQL_POS_ADD);
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_STATIC_SENSITIVITY, &Info, 0, NULL));
  is_num(Info, SQL_SS_DELETIONS | SQL_SS_UPDATES);
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_LOCK_TYPES, &Info, 0, NULL));
  is_num(Info, SQL_LCK_NO_CHANGE);
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_SCROLL_CONCURRENCY, &Info, 0, NULL));
  is_num(Info, SQL_SCCO_READ_ONLY | SQL_SCCO_OPT_VALUES);

  /* This part is more for ODBC-62. Just checking that we return smth for these info types */
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_CONVERT_WCHAR, &Info, 0, NULL));
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_CONVERT_WVARCHAR, &Info, 0, NULL));
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_CONVERT_WLONGVARCHAR, &Info, 0, NULL));

  return OK;
}


/* ODBC-123 Test for SQL_CATALOG_LOCATION info type. The connector incorrectly wrote to the buffer SQLUINTEGER,
   while it has to be SQLUSMALLINT,
   Also testing SQL_GROUP_BY, which has the same problem
   Adding here test from ODBC-277 about SQL_IDENTIFIER_CASE. It's also has to be SQLUSMALLINT*/
ODBC_TEST(odbc123odbc277)
{
  SQLUSMALLINT Info;

  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_CATALOG_LOCATION, &Info, 0, NULL));
  is_num(Info, SQL_CL_START);
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_GROUP_BY, &Info, 0, NULL));
  is_num(Info, SQL_GB_NO_RELATION);
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_IDENTIFIER_CASE, &Info, 0, NULL));
  is_num(Info, SQL_IC_MIXED);

  return OK;
}


/* ODBC-109 It's not clear atm if that is the reason behind the report, but it was found during work on this bug.
   For the info type SQL_SCHEMA_TERM/SQL_OWNER_TERM the connector returned wrong length. Or more exactly - did not
   write anything into application's length buffer */
ODBC_TEST(odbc109)
{
  SQLCHAR     tn;
  SQLSMALLINT StrLen;
  SQLHANDLE henv1;
  SQLHANDLE Connection1;
  SQLCHAR   conn[512];

  /* SQL_OWNER_TERM is the same as SQL_SCHEMA_TERM */
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_OWNER_TERM, NULL, 0, &StrLen));
  is_num(StrLen, 0);
  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_OWNER_TERM, &tn, 1, NULL));
  is_num(tn, 0);

  /* odbc 2 */
  sprintf((char *)conn, "DRIVER=%s;SERVER=%s;UID=%s;PASSWORD=%s;%s;%s",
    my_drivername, my_servername, my_uid, my_pwd, ma_strport, add_connstr);

  CHECK_ENV_RC(henv1, SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv1));
  CHECK_ENV_RC(henv1, SQLSetEnvAttr(henv1, SQL_ATTR_ODBC_VERSION,
    (SQLPOINTER)SQL_OV_ODBC2, SQL_IS_INTEGER));
  CHECK_ENV_RC(henv1, SQLAllocHandle(SQL_HANDLE_DBC, henv1, &Connection1));
  CHECK_DBC_RC(Connection1, SQLDriverConnect(Connection1, NULL, conn, (SQLSMALLINT)strlen((const char*)conn), NULL, 0,
    NULL, SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(Connection1, SQLGetInfo(Connection1, SQL_OWNER_TERM, NULL, 0, &StrLen));
  is_num(StrLen, 0);
  CHECK_DBC_RC(Connection1, SQLGetInfo(Connection1, SQL_OWNER_TERM, &tn, 1, NULL));
  is_num(tn, 0);
  
  return OK;
}


ODBC_TEST(odbc143)
{
  SQLCHAR Info[2];
  SQLWCHAR wInfo[2];
  SQLHANDLE Hdbc1, Stmt1;
  SQLSMALLINT Length;

  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_IDENTIFIER_QUOTE_CHAR, Info, sizeof(Info), &Length));
  IS_STR(Info, "`", 2);
  is_num(Length, 1);
  /* Checking W function as well */
  CHECK_DBC_RC(Connection, SQLGetInfoW(Connection, SQL_IDENTIFIER_QUOTE_CHAR, wInfo, sizeof(wInfo), &Length));
  is_num(wInfo[0], '`');
  is_num(wInfo[1], 0);
  is_num(Length, sizeof(SQLWCHAR));

  AllocEnvConn(&Env, &Hdbc1);
  Stmt1= DoConnect(Hdbc1, FALSE, NULL, NULL, NULL, 0, NULL, 0, NULL, NULL);
  FAIL_IF(Stmt1 == NULL, "Could not connect and/or allocate");

  OK_SIMPLE_STMT(Stmt1, "SET @@SESSION.sql_mode='ANSI_QUOTES'");
  CHECK_DBC_RC(Hdbc1, SQLGetInfo(Hdbc1, SQL_IDENTIFIER_QUOTE_CHAR, Info, sizeof(Info), &Length));
  IS_STR(Info, "\"", 2);
  is_num(Length, 1);

  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_DROP));
  CHECK_DBC_RC(Hdbc1, SQLDisconnect(Hdbc1));
  CHECK_DBC_RC(Hdbc1, SQLFreeConnect(Hdbc1));

  return OK;
}

/**
  Test of connect attributes that used to be treated as wrong data type.
  SQL_ATTR_TXN_ISOLATION aslo had issue, but it is already cared of by other test
*/
ODBC_TEST(odbc317)
{
  const SQLULEN init= sizeof(SQLULEN)==8 ? 0xcacacacacacacaca : 0; //on 32 bit it's the same size with SQLUINTEGER, thus nothing to test
  SQLULEN lenAttr= init;
  /*SQLINTEGER intAttr= 0xacacacac;*/

  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_ODBC_CURSORS, &lenAttr,
    SQL_IS_UINTEGER, NULL));

  /* We ourselves kinda return ODBC(TODO: need to check why), but all other DM's return DRIVER. iODBC 3.52.12 used to do that toTODO: need to check why. The test is anyway about using correct data type for the attribute */
  if (iOdbc() && DmMinVersion(3, 52, 13))
  {
    is_num(lenAttr, SQL_CUR_USE_ODBC);
  }
  else
  {
    is_num(lenAttr, SQL_CUR_USE_DRIVER);
  }

  return OK;
}


/* ODBC-326 Connecting Excel with MariaDB through Microsoft Query - String data right truncated
   The problem occured in the SQLGetInfo call when one of fetches returned SQL_SUCCESS_WITH_INFO
   because of incorrectly detected truncation
 */
ODBC_TEST(odbc326)
{
  SQLLEN len[7]= {0,0,0,0,0,0,0};
  SQLSMALLINT col2, col9, col15, col7;
  /* This are expected ODBCv3 type results */
  const SQLSMALLINT ref2[]= {SQL_BIT, SQL_BIT, SQL_TINYINT, SQL_TINYINT, SQL_BIGINT, SQL_BIGINT, SQL_LONGVARBINARY,
    SQL_LONGVARBINARY, SQL_LONGVARBINARY, SQL_LONGVARBINARY, SQL_LONGVARBINARY, SQL_VARBINARY, SQL_BINARY, SQL_LONGVARCHAR ,
    SQL_LONGVARCHAR, SQL_LONGVARCHAR, SQL_LONGVARCHAR, SQL_LONGVARCHAR, SQL_CHAR, SQL_NUMERIC, SQL_DECIMAL,
    SQL_INTEGER, SQL_INTEGER, SQL_INTEGER, SQL_INTEGER, SQL_INTEGER, SQL_INTEGER, SQL_SMALLINT, SQL_SMALLINT,
    SQL_FLOAT, SQL_DOUBLE, SQL_DOUBLE, SQL_DOUBLE, SQL_VARCHAR, SQL_VARCHAR, SQL_VARCHAR, SQL_TYPE_DATE, SQL_TYPE_TIME,
    SQL_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, SQL_WCHAR, SQL_WVARCHAR, SQL_WLONGVARCHAR};
  SQLINTEGER col3;
  SQLCHAR col4[128], col5[128];
  SQLRETURN rc;
  unsigned int i= 0;

  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_ALL_TYPES));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_DEFAULT, (SQLPOINTER)&col2, 2, &len[0]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 4, SQL_C_CHAR, (SQLPOINTER)col4, 128, &len[1]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 5, SQL_C_CHAR, (SQLPOINTER)col5, 128, &len[2]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 9, SQL_C_DEFAULT, (SQLPOINTER)&col9, 2, &len[3]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_DEFAULT, (SQLPOINTER)&col3, 4, &len[4]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 15, SQL_C_DEFAULT, (SQLPOINTER)&col15, 2, &len[5]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 7, SQL_C_DEFAULT, (SQLPOINTER)&col7, 2, &len[6]));

  while ((rc = SQLFetch(Stmt)) == SQL_SUCCESS)
  {
    IS(i < sizeof(ref2)/sizeof(SQLSMALLINT));
    is_num(col2, ref2[i]);
    ++i;
  }
  
  EXPECT_STMT(Stmt, rc, SQL_NO_DATA_FOUND);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  { t_gettypeinfo, "t_gettypeinfo", NORMAL },
  { sqlgetinfo, "sqlgetinfo", NORMAL },
  { t_stmt_attr_status, "t_stmt_attr_status", NORMAL },
  { t_msdev_bug, "t_msdev_bug", NORMAL },
  { t_bug14639, "t_bug14639", NORMAL },
  { t_bug31055, "t_bug31055", NORMAL },
  { t_bug3780, "t_bug3780", NORMAL },
  { t_bug16653, "t_bug16653", NORMAL },
  { t_bug30626, "t_bug30626", NORMAL },
  { t_bug43855, "t_bug43855", NORMAL },
  { t_bug46910, "t_bug46910", NORMAL },
  { t_bug11749093, "t_bug11749093", NORMAL },
  { bug_odbc15, "odbc15", NORMAL },
  { test_need_long_data_len, "test_need_long_data_len", NORMAL },
  { odbc61, "odbc61_SQL_FILE_USAGE", NORMAL },
  { odbc84_62, "odbc84_WCHAR_types_odbc62_CREATE_PARAMS", NORMAL },
  { odbc71, "odbc71_some_odbc2_types", NORMAL },
  { odbc123odbc277, "odbc123_catalog_start_odbc277", NORMAL },
  { odbc109, "odbc109_shema_owner_term", NORMAL },
  { odbc143, "odbc143_odbc160_ANSI_QUOTES", NORMAL },
  { odbc317, "odbc317_conattributes", NORMAL },
  { odbc326, "odbc326", NORMAL },
  { NULL, NULL }
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
