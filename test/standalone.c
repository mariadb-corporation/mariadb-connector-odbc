// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2025 MariaDB Corporation plc

// Tests without any automatic ODBC actions from the framework
#include "tap.h"

ODBC_TEST(odbc481)
{
  AllocEnvConn(&Env, &Connection);
  Stmt= DoConnect(Connection, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL);
  FAIL_IF(Stmt == NULL, "Failed to connect or allocate statement");
  OK_SIMPLE_STMT(Stmt, "SET SESSION lc_messages='en_US'");
  EXPECT_STMT(Stmt, SQLMoreResults(Stmt), SQL_NO_DATA_FOUND);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLCancel(Stmt));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_DROP));
  CHECK_DBC_RC(Connection, SQLDisconnect(Connection));
  return OK;
}

MA_ODBC_TESTS my_tests[]=
{
  {odbc481, "odbc481-onecallcancel", NORMAL},
  {NULL, NULL, 0}
};


int main(int argc, char **argv)
{
  int ret, tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;

  get_options(argc, argv);

  plan(tests);

  ret= run_tests_bare(my_tests);

  return ret;
}
