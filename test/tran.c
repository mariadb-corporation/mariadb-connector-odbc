/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013, 2024  MariaDB plc

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

/**
  Test transaction behavior using InnoDB tables
*/
ODBC_TEST(my_transaction)
{
  /* set AUTOCOMMIT to OFF */
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection,SQL_ATTR_AUTOCOMMIT,
                                 (SQLPOINTER)SQL_AUTOCOMMIT_OFF,0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t1");

  CHECK_DBC_RC(Connection, SQLTransact(NULL,Connection,SQL_COMMIT));

  /* create the table 't1' using InnoDB */
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t1 (col1 INT, col2 VARCHAR(30)) ENGINE = InnoDB");

  /* insert a row and commit the transaction */
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t1 VALUES(10,'venu')");
  CHECK_DBC_RC(Connection, SQLTransact(NULL,Connection,SQL_COMMIT));

  /* now insert the second row, but roll back that transaction */
  OK_SIMPLE_STMT(Stmt,"INSERT INTO t1 VALUES(20,'mysql')");
  CHECK_DBC_RC(Connection, SQLTransact(NULL,Connection,SQL_ROLLBACK));

  /* delete first row, but roll it back */
  OK_SIMPLE_STMT(Stmt,"DELETE FROM t1 WHERE col1 = 10");
  CHECK_DBC_RC(Connection, SQLTransact(NULL,Connection,SQL_ROLLBACK));

  /* Bug #21588: Incomplete ODBC API implementaion */
  /* insert a row, but roll it back using SQLTransact on the environment */
  OK_SIMPLE_STMT(Stmt,"INSERT INTO t1 VALUES(30,'mysql')");
  CHECK_DBC_RC(Connection, SQLTransact(Env,NULL,SQL_ROLLBACK));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  /* test the results now, only one row should exist */
  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t1");

  FAIL_IF(SQLFetch(Stmt) == SQL_NO_DATA_FOUND, "expected data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  /* now insert some more records to check SQLEndTran */
  OK_SIMPLE_STMT(Stmt,"INSERT INTO t1 "
               "VALUES (30,'test'),(40,'transaction')");
  CHECK_DBC_RC(Connection, SQLTransact(NULL,Connection,SQL_COMMIT));

  /* Commit the transaction using DBC handler */
  OK_SIMPLE_STMT(Stmt,"DELETE FROM t1 WHERE col1 = 30");
  CHECK_DBC_RC(Connection, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));

  /* test the results now, select should not find any data */
  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t1 WHERE col1 = 30");
  FAIL_IF(SQLFetch(Stmt)!= SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  /* Delete a row to check, and commit the transaction using ENV handler */
  OK_SIMPLE_STMT(Stmt,"DELETE FROM t1 WHERE col1 = 40");
  CHECK_DBC_RC(Connection, SQLEndTran(SQL_HANDLE_ENV, Env, SQL_COMMIT));

  /* test the results now, select should not find any data */
  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t1 WHERE col1 = 40");
  FAIL_IF(SQLFetch(Stmt)!= SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  /* drop the table */
  OK_SIMPLE_STMT(Stmt,"DROP TABLE t1");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  return OK;
}


ODBC_TEST(t_tran)
{
  SQLLEN Rows= 0;
  
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_tran");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_tran (a INT, b VARCHAR(30)) ENGINE=InnoDB");

  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0));

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_tran VALUES (10, 'venu')");
  CHECK_STMT_RC(Stmt, SQLTransact(NULL, Connection, SQL_COMMIT));

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_tran VALUES (20, 'mysql')");
  CHECK_STMT_RC(Stmt, SQLTransact(NULL, Connection, SQL_ROLLBACK));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_tran");

  if (RSSTREAMING)
  {
    is_num(my_print_non_format_result_ex(Stmt, FALSE), 1);
  }
  else
  {
    SQLRowCount(Stmt, (SQLLEN *)&Rows);
    is_num(Rows, 1);
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_tran");

  return OK;
}


/**
  Test retrieval and setting of transaction isolation level.
*/
ODBC_TEST(t_isolation)
{
  /* SQL_ATTR_TXN_ISOLATION is in fact 32bit bitmap, according to specs */
  SQLINTEGER isolation= 0;
  SQLCHAR tx_isolation[200];

  /* Check that the default is REPEATABLE READ. */
  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION, &isolation,
                                 SQL_IS_POINTER, NULL));
  is_num(isolation, SQL_TXN_REPEATABLE_READ);

  /* Change it to READ UNCOMMITTED. */
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION,
                                 (SQLPOINTER)SQL_TXN_READ_UNCOMMITTED, 0));

  /* Check that the driver has rmeembered the new value. */
  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION, &isolation,
                                 SQL_IS_POINTER, NULL));
  is_num(isolation, SQL_TXN_READ_UNCOMMITTED);

  /* Check that it was actually changed on the server. */
  if (ServerNotOlderThan(Connection, 11, 1, 1) || IsMysql)
  {
    OK_SIMPLE_STMT(Stmt, "SELECT @@transaction_isolation");
  }
  else
  {
    OK_SIMPLE_STMT(Stmt, "SELECT @@tx_isolation");
  }
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, tx_isolation,
                            sizeof(tx_isolation), NULL));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(tx_isolation, "READ-UNCOMMITTED", 16);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Restoring default SQL_TXN_REPEATABLE_READ */
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION,
    (SQLPOINTER)SQL_TXN_REPEATABLE_READ, 0));
  return OK;
}

/* Testing that if isolation level is changed with SQL command, we still get correct attribute value
 * This covers also the case of ODBC-394
 */
ODBC_TEST(t_isolation2)
{
  SQLINTEGER isolation= 0;

  /* Check that the default is REPEATABLE READ. */
  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION, &isolation,
    SQL_IS_POINTER, NULL));
  is_num(isolation, SQL_TXN_REPEATABLE_READ);

  /* Change it to READ UNCOMMITTED. */
  OK_SIMPLE_STMT(Stmt, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");

  /* Check that the driver has rmeembered the new value. */
  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION, &isolation,
    SQL_IS_POINTER, NULL));
  is_num(isolation, SQL_TXN_READ_COMMITTED);

  /* Restoring default SQL_TXN_REPEATABLE_READ */
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION,
    (SQLPOINTER)SQL_TXN_REPEATABLE_READ, 0));
  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_TXN_ISOLATION, &isolation,
    SQL_IS_POINTER, NULL));
  is_num(isolation, SQL_TXN_REPEATABLE_READ);

  return OK;
}

/* Testing, that setting of isolation level before connection establishing works
 * This covers also the case of ODBC-395
 */
ODBC_TEST(t_isolation3)
{
  SQLINTEGER isolation= 0;
  SQLHANDLE dbc= NULL, Stmt1= NULL;
  char buffer[32];

  IS(AllocEnvConn(&Env, &dbc));

 /* Setting different level(SQL_TXN_READ_COMMITTED) */
  CHECK_DBC_RC(dbc, SQLSetConnectAttr(dbc, SQL_ATTR_TXN_ISOLATION,
    (SQLPOINTER)SQL_TXN_READ_COMMITTED, 0));
  /* We don't need statement, but it's easier to connect this way :), let's neglect, that the error may be in fact stmt allocation error */
  Stmt1= DoConnect(dbc, FALSE, NULL, NULL, NULL, 0, NULL, 0, NULL, NULL);
  FAIL_IF(Stmt1 == NULL, "Could not connect and/or allocate statement");
  
  /* Checking that after connection we have that isolation level */
  CHECK_DBC_RC(dbc, SQLGetConnectAttr(dbc, SQL_ATTR_TXN_ISOLATION, &isolation,
    SQL_IS_POINTER, NULL));
  is_num(isolation, SQL_TXN_READ_COMMITTED);

  if (ServerNotOlderThan(Connection, 11, 1, 1) || IsMysql)
  {
    OK_SIMPLE_STMT(Stmt1, "SELECT @@transaction_isolation");
  }
  else
  {
    OK_SIMPLE_STMT(Stmt1, "SELECT @@tx_isolation");
  }

  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  IS_STR("READ-COMMITTED", my_fetch_str(Stmt1, buffer, 1), sizeof("READ-COMMITTED"));
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_DROP));
  CHECK_DBC_RC(dbc, SQLDisconnect(dbc));
  CHECK_DBC_RC(dbc, SQLFreeConnect(dbc));

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {my_transaction,"my_transaction"},
  {t_tran,        "t_tran"},
  {t_isolation,   "t_isolation"},
  {t_isolation2,  "t_isolation2_value_change_tracking"},
  {t_isolation3,  "t_isolation3_set_before_connect"},
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
