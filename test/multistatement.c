/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013, 2015 MariaDB Corporation AB

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

ODBC_TEST(test_multi_statements)
{
  long num_inserted;
  SQLRETURN rc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t1 (a int)");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t1 VALUES(1);INSERT INTO t1 VALUES(2)");

  SQLRowCount(Stmt, &num_inserted);
  diag("inserted: %d", num_inserted);
  FAIL_IF(num_inserted != 1, "Expected 1 row inserted");
  
  rc= SQLMoreResults(Stmt);
  num_inserted= 0;
  rc= SQLRowCount(Stmt, &num_inserted);
  FAIL_IF(num_inserted != 1, "Expected 1 row inserted");

  rc= SQLMoreResults(Stmt);
 FAIL_IF(rc != SQL_NO_DATA, "expected no more results");

  return OK;
}

ODBC_TEST(test_multi_on_off)
{
  SQLHENV myEnv;
  SQLHDBC myDbc;
  SQLHSTMT myStmt;
  SQLRETURN rc;

  my_options= 0;
  ODBC_Connect(&myEnv, &myDbc, &myStmt);

  rc= SQLPrepare(myStmt, "DROP TABLE IF EXISTS t1; CREATE TABLE t1(a int)", SQL_NTS);
  FAIL_IF(SQL_SUCCEEDED(rc), "Error expected"); 

  ODBC_Disconnect(myEnv, myDbc, myStmt);

  my_options= 67108866;
  ODBC_Connect(&myEnv, &myDbc, &myStmt);

  rc= SQLPrepare(myStmt, "DROP TABLE IF EXISTS t1; CREATE TABLE t1(a int)", SQL_NTS);
  FAIL_IF(!SQL_SUCCEEDED(rc), "Success expected"); 

  ODBC_Disconnect(myEnv, myDbc, myStmt);
  return OK;
}


ODBC_TEST(test_params)
{
  SQLRETURN rc;
  int i, j;

  rc= SQLExecDirect(Stmt, "DROP TABLE IF EXISTS t1; CREATE TABLE t1(a int)", SQL_NTS);
  FAIL_IF(!SQL_SUCCEEDED(rc), "unexpected error"); 

  rc= SQLExecDirect(Stmt, "DROP TABLE IF EXISTS t2; CREATE TABLE t2(a int)", SQL_NTS);
  FAIL_IF(!SQL_SUCCEEDED(rc), "unexpected error"); 

  rc= SQLPrepare(Stmt, "INSERT INTO t1 VALUES (?), (?)", SQL_NTS);
  CHECK_STMT_RC(Stmt, rc);

  CHECK_STMT_RC(Stmt, SQLBindParam(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 10, 0, &i, NULL));

  rc= SQLBindParam(Stmt, 2, SQL_C_LONG, SQL_INTEGER, 10, 0, &j, NULL);
  FAIL_IF(!SQL_SUCCEEDED(rc), "unexpected error"); 

  for (i=0; i < 100; i++)
  {
    j= i + 100;
    CHECK_STMT_RC(Stmt, SQLExecute(Stmt)); 

    while (SQLMoreResults(Stmt) == SQL_SUCCESS);
  }

  return OK;
}


#define TEST_MAX_PS_COUNT 25
ODBC_TEST(test_odbc16)
{
  long num_inserted, i;
  SQLRETURN rc;
  int prev_ps_count, curr_ps_count, increment= 0, no_increment_iterations= 0;

  /* This would be valuable for the test, as driver could even crash when PS number is exhausted.
     But changing such sensible variable in production environment can lead to a problem (if
     the test crashes and default value is not restored). Should not be run done by default */
  /* set_variable(GLOBAL, "max_prepared_stmt_count", TEST_MAX_PS_COUNT - 2); */

  GET_SERVER_STATUS(prev_ps_count, GLOBAL, "Prepared_stmt_count" );

  for (i= 0; i < TEST_MAX_PS_COUNT; ++i)
  {
    OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t1");
    OK_SIMPLE_STMT(Stmt, "CREATE TABLE t1 (a int)");

    OK_SIMPLE_STMT(Stmt, "INSERT INTO t1 VALUES(1);INSERT INTO t1 VALUES(2)");

    SQLRowCount(Stmt, &num_inserted);
    FAIL_IF(num_inserted != 1, "Expected 1 row inserted");
  
    rc= SQLMoreResults(Stmt);
    num_inserted= 0;
    rc= SQLRowCount(Stmt, &num_inserted);
    FAIL_IF(num_inserted != 1, "Expected 1 row inserted");

    rc= SQLMoreResults(Stmt);
    FAIL_IF(rc != SQL_NO_DATA, "expected no more results");
    SQLFreeStmt(Stmt, SQL_CLOSE);

    GET_SERVER_STATUS(curr_ps_count, GLOBAL, "Prepared_stmt_count" );

    if (i == 0)
    {
      increment= curr_ps_count - prev_ps_count;
    }
    else
    {
      /* If only test ran on the server, then increment would be constant on each iteration */
      if (curr_ps_count - prev_ps_count != increment)
      {
        fprintf(stdout, "# This test makes sense to run only on dedicated server!\n");
        return SKIP;
      }
    }

    prev_ps_count=  curr_ps_count;

    /* Counting iterations not incrementing ps count on the server to minimize chances
       that other processes influence that */
    if (increment == 0)
    {
      if (no_increment_iterations >= 10)
      {
        return OK;
      }
      else
      {
        ++no_increment_iterations;
      }
    }
    else
    {
      no_increment_iterations= 0;
    }
  }

  diag("Server's open PS count is %d(on %d iterations)", curr_ps_count, TEST_MAX_PS_COUNT);
  return FAIL;
}
#undef TEST_MAX_PS_COUNT


MA_ODBC_TESTS my_tests[]=
{
  {test_multi_statements, "test_multi_statements"},
  {test_multi_on_off, "test_multi_on_off"},
//  {test_noparams, "test_noparams"},
  {test_params, "test_params"},
  {test_odbc16, "test_odbc16"},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  my_options= 67108866;
  get_options(argc, argv);
  plan(tests);
  mark_all_tests_normal(my_tests);
  return run_tests(my_tests);
}
