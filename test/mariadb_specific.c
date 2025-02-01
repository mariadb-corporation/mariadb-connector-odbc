// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2025 MariaDB Corporation plc

#include "mariadb/sqlmariadb.h"
#include "tap.h"

SQLINTEGER mariadbAttribute[]={SQL_ATTR_EXECDIRECT_ON_SERVER, SQL_ATTR_PREPARE_ON_CLIENT};
SQLLEN attributesTestDefault[sizeof(mariadbAttribute) / sizeof(mariadbAttribute[0])]= {-1};
/* Null pointer if the attribute does not have corresponding connstring option */
SQLCHAR *connstringOption[sizeof(mariadbAttribute) / sizeof(mariadbAttribute[0])]=
{"EDSERVER", "PREPONCLIENT"};

/* Test of setting of mariadb specific connection attributes */
ODBC_TEST(mariadb_conn_attribute)
{
  SQLLEN edPrepareOnServer= -1;
  size_t i;

  for (i= 0; i < sizeof(mariadbAttribute) / sizeof(mariadbAttribute[0]); ++i) {
    CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, mariadbAttribute[i],
      &attributesTestDefault[i], 0, NULL));
    /* Default is SQL_FALSE */
    if (attributesTestDefault[i] != SQL_FALSE) {
      diag("Looks like the default for %d attribute has been changed via connstring, but ok");
    }
    CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, mariadbAttribute[i],
      (SQLPOINTER)SQL_TRUE, 0));
    edPrepareOnServer= -1;
    CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, mariadbAttribute[i], &edPrepareOnServer,
      0, NULL));
    is_num(edPrepareOnServer, SQL_TRUE);

    edPrepareOnServer= -1;
    CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, mariadbAttribute[i],
      (SQLPOINTER)SQL_FALSE, 0));
    CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, mariadbAttribute[i], &edPrepareOnServer,
      0, NULL));
    is_num(edPrepareOnServer, SQL_FALSE);
  }
  return OK;
}

/* Test of setting of mariadb specific statement attributes */
ODBC_TEST(mariadb_stmt_attribute)
{
  SQLLEN edPrepareOnServer= -1;
  size_t i;

  for (i= 0; i < sizeof(mariadbAttribute) / sizeof(mariadbAttribute[0]); ++i) {
    CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, mariadbAttribute[i], &edPrepareOnServer,
      0, NULL));
    /* default is SQL_FALSE as for the server */
    FAIL_IF(edPrepareOnServer != SQL_FALSE,
      "This tests expects SQL_ATTR_EXECDIRECT_ON_SERVER default value not changed via connstring");

    CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, mariadbAttribute[i],
      (SQLPOINTER)SQL_TRUE, 0));
    edPrepareOnServer= -1;
    CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, mariadbAttribute[i], &edPrepareOnServer,
      0, NULL));
    is_num(edPrepareOnServer, SQL_TRUE);

    edPrepareOnServer= -1;
    CHECK_DBC_RC(Stmt, SQLSetStmtAttr(Stmt, mariadbAttribute[i],
      (SQLPOINTER)SQL_FALSE, 0));
    CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, mariadbAttribute[i], &edPrepareOnServer,
      0, NULL));
    is_num(edPrepareOnServer, SQL_FALSE);
  }
  return OK;
}

/* Test of default values of mariadb specific statement attributes changed with
 * the change of corresponding connection attribute for newly created statement handles 
 */
ODBC_TEST(mariadb_attr_default)
{
  SQLLEN edPrepareOnServer= -1;
  SQLHANDLE stmt1= NULL;
  size_t i;

  for (i= 0; i < sizeof(mariadbAttribute) / sizeof(mariadbAttribute[0]); ++i) {
    CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, mariadbAttribute[i],
      (SQLPOINTER)SQL_TRUE, 0));

    CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &stmt1));
    CHECK_STMT_RC(stmt1, SQLGetStmtAttr(stmt1, mariadbAttribute[i], &edPrepareOnServer,
      0, NULL));
    is_num(edPrepareOnServer, SQL_TRUE);

    CHECK_STMT_RC(stmt1, SQLFreeStmt(stmt1, SQL_DROP));

    CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, mariadbAttribute[i],
      (SQLPOINTER)SQL_FALSE, 0));
  }
  return OK;
}

/* Test of setting statement SQL_ATTR_EXECDIRECT_ON_SERVER attribute causes SQLDirectExecute
 * to use SSPS/binary protocol, and CSPS/text protocol otherwise
 */
ODBC_TEST(ed_server)
{
  SQLLEN edPrepareOnServer= -1;
  int    prev_ps_count, curr_ps_count= 0;

  diag("This may fail on non-exclusive server");
  GET_SERVER_STATUS(prev_ps_count, GLOBAL, "Prepared_stmt_count");

  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_EXECDIRECT_ON_SERVER, &edPrepareOnServer,
    0, NULL));
  is_num(edPrepareOnServer, SQL_FALSE);

  OK_SIMPLE_STMT(Stmt, "SELECT 1");
  GET_SERVER_STATUS(curr_ps_count, GLOBAL, "Prepared_stmt_count");
  is_num(curr_ps_count, prev_ps_count);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_EXECDIRECT_ON_SERVER,
    (SQLPOINTER)SQL_TRUE, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT 2");
  GET_SERVER_STATUS(curr_ps_count, GLOBAL, "Prepared_stmt_count");
  is_num(curr_ps_count, prev_ps_count + 1);

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_EXECDIRECT_ON_SERVER,
    (SQLPOINTER)SQL_FALSE, 0));

  return OK;
}

/* Test of setting statement SQL_ATTR_PREPARE_ON_CLIENT attribute causes SQLPrepare
 * to prepare client and use text protocol, and prepare on server and use binary protocol
 * otherwise
 */
ODBC_TEST(prepare_client)
{
  SQLLEN edPrepareOnServer= -1;
  int    prev_ps_count, curr_ps_count= 0;

  diag("This may fail on non-exclusive server");
  GET_SERVER_STATUS(prev_ps_count, GLOBAL, "Prepared_stmt_count");

  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_PREPARE_ON_CLIENT, &edPrepareOnServer,
    0, NULL));
  is_num(edPrepareOnServer, SQL_FALSE);

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "SELECT 3", SQL_NTS));
  GET_SERVER_STATUS(curr_ps_count, GLOBAL, "Prepared_stmt_count");
  is_num(curr_ps_count, prev_ps_count + 1);
  prev_ps_count= curr_ps_count;

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PREPARE_ON_CLIENT,
    (SQLPOINTER)SQL_TRUE, 0));

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "SELECT 4", SQL_NTS));
  GET_SERVER_STATUS(curr_ps_count, GLOBAL, "Prepared_stmt_count");
  is_num(curr_ps_count, prev_ps_count);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PREPARE_ON_CLIENT,
    (SQLPOINTER)SQL_FALSE, 0));
  return OK;
}

/* Test of that default of connection attributes is changed with corresponding connection
 * string option
 */
ODBC_TEST(defaults_via_connstring)
{
  SQLLEN connAttrValue;
  size_t i;
  SQLHANDLE conn= NULL, stmt= NULL;
  char addOption[64]; /* Should be big enough to accomodate any option */

  for (i= 0; i < sizeof(mariadbAttribute) / sizeof(mariadbAttribute[0]); ++i) {
    /* Skipping if there is no corresponding connstring option */
    if (!connstringOption[i]) {
      continue;
    }
    /* Testing only for the value different to the one recorded as default in 1st test,
     * assuming that value has been tested 
     */
    _snprintf(addOption, sizeof(addOption), "%s=%d", connstringOption[i],
      attributesTestDefault[i] == SQL_FALSE ? 1 : 0);
    CHECK_ENV_RC(Env, SQLAllocConnect(Env, &conn));
    stmt= DoConnect(conn, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, addOption);
    FAIL_IF (!stmt, "Could not establish connetion");
    connAttrValue= -1;
    CHECK_DBC_RC(conn, SQLGetConnectAttr(conn, mariadbAttribute[i], &connAttrValue, 0, NULL));
    if (connAttrValue == attributesTestDefault[i]) {
      diag("Attribute value %d should be different for %s", (int)connAttrValue, addOption);
      return FAIL;
    }
    CHECK_STMT_RC(stmt, SQLFreeStmt(stmt, SQL_DROP));
    CHECK_DBC_RC(conn, SQLDisconnect(conn));
    CHECK_DBC_RC(conn, SQLFreeConnect(conn));
  }
  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {mariadb_conn_attribute, "mariadb_specific_conn_attr",    NORMAL},
  {mariadb_stmt_attribute, "mariadb_specific_stmt_attr",    NORMAL},
  {mariadb_attr_default,   "mariadb_stmt_attr_default",     NORMAL},
  {ed_server,                "ed_server_attr_work",           NORMAL},
  {prepare_client,           "prepare_client_attr_work",      NORMAL},
  {defaults_via_connstring,  "attr_defaults_via_conn_string", NORMAL},
  {NULL, NULL, 0}
};


int main(int argc, char **argv)
{
  int ret, tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;

  get_options(argc, argv);

  plan(tests);

  ret= run_tests(my_tests);

  return ret;
}
