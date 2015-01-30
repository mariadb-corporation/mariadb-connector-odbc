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

#include "tap.h"

#include <time.h>

#include <sqlucode.h>

ODBC_TEST(test_CONO1)
{
  /* check SQLColumns with ANSI_QUOTES on and off */
  SQLRETURN ret;
  SQLLEN rowCount;
  SQLWCHAR *create_table= L"CREATE TABLE cono1 (InitialStartDateTime datetime NOT NULL,  TicketId int(11) NOT NULL AUTO_INCREMENT,  CallCount int(11) NOT NULL DEFAULT '1',  CalledNumber varchar(30) DEFAULT NULL,  CallingNumber varchar(30) DEFAULT NULL,  CallType tinyint(3) unsigned DEFAULT NULL,  ChargeUnits smallint(6) DEFAULT NULL,  NetworkAndTrunkNode int(11) DEFAULT NULL,  TrunkGroupIdentity varchar(10) DEFAULT NULL,  EntityId int(11) DEFAULT NULL,  PersonalOrBusiness tinyint(3) unsigned DEFAULT NULL,   WaitingDuration smallint(6) DEFAULT '0',  EffectiveCallDuration int(11) DEFAULT NULL,  ComType tinyint(3) unsigned DEFAULT NULL,  CostInfo double DEFAULT NULL,  InitialDialledNumber varchar(30) DEFAULT NULL,  Carrier varchar(5) DEFAULT NULL,  UserToUserVolume smallint(6) DEFAULT '0',  StartDateTime datetime DEFAULT NULL,  Duration int(11) DEFAULT NULL,  RedirectedCallIndicator tinyint(3) unsigned DEFAULT NULL,  Subaddress varchar(20) DEFAULT NULL,  HighLevelComp tinyint(3) unsigned DEFAULT NULL,  CostType tinyint(3) unsigned DEFAULT NULL,  TrunkIdentity smallint(6) DEFAULT NULL,  SpecificChargeInfo char(7) DEFAULT NULL,  BearerCapability tinyint(3) unsigned DEFAULT NULL,  DataVolume int(11) DEFAULT NULL,  AdditionalEntityId int(11) DEFAULT NULL,  FirstCarrierCost double NOT NULL,  FirstCarrierCostT double DEFAULT NULL,  SecondCarrierCost double NOT NULL,  SecondCarrierCostT double DEFAULT NULL,  FacilityCost double NOT NULL,  FacilityCostT double DEFAULT NULL,  FacturedCost double DEFAULT NULL,  FacturedCostT double DEFAULT NULL,  SubscriptionCost double NOT NULL DEFAULT '0',  SubscriptionCostT double DEFAULT NULL,  FirstCarrierId int(11) DEFAULT NULL,  SecondCarrierId int(11) DEFAULT NULL,  FirstCarrierDirectionId int(11) DEFAULT NULL,  SecondCarrierDirectionId int(11) DEFAULT NULL,  FirstCarrierCcnId int(11) DEFAULT NULL,  SecondCarrierCcnId int(11) DEFAULT NULL,  ActingExtensionNumber varchar(30) DEFAULT NULL,  TransitTrunkGroupIdentity varchar(5) DEFAULT NULL,  NodeTimeOffset smallint(6) DEFAULT NULL,  ExternFacilities binary(5) DEFAULT NULL,  InternFacilities binary(5) DEFAULT NULL,  TicketOrigin tinyint(3) unsigned DEFAULT '0',  TimeDlt int(11) DEFAULT NULL,  PRIMARY KEY (TicketId),  UNIQUE KEY IX_Ticket (TicketId),  KEY IX2_Ticket (EntityId),  KEY IX3_Ticket (InitialStartDateTime),  KEY IX4_Ticket (StartDateTime))";

  CHECK_STMT_RC(Stmt, SQLExecDirectW(Stmt, L"SET SQL_MODE='ANSI_QUOTES'", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLExecDirectW(Stmt, L"DROP TABLE IF EXISTS cono1", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLExecDirectW(Stmt, create_table, SQL_NTS));

  ret= SQLColumnsW(Stmt, NULL, 0, NULL, 0, L"cono1", SQL_NTS, NULL, 0);
  if (!SQL_SUCCEEDED(ret))
    return FAIL;

  SQLRowCount(Stmt, &rowCount);
  diag("row_count: %u", rowCount);

  CHECK_STMT_RC(Stmt, SQLExecDirectW(Stmt, L"SET SQL_MODE=''", SQL_NTS));

  ret= SQLColumnsW(Stmt, NULL, 0, NULL, 0, L"cono1", SQL_NTS, NULL, 0);
  if (!SQL_SUCCEEDED(ret))
    return FAIL;

  SQLRowCount(Stmt, &rowCount);
  diag("row_count: %u", rowCount);

  return OK;
}

ODBC_TEST(test_count)
{
  SQLWCHAR columnname[64];
  SQLSMALLINT columnlength, datatype, digits, nullable;
  SQLLEN columnsize;
  SQLRETURN rc;
 
  rc= SQLExecDirectW(Stmt, L"DROP TABLE IF EXISTS test_count", SQL_NTS);
  rc= SQLExecDirectW(Stmt, L"CREATE TABLE test_count (a int)", SQL_NTS);
  rc= SQLExecDirectW(Stmt, L"INSERT INTO test_count VALUES (1),(2)", SQL_NTS);
  rc= SQLExecDirectW(Stmt, L"SELECT count(*) RELEATED FROM test_count", SQL_NTS);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1L));  
  SQLDescribeColW(Stmt,1, columnname, 64, &columnlength, &datatype, &columnsize, &digits, &nullable);
  SQLBindCol(Stmt, 1, SQL_INTEGER, &columnsize, sizeof(SQLINTEGER), NULL);
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1L));

  wprintf(L"%s: %d", columnname, columnsize);

  return (OK);
}

ODBC_TEST(sqlconnect)
{
  HDBC hdbc1;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1,
                            WC("test"), SQL_NTS,
                            WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlprepare)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLINTEGER data;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1,
                            WC(my_dsn), SQL_NTS,
                            WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(Connection, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLPrepareW(hstmt1,
                             L"SELECT '\x30a1' FROM DUAL WHERE 1 = ?",
                              SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                   SQL_INTEGER, 0, 0, &data, 0, NULL));
  data= 0;
  CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  data= 1;
  CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 1, MAX_ROW_DATA_LEN+1), L"\x30a1", 1);

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  /*
    Some driver managers (like iODBC) will always do the character conversion
    themselves.
  */
  if (!using_dm(Connection))
  {
    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    /* Now try ANSI SQLPrepare. */
    CHECK_STMT_RC(hstmt1, SQLPrepare(hstmt1,
      (SQLCHAR *)"SELECT 0xc3a3 FROM DUAL WHERE 1 = ?",
                               SQL_NTS));

    data= 0;
    CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));

    FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

    data= 1;
    CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 1, MAX_ROW_DATA_LEN+1), L"\x00e3", 1);

    FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


/**
  Test calling SQLPrepareW() on an ANSI connection. Only tested when
  we're not using a driver manager, otherwise the driver manager does
  Unicode to ANSI translations.
*/
ODBC_TEST(sqlprepare_ansi)
{
  SQLINTEGER data;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];

 if (using_dm(Connection))
  skip("not relevant when using driver manager");

  /* Now try SQLPrepareW with an ANSI connection. */
  CHECK_STMT_RC(Stmt, SQLPrepareW(Stmt,
                             W(L"SELECT '\x00e3' FROM DUAL WHERE 1 = ?"),
                             SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &data, 0, NULL));
  data= 0;
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  FAIL_IF(SQLFetch(Stmt)!= SQL_NO_DATA_FOUND, "eof expected");

  data= 1;
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_WSTR(my_fetch_wstr(Stmt, wbuff, 1, MAX_ROW_DATA_LEN+1), L"\x00e3", 1);

  FAIL_IF(SQLFetch(Stmt)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Now try SQLPrepareW with a character that doesn't translate. */
  FAIL_IF(SQLPrepareW(Stmt,  W(L"SELECT '\x30a1' FROM DUAL WHERE 1 = ?"), SQL_NTS) != SQL_ERROR, "Error expected");
              
  IS(check_sqlstate(Stmt, "22018") == OK);

  return OK;
}


ODBC_TEST(sqlchar)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLCHAR data[]= "S\xc3\xA3o Paolo", buff[30];
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];
  wchar_t wcdata[]= L"S\x00e3o Paolo";

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1,
                            WC(my_dsn), SQL_NTS,
                            WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));
  CHECK_DBC_RC(Connection, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLPrepareW(hstmt1, W(L"SELECT ? FROM DUAL"), SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                   SQL_WVARCHAR, 0, 0, data, sizeof(data),
                                   NULL));
  CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  IS_STR(my_fetch_str(hstmt1, buff, 1), data, sizeof(data));

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  /* Do it again so we can try as SQLWCHAR */
  CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 1, MAX_ROW_DATA_LEN+1), wcdata, 9);

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqldriverconnect)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  wchar_t conn_in[512];
  wchar_t dummy[256];
  SQLWCHAR conn_out[1024];
  SQLSMALLINT conn_out_len;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));

  *conn_in= L'\0';
  wcscat(conn_in, L"DRIVER=");
  mbstowcs(dummy, (char *)my_drivername, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";UID=");
  mbstowcs(dummy, (char *)my_uid, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";PWD=");
  mbstowcs(dummy, (char *)my_pwd, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";DATABASE=");
  mbstowcs(dummy, (char *)my_schema, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";SERVER=");
  mbstowcs(dummy, (char *)"localhost", sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
 
  CHECK_DBC_RC(hdbc1, SQLDriverConnectW(hdbc1, NULL, WL(conn_in, wcslen(conn_in)),
                                  wcslen(conn_in), conn_out, sizeof(conn_out),
                                  &conn_out_len, SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(Connection, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1, L"SELECT 1234", SQL_NTS));
  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  is_num(my_fetch_int(hstmt1, 1), 1234);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlnativesql)
{
  HDBC hdbc1;
  SQLWCHAR    out[128];
  wchar_t in[]= L"SELECT * FROM venu";
  SQLINTEGER len;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1,
                            WC(my_dsn), SQL_NTS,
                            WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLNativeSqlW(hdbc1, in, SQL_NTS, out, sizeof(out), &len));
  is_num(len, sizeof(in) / sizeof(wchar_t) - 1);
  IS_WSTR(sqlwchar_to_wchar_t(out), in, sizeof(in) / sizeof(wchar_t) - 1);

  CHECK_DBC_RC(hdbc1, SQLNativeSqlW(hdbc1, in, SQL_NTS, out, 8, &len));
  is_num(len, sizeof(in) / sizeof(wchar_t) - 1);
  IS_WSTR(sqlwchar_to_wchar_t(out), in, 7);
  IS(out[7] == 0);

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlsetcursorname)
{
  HDBC hdbc1;
  HSTMT hstmt1, hstmt_pos;
  SQLLEN  nRowCount;
  SQLCHAR data[10];

  diag("where current of not supported");
  return SKIP;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_demo_cursor");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_demo_cursor (id INT, name VARCHAR(20))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_demo_cursor VALUES (0,'MySQL0'),(1,'MySQL1'),"
         "(2,'MySQL2'),(3,'MySQL3'),(4,'MySQL4')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLSetStmtAttrW(hstmt1, SQL_ATTR_CURSOR_TYPE,
                                  (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0));

  CHECK_STMT_RC(hstmt1, SQLSetCursorNameW(hstmt1, L"a\x00e3b", SQL_NTS));

  /* Open the resultset of table 'my_demo_cursor' */
  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM my_demo_cursor");

  /* goto the last row */
  CHECK_STMT_RC(hstmt1, SQLFetchScroll(hstmt1, SQL_FETCH_LAST, 1L));

  /* create new statement handle */
  CHECK_DBC_RC(hdbc1, SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hstmt_pos));

  /* now update the name field to 'updated' using positioned cursor */
  CHECK_STMT_RC(hstmt_pos,
          SQLExecDirectW(hstmt_pos,
                         L"UPDATE my_demo_cursor SET name='updated' "
                           L"WHERE CURRENT OF a\x00e3b", SQL_NTS));

  CHECK_STMT_RC(hstmt_pos, SQLRowCount(hstmt_pos, &nRowCount));
  is_num(nRowCount, 1);

  CHECK_STMT_RC(hstmt_pos, SQLFreeStmt(hstmt_pos, SQL_CLOSE));
  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  /* Now delete 2nd row */
  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM my_demo_cursor");

  /* goto the second row */
  CHECK_STMT_RC(hstmt1, SQLFetchScroll(hstmt1, SQL_FETCH_ABSOLUTE, 2L));

  /* now delete the current row */
  CHECK_STMT_RC(hstmt_pos,
          SQLExecDirectW(hstmt_pos,
                           L"DELETE FROM my_demo_cursor "
                           L"WHERE CURRENT OF a\x00e3b", SQL_NTS));

  CHECK_STMT_RC(hstmt_pos, SQLRowCount(hstmt_pos, &nRowCount));
  is_num(nRowCount, 1);

  /* free the statement cursor */
  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  /* Free the statement 'hstmt_pos' */
  CHECK_STMT_RC(hstmt_pos, SQLFreeHandle(SQL_HANDLE_STMT, hstmt_pos));

  /* Now fetch and verify the data */
  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM my_demo_cursor");

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  is_num(my_fetch_int(hstmt1, 1), 0);
  IS_STR(my_fetch_str(hstmt1, data, 2), "MySQL0", 6);

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  is_num(my_fetch_int(hstmt1, 1), 2);
  IS_STR(my_fetch_str(hstmt1, data, 2), "MySQL2", 6);

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  is_num(my_fetch_int(hstmt1, 1), 3);
  IS_STR(my_fetch_str(hstmt1, data, 2), "MySQL3", 6);

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  is_num(my_fetch_int(hstmt1, 1), 4);
  IS_STR(my_fetch_str(hstmt1, data, 2), "updated", 7);

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_demo_cursor");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlgetcursorname)
{
  SQLRETURN rc;
  HDBC hdbc1;
  SQLHSTMT hstmt1,hstmt2,hstmt3;
  SQLWCHAR curname[50];
  SQLSMALLINT nlen;


  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));
  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt2));
  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt3));

  rc= SQLGetCursorNameW(hstmt1, curname, 50, &nlen);
  if (SQL_SUCCEEDED(rc))
  {
    is_num(nlen, 8);
    IS_WSTR(sqlwchar_to_wchar_t(curname), L"SQL_CUR0", 8);

    CHECK_STMT_RC(hstmt3,  SQLGetCursorNameW(hstmt3, curname, 50, &nlen));

    FAIL_IF(SQLGetCursorNameW(hstmt1, curname, 4, &nlen) !=
                SQL_SUCCESS_WITH_INFO, "SUCCESS_WITH_INFO expected");
    is_num(nlen, 8);
    IS_WSTR(sqlwchar_to_wchar_t(curname), L"SQL", 4);

    FAIL_IF(SQLGetCursorNameW(hstmt1, curname, 0, &nlen) !=
                SQL_SUCCESS_WITH_INFO, "SUCCESS_WITH_INFO expected");
    //rc = SQLGetCursorNameW(hstmt1, curname, 0, &nlen);
    //mystmt_err(hstmt1,rc == SQL_SUCCESS_WITH_INFO, rc);
    is_num(nlen, 8);

    FAIL_IF(SQLGetCursorNameW(hstmt1, curname, 8, &nlen) !=
                SQL_SUCCESS_WITH_INFO, "SUCCESS_WITH_INFO expected");
    is_num(nlen, 8);
    IS_WSTR(sqlwchar_to_wchar_t(curname), L"SQL_CUR", 8);

    CHECK_STMT_RC(hstmt1, SQLGetCursorNameW(hstmt1, curname, 9, &nlen));
    is_num(nlen, 8);
    IS_WSTR(sqlwchar_to_wchar_t(curname), L"SQL_CUR0", 8);
  }

  CHECK_STMT_RC(hstmt1,  SQLSetCursorNameW(hstmt1, L"venucur123", 7));

  CHECK_STMT_RC(hstmt1,  SQLGetCursorNameW(hstmt1, curname, 8, &nlen));
  is_num(nlen, 7);
  IS_WSTR(sqlwchar_to_wchar_t(curname), L"venucur", 8);

  CHECK_STMT_RC(hstmt3, SQLFreeStmt(hstmt3, SQL_DROP));
  CHECK_STMT_RC(hstmt2, SQLFreeStmt(hstmt2, SQL_DROP));
  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlcolattribute)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];
  SQLSMALLINT len;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_colattrib");
  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1,
                                 L"CREATE TABLE t_colattrib (a\x00E3g INT)",
                                 SQL_NTS));

  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_colattrib AS b");

  CHECK_STMT_RC(hstmt1, SQLColAttributeW(hstmt1, 1, SQL_DESC_NAME,
                                   wbuff, sizeof(wbuff), &len, NULL));
  is_num(len, 3 * sizeof(SQLWCHAR));
  IS_WSTR(sqlwchar_to_wchar_t(wbuff), L"a\x00e3g", 4);

  FAIL_IF(SQLColAttributeW(hstmt1, 1, SQL_DESC_BASE_TABLE_NAME,
                                       wbuff, 5 * sizeof(SQLWCHAR), &len, NULL) != SQL_SUCCESS_WITH_INFO, "Expected success_with_info");
  is_num(len, 11 * sizeof(SQLWCHAR));
  IS_WSTR(sqlwchar_to_wchar_t(wbuff), L"t_co", 5);

  CHECK_STMT_RC(hstmt1, SQLColAttributeW(hstmt1, 1, SQL_DESC_TYPE_NAME,
                                   wbuff, sizeof(wbuff), &len, NULL));
  is_num(len, 7 * sizeof(SQLWCHAR));
  IS_WSTR(sqlwchar_to_wchar_t(wbuff), L"integer", 8);

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_colattrib");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqldescribecol)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];
  SQLSMALLINT len;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_desc");
  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1,
                                 W(L"CREATE TABLE t_desc (a\x00e3g INT)"),
                                 SQL_NTS));

  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_desc");

  CHECK_STMT_RC(hstmt1, SQLDescribeColW(hstmt1, 1, wbuff,
                                  sizeof(wbuff) / sizeof(wbuff[0]), &len,
                                  NULL, NULL, NULL, NULL));
  is_num(len, 3);
  IS_WSTR(sqlwchar_to_wchar_t(wbuff), L"a\x00e3g", 4);

  FAIL_IF(SQLDescribeColW(hstmt1, 1, wbuff, 3, &len,
                                      NULL, NULL, NULL, NULL) != SQL_SUCCESS_WITH_INFO, "Expected SQL_SUCCESS_WITH_INFO");
  is_num(len, 3);
  IS_WSTR(sqlwchar_to_wchar_t(wbuff), L"a\x00e3", 3);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_desc");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlgetconnectattr)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];
  SQLINTEGER len;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLGetConnectAttrW(hdbc1, SQL_ATTR_CURRENT_CATALOG, wbuff,
                                     sizeof(wbuff), &len));
  is_num(len, 9 * sizeof(SQLWCHAR));
  IS_WSTR(sqlwchar_to_wchar_t(wbuff), L"odbc_test", 5);

  FAIL_IF(SQLGetConnectAttrW(hdbc1, SQL_ATTR_CURRENT_CATALOG,
                                         wbuff, 3 * sizeof(SQLWCHAR), &len) != SQL_SUCCESS_WITH_INFO, "expected SUCCESS_WITH_INFO");
  is_num(len, 9 * sizeof(SQLWCHAR));
  IS_WSTR(sqlwchar_to_wchar_t(wbuff), L"od", 3);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlgetdiagrec)
{
  SQLWCHAR   sqlstate[6]= {0};
  SQLWCHAR   message[255]= {0};
  SQLINTEGER native_err= 0;
  SQLSMALLINT msglen= 0;
  HDBC hdbc1;
  HSTMT hstmt1;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));


  FAIL_IF(SQLExecDirect(hstmt1, "DROP TABLE t_odbc3_non_existent_table", SQL_NTS) != SQL_ERROR, "error expected");

#if UNIXODBC_BUG_FIXED
  /*
   This should report no data found, but unixODBC doesn't even pass this
   down to the driver.
  */
  FAIL_IF(SQLGetDiagRecW(SQL_HANDLE_STMT, hstmt1, 2, sqlstate,
                                     &native_err, message, 0, &msglen),
              SQL_NO_DATA_FOUND);
#endif

  CHECK_STMT_RC(Stmt, SQLGetDiagRecW(SQL_HANDLE_STMT, hstmt1, 1, sqlstate,
                                &native_err, message, 255, &msglen));
  diag("%s %s", sqlstate, message);

  /* it has to comply to the bugfix for bug 14285620  */
  FAIL_IF(SQLGetDiagRecW(SQL_HANDLE_STMT, hstmt1, 1, sqlstate,
                                     &native_err, message, 0, &msglen) != SQL_SUCCESS, "success expected");

  FAIL_IF(SQLGetDiagRecW(SQL_HANDLE_STMT, hstmt1, 1, sqlstate,
                                     &native_err, message, 10, &msglen) != SQL_SUCCESS_WITH_INFO, "success with info expected");

  FAIL_IF(SQLGetDiagRecW(SQL_HANDLE_STMT, hstmt1, 1, sqlstate,
                                     &native_err, message, -1, &msglen) != SQL_ERROR, "Error expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlgetdiagfield)
{
  SQLWCHAR   message[255]= {0};
  SQLSMALLINT len;
  SQLINTEGER data;
  HDBC hdbc1;
  HSTMT hstmt1;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));


  FAIL_IF(SQLExecDirect(hstmt1, "DROP TABLE t_odbc3_non_existent_table", SQL_NTS) != SQL_ERROR, "Error expected");

  CHECK_STMT_RC(Stmt, SQLGetDiagFieldW(SQL_HANDLE_STMT, hstmt1, 0,
                                  SQL_DIAG_NUMBER, &data, 0, NULL));
  is_num(data, 1);

  CHECK_STMT_RC(Stmt, SQLGetDiagFieldW(SQL_HANDLE_STMT, hstmt1, 1,
                                  SQL_DIAG_CLASS_ORIGIN, message,
                                  sizeof(message), &len));
  is_num(len, 8 * sizeof(SQLWCHAR));
  IS_WSTR(sqlwchar_to_wchar_t(message), L"ISO 9075", 9);

  FAIL_IF(SQLGetDiagFieldW(SQL_HANDLE_STMT, hstmt1, 1,
                                      SQL_DIAG_SQLSTATE, message,
                                      4 * sizeof(SQLWCHAR), &len) != SQL_SUCCESS_WITH_INFO, "SUCCESS_W_I expected");
  is_num(len, 5 * sizeof(SQLWCHAR));
  IS_WSTR(sqlwchar_to_wchar_t(message), L"42S", 4);

  CHECK_STMT_RC(Stmt, SQLGetDiagFieldW(SQL_HANDLE_STMT, hstmt1, 1,
                                  SQL_DIAG_SUBCLASS_ORIGIN, message,
                                  sizeof(message), &len));
  is_num(len, 8 * sizeof(SQLWCHAR));
  IS_WSTR(sqlwchar_to_wchar_t(message), L"ODBC 3.0", 9);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlcolumns)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_columns");
  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1,
                                 W(L"CREATE TABLE t_columns (a\x00e3g INT)"),
                                 SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLColumnsW(hstmt1, L"odbc_test", 9, NULL, 0,
                              W(L"t_columns"), SQL_NTS,
                              W(L"a\x00e3g"), SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 4, MAX_ROW_DATA_LEN+1), L"a\x00e3g", 4);

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_columns");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqltables)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1,
                                 W(L"DROP TABLE IF EXISTS t_a\x00e3g"),
                                 SQL_NTS));
  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1,
                                 W(L"CREATE TABLE t_a\x00e3g (a INT)"),
                                 SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLTablesW(hstmt1, NULL, 0, NULL, 0,
                             W(L"t_a\x00e3g"), SQL_NTS,
                             NULL, 0));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 3, MAX_ROW_DATA_LEN+1), L"t_a\x00e3g", 6);

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1,
                                 W(L"DROP TABLE IF EXISTS t_a\x00e3g"),
                                 SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlspecialcolumns)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_spec");
  CHECK_STMT_RC(hstmt1,
          SQLExecDirectW(hstmt1,
                         W(L"CREATE TABLE t_spec (a\x00e3g INT PRIMARY KEY)"),
                         SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLSpecialColumnsW(hstmt1, SQL_BEST_ROWID, NULL, 0, NULL, 0,
                             W(L"t_spec"), SQL_NTS, SQL_SCOPE_SESSION,
                             SQL_NULLABLE));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 2, MAX_ROW_DATA_LEN+1), L"a\x00e3g", 4);

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_spec");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlforeignkeys)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];

  /** @todo re-enable this test when I_S based SQLForeignKeys is done. */


  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1,
          SQLExecDirectW(hstmt1,
                         W(L"DROP TABLE IF EXISTS t_fk_\x00e5, t_fk_\x00e3"),
                         SQL_NTS));
  CHECK_STMT_RC(hstmt1,
          SQLExecDirectW(hstmt1,
                         W(L"CREATE TABLE t_fk_\x00e3 (a INT PRIMARY KEY) "
                           L"ENGINE=InnoDB"), SQL_NTS));
  CHECK_STMT_RC(hstmt1,
          SQLExecDirectW(hstmt1,
                         W(L"CREATE TABLE t_fk_\x00e5 (b INT, parent_id INT,"
                         L"                       FOREIGN KEY (parent_id)"
                         L"                        REFERENCES"
                         L"                          t_fk_\x00e3(a)"
                         L"                        ON DELETE SET NULL)"
                         L" ENGINE=InnoDB"), SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLForeignKeysW(hstmt1, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                  NULL, 0, W(L"t_fk_\x00e5"), SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 3, MAX_ROW_DATA_LEN+1), L"t_fk_\x00e3", 7);
  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 4, MAX_ROW_DATA_LEN+1), L"a", 2);
  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 7, MAX_ROW_DATA_LEN+1), L"t_fk_\x00e5", 7);
  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 8, MAX_ROW_DATA_LEN+1), L"parent_id", 10);

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1,
          SQLExecDirectW(hstmt1,
                         W(L"DROP TABLE IF EXISTS t_fk_\x00e5, t_fk_\x00e3"),
                         SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlprimarykeys)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1, W(L"DROP TABLE IF EXISTS t_a\x00e3g"),
                                 SQL_NTS));
  CHECK_STMT_RC(hstmt1,
          SQLExecDirectW(hstmt1,
                         W(L"CREATE TABLE t_a\x00e3g (a INT PRIMARY KEY)"),
                         SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLPrimaryKeysW(hstmt1, NULL, 0, NULL, 0,
                                  W(L"t_a\x00e3g"), SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 3, MAX_ROW_DATA_LEN+1), L"t_a\x00e3g", 6);

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1, W(L"DROP TABLE IF EXISTS t_a\x00e3g"),
                                 SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(sqlstatistics)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1,
                                 W(L"DROP TABLE IF EXISTS t_a\x00e3g"),
                                 SQL_NTS));
  CHECK_STMT_RC(hstmt1,
          SQLExecDirectW(hstmt1,
                         W(L"CREATE TABLE t_a\x00e3g (a INT PRIMARY KEY)"),
                         SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLStatisticsW(hstmt1, NULL, 0, NULL, 0,
                                 W(L"t_a\x00e3g"), SQL_NTS,
                                 SQL_INDEX_UNIQUE, SQL_QUICK));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 3, MAX_ROW_DATA_LEN+1), L"t_a\x00e3g", 6);

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1,
                                 W(L"DROP TABLE IF EXISTS t_a\x00e3g"),
                                 SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


/**
 Bug #32161: SQLDescribeColW returns UTF-8 column as SQL_VARCHAR instead of
 SQL_WVARCHAR
*/
ODBC_TEST(t_bug32161)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLWCHAR wbuff[MAX_ROW_DATA_LEN+1];
  SQLSMALLINT nlen;
  SQLSMALLINT ctype;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, WC(my_dsn), SQL_NTS, WC(my_uid), SQL_NTS,
                            WC(my_pwd), SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_bug32161");
  OK_SIMPLE_STMT(hstmt1, "CREATE TABLE t_bug32161 ("
                 "col1 varchar(32),"
                 "col2 char(32),"
                 "col3 tinytext,"
                 "col4 mediumtext,"
                 "col5 text,"
                 "col6 longtext"
                 ") DEFAULT CHARSET=utf8");

  /* Greek word PSARO - FISH */
  CHECK_STMT_RC(hstmt1,
          SQLExecDirectW(hstmt1,
                         W(L"INSERT INTO t_bug32161 VALUES ("
                         L"\"\x03A8\x0391\x03A1\x039F 1\","
                         L"\"\x03A8\x0391\x03A1\x039F 2\","
                         L"\"\x03A8\x0391\x03A1\x039F 3\","
                         L"\"\x03A8\x0391\x03A1\x039F 4\","
                         L"\"\x03A8\x0391\x03A1\x039F 5\","
                         L"\"\x03A8\x0391\x03A1\x039F 6\")"),
                         SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_bug32161");
  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 1, MAX_ROW_DATA_LEN+1), L"\x03A8\x0391\x03A1\x039F 1", 4);
  CHECK_STMT_RC(hstmt1, SQLDescribeColW(hstmt1, 1, wbuff, MAX_ROW_DATA_LEN, &nlen,
                                  &ctype, NULL, NULL, NULL));
  is_num(ctype, SQL_WVARCHAR);

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 2, MAX_ROW_DATA_LEN+1), L"\x03A8\x0391\x03A1\x039F 2", 4);
  CHECK_STMT_RC(hstmt1, SQLDescribeColW(hstmt1, 2, wbuff, MAX_ROW_DATA_LEN, &nlen,
                                  &ctype, NULL, NULL, NULL));
  is_num(ctype, SQL_WCHAR);

  /* All further calls of SQLDescribeColW should return SQL_WLONGVARCHAR */
  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 3, MAX_ROW_DATA_LEN+1), L"\x03A8\x0391\x03A1\x039F 3", 4);
  CHECK_STMT_RC(hstmt1, SQLDescribeColW(hstmt1, 3, wbuff, MAX_ROW_DATA_LEN, &nlen,
                                  &ctype, NULL, NULL, NULL));
  is_num(ctype, SQL_WLONGVARCHAR);

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 4, MAX_ROW_DATA_LEN+1), L"\x03A8\x0391\x03A1\x039F 4", 4);
  CHECK_STMT_RC(hstmt1, SQLDescribeColW(hstmt1, 4, wbuff, MAX_ROW_DATA_LEN, &nlen,
                                  &ctype, NULL, NULL, NULL));
  is_num(ctype, SQL_WLONGVARCHAR);

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 5, MAX_ROW_DATA_LEN+1), L"\x03A8\x0391\x03A1\x039F 5", 4);
  CHECK_STMT_RC(hstmt1, SQLDescribeColW(hstmt1, 5, wbuff, MAX_ROW_DATA_LEN, &nlen,
                                  &ctype, NULL, NULL, NULL));
  is_num(ctype, SQL_WLONGVARCHAR);

  IS_WSTR(my_fetch_wstr(hstmt1, wbuff, 6, MAX_ROW_DATA_LEN+1), L"\x03A8\x0391\x03A1\x039F 6", 4);
  CHECK_STMT_RC(hstmt1, SQLDescribeColW(hstmt1, 6, wbuff, MAX_ROW_DATA_LEN, &nlen,
                                  &ctype, NULL, NULL, NULL));
  is_num(ctype, SQL_WLONGVARCHAR);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_bug32161");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


/*
  Bug#34672 - Unable to insert surrogate pairs into or fetch surrogate pairs
              from unicode column
*/
ODBC_TEST(t_bug34672)
{
  SQLWCHAR chars[3];
  SQLINTEGER inchars, i;
  SQLWCHAR result[3];
  SQLLEN reslen;

  if (sizeof(SQLWCHAR) == 2)
  {
    chars[0]= 0xd802;
    chars[1]= 0xdc60;
    chars[2]= 0;
    inchars= 2;
  }
  else
  {
    chars[0]= (SQLWCHAR) 0x10860;
    chars[1]= 0;
    inchars= 1;
  }

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR,
                                  SQL_WCHAR, 0, 0, chars,
                                  inchars * sizeof(SQLWCHAR), NULL));

  if (0) //(mysql_min_version(Connection, "6.0.4", 5))
  {
    CHECK_STMT_RC(Stmt, SQLExecDirectW(Stmt, (SQLWCHAR *) L"select ? FROM DUAL", SQL_NTS));
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_WCHAR, result,
                              sizeof(result), &reslen));
    is_num(result[2], 0);
    for (i= 0; i < inchars; ++i)
      is_num(result[i], chars[i]);
    is_num(reslen, inchars * sizeof(SQLWCHAR));
  }
   return OK;
}

/*
Bug#28168 odbc, non 7-bit password, connection failed
*/
ODBC_TEST(t_bug28168)
{
  SQLHANDLE hdbc1, hdbc2;
  SQLHANDLE hstmt1;

  wchar_t conn_in[512]= {0}, work_conn_in[512]= {0};
  wchar_t dummy[256]= {0};
  wchar_t *wstr;
  SQLWCHAR errmsgtxt[256]= {0}, sqlstate[6]= {0};
  SQLWCHAR *grantQuery= W(L"GRANT ALL ON t_bug28168 to "
    L"'\x03A8\x0391\x03A1\x039F uid'@"
    L"localhost identified by "
    L"'\x03A8\x0391\x03A1\x039F pwd'");
  SQLWCHAR *grantQuery2= W(L"GRANT ALL ON t_bug28168 to "
    L"'\x03A8\x0391\x03A1\x039F uid'@"
    L"'%' identified by "
    L"'\x03A8\x0391\x03A1\x039F pwd'");
  SQLSMALLINT errmsglen;
  SQLINTEGER native_error= 0;

  /* Create tables to give permissions */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug28168");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug28168 (x int)");

  *work_conn_in= L'\0';
  wcscat(work_conn_in, L"DSN=");
  mbstowcs(dummy, (char *)my_dsn, sizeof(dummy)/sizeof(wchar_t));
  wcscat(work_conn_in, dummy);
  wcscat(work_conn_in, L";UID=");
  mbstowcs(dummy, (char *)my_uid, sizeof(dummy)/sizeof(wchar_t));
  wcscat(work_conn_in, dummy);
  wcscat(work_conn_in, L";PWD=");
  mbstowcs(dummy, (char *)my_pwd, sizeof(dummy)/sizeof(wchar_t));
  wcscat(work_conn_in, dummy);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  /* Connect using UTF8 as transport to avoid server bug with user names */
  CHECK_DBC_RC(hdbc1, SQLDriverConnectW(hdbc1, NULL, WL(work_conn_in, 
                                  wcslen(work_conn_in)),
                                  wcslen(work_conn_in), NULL, 0,
                                  0, SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  /* 
    Grant for localhost and for all other hosts if the test server
    runs remotely
  */
  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1, grantQuery, SQL_NTS));
  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1, grantQuery2, SQL_NTS));
  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1, L"FLUSH PRIVILEGES", SQL_NTS));

  *conn_in= L'\0';
  wcscat(conn_in, L"DRIVER=");
  mbstowcs(dummy, (char *)my_drivername, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";UID=");
  wcscat(conn_in, L"{\x03A8\x0391\x03A1\x039F uid}");
  wcscat(conn_in, L";PWD=");
  wcscat(conn_in, L"{\x03A8\x0391\x03A1\x039F pwd}");
  wcscat(conn_in, L";DATABASE=");
  mbstowcs(dummy, (char *)my_schema, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";SERVER=");
  mbstowcs(dummy, (char *)"localhost", sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc2));
  CHECK_DBC_RC(hdbc2, SQLDriverConnectW(hdbc2, NULL, W(conn_in), SQL_NTS, NULL, 0,
    NULL, SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(hdbc2, SQLDisconnect(hdbc2));

  /* we change the password in the connection string to test the error msg */
  wstr= wcsstr(conn_in, L" pwd}") - 4;
  *wstr++= 'x';

  FAIL_IF(SQLDriverConnectW(hdbc2, NULL, W(conn_in), SQL_NTS,
                  (SQLWCHAR*)NULL, 0, NULL, SQL_DRIVER_NOPROMPT) != SQL_ERROR, "error expected");

  CHECK_DBC_RC(hdbc2, SQLGetDiagRecW(SQL_HANDLE_DBC, hdbc2, 1,
                               sqlstate, &native_error, errmsgtxt,
                               256 * sizeof(SQLWCHAR), &errmsglen));
  CHECK_DBC_RC(hdbc2, SQLFreeHandle(SQL_HANDLE_DBC, hdbc2));

  /* 
    The returned error message has to contain the substring
    with the username
  */
   wstr= sqlwchar_to_wchar_t(errmsgtxt);
   IS(wcsstr(wstr,  
             L"Access denied for user '\x03A8\x0391\x03A1\x039F uid'@") != NULL);
  CHECK_STMT_RC(hstmt1,SQLExecDirectW(hstmt1,
    W(L"DROP USER "
    L"'\x03A8\x0391\x03A1\x039F uid'@localhost"), SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug28168");

  return OK;
}


/*
  Bug#14363601 MY_ISSPACE CALLS CAUSE ODBC DRIVER CRASHES
*/
ODBC_TEST(t_bug14363601)
{
  HENV Env1;
  HDBC hdbc1;
  HSTMT hstmt1;
  int i;
  wchar_t conn_in[512];
  wchar_t dummy[256];

  SQLWCHAR conn_out[1024];
  SQLSMALLINT conn_out_len;
  SQLLEN strlen_or_ind= 10;
  SQLINTEGER col_id= 1234;
  SQLWCHAR *col_vc= W(L"abcdefg\x30a1"), col_vc_res[30];
  double col_dc= 12345.678, col_dc_res= 0;
  unsigned char col_bc[10]= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, col_bc_res[10];

  CHECK_ENV_RC(Env1, SQLAllocEnv(&Env1));
  CHECK_ENV_RC(Env1, SQLAllocConnect(Env1, &hdbc1));

  *conn_in= L'\0';
  wcscat(conn_in, L"DRIVER=");
  mbstowcs(dummy, (char *)my_drivername, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";UID=");
  mbstowcs(dummy, (char *)my_uid, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";PWD=");
  mbstowcs(dummy, (char *)my_pwd, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";DATABASE=");
  mbstowcs(dummy, (char *)my_schema, sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
  wcscat(conn_in, L";SERVER=");
  mbstowcs(dummy, (char *)"localhost", sizeof(dummy)/sizeof(wchar_t));
  wcscat(conn_in, dummy);
 
  wcscat(conn_in, L";CHARSET=utf8");

  CHECK_DBC_RC(hdbc1, SQLDriverConnectW(hdbc1, NULL, WL(conn_in, wcslen(conn_in)),
                                  wcslen(conn_in), conn_out, sizeof(conn_out),
                                  &conn_out_len, SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS bug14363601");
  OK_SIMPLE_STMT(hstmt1, "CREATE TABLE bug14363601("
                 "id INT, vc VARCHAR(32),"
                 "dc DOUBLE, bc BLOB)CHARSET=UTF8");

  CHECK_STMT_RC(hstmt1, SQLPrepareW(hstmt1, 
		    W(L"INSERT INTO bug14363601 (id, vc, dc, bc) "
                      L"VALUES (?, ?, ?, ?)"), SQL_NTS));

  /* Bind 1st INT param */
  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &col_id, 0, NULL));
  
  /* Bind 2nd VARCHAR param */
  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 2, SQL_PARAM_INPUT, SQL_C_WCHAR,
				   SQL_WCHAR, 10, 0, col_vc, 
                                  10*sizeof(SQLWCHAR), NULL));

  /* Bind 3rd DECIMAL param */
  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 3, SQL_PARAM_INPUT, SQL_C_DOUBLE,
                                  SQL_DOUBLE, 0, 0, &col_dc, sizeof(col_dc),
                                  NULL));

  /* Bind 4th BLOB param */
  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 4, SQL_PARAM_INPUT, SQL_C_BINARY,
                                  SQL_BINARY, (SQLULEN)sizeof(col_bc), 0, &col_bc, 
                                  sizeof(col_bc), &strlen_or_ind));

  CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1, W(L"SELECT * FROM bug14363601"), 
                                 SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  is_num(my_fetch_int(hstmt1, 1), col_id);

  CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 2, SQL_C_WCHAR, col_vc_res,
                             sizeof(col_vc_res), NULL));

  /* we want to compare SQLWCHAR instead of wchar_t */
  for (i= 0; i < 8; i++)
  {
    IS(col_vc[i] == col_vc_res[i]);
  }

  CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 3, SQL_C_DOUBLE, &col_dc_res, 
                             sizeof(col_dc_res), NULL));
  IS(col_dc == col_dc_res);

  CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 4, SQL_C_BINARY, &col_bc_res, 
                             sizeof(col_bc_res), NULL));

  /* check the binary buffer byte by byte */
  for (i= 0; i < sizeof(col_bc_res); i++)
  {
    is_num(col_bc[i], col_bc_res[i]);
  }

  FAIL_IF(SQLFetch(hstmt1)!= SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS bug14363601");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));
  CHECK_ENV_RC(Env1, SQLFreeEnv(Env1));
  
  /* OK if it has not crashed */
  return OK;
}

MA_ODBC_TESTS my_tests[]=
{
  {test_CONO1, "test_CONO1"},
  {test_count, "test_count"},
  {sqlconnect, "sqlconnect"},
  {sqlprepare, "sqlprepare"},
  {sqlprepare_ansi, "sqlprepare_ansi"},
  {sqlchar, "sqlchar"},
  {sqldriverconnect, "sqldriverconnect"},
  {sqlnativesql, "sqlnativesql"},
  {sqlsetcursorname, "sqlsetcursorname"},
  {sqlgetcursorname, "sqlgetcursorname"},
  {sqlcolattribute, "sqlcolattribute"},
  {sqldescribecol, "sqldescribecol"},
  {sqlgetconnectattr, "sqlgetconnectattr"},
  {sqlgetdiagrec, "sqlgetdiagrec"},
  {sqlgetdiagfield, "sqlgetdiagfield"},
  {sqlcolumns, "sqlcolumns"},
  {sqltables, "sqltables"},
  {sqlspecialcolumns, "sqlspecialcolumns"},
  {sqlforeignkeys, "sqlforeignkeys"},
  {sqlprimarykeys, "sqlprimarykeys"},
  {sqlstatistics, "sqlstatistics"},
  {t_bug32161, "t_bug32161"},
  {t_bug34672, "t_bug34672"},
  {t_bug28168, "t_bug28168"},
  {t_bug14363601, "t_bug14363601"},
  {NULL, NULL}
};


int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
