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

#include <time.h>

ODBC_TEST(my_pcbvalue)
{
  SQLRETURN   rc;
  SQLLEN      nRowCount;
  SQLINTEGER  nData= 500;
  SQLLEN      int_pcbValue, pcbValue, pcbValue1, pcbValue2;
  SQLCHAR     szData[255]={0};

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_pcbValue");

  OK_SIMPLE_STMT(Stmt, "create table my_pcbValue(id int, name varchar(30),\
                                                       name1 varchar(30),\
                                                       name2 varchar(30))");

  OK_SIMPLE_STMT(Stmt, "insert into my_pcbValue(id,name) values(100,'venu')");

  OK_SIMPLE_STMT(Stmt, "insert into my_pcbValue(id,name) values(200,'monty')");

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData, 0, &int_pcbValue);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData, 15, &pcbValue);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,3,SQL_C_CHAR,szData, 3, &pcbValue1);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,4,SQL_C_CHAR,szData, 2, &pcbValue2);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY ,(SQLPOINTER)SQL_CONCUR_ROWVER , 0);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE  ,(SQLPOINTER)1 , 0);
  CHECK_STMT_RC(Stmt, rc);

  /* Open the resultset of table 'my_demo_cursor' */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_pcbValue");

  /* goto the last row */
  if (ForwardOnly)
  {
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  }
  else
  {
    CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_LAST, 1L));
  }

  /* Now delete the newly updated record */
  strcpy((char*)szData,"updated");
  nData = 99999;

  int_pcbValue=2;
  pcbValue=3;
  pcbValue1=9;
  pcbValue2=SQL_NTS;

  rc = SQLSetPos(Stmt,1,SQL_UPDATE,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt, &nRowCount);
  CHECK_STMT_RC(Stmt, rc);

  diag(" total rows updated:%d\n",nRowCount);
  is_num(nRowCount, 1);

  /* Free statement cursor resorces */
  rc = SQLFreeStmt(Stmt, SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  /* Now fetch and verify the data */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_pcbValue");

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt,1,SQL_C_LONG,&nData,0,NULL);
  CHECK_STMT_RC(Stmt,rc);
  is_num(nData, 99999);

  rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,50,NULL);
  CHECK_STMT_RC(Stmt,rc);
  IS_STR(szData, "upd", 4);

  rc = SQLGetData(Stmt,3,SQL_C_CHAR,szData,50,NULL);
  CHECK_STMT_RC(Stmt,rc);
  IS_STR(szData, "updated", 8);

  rc = SQLGetData(Stmt,4,SQL_C_CHAR,szData,50,NULL);
  CHECK_STMT_RC(Stmt,rc);
  IS_STR(szData, "updated", 8);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
  SQLFreeStmt(Stmt, SQL_UNBIND);
  SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_pcbValue");

  return OK;
}


/* to test the pcbValue on cursor ops **/
ODBC_TEST(my_pcbvalue_add)
{
  SQLRETURN   rc;
  SQLLEN      nRowCount;
  SQLINTEGER  nData= 500;
  SQLLEN      int_pcbValue, pcbValue, pcbValue1, pcbValue2;
  SQLCHAR     szData[255]={0};

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_pcbValue_add");

  OK_SIMPLE_STMT(Stmt, "create table my_pcbValue_add(id int, name varchar(30),\
                                                       name1 varchar(30),\
                                                       name2 varchar(30))");

  OK_SIMPLE_STMT(Stmt,"insert into my_pcbValue_add(id,name) values(100,'venu')");

  OK_SIMPLE_STMT(Stmt,"insert into my_pcbValue_add(id,name) values(200,'monty')");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,0,&int_pcbValue);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,15,&pcbValue);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,3,SQL_C_CHAR,szData,3,&pcbValue1);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,4,SQL_C_CHAR,szData,2,&pcbValue2);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY ,(SQLPOINTER)SQL_CONCUR_ROWVER , 0);
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE  ,(SQLPOINTER)1 , 0);
    CHECK_STMT_RC(Stmt, rc);

    /* Open the resultset of table 'my_pcbValue_add' */
    OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_pcbValue_add");
    CHECK_STMT_RC(Stmt,rc);

    /* goto the last row */
    rc = SQLFetchScroll(Stmt, SQL_FETCH_LAST, 1L);
    CHECK_STMT_RC(Stmt,rc);

    /* Now delete the newly updated record */
    strcpy((char*)szData,"inserted");
    nData = 99999;

    int_pcbValue=2;
    pcbValue=3;
    pcbValue1=6;
    pcbValue2=SQL_NTS;

    rc = SQLSetPos(Stmt,1,SQL_ADD,SQL_LOCK_NO_CHANGE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLRowCount(Stmt, &nRowCount);
    CHECK_STMT_RC(Stmt, rc);

    diag(" total rows updated:%d\n",nRowCount);
    is_num(nRowCount, 1);

    /* Free statement cursor resorces */
    rc = SQLFreeStmt(Stmt, SQL_UNBIND);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    /* commit the transaction */
    rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    /* Now fetch and verify the data */
    OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_pcbValue_add");
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLGetData(Stmt,1,SQL_C_LONG,&nData,0,NULL);
    CHECK_STMT_RC(Stmt,rc);
    is_num(nData, 99999);

    rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,50,NULL);
    CHECK_STMT_RC(Stmt,rc);
    IS_STR(szData, "ins", 4);

    rc = SQLGetData(Stmt,3,SQL_C_CHAR,szData,50,NULL);
    CHECK_STMT_RC(Stmt,rc);
    IS_STR(szData, "insert", 7);

    rc = SQLGetData(Stmt,4,SQL_C_CHAR,szData,50,NULL);
    CHECK_STMT_RC(Stmt,rc);
    IS_STR(szData, "inserted", 9);

    rc = SQLFetch(Stmt);
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"eof expected");

    SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt, SQL_UNBIND);
    SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_pcbValue_add");

  return OK;
}


/* spaces in column names */
ODBC_TEST(my_columnspace)
{
    SQLRETURN   rc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS TestColNames");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE `TestColNames`(`Value One` text, `Value Two` text,`Value Three` text)");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO TestColNames VALUES ('venu','anuganti','mysql ab')");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO TestColNames VALUES ('monty','widenius','mysql ab')");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM `TestColNames`");

  is_num(my_print_non_format_result(Stmt), 2);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT `Value One`,`Value Two`,`Value Three` FROM `TestColNames`");

  is_num(my_print_non_format_result(Stmt), 2);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS TestColNames");

  return OK;
}


/* to test the empty string returning NO_DATA */
ODBC_TEST(my_empty_string)
{
    SQLRETURN   rc;
    SQLLEN      pcbValue;
    SQLCHAR     szData[255]={0};

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_empty_string");

  OK_SIMPLE_STMT(Stmt, "create table my_empty_string(name varchar(30))");

  OK_SIMPLE_STMT(Stmt, "insert into my_empty_string values('')");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    /* Now fetch and verify the data */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_empty_string");

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLGetData(Stmt,1,SQL_C_CHAR,szData,50,&pcbValue);
    CHECK_STMT_RC(Stmt,rc);
    diag("szData:%s(%d)\n",szData,pcbValue);

    rc = SQLFetch(Stmt);
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"expected eof");

    SQLFreeStmt(Stmt, SQL_UNBIND);
    SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_empty_string");

  return OK;
}


ODBC_TEST(t_fetch_array)
{
  SQLINTEGER id[3];
  SQLINTEGER i= 0, j;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_fetch_array");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_fetch_array (id INT NOT NULL)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_fetch_array VALUES(1),(2),(3),(4),(5),(6),(7),(8),(9),(10),(11),(12)");

  OK_SIMPLE_STMT(Stmt, "SELECT id FROM t_fetch_array");

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
    (SQLPOINTER)(sizeof(id)/sizeof(SQLINTEGER)), 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_BIND_TYPE,
    (SQLPOINTER)SQL_BIND_BY_COLUMN, 0));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &id, 0, NULL));

  for (i= 0; i < 12 /(sizeof(id)/sizeof(SQLINTEGER)); ++i)
  {
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

    for (j= 0; j < sizeof(id)/sizeof(SQLINTEGER); ++j)
    {
      is_num(id[j], i*sizeof(id)/sizeof(SQLINTEGER) + j + 1);
    }
    /* Not gonna fix this in 2.0. Not a big problem anyway */
    /* is_num(my_fetch_int(Stmt, 1), i*sizeof(id)/sizeof(SQLINTEGER) + 1); */
  }

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_fetch_array");

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {my_pcbvalue,     "my_pcbvalue"},
  {my_pcbvalue_add, "my_pcbvalue_add"},
  {my_columnspace,  "my_columnspace"},
  {my_empty_string, "my_empty_string"},
  {t_fetch_array,   "t_fetch_array"},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  mark_all_tests_normal(my_tests);
  return run_tests(my_tests);
}
