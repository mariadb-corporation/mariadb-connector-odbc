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

/* Testing basic scrolling feature */
ODBC_TEST(t_scroll)
{
  SQLUINTEGER i;

  if (ForwardOnly == TRUE)
  {
    skip("This test cannot be run with FORWARDONLY option selected");
  }
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_scroll");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_scroll (col1 INT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_scroll VALUES (1),(2),(3),(4),(5)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_scroll");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_ULONG, &i, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_LAST, 0)); /* 5 */
  is_num(i, 5);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_PREV, 0));/* 4 */
  is_num(i, 4);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -3));/* 1 */
  is_num(i, 1);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1) != SQL_NO_DATA_FOUND, "Expected no data"); /* 0 */

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_PREV, 1) != SQL_NO_DATA_FOUND, "Expected no data"); /* 0 */

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, -1));/* 1 */
  is_num(i, 1);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, 4));/* 4 */
  is_num(i, 4);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 2) != SQL_NO_DATA_FOUND, "Expected no data"); /* 0 */

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_PREV, 2));/* last */
  is_num(i, 5);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 2) != SQL_NO_DATA_FOUND, "Expected no data");  /* last + 1 */

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, -7) != SQL_NO_DATA_FOUND, "Expected no data");  /* 0 */

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 2));/* 1 */
  is_num(i, 1);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_PREV, 2) != SQL_NO_DATA_FOUND, "Expected no data"); /* 0 */

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));/* 1 */
  is_num(i, 1);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_PREV, 0) != SQL_NO_DATA_FOUND, "Expected no data"); /* 0 */

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1) != SQL_NO_DATA_FOUND, "Expected no data"); /* 0 */

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 1));/* 1 */
  is_num(i, 1);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -1) != SQL_NO_DATA_FOUND, "Expected no data"); /* 0 */

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 1));/* 1 */
  is_num(i, 1);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 1));/* 1 */
  is_num(i, 2);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -2) != SQL_NO_DATA_FOUND, "Expected no data"); /* 0 */

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 6) != SQL_NO_DATA_FOUND, "Expected no data"); /* last + 1 */

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_PREV, 6));/* 1 */
  is_num(i, 5);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_scroll");

  return OK;
}


/* Testing SQL_FETCH_RELATIVE with row_set_size as 10 */
ODBC_TEST(t_array_relative_10)
{
  SQLRETURN rc;
  SQLINTEGER iarray[15];
  SQLLEN   nrows, index;
  SQLUINTEGER i;
  char name[21];
  if (ForwardOnly == TRUE)
  {
    skip("This test cannot be run with FORWARDONLY option selected");
  }
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_array_relative_10");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_array_relative_10(id INT,name CHAR(20))");

  rc = SQLPrepare(Stmt,(SQLCHAR *)"INSERT INTO t_array_relative_10 VALUES(?,?)",SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT, SQL_C_ULONG,
                        SQL_INTEGER,0,0,&i,0,NULL);
  CHECK_STMT_RC(Stmt,rc);
  rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT, SQL_C_CHAR,
                        SQL_CHAR,20,0,name,20,NULL);
  CHECK_STMT_RC(Stmt,rc);

  for ( i = 1; i <= 50; i++ )
  {
      sprintf(name,"my%d",i);
      rc = SQLExecute(Stmt);
      CHECK_STMT_RC(Stmt,rc);
  }

  SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
  SQLFreeStmt(Stmt,SQL_CLOSE);

  rc = SQLEndTran(SQL_HANDLE_DBC,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  /* set row size as 10 */
  rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)10,0);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROWS_FETCHED_PTR, &nrows, 0);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_array_relative_10");

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,iarray,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);/* 1-10 */
  CHECK_STMT_RC(Stmt,rc);

  diag("1-10, total rows:%ld\n",(long)nrows);

  for (index= 0; index < nrows; index++)
  {
      diag("%d %d ", index, iarray[index]);
      is_num(iarray[index], index + 1);
  }    

  rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);/* 10-20 */
  CHECK_STMT_RC(Stmt,rc);

  diag("\n10-20, total rows:%ld\n",(long)nrows);

  for (index= 0; index < nrows; index++)
  {
      diag("%d %d ", index, iarray[index]);
      is_num(iarray[index], index + 1 + 10);
  }    

  rc = SQLFetchScroll(Stmt,SQL_FETCH_PREV,0);/* 1-10 */
  CHECK_STMT_RC(Stmt,rc);

  diag("\n1-10, total rows:%ld\n",(long)nrows);

  for (index=1; index<=nrows; index++)
  {
      diag(" %d ",iarray[index-1]);
      IS(iarray[index-1] == index);
  }    

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* 2-11 */
  CHECK_STMT_RC(Stmt,rc);

  diag("\n2-12, total rows:%ld\n",(long)nrows);

  for (index=1; index<=nrows; index++)
  {
      diag(" %d ",iarray[index-1]);
      IS(iarray[index-1] == index+1);
  } 

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* 1-10 */
  CHECK_STMT_RC(Stmt,rc);

  diag("\n1-10, total rows:%ld\n",(long)nrows);

  for (index=1; index<=nrows; index++)
  {
      diag(" %d",iarray[index-1]);
      IS(iarray[index-1] == index);
  }       

  rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,0);/* 1-10 */
  CHECK_STMT_RC(Stmt,rc);

  diag("\n1-10, total rows:%ld\n",(long)nrows);

  for (index=1; index<=nrows; index++)
  {
      diag(" %d",iarray[index-1]);
      IS(iarray[index-1] == index);
  }      

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* BOF */
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* 1-10 */
  CHECK_STMT_RC(Stmt,rc);

  diag("\n1-10, total rows:%ld\n",(long)nrows);

  for (index=1; index<=nrows; index++)
  {
      diag(" %d",iarray[index-1]);
      IS(iarray[index-1] == index);
  } 

  SQLFreeStmt(Stmt,SQL_UNBIND);    
  SQLFreeStmt(Stmt,SQL_CLOSE);          

  rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_array_relative_10");

  return OK;
}


/* Testing SQL_FETCH_RELATIVE with row_set_size as 1 */
ODBC_TEST(t_relative_1)
{
  SQLRETURN rc;
  SQLLEN nrows;
  SQLUINTEGER i;
  const SQLUINTEGER max_rows=10;

  if (ForwardOnly == TRUE)
  {
    skip("This test cannot be run with FORWARDONLY option selected");
  }

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_relative_1");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_relative_1(id int)");

  rc = SQLPrepare(Stmt, (SQLCHAR *)"INSERT INTO t_relative_1 VALUES(?)",SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

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

  /* set row_size as 1 */
  rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROWS_FETCHED_PTR,&nrows,0);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_relative_1");

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&i,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  /* row 1 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);/* 1 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i==1);

  /* Before start */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */
  FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* jump to last row */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,max_rows);/* last row */
  CHECK_STMT_RC(Stmt,rc);
  IS(i==max_rows);

  /* jump to last row+1 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* after last */    
  FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* goto first row */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* 1 */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==1);

  /* before start */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */    
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* goto fifth  row */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,5);/* 5 */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==5);

  /* goto after end */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,max_rows);/* after last */    
  FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* 
      the scenarios from ODBC spec     
  */    

  /* CASE 1 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* 1 */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==1);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */    
  FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* BeforeStart AND FetchOffset <= 0 */    
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-20);/* before start */    
  FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */    
  FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,0);/* before start */    
  FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* case 1: Before start AND FetchOffset > 0 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* 1 */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==1);

  /* CASE 2 */    
  rc = SQLFetchScroll(Stmt,SQL_FETCH_LAST,1);/* last row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==max_rows);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* after end */    
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* After end AND FetchOffset >= 0 */    
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,10);/* after end */    
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,20);/* after end */    
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* after end */    
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,0);/* after end */    
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* After end AND FetchOffset < 0 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* last row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==max_rows);


  /* CASE 3 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==1);

  /* CurrRowsetStart = 1 AND FetchOffset < 0 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,0);/* first row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==1);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */    
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* CASE 4 */
  /* CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
      | FetchOffset | > RowsetSize
  */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==1);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,3);/* fourth row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==4);

  /* the following call satisfies 4 > 1 AND (3-4) < 1 AND |-4| > 1 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-4);/* before start */    
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

  /* CASE 5 */
  /* 1 <= CurrRowsetStart + FetchOffset <= LastResultRow */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==1);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,5);/* sixth row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==6);

  /* 1 <= 6-2 <= 10 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-2);/* fourth row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==4);

  /* CASE 6 */
  /*  CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
      | FetchOffset | <= RowsetSize
    */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==1);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,3);/* fourth row */    
  CHECK_STMT_RC(Stmt,rc);
  IS(i==4);

  /* 4 >1 AND 4-4 <1 AND |-4| <=10 */
  rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-4);/* before start */    
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");


  SQLFreeStmt(Stmt,SQL_UNBIND);    
  SQLFreeStmt(Stmt,SQL_CLOSE);          

  rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_relative_1");

  return OK;
}


/* Testing SQL_FETCH_RELATIVE with row_set_size as 2 */
ODBC_TEST(t_array_relative_2)
{
  SQLRETURN rc;
  SQLUINTEGER i;
  SQLLEN nrows;
  SQLINTEGER iarray[15];
  const SQLUINTEGER max_rows=10;

  if (ForwardOnly == TRUE)
  {
    skip("This test cannot be run with FORWARDONLY option selected");
  }

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_array_relative_2");

  OK_SIMPLE_STMT(Stmt,"CREATE TABLE t_array_relative_2(id INT)");

    rc = SQLPrepare(Stmt, (SQLCHAR *)"INSERT INTO t_array_relative_2 VALUES(?)",SQL_NTS);
    CHECK_STMT_RC(Stmt,rc);

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

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_array_relative_2");

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,iarray,0,NULL);
    CHECK_STMT_RC(Stmt,rc);

    /* row 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);/* 1 */
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);


    /* Before start */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* jump to last row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,max_rows);/* last row */
    CHECK_STMT_RC(Stmt,rc);        
    IS(nrows == 1);
    IS(iarray[0]==max_rows);

    /* jump to last row+1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* after last */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* goto first row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    /* before start */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* goto fifth  row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,5);/* 5 */    
    CHECK_STMT_RC(Stmt,rc);    
    IS(nrows == 2);
    IS(iarray[0]==5);
    IS(iarray[1]==6);

    /* goto after end */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,max_rows);/* after last */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* 
       the scenarios from ODBC spec     
    */    

    /* CASE 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* BeforeStart AND FetchOffset <= 0 */    
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-20);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,0);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* case 1: Before start AND FetchOffset > 0 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    /* CASE 2 */    
    rc = SQLFetchScroll(Stmt,SQL_FETCH_LAST,1);/* last row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==max_rows-1);    
    IS(iarray[1]==max_rows);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* last row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 1);
    IS(iarray[0]==max_rows);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* after last row */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* After end AND FetchOffset >= 0 */    
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,10);/* after end */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,20);/* after end */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,1);/* after end */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,0);/* after end */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* After end AND FetchOffset < 0 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* last row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 1);
    IS(iarray[0]==max_rows);    


    /* CASE 3 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    /* CurrRowsetStart = 1 AND FetchOffset < 0 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,0);/* first row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-1);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* CASE 4 */
    /* CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
       | FetchOffset | > RowsetSize
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,3);/* fourth row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==4);
    IS(iarray[1]==5);

    /* the following call satisfies 4 > 1 AND (3-4) < 1 AND |-4| > 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-4);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* CASE 5 */
    /* 1 <= CurrRowsetStart + FetchOffset <= LastResultRow */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,5);/* sixth row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==6);
    IS(iarray[1]==7);

    /* 1 <= 6-2 <= 10 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-2);/* fourth row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==4);
    IS(iarray[1]==5);

    /* CASE 6 */
    /*  CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
        | FetchOffset | <= RowsetSize
     */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* first row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,3);/* fourth row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==4);
    IS(iarray[1]==5);

    /* 4 >1 AND 4-4 <1 AND |-4| <=10 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-4);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");


    SQLFreeStmt(Stmt,SQL_UNBIND);    
    SQLFreeStmt(Stmt,SQL_CLOSE); 

    /***
      for rowset_size > max_rows...
    */
    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)25,0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROWS_FETCHED_PTR,&nrows,0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_array_relative_2");
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,iarray,0,NULL);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,2);/* 2 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == max_rows-1);
    IS(iarray[0]==2);
    IS(iarray[max_rows-2]==10);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,1);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == max_rows);
    IS(iarray[0]==1);
    IS(iarray[max_rows-1]==max_rows);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,5);/* 5 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == max_rows-4);
    IS(iarray[0]==5);
    IS(iarray[max_rows-5]==max_rows);


    /* CurrRowsetStart > 1 AND 
       CurrRowsetStart + FetchOffset < 1 AND
       | FetchOffset | > RowsetSize

      ==> before start
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-30);/* 1 */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,2);/* 2 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == max_rows-1);
    IS(iarray[0]==2);
    IS(iarray[max_rows-2]==10);

    /* CurrRowsetStart > 1 AND 
       CurrRowsetStart + FetchOffset < 1 AND
       | FetchOffset | <= RowsetSize

      ==> 1

    rc = SQLFetchScroll(Stmt,SQL_FETCH_RELATIVE,-13);  
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == max_rows);
    IS(iarray[0]==1);
    IS(iarray[max_rows-1]==max_rows);
    */

    SQLFreeStmt(Stmt,SQL_UNBIND);    
    SQLFreeStmt(Stmt,SQL_CLOSE); 

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_array_relative_2");

  return OK;
}


/* Testing SQL_FETCH_ABSOLUTE with row_set_size as 1 */
ODBC_TEST(t_absolute_1)
{
  SQLRETURN rc;
  SQLLEN nrows;
  SQLUINTEGER i;
  const SQLUINTEGER max_rows=10;

  if (ForwardOnly == TRUE)
  {
    skip("This test cannot be run with FORWARDONLY option selected");
  }

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_absolute_1");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_absolute_1(id INT)");

    rc = SQLPrepare(Stmt, (SQLCHAR *)"INSERT INTO t_absolute_1 VALUES(?)", SQL_NTS);
    CHECK_STMT_RC(Stmt,rc);

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

    /* set row_size as 1 */
    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROWS_FETCHED_PTR,&nrows,0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_absolute_1");

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&i,0,NULL);
    CHECK_STMT_RC(Stmt,rc);

    /* row 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);/* 1 */
    CHECK_STMT_RC(Stmt,rc);
    IS(i==1);

    /* Before start */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-12);/* before start */
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* jump to last row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows);/* last row */
    CHECK_STMT_RC(Stmt,rc);
    IS(i==max_rows);

    /* jump to last row+1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows+1);/* after last */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* goto first row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,1);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(i==1);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* before start */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-15);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* goto fifth  row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,5);/* 5 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(i==5);

    /* goto after end */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows+5);/* after last */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* 
       the scenarios from ODBC spec     
    */    

    /* CASE 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(i==1);

    /* FetchOffset < 0 AND | FetchOffset | <= LastResultRow ,
       ==> should yield LastResultRow + FetchOffset + 1 
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-1);
    CHECK_STMT_RC(Stmt,rc);
    IS(i==(max_rows-1+1));

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-4);
    CHECK_STMT_RC(Stmt,rc);
    IS(i==(max_rows-4+1));

    /* CASE 2 :
      FetchOffset < 0 AND
      | FetchOffset | > LastResultRow AND
      | FetchOffset | > RowsetSize

      ==> Before start
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-11);
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* CASE 4:
    
      FetchOffset = 0
     
      ==> before start
    */  
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* CASE 5: 
       1 <= FetchOffset <= LastResultRow

      ==> FetchOffset
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,2);/* 2 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(i==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,9);/* 9 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(i==9);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,6);/* 6 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(i==6);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* BOF */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* CASE 6:
      FetchOffset > LastResultRow

      ==> after end
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows);/* last row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(i==max_rows);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows+1);/* after end */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows+max_rows);/* after end */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,5);/* 5 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(i==5);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,12);/* 5 */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    SQLFreeStmt(Stmt,SQL_UNBIND);    
    SQLFreeStmt(Stmt,SQL_CLOSE);          

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_absolute_1");

  return OK;
}


/* Testing SQL_FETCH_ABSOLUTE with row_set_size as 2 */
ODBC_TEST(t_absolute_2)
{
  SQLRETURN rc;
  SQLLEN nrows;
  SQLINTEGER iarray[15];
  const SQLUINTEGER max_rows=10;
  SQLUINTEGER i;

  if (ForwardOnly == TRUE)
  {
    skip("This test cannot be run with FORWARDONLY option selected");
  }

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_absolute_2");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_absolute_2(id INT)");

    rc = SQLPrepare(Stmt, (SQLCHAR *)"INSERT INTO t_absolute_2 VALUES(?)",SQL_NTS);
    CHECK_STMT_RC(Stmt,rc);

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

    /* set row_size as 1 */
    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)2,0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROWS_FETCHED_PTR,&nrows,0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_absolute_2");
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,iarray,0,NULL);
    CHECK_STMT_RC(Stmt,rc);

    /* row 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,0);/* 1 */
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    /* Before start */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-12);/* before start */
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* jump to last row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows);/* last row */
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 1);
    IS(iarray[0]==max_rows);


    /* jump to last row+1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows+1);/* after last */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* goto first row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,1);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* before start */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-15);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* goto fifth  row */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,5);/* 5 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==5);
    IS(iarray[1]==6);

    /* goto after end */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows+5);/* after last */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* 
       the scenarios from ODBC spec     
    */    

    /* CASE 1 */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_FIRST,1);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==1);
    IS(iarray[1]==2);

    /* FetchOffset < 0 AND | FetchOffset | <= LastResultRow ,
       ==> should yield LastResultRow + FetchOffset + 1 
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-1);
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 1);
    IS(iarray[0]==(max_rows-1+1));

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-4);
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==(max_rows-4+1));
    IS(iarray[1]==(max_rows-4+1+1));

    /* CASE 2 :
      FetchOffset < 0 AND
      | FetchOffset | > LastResultRow AND
      | FetchOffset | > RowsetSize

      ==> Before start
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-11);
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* CASE 4:
    
      FetchOffset = 0
     
      ==> before start
    */  
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* CASE 5: 
       1 <= FetchOffset <= LastResultRow

      ==> FetchOffset
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,2);/* 2 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==2);
    IS(iarray[1]==3);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,9);/* 9 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==9);
    IS(iarray[1]==10);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,6);/* 6 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==6);
    IS(iarray[1]==7);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* BOF */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    /* CASE 6:
      FetchOffset > LastResultRow

      ==> after end
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows);/* last row */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 1);
    IS(iarray[0]==max_rows);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows+1);/* after end */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,max_rows+max_rows);/* after end */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,5);/* 5 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == 2);
    IS(iarray[0]==5);
    IS(iarray[1]==6);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,12);/* 5 */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,0);/* before start */    
     FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data expected");

    SQLFreeStmt(Stmt,SQL_UNBIND);    
    SQLFreeStmt(Stmt,SQL_CLOSE);          

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
    CHECK_STMT_RC(Stmt,rc);

    /* for rowset_size > max_rows...*/
    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)25,0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROWS_FETCHED_PTR,&nrows,0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_absolute_2");
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,iarray,0,NULL);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,2);/* 2 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == max_rows-1);
    IS(iarray[0]==2);
    IS(iarray[max_rows-2]==10);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,1);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == max_rows);
    IS(iarray[0]==1);
    IS(iarray[max_rows-1]==max_rows);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,5);/* 5 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == max_rows-4);
    IS(iarray[0]==5);
    IS(iarray[max_rows-5]==max_rows);


    /* FetchOffset < 0 AND
      | FetchOffset | > LastResultRow AND
      | FetchOffset | <= RowsetSize
    */
    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,-13);/* 1 */    
    CHECK_STMT_RC(Stmt,rc);
    IS(nrows == max_rows);
    IS(iarray[0]==1);
    IS(iarray[max_rows-1]==max_rows);

    SQLFreeStmt(Stmt,SQL_UNBIND);    
    SQLFreeStmt(Stmt,SQL_CLOSE); 

    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_absolute_2");

  return OK;
}

/*{{{ t_unbind_before_fetch */
/**
  ODBC-110: Unbinding columns before fetching new result causes crash in the connector whilie fetching the row
  Crash would occur also if ordinary SQLFetch was used. But it had been SQLFetchScroll, when the bug was observed for the 1st time
  Affectts Microsoft ODBC test tool
*/
ODBC_TEST(t_unbind_before_fetch)
{
  SQLUINTEGER dummy;

  OK_SIMPLE_STMT(Stmt, "SELECT 1");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_ULONG, &dummy, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1));

  return OK;
}
/*}}}*/

MA_ODBC_TESTS my_tests[]=
{
  {t_scroll, "t_scroll"},
  {t_array_relative_10, "t_array_relative_10"},
  {t_relative_1, "t_relative_1"},
  {t_array_relative_2, "t_array_relative_2"},
  {t_absolute_1,"t_absolute_1"},
  {t_absolute_2, "t_absolute_2"},
  {t_unbind_before_fetch, "t_odbc_110_unbind_before_fetch"},
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
