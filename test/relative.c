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

/* Testing SQL_FETCH_RELATIVE with row_set_size as 10 */
ODBC_TEST(t_relative)
{
  SQLUINTEGER i, iarray[15];
  SQLULEN nrows, index;
  SQLCHAR name[21];

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_relative");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_relative (id INT, name CHAR(20))");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt,
                            (SQLCHAR *)"INSERT INTO t_relative VALUES (?,?)",
                            SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
                                  SQL_INTEGER, 0, 0, &i, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_CHAR, 20, 0, name, 20, NULL));

  for (i= 1; i <= 50; i++)
  {
    sprintf((char *)name, "my%d", i);
    CHECK_STMT_RC(Stmt, SQLExecute(Stmt));
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* set row size as 10 */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)10, 0));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &nrows, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_relative");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_ULONG, &iarray, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0)); /* 1-10 */
  IS_NUM(nrows, 10);

  for (index= 1; index <= nrows; index++)
    IS_NUM(iarray[index - 1], index);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0)); /* 10-20 */
  IS_NUM(nrows, 10);

  for (index= 1; index <= nrows; index++)
    IS_NUM(iarray[index - 1], index + 10);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_PREV, 0)); /* 1-10 */
  IS_NUM(nrows, 10);

  for (index= 1; index <= nrows; index++)
    IS_NUM(iarray[index - 1], index);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 1)); /* 2-11 */
  IS_NUM(nrows, 10);

  for (index= 1; index <= nrows; index++)
    IS_NUM(iarray[index - 1], index + 1);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1)); /* 1-10 */
  IS_NUM(nrows, 10);

  for (index= 1; index <= nrows; index++)
    IS_NUM(iarray[index - 1], index);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 0)); /* 1-10 */
  IS_NUM(nrows, 10);

  for (index= 1; index <= nrows; index++)
    IS_NUM(iarray[index - 1], index);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1) != SQL_NO_DATA_FOUND, "expected eof"); /* BOF */

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 1)); /* 1-10 */
  IS_NUM(nrows, 10);

  for (index= 1; index <= nrows; index++)
    IS_NUM(iarray[index - 1], index);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* reset row size */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)1, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_relative");

  return OK;
}


/* Testing SQL_FETCH_RELATIVE with row_set_size as 1 */
ODBC_TEST(t_relative1)
{
  SQLULEN nrows;
  SQLUINTEGER i;
  const SQLUINTEGER max_rows= 10;

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_relative1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_relative1 (id INT)");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt,
                            (SQLCHAR *)"INSERT INTO t_relative1 VALUES (?)",
                            SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
                                  SQL_INTEGER, 0, 0, &i, 0, NULL));

  for (i= 1; i <= max_rows; i++)
  {
    CHECK_STMT_RC(Stmt, SQLExecute(Stmt));
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* set row_size as 1 */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)1, 0));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &nrows, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_relative1");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &i, 0, NULL));

  /* row 1 */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));
  IS_NUM(i, 1);

  /* Before start */
  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1) != SQL_NO_DATA_FOUND, "expected eof");

  /* jump to last row */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, max_rows));
  IS_NUM(i, max_rows);

  /* jump to last row+1 */
  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 1) != SQL_NO_DATA_FOUND, "expected eof");

  /* goto first row */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1));
  IS_NUM(i, 1);

  /* before start */
  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1) != SQL_NO_DATA_FOUND, "Expected eof");

  /* goto fifth  row */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 5));
  IS_NUM(i, 5);

  /* goto after end */
  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, max_rows) != SQL_NO_DATA_FOUND, "Expected EOF");

  /* the scenarios from ODBC spec */

  /* CASE 1 */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1));
  IS_NUM(i, 1);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1) != SQL_NO_DATA_FOUND, "expected eof");

  /* BeforeStart AND FetchOffset <= 0 */
  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -20) !=
              SQL_NO_DATA_FOUND, "expected eof");

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1) !=
              SQL_NO_DATA_FOUND, "expected eof");

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 0) !=
              SQL_NO_DATA_FOUND, "expected eof");

  /* case 1: Before start AND FetchOffset > 0 */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 1));
  IS_NUM(i, 1);

  /* CASE 2 */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_LAST, 1));
  IS_NUM(i, max_rows);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 1) !=
              SQL_NO_DATA_FOUND, "expected eof");

  /* After end AND FetchOffset >= 0 */
  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 10) !=
              SQL_NO_DATA_FOUND, "expected eof");

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 20) !=
              SQL_NO_DATA_FOUND, "expected eof");

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 1) !=
              SQL_NO_DATA_FOUND, "expected eof");

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 0) !=
              SQL_NO_DATA_FOUND, "expected eof");

  /* After end AND FetchOffset < 0 */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1));
  IS_NUM(i, max_rows);

  /* CASE 3 */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1));
  IS_NUM(i, 1);

  /* CurrRowsetStart = 1 AND FetchOffset < 0 */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 0));
  IS_NUM(i, 1);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1) !=
              SQL_NO_DATA_FOUND, "expected eof");

  /* CASE 4 */
  /* CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
     | FetchOffset | > RowsetSize
  */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1));
  IS_NUM(i, 1);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 3));
  IS_NUM(i, 4);

  /* the following call satisfies 4 > 1 AND (3-4) < 1 AND |-4| > 1 */
  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -4) !=
              SQL_NO_DATA_FOUND, "expected eof");

  /* CASE 5 */
  /* 1 <= CurrRowsetStart + FetchOffset <= LastResultRow */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1));
  IS_NUM(i, 1);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 5));
  IS_NUM(i, 6);

  /* 1 <= 6-2 <= 10 */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -2));
  IS_NUM(i, 4);

  /* CASE 6 */
  /*  CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
      | FetchOffset | <= RowsetSize
   */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1));
  IS_NUM(i, 1);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 3));
  IS_NUM(i, 4);

  /* 4 >1 AND 4-4 <1 AND |-4| <=10 */
  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -4) !=
              SQL_NO_DATA_FOUND, "expected eof");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_relative1");

  return OK;
}


/* Testing SQL_FETCH_RELATIVE with row_set_size as 2 */
ODBC_TEST(t_relative2)
{
  SQLRETURN rc;
  SQLULEN nrows;
  SQLUINTEGER i, iarray[15];
  const SQLUINTEGER max_rows=10;

    CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                  (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_relative2");

  OK_SIMPLE_STMT(Stmt, "create table t_relative2(id int)");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt,
                            (SQLCHAR *) "insert into t_relative2 values(?)",
                            SQL_NTS));

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT, SQL_C_ULONG,
                          SQL_INTEGER,0,0,&i,0,NULL);
    CHECK_STMT_RC(Stmt,rc);

    for ( i = 1; i <= max_rows; i++ )
    {
        rc = SQLExecute(Stmt);
        CHECK_STMT_RC(Stmt,rc);
    }

    SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt,SQL_CLOSE);

    rc = SQLEndTran(SQL_HANDLE_DBC,Connection,SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    /* set row_size as 2 */
    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)2,0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROWS_FETCHED_PTR,&nrows,0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "select * from t_relative2");
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&iarray,0,NULL);
    CHECK_STMT_RC(Stmt,rc);

    /* row 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);/* 1 */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==1);
    assert(iarray[1]==2);


    /* Before start */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /* jump to last row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,max_rows);/* last row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 1);
    assert(iarray[0]==max_rows);

    /* jump to last row+1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* after last */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /* goto first row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* 1 */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==1);
    assert(iarray[1]==2);

    /* before start */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /* goto fifth  row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,5);/* 5 */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==5);
    assert(iarray[1]==6);

    /* goto after end */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,max_rows);/* after last */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /*
       the scenarios from ODBC spec
    */

    /* CASE 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* 1 */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==1);
    assert(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /* BeforeStart AND FetchOffset <= 0 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-20);/* before start */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,0);/* before start */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /* case 1: Before start AND FetchOffset > 0 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* 1 */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==1);
    assert(iarray[1]==2);

    /* CASE 2 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_LAST,1);/* last row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==max_rows-1);
    assert(iarray[1]==max_rows);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* last row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 1);
    assert(iarray[0]==max_rows);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* after last row */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /* After end AND FetchOffset >= 0 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,10);/* after end */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,20);/* after end */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* after end */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,0);/* after end */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /* After end AND FetchOffset < 0 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* last row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 1);
    assert(iarray[0]==max_rows);


    /* CASE 3 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==1);
    assert(iarray[1]==2);

    /* CurrRowsetStart = 1 AND FetchOffset < 0 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,0);/* first row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==1);
    assert(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /* CASE 4 */
    /* CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
       | FetchOffset | > RowsetSize
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==1);
    assert(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,3);/* fourth row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==4);
    assert(iarray[1]==5);

    /* the following call satisfies 4 > 1 AND (3-4) < 1 AND |-4| > 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-4);/* before start */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");

    /* CASE 5 */
    /* 1 <= CurrRowsetStart + FetchOffset <= LastResultRow */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==1);
    assert(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,5);/* sixth row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==6);
    assert(iarray[1]==7);

    /* 1 <= 6-2 <= 10 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-2);/* fourth row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==4);
    assert(iarray[1]==5);

    /* CASE 6 */
    /*  CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
        | FetchOffset | <= RowsetSize
     */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==1);
    assert(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,3);/* fourth row */
    CHECK_STMT_RC(Stmt,rc);
    assert(nrows == 2);
    assert(iarray[0]==4);
    assert(iarray[1]==5);

    /* 4 >1 AND 4-4 <1 AND |-4| <=10 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-4);/* before start */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND, "expected eof");


    SQLFreeStmt(Stmt,SQL_UNBIND);
    SQLFreeStmt(Stmt,SQL_CLOSE);

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_relative2");

  return OK;
}


ODBC_TEST(t_rows_fetched_ptr)
{
    SQLRETURN    rc;
    SQLULEN rowsFetched, rowsSize;
    long         i;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_rows_fetched_ptr");

  OK_SIMPLE_STMT(Stmt,"create table t_rows_fetched_ptr(a int)");

  OK_SIMPLE_STMT(Stmt,"insert into t_rows_fetched_ptr values(0)");
  OK_SIMPLE_STMT(Stmt,"insert into t_rows_fetched_ptr values(1)");
  OK_SIMPLE_STMT(Stmt,"insert into t_rows_fetched_ptr values(2)");
  OK_SIMPLE_STMT(Stmt,"insert into t_rows_fetched_ptr values(3)");
  OK_SIMPLE_STMT(Stmt,"insert into t_rows_fetched_ptr values(4)");
  OK_SIMPLE_STMT(Stmt,"insert into t_rows_fetched_ptr values(5)");

    rowsSize= 1;
    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowsSize, 0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_rows_fetched_ptr");

    i= 0;
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        assert(rowsFetched == rowsSize);
        i++; rowsFetched= 0;
        rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    }
    assert( i == 6);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    rowsSize= 2;
    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowsSize, 0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_rows_fetched_ptr");
    CHECK_STMT_RC(Stmt,rc);

    i= 0;
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        diag("\n total rows fetched: %ld", rowsFetched);
        assert(rowsFetched == rowsSize);
        i++;rowsFetched= 0;
        rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    }
    assert( i == 3);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    rowsSize= 3;
    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowsSize, 0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_rows_fetched_ptr");
    CHECK_STMT_RC(Stmt,rc);

    i= 0;
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        diag("\n total rows fetched: %ld", rowsFetched);
        assert(rowsFetched == rowsSize);
        i++;rowsFetched= 0;
        rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    }
    assert( i == 2);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    rowsSize= 4;
    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowsSize, 0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_rows_fetched_ptr");
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    diag("\n total rows fetched: %ld", rowsFetched);
    assert(rowsFetched == rowsSize);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    diag("\n total rows fetched: %ld", rowsFetched);
    assert(rowsFetched == 2);

    rc = SQLFetch(Stmt);
    assert(rc == SQL_NO_DATA);

    SQLFreeStmt(Stmt, SQL_CLOSE);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)1, 0);/* reset */
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, NULL, 0);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_rows_fetched_ptr");

  return OK;
}


ODBC_TEST(t_rows_fetched_ptr1)
{
  SQLRETURN   rc;
  SQLULEN     rowsFetched, rowsSize;
  SQLINTEGER  i;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_rows_fetched_ptr");

  OK_SIMPLE_STMT(Stmt, "create table t_rows_fetched_ptr(a int)");
  OK_SIMPLE_STMT(Stmt, "insert into t_rows_fetched_ptr values(0)");
  OK_SIMPLE_STMT(Stmt, "insert into t_rows_fetched_ptr values(1)");
  OK_SIMPLE_STMT(Stmt, "insert into t_rows_fetched_ptr values(2)");
  OK_SIMPLE_STMT(Stmt, "insert into t_rows_fetched_ptr values(3)");
  OK_SIMPLE_STMT(Stmt, "insert into t_rows_fetched_ptr values(4)");
  OK_SIMPLE_STMT(Stmt, "insert into t_rows_fetched_ptr values(5)");

    rowsSize= 1;
    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowsSize, 0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_rows_fetched_ptr");
    CHECK_STMT_RC(Stmt,rc);

    i= 0;
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
      fprintf(stdout,"total rows fetched: %ld\n", rowsFetched);
      assert(rowsFetched == rowsSize);
      i++; rowsFetched= 0;
      rc = SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0);
    }
    assert( i == 6);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    rowsSize= 2;
    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowsSize, 0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_rows_fetched_ptr");
    CHECK_STMT_RC(Stmt,rc);

    i= 0;
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
      fprintf(stdout,"total rows fetched: %ld\n", rowsFetched);
      assert(rowsFetched == rowsSize);
      i++;rowsFetched= 0;
      rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    }
    assert( i == 3);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    rowsSize= 3;
    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowsSize, 0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_rows_fetched_ptr");
    CHECK_STMT_RC(Stmt,rc);

    i= 0;
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);
    while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
      diag("total rows fetched: %ld\n", rowsFetched);
      assert(rowsFetched == rowsSize);
      i++;rowsFetched= 0;
      rc = SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0);
    }
    assert( i == 2);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    rowsSize= 4;
    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowsSize, 0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_rows_fetched_ptr");
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    diag("total rows fetched: %ld\n", rowsFetched);
    assert(rowsFetched == rowsSize);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    diag("total rows fetched: %ld\n", rowsFetched);
    assert(rowsFetched == 2);

    rc = SQLFetch(Stmt);
    assert(rc == SQL_NO_DATA);

    SQLFreeStmt(Stmt, SQL_CLOSE);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)1, 0);/* reset */
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, NULL, 0);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_rows_fetched_ptr");

  return OK;
}



MA_ODBC_TESTS my_tests[]=
{
  {t_relative, "t_relative"},
  {t_relative1, "t_relative1"},
  {t_relative2, "t_relative2"},
  {t_rows_fetched_ptr, "t_rows_fetched_ptr"},
  {t_rows_fetched_ptr1, "t_rows_fetched_ptr1"},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}