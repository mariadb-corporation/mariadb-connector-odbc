/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013, 2019 MariaDB Corporation AB

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
  SQLLEN    num_inserted;
  SQLRETURN rc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t1 (a int)");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t1 VALUES(1);INSERT INTO t1 VALUES(2), (3)");

  SQLRowCount(Stmt, &num_inserted);
  diag("inserted: %ld", (long)num_inserted);
  FAIL_IF(num_inserted != 1, "Expected 1 row inserted");
  
  rc= SQLMoreResults(Stmt);
  num_inserted= 0;
  rc= SQLRowCount(Stmt, &num_inserted);
  FAIL_IF(num_inserted != 2, "Expected 2 row inserted");

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
  //add_connstr= "";
  ODBC_Connect(&myEnv, &myDbc, &myStmt);

  rc= SQLPrepare(myStmt, (SQLCHAR*)"DROP TABLE IF EXISTS t1; CREATE TABLE t1(a int)", SQL_NTS);
  FAIL_IF(SQL_SUCCEEDED(rc), "Error expected"); 

  ODBC_Disconnect(myEnv, myDbc, myStmt);

  my_options= 67108866;
  ODBC_Connect(&myEnv, &myDbc, &myStmt);

  rc= SQLPrepare(myStmt, (SQLCHAR*)"DROP TABLE IF EXISTS t1; CREATE TABLE t1(a int)", SQL_NTS);
  FAIL_IF(!SQL_SUCCEEDED(rc), "Success expected");

  ODBC_Disconnect(myEnv, myDbc, myStmt);
  return OK;
}


ODBC_TEST(test_params)
{
  int       i, j;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t1; CREATE TABLE t1(a int)");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t2; CREATE TABLE t2(a int)");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR*)"INSERT INTO t1 VALUES (?), (?)", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 10, 0, &i, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 10, 0, &j, 0, NULL));


  for (i=0; i < 100; i++)
  {
    j= i + 100;
    CHECK_STMT_RC(Stmt, SQLExecute(Stmt)); 

    while (SQLMoreResults(Stmt) == SQL_SUCCESS);
  }

  return OK;
}

ODBC_TEST(test_params_last_count_smaller)
{
  int       i, j, k;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t1; CREATE TABLE t1(a int, b int)");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t2; CREATE TABLE t2(a int)");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR*)"INSERT INTO t1 VALUES (?,?); INSERT INTO t2 VALUES (?)", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 10, 0, &i, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 10, 0, &j, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 10, 0, &k, 0, NULL));

  for (i=0; i < 100; i++)
  {
    j= i + 100;
    k= i + 1000;
    CHECK_STMT_RC(Stmt, SQLExecute(Stmt)); 

    while (SQLMoreResults(Stmt) == SQL_SUCCESS);
  }

  return OK;
}


#define TEST_MAX_PS_COUNT 25
ODBC_TEST(t_odbc_16)
{
  SQLLEN    num_inserted;
  SQLRETURN rc;
  int       i, prev_ps_count, curr_ps_count, increment= 0, no_increment_iterations= 0;

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
    is_num(num_inserted, 1);
  
    rc= SQLMoreResults(Stmt);
    num_inserted= 0;
    rc= SQLRowCount(Stmt, &num_inserted);
    is_num(num_inserted, 1);

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

/* Test that connector does not get confused by ; inside string */
ODBC_TEST(test_semicolon)
{
  SQLCHAR val[64];

  OK_SIMPLE_STMT(Stmt, "SELECT \"Semicolon ; insert .. er... inside string\"");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, val, 1), "Semicolon ; insert .. er... inside string", sizeof("Semicolon ; insert .. er... inside string"));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}

/* Double quote inside single quotes caused error in parsing while
   Also tests ODBC-97*/
ODBC_TEST(t_odbc74)
{
  SQLCHAR ref[][4]= {"\"", "'", "*/", "/*", "end", "one\\", "two\\"}, val[8];
  unsigned int i;
  SQLHDBC     hdbc1;
  SQLHSTMT    Stmt1;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc74; CREATE TABLE odbc74(id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,\
                        val VARCHAR(64) NOT NULL)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO odbc74 (val) VALUES('\"');INSERT INTO odbc74 (val) VALUES(\"'\");\
                        /*\"*//*'*//*/**/INSERT INTO odbc74 (val) VALUES('*/');\
                        # Pound-sign comment\"'--; insert into non_existent VALUES(1)\n\
                        # 2 lines of comments \"'--;\n\
                        INSERT INTO odbc74 (val) VALUES('/*');-- comment\"'; insert into non_existent VALUES(1)\n\
                        INSERT INTO odbc74 (val) VALUES('end')\n\
                        # ;Unhappy comment at the end ");
  OK_SIMPLE_STMT(Stmt, "-- comment ;1 \n\
                        # comment ;2 \n\
                        INSERT INTO odbc74 (val) VALUES('one\\\\');\
                        INSERT INTO odbc74 (val) VALUES(\"two\\\\\");");
  OK_SIMPLE_STMT(Stmt, "SELECT val FROM odbc74 ORDER BY id");

  for (i= 0; i < sizeof(ref)/sizeof(ref[0]); ++i)
  {
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    IS_STR(my_fetch_str(Stmt, val, 1), ref[i], sizeof(ref[i]));
  }
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "TRUNCATE TABLE odbc74");

  AllocEnvConn(&Env, &hdbc1);
  Stmt1= DoConnect(hdbc1, FALSE, NULL, NULL, NULL, 0, NULL, 0, NULL, NULL);
  FAIL_IF(Stmt1 == NULL, "Could not connect and/or allocate");

  OK_SIMPLE_STMT(Stmt1, "SET @@SESSION.sql_mode='NO_BACKSLASH_ESCAPES'");

  OK_SIMPLE_STMT(Stmt1, "INSERT INTO odbc74 (val) VALUES('one\\');\
                        INSERT INTO odbc74 (val) VALUES(\"two\\\");");
  OK_SIMPLE_STMT(Stmt1, "SELECT val FROM odbc74 ORDER BY id");

  /* We only have 2 last rows */
  for (i= sizeof(ref)/sizeof(ref[0]) - 2; i < sizeof(ref)/sizeof(ref[0]); ++i)
  {
    CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
    IS_STR(my_fetch_str(Stmt1, val, 1), ref[i], sizeof(ref[i]));
  }
  EXPECT_STMT(Stmt1, SQLFetch(Stmt1), SQL_NO_DATA);

  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc74");

  return OK;
}

ODBC_TEST(t_odbc95)
{
  EXPECT_STMT(Stmt, SQLPrepare(Stmt, (SQLCHAR*)"SELECT 1;INSERT INTO non_existing VALUES(2)", SQL_NTS), SQL_ERROR);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_DROP));
  Stmt= NULL;

  return OK;
}

ODBC_TEST(t_odbc126)
{
  SQLCHAR Query[][24]= { "CALL odbc126_1", "CALL odbc126_2", "SELECT 1, 2; SELECT 3", "SELECT 4; SELECT 5,6" };
  unsigned int i, ExpectedRows[]= {3, 3, 1, 1}, resCount;
  SQLLEN affected;
  SQLRETURN rc, Expected= SQL_SUCCESS;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc126");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS odbc126_1");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS odbc126_2");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE odbc126(col1 INT, col2 INT)");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE odbc126_1()\
                        BEGIN\
                          SELECT col1, col2 FROM odbc126;\
                          SELECT col1 FROM odbc126;\
                        END");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE odbc126_2()\
                        BEGIN\
                          SELECT col1 FROM odbc126;\
                          SELECT col1, col2 FROM odbc126;\
                        END");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO odbc126 VALUES(1, 2), (3, 4), (5, 6)");

  for (i= 0; i < sizeof(Query)/sizeof(Query[0]); ++i)
  {
    OK_SIMPLE_STMT(Stmt, Query[i]);
    resCount= 0;
    do {
      if (i > 1 || resCount < 2)
      {
        is_num(my_print_non_format_result_ex(Stmt, FALSE), ExpectedRows[i]);
      }
      else if (i < 2)
      {
        CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &affected));
        is_num(affected, 0);
        Expected= SQL_NO_DATA;
      }
      rc= SQLMoreResults(Stmt);
      is_num(rc, Expected);
      ++resCount;

      if (i > 1)
      {
        Expected= SQL_NO_DATA;
      }
    } while (rc != SQL_NO_DATA);

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
    Expected= SQL_SUCCESS;
  }
  
  OK_SIMPLE_STMT(Stmt, "DROP TABLE odbc126");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE odbc126_1");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE odbc126_2");

  return OK;
}

ODBC_TEST(diff_column_binding)
{
  char bindc1[64];
  int bind1, bind2, bind3;
  SQLBIGINT bindb1;
  SQLRETURN Expected= SQL_SUCCESS;
  SQLLEN indicator = 0, indicator2 = 0, indicator3 = 0, indicator4 = 0;
  SQLLEN indicatorc = SQL_NTS;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS diff_column_binding");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS diff_column_binding_1");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE diff_column_binding(col1 INT, col2 VARCHAR(64), col3 BIGINT unsigned)");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE diff_column_binding_1()\
                        BEGIN\
                          SELECT 1017, 1370;\
                          SELECT * FROM diff_column_binding;\
                        END");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO diff_column_binding VALUES(1370, \"abcd\", 12345), (1417, \"abcdef\", 2390), (1475, \"@1475\", 0)");
  OK_SIMPLE_STMT(Stmt, "CALL diff_column_binding_1");

  // bind first result set
  SQLBindCol(Stmt, 1, SQL_C_LONG, &bind1, sizeof(int), &indicator);
  SQLBindCol(Stmt, 2, SQL_C_LONG, &bind2, sizeof(int), &indicator2);
  SQLFetch(Stmt);
  is_num(bind1, 1017);
  is_num(bind2, 1370);

  SQLMoreResults(Stmt);

  // bind second result set
  SQLBindCol(Stmt, 1, SQL_C_LONG, &bind3, sizeof(int), &indicator3);
  SQLBindCol(Stmt, 2, SQL_C_CHAR, bindc1, sizeof(bindc1), &indicatorc);
  SQLBindCol(Stmt, 3, SQL_C_SBIGINT, &bindb1, sizeof(SQLBIGINT), &indicator4);
  SQLFetch(Stmt);
  is_num(bind3, 1370);
  is_num(strcmp(bindc1, "abcd"), 0);
  is_num(bindb1, 12345);
  SQLFetch(Stmt);
  is_num(bind3, 1417);
  is_num(strcmp(bindc1, "abcdef"), 0);
  is_num(bindb1, 2390);
  SQLFetch(Stmt);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE diff_column_binding");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE diff_column_binding_1");

  return OK;
}


ODBC_TEST(t_odbc159)
{
  unsigned int j= 0, ExpectedRows[]= {0, 0, 5};
  SQLLEN Rows, ExpRowCount[]= {0, 0, 0};
  SQLSMALLINT ColumnsCount, expCols[]= {0,0,16};
  SQLRETURN rc;

  if (ServerNotOlderThan(Connection, 10, 6, 0))
  {
    /* INFORMATION_SCHEMA.STATISTICS has 17 columns in 10.6 */
    expCols[2]= 17;
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS _temp_odbc159;\
                        CREATE TEMPORARY TABLE _temp_odbc159 AS(SELECT * FROM INFORMATION_SCHEMA.STATISTICS);\
                        SELECT * FROM _temp_odbc159 LIMIT 5;");

  do {
    CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &Rows));
    if (j == 1)
    {
      diag("Rows in created table: %lld\n", (long long)Rows);
      ExpRowCount[2]= Rows < ExpRowCount[2] ? Rows : ExpRowCount[2];
    }
    else
    {
      is_num(Rows, ExpRowCount[j]);
    }
    
    CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &ColumnsCount));
    is_num(ColumnsCount, expCols[j]);

    if (!iOdbc() || j != 2)
    {
      is_num(ma_print_result_getdata_ex(Stmt, FALSE), ExpectedRows[j]);
    }

    rc= SQLMoreResults(Stmt);
    ++j;
  } while (rc != SQL_NO_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS _temp_odbc159");

  return OK;
}


ODBC_TEST(t_odbc177)
{
  SQLLEN count;

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS odbc177");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS odbc177_1");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc177");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc177_non_existent");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc177(col1 INT)");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE odbc177()\
                        BEGIN\
                          SELECT 1;\
                          SELECT 2 as id, 'val' as `Value` FROM dual WHERE 1=0;\
                          INSERT INTO t_odbc177 VALUES(3);\
                          DELETE FROM t_odbc177 WHERE 1=0;\
                          SELECT 5, 4;\
                          DELETE FROM t_odbc177;\
                          SELECT * FROM odbc177_non_existent;\
                          SELECT 6;\
                        END");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE odbc177_1()\
                        BEGIN\
                          SELECT 1;\
                          SELECT 2, 'val' FROM dual WHERE 1=0;\
                          INSERT INTO t_odbc177 VALUES(3), (7);\
                          DELETE FROM t_odbc177 WHERE 1=0;\
                          SELECT 5, 4;\
                          SELECT 6;\
                          DELETE FROM t_odbc177;\
                        END");

  OK_SIMPLE_STMT(Stmt, "CALL odbc177()");
  /* First SELECT */
  is_num(my_print_non_format_result_ex(Stmt, FALSE), 1);
  is_num(SQLMoreResults(Stmt), SQL_SUCCESS);

  /* 2nd SELECT */
  is_num(my_print_non_format_result_ex(Stmt, FALSE), 0);
  is_num(SQLMoreResults(Stmt), SQL_SUCCESS);

  /* 3rd SELECT */
  is_num(my_print_non_format_result_ex(Stmt, FALSE), 1);

  /* 4th SELECT - error */
  is_num(SQLMoreResults(Stmt), SQL_ERROR);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Now without error, and verifying that we read last result with total affected rows */
  OK_SIMPLE_STMT(Stmt, "CALL odbc177_1()");
  /* First SELECT */
  is_num(my_print_non_format_result_ex(Stmt, FALSE), 1);
  is_num(SQLMoreResults(Stmt), SQL_SUCCESS);

  /* 2nd SELECT */
  is_num(my_print_non_format_result_ex(Stmt, FALSE), 0);
  is_num(SQLMoreResults(Stmt), SQL_SUCCESS);

  /* 3rd SELECT */
  is_num(my_print_non_format_result_ex(Stmt, FALSE), 1);
  is_num(SQLMoreResults(Stmt), SQL_SUCCESS);

  /* 4th SELECT */
  is_num(my_print_non_format_result_ex(Stmt, FALSE), 1);
  is_num(SQLMoreResults(Stmt), SQL_SUCCESS);

  /* Final result with affected rows */
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &count));
  /* Starting from 10.3 we have total number of affected (by SP) rows*/
  if (ServerNotOlderThan(Connection, 10, 3, 0))
  {
    is_num(count, 4); /* 1 inserted, 1 deleted */
  }
  else /* In earlier servers that is only for last statement in SP */
  {
    is_num(count, 2);
  }
  is_num(SQLMoreResults(Stmt), SQL_NO_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc177");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE odbc177");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE odbc177_1");

  return OK;
}


ODBC_TEST(t_odbc169)
{
  SQLCHAR Query[][80]= {"SELECT 1 Col1; SELECT * from t_odbc169", "SELECT * FROM t_odbc169 ORDER BY col1 DESC; SELECT col3, col2 from t_odbc169",
                        "INSERT INTO t_odbc169 VALUES(8, 7, 'Row #4');SELECT * from t_odbc169"};
  char Expected[][3][7]={ {"1", "", "" },       /* RS 1*/
                          {"1", "2", "Row 1"},  /* RS 2*/
                          {"3", "4", "Row 2"},
                          {"5", "6", "Row 3"},
                          {"5", "6", "Row 3"},  /* RS 3*/
                          {"3", "4", "Row 2"},
                          {"1", "2", "Row 1"},
                          {"Row 1", "2" , ""},  /* RS 4*/
                          {"Row 2", "4" , ""},
                          {"Row 3", "6" , ""},

                          /* RS 5 is empty */
                          {"1", "2", "Row 1" }, /* RS 6*/
                          {"3", "4", "Row 2" },
                          {"5", "6", "Row 3" },
                          {"8", "7", "Row #4"}
                        };
  unsigned int i, RsIndex= 0, ExpectedRows[]= {1, 3, 3, 3, 0, 4};
  SQLLEN Rows, ExpRowCount[]= {0, 0, 0, 0, 1, 0};
  SQLSMALLINT ColumnsCount, expCols[]= {1, 3, 3, 2, 0, 3};
  SQLRETURN rc;
  SQLSMALLINT Column, Row= 0;
  SQLCHAR     ColumnData[MAX_ROW_DATA_LEN]={ 0 };

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc169");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc169(col1 INT, col2 INT, col3 varchar(32) not null)");
  
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc169 VALUES(1, 2, 'Row 1'),(3, 4, 'Row 2'), (5, 6, 'Row 3')");

  for (i= 0; i < sizeof(Query)/sizeof(Query[0]); ++i)
  {
    OK_SIMPLE_STMT(Stmt, Query[i]);

    do {
      CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &Rows));
      is_num(Rows, ExpRowCount[RsIndex]);
      CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &ColumnsCount));
      is_num(ColumnsCount, expCols[RsIndex]);

      if (iOdbc() && RsIndex == 5)
      {
        diag("Skipping values check in the last resultset, because of the bug in the iODBC");
        break;
      }

      Rows= 0;
      while (SQL_SUCCEEDED(SQLFetch(Stmt)))
      {
        for (Column= 0; Column < ColumnsCount; ++Column)
        {
          IS_STR(my_fetch_str(Stmt, ColumnData, Column + 1), Expected[Row][Column], strlen(Expected[Row][Column]));
        }
        ++Row;
        ++Rows;
      }
      is_num(Rows, ExpectedRows[RsIndex]);

      rc= SQLMoreResults(Stmt);
      ++RsIndex;
    } while (rc != SQL_NO_DATA);

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc169");

  return OK;
}


ODBC_TEST(t_odbc219)
{
  SQLSMALLINT ColumnCount;
  SQLCHAR ColumnName[8], Query[][80]= {"CALL odbc219()", "SELECT 1 Col1; INSERT INTO t_odbc219 VALUES(2)"};
  unsigned int i;
  long long ExpectedColCount[]= {2, 1};

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc219");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS odbc219");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc219(col1 INT)");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE odbc219()\
                        BEGIN\
                          SELECT 1 as id, 'text' as val;\
                        END");
  for (i= 0; i < sizeof(Query)/sizeof(Query[0]); ++i)
  {
    OK_SIMPLE_STMT(Stmt, Query[i]);
    CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &ColumnCount));
    is_num(ColumnCount, ExpectedColCount[i]);
    my_print_non_format_result_ex(Stmt, FALSE);
    CHECK_STMT_RC(Stmt, SQLMoreResults(Stmt));

    CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &ColumnCount));
    is_num(ColumnCount, 0);
    EXPECT_STMT(Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_NAME, (SQLPOINTER)ColumnName, sizeof(ColumnName), NULL, NULL), SQL_ERROR);

    EXPECT_STMT(Stmt, SQLMoreResults(Stmt), SQL_NO_DATA);

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc219");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE odbc219");

  return OK;
}

/* As part of ODBC-313 - if multistmt is allowed, all queries run at connect time are assembled in single batch.
   This test verifies, that all is fine in this case */
ODBC_TEST(test_autocommit)
{
  SQLHDBC Dbc;
  SQLHSTMT Stmt;
  SQLUINTEGER ac;
  unsigned long noMsOptions= my_options & (~67108864);
  SQLCHAR tracked[256];

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Dbc));
  CHECK_DBC_RC(Dbc, SQLSetConnectOption(Dbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON));
  /* First testing with multistatements off */
  Stmt = DoConnect(Dbc, FALSE, NULL, NULL, NULL, 0, NULL, &noMsOptions, NULL, "INITSTMT={SELECT 1}");
  FAIL_IF(Stmt == NULL, "Connection error");

  CHECK_DBC_RC(Dbc, SQLGetConnectOption(Dbc, SQL_AUTOCOMMIT, (SQLPOINTER)&ac));
  is_num(ac, SQL_AUTOCOMMIT_ON);
  OK_SIMPLE_STMT(Stmt, "SELECT @@autocommit");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_DROP));
  CHECK_DBC_RC(Dbc, SQLDisconnect(Dbc));

  CHECK_DBC_RC(Dbc, SQLSetConnectOption(Dbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF));

  Stmt = DoConnect(Dbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "INITSTMT={SELECT 1}");
  FAIL_IF(Stmt == NULL, "Connection error");

  CHECK_DBC_RC(Dbc, SQLGetConnectOption(Dbc, SQL_AUTOCOMMIT, (SQLPOINTER)&ac));
  is_num(ac, SQL_AUTOCOMMIT_OFF);
  OK_SIMPLE_STMT(Stmt, "SELECT @@autocommit");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 0);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  if (SQL_SUCCEEDED(SQLExecDirect(Stmt, "SELECT @@session_track_system_variables", SQL_NTS)))
  {
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_CHAR, tracked, sizeof(tracked), NULL));
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

    OK_SIMPLE_STMT(Stmt, "SET autocommit=1;");
    CHECK_DBC_RC(Dbc, SQLGetConnectOption(Dbc, SQL_AUTOCOMMIT, (SQLPOINTER)&ac));
    is_num(ac, SQL_AUTOCOMMIT_ON);
  }
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_DROP));
  CHECK_DBC_RC(Dbc, SQLDisconnect(Dbc));

  CHECK_DBC_RC(Dbc, SQLSetConnectOption(Dbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON));

  Stmt = DoConnect(Dbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, "INITSTMT={SELECT 1}");
  FAIL_IF(Stmt == NULL, "Connection error");

  CHECK_DBC_RC(Dbc, SQLGetConnectOption(Dbc, SQL_AUTOCOMMIT, (SQLPOINTER)&ac));
  is_num(ac, SQL_AUTOCOMMIT_ON);
  OK_SIMPLE_STMT(Stmt, "SELECT @@autocommit");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_DROP));
  CHECK_DBC_RC(Dbc, SQLDisconnect(Dbc));
  CHECK_DBC_RC(Dbc, SQLFreeConnect(Dbc));

  return OK;
}

MA_ODBC_TESTS my_tests[]=
{
  {test_multi_statements, "test_multi_statements"},
  {test_multi_on_off, "test_multi_on_off"},
  {test_params, "test_params"},
  {test_params_last_count_smaller, "test_params_last_count_smaller"},
  {t_odbc_16, "test_odbc_16"},
  {test_semicolon, "test_semicolon_in_string"},
  {t_odbc74, "t_odbc74and_odbc97"},
  {t_odbc95, "t_odbc95"},
  {t_odbc126, "t_odbc126"},
  {diff_column_binding, "diff_column_binding"},
  {t_odbc159, "t_odbc159"},
  {t_odbc177, "t_odbc177"},
  {t_odbc169, "t_odbc169"},
  {t_odbc219, "t_odbc219"},
  {test_autocommit, "test_autocommit"},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  CHANGE_DEFAULT_OPTIONS(67108866);
  get_options(argc, argv);
  plan(tests);
  mark_all_tests_normal(my_tests);

  return run_tests(my_tests);
}
