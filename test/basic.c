/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013, 2023 MariaDB Corporation AB

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

ODBC_TEST(test_CONO1)
{
  /* check SQLColumns with ANSI_QUOTES on and off */
  SQLLEN rowCount;
  SQLCHAR *create_table= (SQLCHAR *)"CREATE TABLE cono1 (InitialStartDateTime datetime NOT NULL,  TicketId int(11) NOT NULL AUTO_INCREMENT,  CallCount int(11) NOT NULL DEFAULT '1',  CalledNumber varchar(30) DEFAULT NULL,  CallingNumber varchar(30) DEFAULT NULL,  CallType tinyint(3) unsigned DEFAULT NULL,  ChargeUnits smallint(6) DEFAULT NULL,  NetworkAndTrunkNode int(11) DEFAULT NULL,  TrunkGroupIdentity varchar(10) DEFAULT NULL,  EntityId int(11) DEFAULT NULL,  PersonalOrBusiness tinyint(3) unsigned DEFAULT NULL,   WaitingDuration smallint(6) DEFAULT '0',  EffectiveCallDuration int(11) DEFAULT NULL,  ComType tinyint(3) unsigned DEFAULT NULL,  CostInfo double DEFAULT NULL,  InitialDialledNumber varchar(30) DEFAULT NULL,  Carrier varchar(5) DEFAULT NULL,  UserToUserVolume smallint(6) DEFAULT '0',  StartDateTime datetime DEFAULT NULL,  Duration int(11) DEFAULT NULL,  RedirectedCallIndicator tinyint(3) unsigned DEFAULT NULL,  Subaddress varchar(20) DEFAULT NULL,  HighLevelComp tinyint(3) unsigned DEFAULT NULL,  CostType tinyint(3) unsigned DEFAULT NULL,  TrunkIdentity smallint(6) DEFAULT NULL,  SpecificChargeInfo char(7) DEFAULT NULL,  BearerCapability tinyint(3) unsigned DEFAULT NULL,  DataVolume int(11) DEFAULT NULL,  AdditionalEntityId int(11) DEFAULT NULL,  FirstCarrierCost double NOT NULL,  FirstCarrierCostT double DEFAULT NULL,  SecondCarrierCost double NOT NULL,  SecondCarrierCostT double DEFAULT NULL,  FacilityCost double NOT NULL,  FacilityCostT double DEFAULT NULL,  FacturedCost double DEFAULT NULL,  FacturedCostT double DEFAULT NULL,  SubscriptionCost double NOT NULL DEFAULT '0',  SubscriptionCostT double DEFAULT NULL,  FirstCarrierId int(11) DEFAULT NULL,  SecondCarrierId int(11) DEFAULT NULL,  FirstCarrierDirectionId int(11) DEFAULT NULL,  SecondCarrierDirectionId int(11) DEFAULT NULL,  FirstCarrierCcnId int(11) DEFAULT NULL,  SecondCarrierCcnId int(11) DEFAULT NULL,  ActingExtensionNumber varchar(30) DEFAULT NULL,  TransitTrunkGroupIdentity varchar(5) DEFAULT NULL,  NodeTimeOffset smallint(6) DEFAULT NULL,  ExternFacilities binary(5) DEFAULT NULL,  InternFacilities binary(5) DEFAULT NULL,  TicketOrigin tinyint(3) unsigned DEFAULT '0',  TimeDlt int(11) DEFAULT NULL,  PRIMARY KEY (TicketId),  UNIQUE KEY IX_Ticket (TicketId),  KEY IX2_Ticket (EntityId),  KEY IX3_Ticket (InitialStartDateTime),  KEY IX4_Ticket (StartDateTime))";

  OK_SIMPLE_STMT(Stmt, "SET SQL_MODE='ANSI_QUOTES'");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS cono1");
  OK_SIMPLE_STMT(Stmt, create_table);
  

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0, (SQLCHAR*)"cono1", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowCount));
  diag("row_count: %u", rowCount);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SET SQL_MODE=''");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0, (SQLCHAR*)"cono1", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowCount));
  diag("row_count: %u", rowCount);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}

/* I don't understand what this test is about. About binding column when there is no result? that it does not cause an error? */
ODBC_TEST(test_CONO3)
{
  int i= 0;

  OK_SIMPLE_STMT(Stmt, "SET @a:=1");
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, (SQLPOINTER)(SQLLEN)&i, 0, NULL));

  return OK;
}


ODBC_TEST(simple_test)
{
  SQLSMALLINT value=3;
  SQLWCHAR Buffer[20];

  char buffer[128];

  OK_SIMPLE_STMT(Stmt, "SHOW VARIABLES LIKE 'character_set_client'");
  SQLFetch(Stmt);
  SQLGetData(Stmt, 2, SQL_CHAR, buffer, 20, NULL);
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "Eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMTW(Stmt, CW("DROP TABLE IF EXISTS smpltest"));
  OK_SIMPLE_STMTW(Stmt, CW("CREATE TABLE smpltest (a int, b varchar(25))"));
  OK_SIMPLE_STMTW(Stmt, CW("INSERT INTO smpltest VALUES (1, 'Row no 1')"));
  OK_SIMPLE_STMTW(Stmt, CW("INSERT INTO smpltest VALUES (2, 'Row no 2')"));
  
  CHECK_STMT_RC(Stmt, SQLPrepareW(Stmt, CW("SELECT a, b FROM smpltest"), SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));
  
  SQLFetch(Stmt);
  SQLGetData(Stmt, 1, SQL_C_USHORT, &value, sizeof(value), 0);
  SQLGetData(Stmt, 2, SQL_C_WCHAR, Buffer, sizeof(Buffer), 0);
  is_num(value, 1);

  IS_WSTR(Buffer, CW("Row no 1"), 9);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  SQLGetData(Stmt, 1, SQL_C_USHORT, &value, sizeof(value), 0);
  SQLGetData(Stmt, 2, SQL_C_WCHAR, Buffer,  sizeof(Buffer), 0);
  FAIL_IF(value != 2, "Expected value=2");
  IS_WSTR(Buffer, CW("Row no 2"), 9);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMTW(Stmt, CW("DROP TABLE IF EXISTS smpltest"));

  return OK;
}


ODBC_TEST(simple_test1)
{
  SQLLEN nRowCount;
  SQLRETURN rc;

  OK_SIMPLE_STMTW(Stmt, CW("DROP TABLE IF EXISTS t_basic, t_basic_2"));

  /* create the table 'myodbc3_demo_result' */
  OK_SIMPLE_STMTW(Stmt,
         CW("CREATE TABLE t_basic (id INT PRIMARY KEY, name VARCHAR(20))"));

  /* insert 3 rows of data */
  OK_SIMPLE_STMTW(Stmt, CW("INSERT INTO t_basic VALUES (1,'foo'),(2,'bar'),(3,'baz')"));

  /* update second row */
  OK_SIMPLE_STMTW(Stmt, CW("UPDATE t_basic SET name = 'bop' WHERE id = 2"));

  /* get the rows affected by update statement */
  rc= SQLRowCount(Stmt, &nRowCount);
  CHECK_STMT_RC(Stmt, rc);
  FAIL_IF(nRowCount != 1, "Rowcount != 1");
  
  /* delete third row */
  OK_SIMPLE_STMTW(Stmt, CW("DELETE FROM t_basic WHERE id = 3"));

  /* get the rows affected by delete statement */
  rc= SQLRowCount(Stmt, &nRowCount);
  CHECK_STMT_RC(Stmt, rc);
  FAIL_IF(nRowCount != 1, "Rowcount != 1");

  /* alter the table 't_basic' to 't_basic_2' */
  OK_SIMPLE_STMTW(Stmt, CW("ALTER TABLE t_basic RENAME t_basic_2"));

  /*
    drop the table with the original table name, and it should
    return error saying 'table not found'
  */
  ERR_SIMPLE_STMTW(Stmt, CW("DROP TABLE t_basic"));

 /* now drop the table, which is altered..*/
  OK_SIMPLE_STMTW(Stmt, CW("DROP TABLE t_basic_2"));

  return OK;
}

ODBC_TEST(select1000)
{
  SQLRETURN rc;
  SQLINTEGER num;
  SQLCHAR    szData[20];

  OK_SIMPLE_STMTW(Stmt, CW("DROP TABLE IF EXISTS t_max_select"));

  OK_SIMPLE_STMTW(Stmt, CW("CREATE TABLE t_max_select (a INT, b VARCHAR(30))"));

  rc= SQLPrepareW(Stmt, CW("INSERT INTO t_max_select VALUES (?,?)"), SQL_NTS);
  CHECK_STMT_RC(Stmt, rc);

  rc= SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &num, 0, NULL);
  CHECK_STMT_RC(Stmt, rc);
  rc= SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_CHAR, 0, 0, szData, sizeof(szData),
                                  NULL);
  CHECK_STMT_RC(Stmt, rc);

  for (num= 1; num <= 1000; num++)
  {
    sprintf((char *)szData, "MySQL%d", (int)num);
    rc= SQLExecute(Stmt);
    CHECK_STMT_RC(Stmt, rc);
  }

  rc= SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
  rc= SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMTW(Stmt, CW("SELECT * FROM t_max_select"));
  num= 0;

  while (SQL_SUCCESS == SQLFetch(Stmt))
    num++;

  FAIL_IF(num != 1000, "Expected 1000 rows");

  rc= SQLFreeStmt(Stmt, SQL_UNBIND);
  rc= SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMTW(Stmt, CW("DROP TABLE IF EXISTS t_max_select"));

  return OK;
}

ODBC_TEST(simple_2)
{
  SQLINTEGER nRowCount= 0, nInData= 1, nOutData= 0;
  SQLCHAR szOutData[31]= {'\0'};
  SQLRETURN rc;

  OK_SIMPLE_STMTW(Stmt, CW("DROP TABLE IF EXISTS t_myodbc"));

  OK_SIMPLE_STMTW(Stmt, CW("CREATE TABLE t_myodbc (a INT, b VARCHAR(30))"));

  rc= SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt, rc);

  /* DIRECT INSERT */
  OK_SIMPLE_STMTW(Stmt, CW("INSERT INTO t_myodbc VALUES (10, 'direct')"));

  /* PREPARE INSERT */
  rc= SQLPrepareW(Stmt, CW("INSERT INTO t_myodbc VALUES (?, 'param')"), SQL_NTS);
  CHECK_STMT_RC(Stmt, rc);

  rc= SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                       SQL_INTEGER, 0, 0, &nInData, 0, NULL);
  CHECK_STMT_RC(Stmt, rc);

  for (nInData= 20; nInData < 100; nInData= nInData+10)
  {
    rc= SQLExecute(Stmt);
    CHECK_STMT_RC(Stmt, rc);
  }

  /* FREE THE PARAM BUFFERS */
  rc= SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
  CHECK_STMT_RC(Stmt, rc);
  rc= SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt, rc);

  /* FETCH RESULT SET */
  OK_SIMPLE_STMTW(Stmt, CW("SELECT * FROM t_myodbc"));

  rc= SQLBindCol(Stmt, 1, SQL_C_LONG, &nOutData, 0, NULL);
  CHECK_STMT_RC(Stmt, rc);
  rc= SQLBindCol(Stmt, 2, SQL_C_CHAR, szOutData, sizeof(szOutData), NULL);
  CHECK_STMT_RC(Stmt, rc);

  nInData= 10;
  while (SQLFetch(Stmt) == SQL_SUCCESS)
  {
    FAIL_IF(nOutData != nInData, "OutData != InData");
    FAIL_IF(strncmp((const char*)szOutData, nRowCount++ ? "param" : "direct", 5), "");
    nInData += 10;
  }

  FAIL_IF(nRowCount != (nInData - 10) / 10, "");

  /* FREE THE OUTPUT BUFFERS */
  rc= SQLFreeStmt(Stmt, SQL_UNBIND);
  CHECK_STMT_RC(Stmt, rc);
  rc= SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt, rc);
  OK_SIMPLE_STMTW(Stmt, CW("DROP TABLE IF EXISTS t_myodbc"));

  return OK;
}

ODBC_TEST(test_reconnect)
{
  SQLHDBC hdbc1;
  SQLRETURN rc;
  int i;
  SQLWCHAR dsn[256],
           username[64],
           passwd[64];

  for (i= 0; i < 10; i++)
  {
    rc= SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1);
    CHECK_ENV_RC(Env, rc);
    
    rc= SQLConnectW(hdbc1, latin_as_sqlwchar((char*)my_dsn, dsn), SQL_NTS, latin_as_sqlwchar((char*)my_uid, username), SQL_NTS,
                   latin_as_sqlwchar((char*)my_pwd, passwd), SQL_NTS);
    CHECK_DBC_RC(hdbc1, rc);
    rc= SQLDisconnect(hdbc1);
    CHECK_DBC_RC(hdbc1, rc);
    rc= SQLFreeHandle(SQL_HANDLE_DBC, hdbc1);
    if (!SQL_SUCCEEDED(rc))
      CHECK_DBC_RC(hdbc1, rc);
  }

  return OK;
}

ODBC_TEST(t_disconnect)
{
  SQLHDBC hdbc1;
  SQLRETURN rc;
  int i;
  SQLHSTMT hstmt;

  rc= SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1);
  CHECK_ENV_RC(Env, rc);
  rc= SQLConnectW(hdbc1, wdsn, SQL_NTS, wuid, SQL_NTS, wpwd, SQL_NTS);
  CHECK_DBC_RC(hdbc1, rc);

  for (i=0; i < 100; i++)
  {
    rc= SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hstmt); 
    CHECK_DBC_RC(hdbc1, rc);

    rc= SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_FORWARD_ONLY, 0);
    CHECK_STMT_RC(hstmt, rc);
    rc= SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    CHECK_STMT_RC(hstmt, rc);
    rc= SQLEndTran(SQL_HANDLE_DBC, hdbc1, 1);
    CHECK_DBC_RC(hdbc1, rc);
  }
  rc= SQLDisconnect(hdbc1);
  CHECK_DBC_RC(hdbc1, rc);
  rc= SQLFreeHandle(SQL_HANDLE_DBC, hdbc1);
  if (!SQL_SUCCEEDED(rc))
    CHECK_DBC_RC(hdbc1, rc);
  
  return OK;
}

ODBC_TEST(bug19823)
{
  SQLHDBC Hdbc;

  SQLUINTEGER timeout;
  SQLRETURN rc;

  rc= SQLAllocHandle(SQL_HANDLE_DBC, Env, &Hdbc);
  CHECK_ENV_RC(Env, rc);

  rc= SQLSetConnectAttr(Hdbc, SQL_ATTR_LOGIN_TIMEOUT,
                              (SQLPOINTER)9, 0);
  rc= SQLSetConnectAttr(Hdbc, SQL_ATTR_CONNECTION_TIMEOUT,
                              (SQLPOINTER)10, 0);
  CHECK_DBC_RC(Hdbc, rc);

  rc= SQLConnect(Hdbc, my_dsn, SQL_NTS, my_uid, SQL_NTS,
                   my_pwd, SQL_NTS);
  CHECK_DBC_RC(Hdbc, rc);

  rc= SQLGetConnectAttr(Hdbc, SQL_ATTR_LOGIN_TIMEOUT,
                               &timeout, 0, NULL);
  CHECK_DBC_RC(Hdbc, rc);
  FAIL_IF(timeout != 9, "Login_timeout != 9");

  rc= SQLGetConnectAttr(Hdbc, SQL_ATTR_CONNECTION_TIMEOUT,
                               &timeout, 0, NULL);

  /* Since connection timeout is not supported, the value
     must be 0 */
  CHECK_DBC_RC(Hdbc, rc);
  FAIL_IF(timeout != 0, "connection_timeout != 0");

  rc= SQLDisconnect(Hdbc);
  rc= SQLFreeHandle(SQL_HANDLE_DBC, Hdbc);

  return OK;
}

ODBC_TEST(t_basic)
{
  SQLINTEGER nRowCount= 0, nInData= 1, nOutData;
  SQLCHAR szOutData[31];
  SQLRETURN rc;

 
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_myodbc");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_myodbc (a INT, b VARCHAR(30))");

  rc= SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  /* DIRECT INSERT */
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_myodbc VALUES (10, 'direct')");

  /* PREPARE INSERT */
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLPrepare(Stmt,
                            (SQLCHAR *)
                            "INSERT INTO t_myodbc VALUES (?, 'param')",
                            SQL_NTS));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &nInData, 0, NULL));

  for (nInData= 20; nInData < 100; nInData= nInData+10)
  {
    CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecute(Stmt));
  }

  /* FREE THE PARAM BUFFERS */
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* FETCH RESULT SET */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_myodbc");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nOutData, 0, NULL));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szOutData, sizeof(szOutData),
                            NULL));

  nInData= 10;
  while (SQLFetch(Stmt) == SQL_SUCCESS)
  {
    FAIL_IF(nOutData != nInData, "in != out");
    FAIL_IF(strncmp((const char*)szOutData, nRowCount++ ? "param" : "direct", 5) != 0, "comparison failed");
    nInData += 10;
  }

  FAIL_IF(nRowCount != (nInData - 10) / 10, "comparison failed");

  /* FREE THE OUTPUT BUFFERS */
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_myodbc");

  return OK;
}

ODBC_TEST(t_reconnect)
{
  SQLHDBC hdbc1;
  long i;
  SQLRETURN ret;

  for (i= 0; i < 10; i++)
  {
    ret= SQLAllocConnect(Env, &hdbc1);
    CHECK_HANDLE_RC(SQL_HANDLE_ENV, Env, ret);
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, hdbc1, SQLConnect(hdbc1, my_dsn, SQL_NTS, my_uid, SQL_NTS,
                             my_pwd, SQL_NTS));
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, hdbc1, SQLDisconnect(hdbc1));
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, hdbc1, SQLFreeConnect(hdbc1));
  }

  return OK;
}


ODBC_TEST(charset_utf8)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLCHAR conn[512], conn_out[1024];
  SQLLEN len;
  SQLSMALLINT conn_out_len, in_len;
  SQLINTEGER str_size;
  SQLWCHAR wc[20];

  /**
   Bug #19345: Table column length multiplies on size session character set
  */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug19345");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug19345 (a VARCHAR(10), b VARBINARY(10)) CHARACTER SET Utf8");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug19345 VALUES ('abc','def')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  in_len= (SQLSMALLINT)_snprintf((char *)conn, sizeof(conn), "DSN=%s;UID=%s;PWD=%s;PORT=%u;CHARSET=utf8",
         my_dsn, my_uid, my_pwd, my_port);
  
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, in_len, conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "SELECT _latin1 0x73E36F207061756C6F");

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  SQLGetData(hstmt1, 1, SQL_CHAR, conn_out, 512, NULL);
  FAIL_IF(strncmp((const char*)conn_out, "s\xC3\xA3o paulo", 10) != 0, "Comparison mismatch");
    
  FAIL_IF(SQLFetch(hstmt1) != SQL_NO_DATA, "End of result expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLColumns(hstmt1, (SQLCHAR *)my_schema, SQL_NTS, NULL, 0,
                             (SQLCHAR *)"t_bug19345", SQL_NTS,
                             (SQLCHAR *)"%", 1));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  is_num(my_fetch_int(hstmt1, 7), 10);
  str_size= my_fetch_int(hstmt1, 8);
  /* utf8 mbmaxlen = 3 in libmysql before MySQL 6.0 */
  
  if (str_size == 30)
  {
    is_num(my_fetch_int(hstmt1, 16), 30);
  }
  else
  {
    is_num(str_size, 40);
    is_num(my_fetch_int(hstmt1, 16), 40);
  }

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  is_num(my_fetch_int(hstmt1, 7), 10);
  is_num(my_fetch_int(hstmt1, 8), 10);
  is_num(my_fetch_int(hstmt1, 16), 10);
    
  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  /* Seemingly xpand does not have big5 */
  if (!IsXpand)
  {
    OK_SIMPLE_STMT(hstmt1, "SELECT _big5 0xA4A4");

    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_C_CHAR, conn, 2, &len));
    is_num(0xE4, conn[0]);
    is_num(3, len);

    CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_C_CHAR, conn, 2, &len));
    is_num(0xB8, conn[0]);
    is_num(2, len);

    CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_C_CHAR, conn, 2, &len));
    is_num(0xAD, conn[0]);
    is_num(1, len);

    FAIL_IF(SQLGetData(hstmt1, 1, SQL_C_CHAR, conn, 2, &len) != SQL_NO_DATA_FOUND, "Expected SQL_NO_DATA_FOUND");
    FAIL_IF(SQLFetch(hstmt1) != SQL_NO_DATA_FOUND, "Expected SQL_NO_DATA_FOUND");

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
  }

  OK_SIMPLE_STMT(hstmt1, "SELECT 'abcdefghijklmnopqrstuvw'");
  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_WCHAR, wc, 19, &len));
  CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_WCHAR, wc, 19, &len));
  CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_WCHAR, wc, 19, &len));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug19345");
  
  return OK;
}

ODBC_TEST(charset_gbk)
{
  SQLHANDLE hdbc1;
  SQLHANDLE hstmt1;
  SQLCHAR conn[512], conn_out[512];
  
  SKIPIF(IsXpand, "Xpand does not have gbk");
  /*
    The fun here is that 0xbf5c is a valid GBK character, and we have 0x27
    as the second byte of an invalid GBK character. mysql_real_escape_string()
    handles this, as long as it knows the character set is GBK.
  */
  SQLCHAR str[]= "\xef\xbb\xbf\x27\xbf\x10";
  SQLSMALLINT conn_out_len;

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;CHARSET=gbk",
          my_dsn, my_uid, my_pwd);
  
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, sizeof(conn), conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLPrepare(hstmt1, (SQLCHAR *)"SELECT ?", SQL_NTS));
  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                   SQL_CHAR, 0, 0, str, sizeof(str),
                                   NULL));

  CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_CHAR, conn_out, sizeof(conn_out), NULL));
  FAIL_IF(strcmp((const char*)conn_out, (const char*)str) != 0, "comparison failed");
  
  FAIL_IF(SQLFetch(hstmt1) != SQL_NO_DATA, "SQL_NO_DATA expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}

ODBC_TEST(t_bug30774)
{
  SQLHDBC hdbc1;
  SQLHSTMT hstmt1;
  SQLCHAR username[65]= {0};

  strcat((char *)username, (char *)my_uid);
  strcat((char *)username, "!!!");

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnect(hdbc1, my_dsn, SQL_NTS,
                           username, (SQLSMALLINT)strlen((char *)username) -3,
                           my_pwd, SQL_NTS));
  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "SELECT USER()");
  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
  CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_CHAR, username, 65, NULL));

  FAIL_IF(strstr((const char*)username, "!!!"), "Username changed");
  
  FAIL_IF(SQLFetch(hstmt1) != SQL_NO_DATA_FOUND, "Expected end of data");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_STMT_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


#ifdef _WIN32
#  define WE_HAVE_SETUPLIB
#endif

#ifdef WE_HAVE_SETUPLIB
ODBC_TEST(t_bug30840)
{
  HDBC hdbc1;
  SQLCHAR   conn[512], conn_out[1024];
  SQLSMALLINT conn_out_len;

  if (using_dm(Connection))
  {
    diag("test doesn't work with (all) driver manager(s)");
    return SKIP;
  }

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;NO_PROMPT=1",
          my_dsn, my_uid, my_pwd);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, (HWND)HWND_DESKTOP, conn, (SQLSMALLINT)strlen(conn),
                                 conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_PROMPT));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}
#endif


/**
  Bug #30983: SQL Statements limited to 64k
*/
ODBC_TEST(t_bug30983)
{
  SQLCHAR buf[(80 * 1024) + 100]; /* ~80k */
  SQLCHAR *bufp = buf;
  SQLLEN buflen;
  int i, j;

  bufp+= sprintf((char *)bufp, "select '");

  /* fill 1k of each value */
  for (i= 0; i < 80; ++i)
    for (j= 0; j < 512; ++j, bufp += 2)
      sprintf((char *)bufp, "%02x", i);

  sprintf((char *)bufp, "' as val");

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, buf, SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, buf, 0, &buflen));
  is_num(buflen, 80 * 1024);
  return OK;
}


/*
   Test the output string after calling SQLDriverConnect
*/
ODBC_TEST(t_driverconnect_outstring)
{
  HDBC        hdbc1;
  SQLWCHAR    *connw, connw_out[1024];
  SQLSMALLINT conn_out_len;
  SQLCHAR     conna[512], conna_out[1024];
  size_t      lenCorrector= 1; 

  if (iOdbc() && !DmMinVersion(3, 52,14))
  {
    /* Old iODBC returns octets count. Thus must multiply(actually divide) by 4 this case(sizeof(SQLWCHAR)==4) */
    lenCorrector= sizeof(SQLWCHAR);
  }
  /* Testing how driver's doing if no out string given. ODBC-17
     ';' at the end is important - otherwise DM adds it while converting connstring for W function */
  sprintf((char*)conna, "DSN=%s;UID=%s;PWD=%s;CHARSET=utf8;", my_dsn, my_uid, my_pwd);
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conna, SQL_NTS, NULL,
                                 0, &conn_out_len, SQL_DRIVER_NOPROMPT));
  diag("OutString Length: %d", conn_out_len);
  is_num(conn_out_len, strlen((const char*)conna));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conna, SQL_NTS, conna_out,
                                 sizeof(conna_out), &conn_out_len, SQL_DRIVER_NOPROMPT));

  is_num(conn_out_len, strlen((const char*)conna));
  FAIL_IF(strncmp((const char*)conna_out, (const char*)conna, conn_out_len), "In and Out connstrings do not match");

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  
  /* Checking that COMPLETE and COMPLETE_REQUIRED do not fire dialog, if they have enough
     info to establish connection. Also checking that the out connstring in this case is just
     a copy of incoming connstring */
  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conna, SQL_NTS, NULL,
                                 0, &conn_out_len, SQL_DRIVER_COMPLETE));

  is_num(conn_out_len, strlen((const char*)conna));
  /* Not to show sensitive data in automatic testing logs */
  IS_STR_EX(conna_out, conna, conn_out_len, FALSE);

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conna, SQL_NTS, NULL,
                                 0, &conn_out_len, SQL_DRIVER_COMPLETE_REQUIRED));
 
  is_num(conn_out_len, strlen((const char*)conna));
  IS_STR_EX(conna_out, conna, conn_out_len, FALSE);

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  connw= CW(conna);
  CHECK_DBC_RC(hdbc1, SQLDriverConnectW(hdbc1, NULL, connw, SQL_NTS, connw_out,
                                        sizeof(connw_out)/sizeof(SQLWCHAR), &conn_out_len,
                                        SQL_DRIVER_NOPROMPT));
  /* Old iODBC returns octets count here */
  is_num(conn_out_len, strlen((const char*)conna)*lenCorrector);
  IS_WSTR_EX(connw_out, connw, strlen((const char*)conna), FALSE);

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnectW(hdbc1, NULL, connw, SQL_NTS, connw_out,
                                        sizeof(connw_out), &conn_out_len,
                                        SQL_DRIVER_COMPLETE));
  /* Old iODBC returns octets count here */
  is_num(conn_out_len, strlen((const char*)conna)*lenCorrector);
  IS_WSTR(connw_out, connw, strlen((const char*)conna));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnectW(hdbc1, NULL, connw, SQL_NTS, connw_out,
                                        sizeof(connw_out), &conn_out_len,
                                        SQL_DRIVER_COMPLETE_REQUIRED));
  /* Old iODBC returns octets count here */
  is_num(conn_out_len, strlen((const char*)conna)*lenCorrector);
  IS_WSTR(connw_out, connw, strlen((const char*)conna));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));
  
  return OK;
}


ODBC_TEST(setnames)
{
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, (SQLCHAR*)"SET NAMES utf8", SQL_NTS), SQL_ERROR);
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, (SQLCHAR*)"SeT NamES utf8", SQL_NTS), SQL_ERROR);
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, (SQLCHAR*)"   set names utf8", SQL_NTS), SQL_ERROR);
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, (SQLCHAR*)"	set names utf8", SQL_NTS), SQL_ERROR);
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, (SQLCHAR*)"/* comment */	set names utf8", SQL_NTS), SQL_ERROR);
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, (SQLCHAR*)"set /* comment */ names utf8", SQL_NTS), SQL_ERROR);
  return OK;
}


ODBC_TEST(setnames_conn)
{
  HDBC hdbc1;
  SQLCHAR conn[512], conn_out[512];
  SQLSMALLINT conn_out_len;

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;INITSTMT=set names utf8",
          my_dsn, my_uid, my_pwd);
  
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  EXPECT_DBC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, SQL_NTS, conn_out,
                                     sizeof(conn_out), &conn_out_len,
                                     SQL_DRIVER_NOPROMPT),
             SQL_ERROR);
  SQLDisconnect(hdbc1);

  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


/**
 Bug #15601: SQLCancel does not work to stop a query on the database server
*/
ODBC_TEST(sqlcancel)
{
  SQLLEN     pcbLength= SQL_LEN_DATA_AT_EXEC(0);

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR*)"SELECT ?", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1,SQL_PARAM_INPUT,SQL_C_CHAR,
                          SQL_VARCHAR,0,0,(SQLPOINTER)1,0,&pcbLength));

  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_NEED_DATA);

  /* Without SQLCancel we would get "out of sequence" DM error */
  CHECK_STMT_RC(Stmt, SQLCancel(Stmt));

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR*)"SELECT 1", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  return OK;
}


#ifdef _WIN32
DWORD WINAPI cancel_in_one_second(LPVOID arg)
{
  HSTMT Stmt= (HSTMT)arg;

  Sleep(1000);

  if (SQLCancel(Stmt) != SQL_SUCCESS)
    diag("SQLCancel failed!");

  return 0;
}


DWORD WINAPI cancel_dbc(LPVOID arg)
{
  HDBC Dbc= (HDBC)arg;

  Sleep(1000);

  if (SQLCancelHandle(SQL_HANDLE_DBC, Dbc) != SQL_SUCCESS)
    diag("SQLCancel failed!");

  return 0;
}


ODBC_TEST(sqlcancel_threaded)
{
  HANDLE thread;
  DWORD waitrc;

  thread= CreateThread(NULL, 0, cancel_in_one_second, Stmt, 0, NULL);

  /* */
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, "SELECT SLEEP(5)", SQL_NTS), SQL_ERROR);

  waitrc= WaitForSingleObject(thread, 10000);
  IS(!(waitrc == WAIT_TIMEOUT));

  return OK;
}


ODBC_TEST(sqlcancelhandle)
{
  HANDLE thread;
  DWORD waitrc;

  thread= CreateThread(NULL, 0, cancel_dbc, Connection, 0, NULL);

  /*  */
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, "SELECT SLEEP(5)", SQL_NTS), SQL_ERROR);
  /*OK_SIMPLE_STMT(Stmt, "SELECT SLEEP(5)");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(1, my_fetch_int(Stmt, 1));*/

  waitrc= WaitForSingleObject(thread, 10000);
  IS(!(waitrc == WAIT_TIMEOUT));

  return OK;
}

#else
void *cancel_in_one_second(void *arg)
{
  HSTMT *Stmt= arg;

  sleep(1);

  if (SQLCancel(Stmt) != SQL_SUCCESS)
    diag("SQLCancel failed!");

  return NULL;
}


void *cancel_dbc(void *arg)
{
  HDBC Dbc= (HDBC)arg;
  sleep(1);

  if (SQLCancelHandle(Dbc) != SQL_SUCCESS)
    diag("SQLCancel failed!");

  return NULL;
}

#include <pthread.h>

ODBC_TEST(sqlcancel_threaded)
{
  pthread_t thread;

  pthread_create(&thread, NULL, cancel_in_one_second, Stmt);

  /* Error "execution was interrupted" is returned when it's killed,
     "SLEEP(n) returns 1 when it is killed" - that is probably for older versions. Need to verify. */
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, "SELECT SLEEP(5)", SQL_NTS), SQL_ERROR);

  pthread_join(thread, NULL);

  return OK;
}


ODBC_TEST(sqlcancelhandle)
{
  pthread_t thread;

  pthread_create(&thread, NULL, cancel_dbc, Connection);

  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, "SELECT SLEEP(5)", SQL_NTS), SQL_ERROR);

  pthread_join(thread, NULL);

  return OK;
}
#endif  // ifdef _WIN32


ODBC_TEST(t_describe_nulti)
{
  SQLHENV   henv1;
  SQLHDBC   hdbc1;
  SQLHSTMT  hstmt1;
  SQLCHAR   ColumnName[64];

  my_options= 67108866;

  ODBC_Connect(&henv1, &hdbc1, &hstmt1);

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t1");
  OK_SIMPLE_STMT(hstmt1, "CREATE TABLE t1 (columnX VARCHAR(255))");

  OK_SIMPLE_STMT(hstmt1, "INSERT INTO t1 VALUES ('test')");
  CHECK_STMT_RC(hstmt1, SQLPrepare(hstmt1, (SQLCHAR*)"SELECT * FROM t1", SQL_NTS));
 
  CHECK_STMT_RC(hstmt1, SQLDescribeCol(hstmt1, 1, ColumnName, 64, NULL, 0, 0, 0, 0));

  ODBC_Disconnect(henv1, hdbc1, hstmt1);

  return OK;
}


/**
Bug #32014: MyODBC / ADO Unable to open record set using dynamic cursor
*/
ODBC_TEST(t_bug32014)
{
  SQLHENV     henv1;
  SQLHDBC     hdbc1;
  SQLHSTMT    hstmt1;
  SQLUINTEGER info;
  SQLULEN     attr= 0;
  long        i=    0;
  SQLSMALLINT value_len;

  long flags[]= { 0,
                  (131072L << 4)   /*FLAG_FORWARD_CURSOR*/,
                  32               /*FLAG_DYNAMIC_CURSOR*/,
                  (131072L << 4) | 32,
                  0 };

  long expectedInfo[]= { SQL_SO_FORWARD_ONLY|SQL_SO_STATIC,
                         SQL_SO_FORWARD_ONLY,
                         SQL_SO_FORWARD_ONLY|SQL_SO_STATIC|SQL_SO_DYNAMIC,
                         SQL_SO_FORWARD_ONLY|SQL_SO_DYNAMIC };

  long expectedCurType[][4]= {
      {SQL_CURSOR_FORWARD_ONLY, SQL_CURSOR_STATIC,        SQL_CURSOR_STATIC,          SQL_CURSOR_STATIC},
      {SQL_CURSOR_FORWARD_ONLY, SQL_CURSOR_FORWARD_ONLY,  SQL_CURSOR_FORWARD_ONLY,    SQL_CURSOR_FORWARD_ONLY},
      {SQL_CURSOR_FORWARD_ONLY, SQL_CURSOR_STATIC,        SQL_CURSOR_DYNAMIC,         SQL_CURSOR_STATIC},
      {SQL_CURSOR_FORWARD_ONLY, SQL_CURSOR_FORWARD_ONLY,  SQL_CURSOR_FORWARD_ONLY,    SQL_CURSOR_FORWARD_ONLY}};

  add_connstr= "";
  do
  {
    my_options= flags[i] | 67108866;
    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    diag("checking %d (%d)", i, flags[i]);

    /*Checking that correct info is returned*/

    CHECK_STMT_RC(hstmt1, SQLGetInfo(hdbc1, SQL_SCROLL_OPTIONS,
            (SQLPOINTER) &info, sizeof(long), &value_len));
    is_num(info, expectedInfo[i]);

    /*Checking that correct cursor type is set*/
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE
            , (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY, 0));
    CHECK_STMT_RC(hstmt1, SQLGetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
            (SQLPOINTER) &attr, 0, NULL));
    is_num(attr, expectedCurType[i][SQL_CURSOR_FORWARD_ONLY]);

    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
            (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0));
    CHECK_STMT_RC(hstmt1, SQLGetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
            (SQLPOINTER) &attr, 0, NULL));
    is_num(attr, expectedCurType[i][SQL_CURSOR_KEYSET_DRIVEN]);

    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
            (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0));
    CHECK_STMT_RC(hstmt1, SQLGetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
            (SQLPOINTER) &attr, 0, NULL));
    is_num(attr, expectedCurType[i][SQL_CURSOR_DYNAMIC]);

    /* SQLSet/GetOption are deprecated in favour of SQLSet/GetAttr
       Leaving one just to make sure we don't have problem with old apps,
       but disabling possible warning */
#pragma warning(disable: 4996)
#pragma warning(push)
    CHECK_STMT_RC(hstmt1, SQLSetStmtOption(hstmt1, SQL_CURSOR_TYPE,
            SQL_CURSOR_STATIC ));
    CHECK_STMT_RC(hstmt1, SQLGetStmtOption(hstmt1, SQL_CURSOR_TYPE,
            (SQLPOINTER) &attr));
#pragma warning(pop)
    is_num(attr, expectedCurType[i][SQL_CURSOR_STATIC]);

    ODBC_Disconnect(henv1, hdbc1, hstmt1);

  } while (flags[++i]);

  return OK;
}


/*
  Bug #10128 Error in evaluating simple mathematical expression
  ADO calls SQLNativeSql with a NULL pointer for the result length,
  but passes a non-NULL result buffer.
*/
ODBC_TEST(t_bug10128)
{
  SQLCHAR *query= (SQLCHAR *) "select 1,2,3,4";
  SQLCHAR nativesql[1000];
  SQLINTEGER nativelen;
  SQLINTEGER querylen= (SQLINTEGER) strlen((char *)query);

  CHECK_DBC_RC(Connection, SQLNativeSql(Connection, query, SQL_NTS, NULL, 0, &nativelen));
  is_num(nativelen, querylen);

  CHECK_DBC_RC(Connection, SQLNativeSql(Connection, query, SQL_NTS, nativesql, 1000, NULL));
  diag("%s", nativesql);
  IS_STR(nativesql, query, querylen + 1);

  return OK;
}


/**
 Bug #32727: Unable to abort distributed transactions enlisted in MSDTC
*/
ODBC_TEST(t_bug32727)
{
  is_num(SQLSetConnectAttr(Connection, SQL_ATTR_ENLIST_IN_DTC,
                       (SQLPOINTER)1, SQL_IS_UINTEGER), SQL_ERROR);
  return OK;
}


/*
  Bug #28820: Varchar Field length is reported as larger than actual
*/
ODBC_TEST(t_bug28820)
{
  SQLULEN length;
  SQLCHAR dummy[20];
  SQLSMALLINT i, columnCount= IsXpand ? 2 : 3;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug28820");

  if (IsXpand)
  {
    OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug28820 ("
      "x VARCHAR(90) CHARACTER SET latin1,"
      "z VARCHAR(90) CHARACTER SET utf8)");
    OK_SIMPLE_STMT(Stmt, "SELECT x,z FROM t_bug28820");
  }
  else
  {
    OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug28820 ("
      "x VARCHAR(90) CHARACTER SET latin1,"
      "y VARCHAR(90) CHARACTER SET big5,"
      "z VARCHAR(90) CHARACTER SET utf8)");
    OK_SIMPLE_STMT(Stmt, "SELECT x,y,z FROM t_bug28820");
  }

  for (i= 0; i < columnCount; ++i)
  {
    length= 0;
    CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, i+1, dummy, sizeof(dummy), NULL,
                                  NULL, &length, NULL, NULL));
    diag("length: %d", length);
    is_num(length, 90);
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug28820");
  return OK;
}

/* What is tested here? */
ODBC_TEST(t_count)
{
  SQLULEN length;
  SQLCHAR dummy[20];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_count");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_count (a INT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_count VALUES (1),(2),(3)");
                
  OK_SIMPLE_STMT(Stmt, "SELECT COUNT(*) regcount FROM t_count");

  length= 0;
  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 1, dummy, sizeof(dummy), NULL,
                                     NULL, &length, NULL, NULL));
  IS_STR(dummy, "regcount", sizeof("regcount"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_count");
  return OK;
}

/*
  Bug #31959 - Allows dirty reading with SQL_TXN_READ_COMMITTED
               isolation through ODBC
*/
ODBC_TEST(t_bug31959)
{
  SQLCHAR level[50] = "uninitialized";
  SQLINTEGER i;
  SQLLEN levelid[] = {SQL_TXN_SERIALIZABLE, SQL_TXN_REPEATABLE_READ,
                      SQL_TXN_READ_COMMITTED, SQL_TXN_READ_UNCOMMITTED};
  SQLCHAR *levelname[] = {(SQLCHAR *)"SERIALIZABLE",
                          (SQLCHAR *)"REPEATABLE-READ",
                          (SQLCHAR *)"READ-COMMITTED",
                          (SQLCHAR *)"READ-UNCOMMITTED"};

  if (ServerNotOlderThan(Connection, 11, 1, 1))
  {
    CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"SELECT @@transaction_isolation", SQL_NTS));
  }
  else
  {
    CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"SELECT @@tx_isolation", SQL_NTS));
  }

  /* check all 4 valid isolation levels */
  for(i = 3; i >= 0; --i)
  {
    CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION,
                                   (SQLPOINTER)levelid[i], 0));
    CHECK_STMT_RC(Stmt, SQLExecute(Stmt));
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, level, 50, NULL));
    IS_STR(level, levelname[i], strlen((char *)levelname[i]));
    diag("Level = %s\n", level);
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }

  /* check invalid value (and corresponding SQL state) */
  is_num(SQLSetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION, (SQLPOINTER)999, 0),
     SQL_ERROR);
  {
  SQLCHAR     sql_state[6];
  SQLINTEGER  err_code= 0;
  SQLCHAR     err_msg[SQL_MAX_MESSAGE_LENGTH]= {0};
  SQLSMALLINT err_len= 0;

  memset(err_msg, 'C', SQL_MAX_MESSAGE_LENGTH);
  SQLGetDiagRec(SQL_HANDLE_DBC, Connection, 1, sql_state, &err_code, err_msg,
                SQL_MAX_MESSAGE_LENGTH - 1, &err_len);

  IS_STR(sql_state, (SQLCHAR *)"HY024", 5);
  }
  return OK;
}


/*
  Bug #41256 - NULL parameters don't work correctly with ADO.
  The null indicator pointer can be set separately through the
  descriptor field. This wasn't being checked separately.
*/
ODBC_TEST(t_bug41256)
{
  SQLHANDLE apd;
  SQLINTEGER val= 40;
  SQLLEN vallen= 19283;
  SQLLEN ind= SQL_NULL_DATA;
  SQLLEN reslen= 40;
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_PARAM_DESC,
                                &apd, SQL_IS_POINTER, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_INTEGER,
                                  SQL_C_LONG, 0, 0, &val, 0, &vallen));
  CHECK_DESC_RC(apd, SQLSetDescField(apd, 1, SQL_DESC_INDICATOR_PTR,
                               &ind, SQL_IS_POINTER));
  OK_SIMPLE_STMT(Stmt, "select ?");
  val= 80;
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_LONG, &val, 0, &reslen));
  is_num(SQL_NULL_DATA, reslen);
  is_num(80, val);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  return OK;
}


ODBC_TEST(t_bug44971)
{
  char buffer[MAX_NAME_LEN];
  SQLINTEGER len;

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS bug44971");
  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE bug44971");
  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, buffer, sizeof(buffer), &len));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, "bug44971xxx", 8));
  /* Restoring original schema back. This also covers ODBC-392. Somehow it always hits here, but not in the previous line */
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, buffer, len));

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS bug44971");
  return OK;
}


ODBC_TEST(t_bug48603)
{
  SQLINTEGER timeout, interactive, diff= 1000;
  SQLSMALLINT conn_out_len;
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLCHAR conn[512], conn_out[512], query[53];

  OK_SIMPLE_STMT(Stmt, "SELECT @@wait_timeout, @@interactive_timeout");
  CHECK_STMT_RC(Stmt,SQLFetch(Stmt));

  timeout=      my_fetch_int(Stmt, 1);
  interactive=  my_fetch_int(Stmt, 2);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  if (timeout == interactive)
  {
    diag("Changing interactive timeout globally as it is equal to wait_timeout");
    /* Changing globally interactive timeout to be able to test
       if INTERACTIVE option works */
    sprintf((char *)query, "SET GLOBAL interactive_timeout=%d", timeout + diff);

    if (!SQL_SUCCEEDED(SQLExecDirect(Stmt, query, SQL_NTS)))
    {
      diag("Don't have rights to change interactive timeout globally - so can't really test if option INTERACTIVE works");
      // Let the testcase does not fail
      diff= 0;
      //return FAIL;
    }

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }
  else
  {
    diag("Interactive: %d, wait: %d", interactive, timeout);
    diff= interactive - timeout;
  }

  /* INITSTMT={set @@wait_timeout=%d} */
  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;CHARSET=utf8;INITSTMT=set @@interactive_timeout=%d;INTERACTIVE=1",
    my_dsn, my_uid, my_pwd, timeout+diff);

  if (my_port)
  {
    char pbuff[20];
    sprintf(pbuff, ";PORT=%d", my_port);
    strcat((char *)conn, pbuff);
  }

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, sizeof(conn), conn_out,
    sizeof(conn_out), &conn_out_len,
    SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));
  OK_SIMPLE_STMT(hstmt1, "SELECT @@interactive_timeout");
  CHECK_STMT_RC(hstmt1,SQLFetch(hstmt1));

  {
    SQLINTEGER cur_timeout= my_fetch_int(hstmt1, 1);

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
    CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
    CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

    if (timeout == interactive)
    {
      /* setting global interactive timeout back if we changed it */
      sprintf((char *)query, "SET GLOBAL interactive_timeout=%d", timeout);
      CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, query, SQL_NTS));
    }

    is_num(timeout + diff, cur_timeout);
  }
  return OK;
}

/*
  Bug#45378 - spaces in connection string aren't removed
*/
ODBC_TEST(t_bug45378)
{
  HDBC hdbc1;
  SQLCHAR conn[512], conn_out[512];
  SQLSMALLINT conn_out_len;

  sprintf((char *)conn, "DSN=%s; UID = {%s} ;PWD= %s ",
          my_dsn, my_uid, my_pwd);
  
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, SQL_NTS, conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


ODBC_TEST(t_mysqld_stmt_reset)
{
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_reset");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_reset (a int)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_reset(a) VALUES (1),(2),(3)");

  CHECK_STMT_RC(Stmt, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));
  /* Succesful query deploying PS */
  OK_SIMPLE_STMT(Stmt, "SELECT count(*) FROM t_reset");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 3);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Unsuccessful query */
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, (SQLCHAR*)"SELECT count(*) FROM t_reset_nonexistent", SQL_NTS), SQL_ERROR);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Successful directly executed query */
  OK_SIMPLE_STMT(Stmt, "DELETE FROM t_reset WHERE a=2");
  CHECK_STMT_RC(Stmt, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* And now successful query again */
  OK_SIMPLE_STMT(Stmt, "SELECT count(*) FROM t_reset");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 2);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_reset");
  return OK;
}


ODBC_TEST(t_odbc32)
{
  HDBC        hdbc1;
  SQLCHAR     conn[512];
  SQLUINTEGER packet_size= 0;

  sprintf((char *)conn, "DSN=%s;PORT=%u", my_dsn, my_port);
  
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLSetConnectAttr(hdbc1, SQL_ATTR_PACKET_SIZE, (SQLPOINTER)(4096*1024), 0));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc1, SQLGetConnectAttr(hdbc1, SQL_ATTR_PACKET_SIZE, (SQLPOINTER)&packet_size, 0, NULL));

  diag("Packet size is %u", packet_size);

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


ODBC_TEST(t_gh_issue3)
{
  OK_SIMPLE_STMT(Stmt, "\nSELECT 1");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "\tSELECT 2");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 2);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "\t SELECT 3");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 3);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "\n\t\n  \t\n  \n \t\t\t\t SELECT 4" );
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 4);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


ODBC_TEST(t_odbc48)
{
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS test_odbc_48");
  OK_SIMPLE_STMT(Stmt,
    "CREATE PROCEDURE test_odbc_48()"
    "BEGIN"
    " SELECT 1 AS ret;"
    "END");
  OK_SIMPLE_STMT(Stmt, "{ CALL test_odbc_48() }");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE test_odbc_48");

  return OK;
}

/* Verifying that charset names are case insensitive */
ODBC_TEST(t_odbc69)
{
  HDBC hdbc1;
  SQLCHAR   conn[512], conn_out[1024];
  SQLSMALLINT conn_out_len;

  /* Testing also that key names are case insensitve. Supposing, that there is no mariadb/mysql on 3310 port with same login credentials */
  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;PORT=3310;DATABASE=%s;OPTION=%lu;SERVER=%s;PoRt=%u;charset=UTF8",
    my_dsn, my_uid, my_pwd, my_schema, my_options, my_servername, my_port);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  /* If everything is right, right port value will override the wrong one, and connect will be successful */
  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, (SQLSMALLINT)strlen((const char*)conn),
    conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
    SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}

/* If connection handle re-used, it would try to select database used in previous connection.
   Additionnaly tests if "manual" changes of schema by application are detected(with session tracking) */
ODBC_TEST(t_odbc91)
{
  HDBC      hdbc;
  HSTMT     hstmt;
  SQLCHAR   conn[512], conn_out[1024], buffer[32];
  SQLSMALLINT conn_out_len;

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS t_odbc91");
  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE t_odbc91");

  /* Connecting to newly created tatabase. Need multistatement allowed for some tests */
  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;DATABASE=%s;OPTION=%lu;SERVER=%s;%s",
    my_dsn, my_uid, my_pwd, "t_odbc91", my_options | 67108864, my_servername, ma_strport);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc));

  CHECK_DBC_RC(hdbc, SQLDriverConnect(hdbc, NULL, conn, (SQLSMALLINT)strlen((const char*)conn),
    conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
    SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc, SQLGetConnectAttr(hdbc, SQL_ATTR_CURRENT_CATALOG, buffer, sizeof(buffer), NULL));
  IS_STR(buffer, "t_odbc91", sizeof("t_odbc91"));
  /* Checking if selescting schema with USE is reflected by SQLGetConnectAttr.
     Re-using conn_out buffer here, as it is > NAME_LEN + "USE " and harmless to reuse */
  sprintf(conn_out, "USE %s", my_schema);
  CHECK_DBC_RC(hdbc, SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt));
  OK_SIMPLE_STMT(hstmt, conn_out);
  CHECK_DBC_RC(hdbc, SQLGetConnectAttr(hdbc, SQL_ATTR_CURRENT_CATALOG, buffer, sizeof(buffer), NULL));
  IS_STR(buffer, my_schema, strlen(my_schema));

  /* No checking the same if changing schema is part ot a batch */
  sprintf(conn_out, "USE t_odbc91;USE %s", my_schema);
  OK_SIMPLE_STMT(hstmt, conn_out);
  /* Session tracked after result is read */
  CHECK_STMT_RC(hstmt, SQLMoreResults(hstmt));
  CHECK_DBC_RC(hdbc, SQLGetConnectAttr(hdbc, SQL_ATTR_CURRENT_CATALOG, buffer, sizeof(buffer), NULL));
  IS_STR(buffer, my_schema, strlen(my_schema));

  CHECK_DBC_RC(hdbc, SQLDisconnect(hdbc));

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE t_odbc91");

  /* Now we do not specify any database */
  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;OPTION=%lu;SERVER=%s;DATABASE=%s;%s",
    my_dsn, my_uid, my_pwd, my_options, my_servername, my_schema, ma_strport);

  CHECK_DBC_RC(hdbc, SQLDriverConnect(hdbc, NULL, conn, (SQLSMALLINT)strlen((const char*)conn),
    conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
    SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc, SQLDisconnect(hdbc));

  /* Now testing scenario, there default database is set via connetion attribute, and connection handler is re-used
     after disconnect. This doesn't work with UnixODBC, because smart UnixODBC implicicitly deallocates connection handle
     when SQLDisconnect is called */
  if (UnixOdbc())
  {
    diag("UnixODBC detected - Skipping part of the test");
    return OK;
  }
  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE t_odbc91");
  CHECK_DBC_RC(hdbc, SQLSetConnectAttr(hdbc, SQL_ATTR_CURRENT_CATALOG, (SQLPOINTER)"t_odbc91", SQL_NTS));

  diag("Connection string: Driver=%s;UID=%s;PWD=%s;OPTION=%lu;SERVER=%s;%s;%s",
    my_drivername, my_uid, "*****" , my_options, my_servername, add_connstr, ma_strport);
  sprintf((char *)conn, "Driver=%s;UID=%s;PWD=%s;OPTION=%lu;SERVER=%s;%s;%s",
    my_drivername, my_uid, my_pwd, my_options, my_servername, add_connstr, ma_strport);

  CHECK_DBC_RC(hdbc, SQLDriverConnect(hdbc, NULL, conn, (SQLSMALLINT)strlen((const char*)conn),
    conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
    SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc, SQLGetConnectAttr(hdbc, SQL_ATTR_CURRENT_CATALOG, (SQLPOINTER)buffer, sizeof(buffer), NULL));

  IS_STR(buffer, "t_odbc91", sizeof("t_odbc91"));
  buffer[0]= '\0';

  CHECK_DBC_RC(hdbc, SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt));

  OK_SIMPLE_STMT(hstmt, "SELECT DATABASE()");
  CHECK_STMT_RC(hstmt, SQLFetch(hstmt));
  IS_STR(my_fetch_str(hstmt, buffer, 1), "t_odbc91", sizeof("t_odbc91"));

  CHECK_DBC_RC(hdbc, SQLDisconnect(hdbc));

  buffer[0]= '\0';

  /* Checking that attribute value is preserved */
  CHECK_DBC_RC(hdbc, SQLGetConnectAttr(hdbc, SQL_ATTR_CURRENT_CATALOG, (SQLPOINTER)buffer, sizeof(buffer), NULL));
  IS_STR(buffer, "t_odbc91", sizeof("t_odbc91"));

  CHECK_DBC_RC(hdbc, SQLDriverConnect(hdbc, NULL, conn, (SQLSMALLINT)strlen((const char*)conn),
    conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
    SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(hdbc, SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt));

  OK_SIMPLE_STMT(hstmt, "SELECT DATABASE()");
  CHECK_STMT_RC(hstmt, SQLFetch(hstmt));
  IS_STR(my_fetch_str(hstmt, buffer, 1), "t_odbc91", sizeof("t_odbc91"));
 
  CHECK_STMT_RC(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  CHECK_DBC_RC(hdbc, SQLDisconnect(hdbc));
  CHECK_DBC_RC(hdbc, SQLFreeHandle(SQL_HANDLE_DBC, hdbc));
  OK_SIMPLE_STMT(Stmt, "DROP DATABASE t_odbc91");

  return OK;
}


ODBC_TEST(t_odbc137)
{
  SQLHDBC    Hdbc;
  SQLHSTMT   Hstmt;
  char       buffer[256], AllAnsiChars[258], AllAnsiHex[512];
  const char Charset[][16]= {"latin1", "cp850", "cp1251", "cp866", "cp852", "cp1250", "latin2", "latin5" ,"latin7",
                             "cp1256", "cp1257", "geostd8", "greek", "koi8u", "koi8r", "hebrew", "macce", "macroman",
                             "dec8", "hp8", "armscii8", "ascii", "swe7", "tis620", "keybcs2"};
  const char CreateStmtTpl[]= "CREATE TABLE `t_odbc137` (\
                          `val` TEXT DEFAULT NULL\
                          ) ENGINE=InnoDB DEFAULT CHARSET=%s";
  char       CreateStmt[sizeof(CreateStmtTpl) + sizeof(Charset[0])];
  const char InsertStmtTpl[]= "INSERT INTO t_odbc137(val)  VALUES('%s')";
  char       InsertStmt[sizeof(InsertStmtTpl) + sizeof(AllAnsiChars)];// TestStr[0])];
  const char SelectStmtTpl[]= "SELECT * FROM t_odbc137 WHERE val = 0x%s";
  char       SelectStmt[sizeof(SelectStmtTpl) + sizeof(AllAnsiHex)];// HexStr[0])];
  unsigned int i, j, Escapes= 0;
  BOOL _7bitOnly= iOdbc() || IsSkySql || IsSkySqlHa;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS `t_odbc137`");
  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));

  /* In fact it's probably does not make sense to test with iODBC. The only way to use connector with ansi connection charsets for
     applications, is to use utf8 anyway */
  /* On iOdbc it takes utf as source cs for recoding, and values starting from 0x80 require special values in the following byte.
     Thus testing only till 0x7f */
  for (i= 1; i < (_7bitOnly ? 0x80U : 256U); ++i)
  {
    if (i == '\'' || i == '\\')
    {
      AllAnsiChars[i-1 + Escapes]= '\\';
      ++Escapes;
    }

    AllAnsiChars[i-1 + Escapes]= i;
    sprintf(AllAnsiHex + (i-1)*2, "%02x", i);
  }

  AllAnsiChars[(_7bitOnly ? 0x7f : 255) + Escapes]= AllAnsiHex[(_7bitOnly ? 0x7f : 255)*2 + 1]= '\0';
  
  /* Xpand does not support bunch of charsets. First 2 it supports, 3rd - does not. Not checking all of them, just leaving 2 so far */
  for (i= 0; i < (IsXpand ? 2U : sizeof(Charset)/sizeof(Charset[0])); ++i)
  {
    if (iOdbc() && strcmp(Charset[i], "koi8u")==0)
    {
      diag("Charset %s is not supported with iODBC", Charset[i]);
      break;
    }
    else
    {
      diag("Charset: %s", Charset[i]);
    }
    
    Hstmt= ConnectWithCharset(&Hdbc, Charset[i], NULL);
    FAIL_IF(Hstmt == NULL, "");

    sprintf(CreateStmt, CreateStmtTpl, Charset[i]);
    OK_SIMPLE_STMT(Hstmt, CreateStmt);

    sprintf(InsertStmt, InsertStmtTpl, AllAnsiChars);
    OK_SIMPLE_STMT(Hstmt, InsertStmt);

    sprintf(SelectStmt, SelectStmtTpl, AllAnsiHex);
    OK_SIMPLE_STMT(Hstmt, SelectStmt);

    FAIL_IF(SQLFetch(Hstmt) == SQL_NO_DATA, "Wrong data has been stored in the table");
    /* We still need to make sure that the string has been delivered correctly */
    my_fetch_str(Hstmt, (SQLCHAR*)buffer, 1);
    /* AllAnsiChars is escaped, so we cannot compare result string against it */
    for (j= 1; j < (_7bitOnly ? 0x80U : 256U); ++j)
    {
      is_num((unsigned char)buffer[j - 1], j);
    }
    CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
    OK_SIMPLE_STMT(Stmt, "DROP TABLE `t_odbc137`");
    CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
  }

  CHECK_DBC_RC(Hdbc, SQLFreeConnect(Hdbc));

  return OK;
}

#ifdef _WIN32
DWORD WINAPI FireQueryInThread(LPVOID arg)
{
  HSTMT hStmt= (HSTMT)arg;

  SQLExecDirect(hStmt, "SELECT 1", SQL_NTS);

  return 0;
}


ODBC_TEST(t_odbc139)
{
  SQLHDBC  Hdbc;
  SQLHSTMT Hstmt;
  unsigned long Compression= 2048;
  HANDLE Thread;
  DWORD WaitRc;

  /* Not sure when this was fixed, but works with 10.4, it seems */
  if (ServerNotOlderThan(Connection, 10, 2, 0) && !ServerNotOlderThan(Connection, 10, 4, 0))
  {
    skip("Waiting for the fix in Connector/C for servers > 10.2.0");
  }
  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));

  CHECK_DBC_RC(Hdbc, SQLSetConnectAttr(Hdbc, SQL_ATTR_CURRENT_CATALOG, (SQLPOINTER)"test", 4));
  Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, &Compression, NULL, NULL);

  FAIL_IF(Hstmt == NULL, "Connection with compression failed");

  Thread= CreateThread(NULL, 0, FireQueryInThread, Hstmt, 0, NULL);
  WaitRc= WaitForSingleObject(Thread, 1500);
  FAIL_IF(WaitRc == WAIT_TIMEOUT, "Direct Execution has taken too long time");

  CHECK_STMT_RC(Hstmt, SQLFetch(Hstmt));

  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
  CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
  CHECK_DBC_RC(Hdbc, SQLFreeConnect(Hdbc));

  return OK;
}
#endif

ODBC_TEST(t_odbc162)
{
  SQLSMALLINT ColumnCount;
  SQLRETURN rc;

  rc= SQLExecDirect(Stmt, (SQLCHAR*)"WITH x AS (SELECT 1 as `val`\
                                   UNION ALL\
                                   SELECT 2 AS `val`\
                                   UNION ALL\
                                   SELECT 3 AS `val`)\
                                   SELECT REPEAT(CAST(x.val AS nchar), x.val * 2) AS `string`,\
                                   REPEAT(CAST(x.val AS char), x.val * 2) AS `c_string`,\
                                   CAST(REPEAT(CHAR(x.val), x.val * 12000) as BINARY) as `binary`,\
                                    x.val AS `index`\
                                  FROM x", SQL_NTS);

  if (SQL_SUCCEEDED(rc))
  {
    CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &ColumnCount));

    is_num(ColumnCount, 4);
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }
  else
  {
    if (IsXpand)
    {
      /* Xpand seemingly can't digest such queries, but setting general sqlstate*/
      CHECK_SQLSTATE(Stmt, "HY000");
    }
    else
    {
      /* Old server does not support it */
      CHECK_SQLSTATE(Stmt, "42000");
    }
  }

  return OK;
}

/* This testcase is incompelete. and its idea is lost and forgotten. Keeping it just to have REPLACE statement. */
ODBC_TEST(t_replace)
{
  if (!ServerNotOlderThan(Connection, 10, 2, 0))
  {
    skip("REPLACE SQL command is not supported by your server version");
  }
  /* I think this test is not complete */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS `t_odbc_replace`");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE `t_odbc_replace` (`uuid` VARCHAR(64), PRIMARY KEY (`uuid`)) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "REPLACE INTO `t_odbc_replace` (`uuid`) VALUES(\"5b7fe80c-7de1-4744-ab33-3f65f26726f6\"),\
(\"3ce73838-72f2-4aed-9f45-b7cf15f1279b\"),\
(\"d56b1182-a077-47bd-9954-e6d62a75a05a\"),\
(\"bded059e-b770-4211-ad73-bfb66ed53dca\")");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE `t_odbc_replace`");

  return OK;
}

/* Parsing error caused driver to think this is musltistatement */
ODBC_TEST(t_odbc181)
{
  char buffer[128];
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc181");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc181(col1 DECIMAL(10,4), col2 longtext)");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc181 VALUES(-0.0042, 'text ending with newline\r\n some quetion ? end; and something after semicolon');");

  OK_SIMPLE_STMT(Stmt, "SELECT col1, col2 FROM t_odbc181");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, (SQLCHAR*)buffer, 1), "-0.0042", 8);
  IS_STR(my_fetch_str(Stmt, (SQLCHAR*)buffer, 2), "text ending with newline\r\n some quetion ? end; and something after semicolon",
    sizeof("text ending with newline\r\n some quetion ? end; and something after semicolon"));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc181");

  return OK;
}


ODBC_TEST(t_local_data_infile)
{
  SQLHDBC  Hdbc;
  SQLHSTMT Hstmt;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));

  /* Connection with *disabled* LOAD DATA LOCAL INFILE */
  Hstmt = DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "NOLOCALINFILE=1");
  EXPECT_STMT(Hstmt, SQLExecDirect(Hstmt, "LOAD DATA LOCAL INFILE 'nonexistent.txt' INTO TABLE nonexistent(b)", SQL_NTS), SQL_ERROR);
  check_sqlstate(Hstmt, "42000");
  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
  CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));

  /* Connection with *enabled* LOAD DATA LOCAL INFILE */
  Hstmt = DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "NOLOCALINFILE=0");
  EXPECT_STMT(Hstmt, SQLExecDirect(Hstmt, "LOAD DATA LOCAL INFILE 'nonexistent.txt' INTO TABLE nonexistent(b)", SQL_NTS), SQL_ERROR);
  /* Non-existent error */
  check_sqlstate(Hstmt, "42S02");
  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
  CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
  CHECK_DBC_RC(Hdbc, SQLFreeConnect(Hdbc));

  return OK;
}

#ifdef _WIN32
// The testcase is for Windows only. It verifies if named pipe plugin could be founded & loaded
ODBC_TEST(t_odbc384)
{
  SQLHDBC  Hdbc;
  SQLHSTMT Hstmt;
  SQLINTEGER NativeError;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));

  /* Connection with *disabled* LOAD DATA LOCAL INFILE */
  Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "NAMEDPIPE=1;SOCKET=NotConfiguredPipeName");
  if (Hstmt != NULL)
  {
    CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
    CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
    FAIL_IF(1,"This should not happen - the connection apparently was done some other that named pipe way");
  }
  SQLGetDiagRec(SQL_HANDLE_DBC, Hdbc, 1, NULL, &NativeError, NULL, 0, NULL);
  FAIL_IF(NativeError == 2059, "Named Pipe plugin could not be loaded");
  FAIL_IF(NativeError != 2017, "Looks like connector did not connect to named pipe");
  CHECK_DBC_RC(Hdbc, SQLFreeConnect(Hdbc));

  return OK;
}
#endif

/* ODBC-377 Timeout attributes have no effect. We don't support CONNECT_TIMEOUT(our C/C CONNECT_TIMEOUT is ODBC LOGINE_TIMEOUT attr) */
ODBC_TEST(t_odbc377)
{
  SQLHDBC  Hdbc;
  SQLHSTMT Hstmt;
  SQLCHAR  Sqlstate[6], ErrMsg[SQL_MAX_MESSAGE_LENGTH];

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));

  CHECK_DBC_RC(Hdbc, SQLSetConnectAttr(Hdbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)2, 0));
  EXPECT_DBC(Hdbc, SQLSetConnectAttr(Hdbc, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)2, 0), SQL_SUCCESS_WITH_INFO);
  /* Connection with *disabled* LOAD DATA LOCAL INFILE */
  Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "INITSTMT={SELECT SLEEP(4)}");
  if (Hstmt != NULL)
  {
    CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
    CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
    FAIL_IF(1, "Login Timeout did not work");
  }
  /* The sqlstate has to be HYT00 */
  CHECK_SQLSTATE_EX(Hdbc, SQL_HANDLE_DBC, "HYT00");
  /* Login timeout from connstring(CONN_TIMEOUT) takes precedence over the one set as attribute */
  CHECK_DBC_RC(Hdbc, SQLSetConnectAttr(Hdbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)1, 0));
  Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "INITSTMT={SELECT SLEEP(2)};CONN_TIMEOUT=4");
  FAIL_IF(Hstmt == NULL, "Timeout shouldn't have occurred");

  CHECK_STMT_RC(Hstmt, SQLSetStmtAttr(Hstmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)1, 0));
  EXPECT_STMT(Hstmt, SQLExecDirect(Hstmt, "SELECT SLEEP(2)", SQL_NTS), SQL_ERROR);
  Sqlstate[0]= '\0';
  CHECK_STMT_RC(Hstmt, SQLGetDiagRec(SQL_HANDLE_STMT, Hstmt, 1, Sqlstate, NULL, ErrMsg, sizeof(ErrMsg), NULL));
  if (strncmp(Sqlstate, "70100", 6) != 0 && strncmp(Sqlstate, "HY018", 6) != 0 && (!IsXpand || strncmp(Sqlstate, "HYOOO", 6) != 0))
  {
    diag("Unexpected SQL State %s(%s)", Sqlstate, ErrMsg);
    return FAIL;
  }
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Verifying the same(i.e. query timeout works) for the case of prepare + execute */
  CHECK_STMT_RC(Hstmt, SQLPrepare(Hstmt, "SELECT SLEEP(2)", SQL_NTS));
  EXPECT_STMT(Hstmt, SQLExecute(Hstmt), SQL_ERROR);
  Sqlstate[0]= '\0';
  CHECK_STMT_RC(Hstmt, SQLGetDiagRec(SQL_HANDLE_STMT, Hstmt, 1, Sqlstate, NULL, ErrMsg, sizeof(ErrMsg), NULL));
  /* Not sure what to expect from Xpand here, thus expecting same general error state as before */
  if (strncmp(Sqlstate, "70100", 6) != 0 && strncmp(Sqlstate, "HY018", 6) != 0 && (!IsXpand || strncmp(Sqlstate, "HYOOO", 6) != 0))
  {
    diag("Unexpected SQL State %s(%s)", Sqlstate, ErrMsg);
    return FAIL;
  }

  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
  CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
  CHECK_DBC_RC(Hdbc, SQLFreeConnect(Hdbc));
  return OK;
}


/* ODBC-388 Support of application attributes for performance schema. */
ODBC_TEST(t_odbc388)
{
  SQLHDBC  Hdbc;
  SQLHSTMT Hstmt;
  SQLCHAR  buffer[64];

  if (PerfSchema == FALSE)
  {
    skip("Test requires perf_schema to be on");
  }
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "SELECT ATTR_VALUE FROM performance_schema.session_connect_attrs "
                                       "WHERE processlist_id=CONNECTION_ID() AND ATTR_NAME=?", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLPOINTER)"_client_name2", sizeof("_client_name2"), NULL));
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR("maodbc", my_fetch_str(Stmt, buffer, 1), sizeof("maodbc"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLPOINTER)"_client_version2", sizeof("_client_version2"), NULL));
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(DriverVersion, my_fetch_str(Stmt, buffer, 1), strlen(DriverVersion));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));

  Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "ATTR={_client_attr1 = attr1_value , _client_attr2= attr2_value}");
  FAIL_IF(Hstmt == NULL, "Connection error");

  CHECK_STMT_RC(Hstmt, SQLPrepare(Hstmt, "SELECT ATTR_VALUE FROM performance_schema.session_connect_attrs "
                                         "WHERE processlist_id=CONNECTION_ID() AND ATTR_NAME=?", SQL_NTS));
  CHECK_STMT_RC(Hstmt, SQLBindParameter(Hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLPOINTER)"_client_attr1", sizeof("_client_attr1"), NULL));
  CHECK_STMT_RC(Hstmt, SQLExecute(Hstmt));
  CHECK_STMT_RC(Hstmt, SQLFetch(Hstmt));
  IS_STR("attr1_value", my_fetch_str(Hstmt, buffer, 1), sizeof("attr1_value"));
  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_CLOSE));
  CHECK_STMT_RC(Hstmt, SQLBindParameter(Hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLPOINTER)"_client_attr2", sizeof("_client_attr2"), NULL));
  CHECK_STMT_RC(Hstmt, SQLExecute(Hstmt));
  CHECK_STMT_RC(Hstmt, SQLFetch(Hstmt));
  IS_STR("attr2_value", my_fetch_str(Hstmt, buffer, 1), sizeof("attr2_value"));
  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_CLOSE));

  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
  CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
  CHECK_DBC_RC(Hdbc, SQLFreeConnect(Hdbc));

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));
  Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "ATTR={_client_attr12, _client_attr22= attr2_value2}");
  FAIL_IF(Hstmt == NULL, "Connection error");
  CHECK_STMT_RC(Hstmt, SQLPrepare(Hstmt, "SELECT ATTR_VALUE FROM performance_schema.session_connect_attrs "
                                         "WHERE processlist_id=CONNECTION_ID() AND ATTR_NAME=?", SQL_NTS));
  CHECK_STMT_RC(Hstmt, SQLBindParameter(Hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLPOINTER)"_client_attr22", sizeof("_client_attr22"), NULL));
  CHECK_STMT_RC(Hstmt, SQLExecute(Hstmt));
  CHECK_STMT_RC(Hstmt, SQLFetch(Hstmt));
  IS_STR("attr2_value2", my_fetch_str(Hstmt, buffer, 1), sizeof("attr2_value2"));
  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_CLOSE));
  CHECK_STMT_RC(Hstmt, SQLBindParameter(Hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLPOINTER)"_client_attr12", sizeof("_client_attr12"), NULL));
  CHECK_STMT_RC(Hstmt, SQLExecute(Hstmt));
  EXPECT_STMT(Hstmt, SQLFetch(Hstmt), SQL_NO_DATA);
  /*CHECK_STMT_RC(Hstmt, SQLFetch(Hstmt));
  // Not sure why, but setting attribute with name, but without value results in blank space value of the attribute(with Windows client)
  IS_STR(" ", my_fetch_str(Hstmt, buffer, 1), 2);*/
  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
  CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
  CHECK_DBC_RC(Hdbc, SQLFreeConnect(Hdbc));

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));
  Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "ATTR={_client_attr13 :attr1_value3,_client_attr23}");
  FAIL_IF(Hstmt == NULL, "Connection error");
  CHECK_STMT_RC(Hstmt, SQLPrepare(Hstmt, "SELECT ATTR_VALUE FROM performance_schema.session_connect_attrs "
                                         "WHERE processlist_id=CONNECTION_ID() AND ATTR_NAME=?", SQL_NTS));
  CHECK_STMT_RC(Hstmt, SQLBindParameter(Hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLPOINTER)"_client_attr13", sizeof("_client_attr13"), NULL));
  CHECK_STMT_RC(Hstmt, SQLExecute(Hstmt));
  CHECK_STMT_RC(Hstmt, SQLFetch(Hstmt));
  IS_STR("attr1_value3", my_fetch_str(Hstmt, buffer, 1), sizeof("attr1_value3"));
  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_CLOSE));
  CHECK_STMT_RC(Hstmt, SQLBindParameter(Hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLPOINTER)"_client_attr23", sizeof("_client_attr23"), NULL));
  CHECK_STMT_RC(Hstmt, SQLExecute(Hstmt));
  EXPECT_STMT(Hstmt, SQLFetch(Hstmt), SQL_NO_DATA);
  /*CHECK_STMT_RC(Hstmt, SQLFetch(Hstmt));
  IS_STR(" ", my_fetch_str(Hstmt, buffer, 1), 2);*/

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));
  Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "ATTR={_client_attr14}");
  FAIL_IF(Hstmt == NULL, "Connection error");
  
  CHECK_STMT_RC(Hstmt, SQLBindParameter(Hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLPOINTER)"_client_attr14", sizeof("_client_attr14"), NULL));
  OK_SIMPLE_STMT(Hstmt, "SELECT ATTR_VALUE FROM performance_schema.session_connect_attrs "
                        "WHERE processlist_id=CONNECTION_ID() AND ATTR_NAME=?");
  EXPECT_STMT(Hstmt, SQLFetch(Hstmt), SQL_NO_DATA);
  /*CHECK_STMT_RC(Hstmt, SQLFetch(Hstmt));
  IS_STR("", my_fetch_str(Hstmt, buffer, 1), 2);*/
  
  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
  CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
  CHECK_DBC_RC(Hdbc, SQLFreeConnect(Hdbc));
  return OK;
}

ODBC_TEST(connection_reset)
{
  SQLCHAR    db[255];
  SQLINTEGER  len;

  skip("This test won't work with DM");

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS test_reset");
  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE test_reset");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, "test_reset", SQL_NTS));
  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, db, sizeof(db), &len));

  is_num(len, strlen("test_reset"));
  IS_STR(db, "test_reset", 10);

  /* reset */
  /* SQL_RESET_CONNECTION_YES is the only valid value - checking error */
  /* Only DM may set this attribute */
  /*EXPECT_DBC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_RESET_CONNECTION, (SQLPOINTER)(SQL_RESET_CONNECTION_YES + 1), 0), SQL_ERROR);
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_RESET_CONNECTION, (SQLPOINTER)SQL_RESET_CONNECTION_YES, 0));*/

  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, db, sizeof(db), &len));

  /*is_num(len, strlen(my_schema));
  IS_STR(db, my_schema, strlen(my_schema));*/
  diag("Db after reset: %s", db);
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, my_schema, SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE test_reset");
  return OK;
}

MA_ODBC_TESTS my_tests[]=
{
  {t_disconnect, "t_disconnect",      NORMAL},
  {t_describe_nulti, "t_describe_nulti", NORMAL},
  {test_CONO1,     "test_CONO1",     NORMAL},
  {test_CONO3,     "test_CONO3",     NORMAL},
  {t_count,        "t_count",        NORMAL},
  {simple_test,    "Simple test",    NORMAL},
  {simple_test1,   "Simple test1",   NORMAL},
  {select1000,     "select1000",     NORMAL},
  {simple_2,       "simple_2",       NORMAL},
  {test_reconnect, "test_reconnect", NORMAL},
  {bug19823,       "bug19823",       NORMAL},
  {t_basic,        "t_basic",        NORMAL},
  {t_reconnect,    "t_reconnect",    NORMAL},
  {charset_utf8,   "charset_utf8",   NORMAL},
  {charset_gbk,    "charset_gbk",    NORMAL},
  {t_bug30774,     "t_bug30774",     NORMAL},
#ifdef WE_HAVE_SETUPLIB
  {t_bug30840,     "t_bug30840",     NORMAL},
#endif
  {t_bug30983,     "t_bug30983",     NORMAL},
  {t_driverconnect_outstring, "t_driverconnect_outstring", NORMAL},
  {setnames,       "setnames",       NORMAL},
  {setnames_conn,  "setnames_conn",  NORMAL},
  {sqlcancel,      "sqlcancel",      NORMAL},
  {sqlcancel_threaded, "sqlcancel_threaded", NORMAL},
  {t_bug32014,     "t_bug32014",     NORMAL},
  {t_bug10128,     "t_bug10128",     NORMAL},
  {t_bug32727,     "t_bug32727",     NORMAL},
  {t_bug28820,     "t_bug28820",     NORMAL},
  {t_bug31959,     "t_bug31959",     NORMAL},
  {t_bug41256,     "t_bug41256",     NORMAL},
  {t_bug44971,     "t_bug44971",     NORMAL},
  {t_bug48603,     "t_bug48603",     NORMAL},
  {t_bug45378,     "t_bug45378",     NORMAL},
  {t_mysqld_stmt_reset, "tmysqld_stmt_reset bug", NORMAL},
  {t_odbc32,      "odbc32_SQL_ATTR_PACKET_SIZE_option", NORMAL},
  {t_gh_issue3,   "leading_space_gh_issue3",  NORMAL},
  {t_odbc48,      "odbc48_iso_call_format",   NORMAL},
  {t_odbc69,      "odbc69_ci_connstring",     NORMAL},
  {t_odbc91,      "odbc91_hdbc_reuse",        NORMAL},
  {t_odbc137,     "odbc137_ansi",             NORMAL},
  {t_odbc162,     "t_odbc162_CTE_query",      NORMAL },
  {t_replace,     "t_replace",      NORMAL },
  {t_odbc181,     "t_odbc181",      NORMAL },
  {t_local_data_infile, "odbc347_local_data_infile", NORMAL },
#ifdef _WIN32
  {t_odbc139,     "odbc139_compression",      NORMAL},
  {t_odbc384,     "odbc384_namedpipe_plugin", NORMAL},
#endif
  {t_odbc377,     "odbc377_timeout_attrs",    NORMAL},
  {t_odbc388,     "odbc388_perfschema_attrs",    NORMAL},
  {sqlcancelhandle, "sqlcancelhandle", NORMAL},
  {connection_reset, "test_SQL_ATTR_RESET_CONNECTION", NORMAL},
  {NULL, NULL, 0}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
