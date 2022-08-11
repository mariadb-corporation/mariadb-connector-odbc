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

ODBC_TEST(my_dynamic_pos_cursor)
{
  SQLRETURN   rc;
  SQLLEN      nRowCount;
  SQLHSTMT    hstmt_pos;
  SQLINTEGER  nData = 500;
  SQLCHAR     szData[255]={0};

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }
  /* initialize data */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_dynamic_cursor");

  OK_SIMPLE_STMT(Stmt, "create table my_dynamic_cursor(id int, name varchar(30))");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(100,'venu')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(200,'monty')");

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  /* create new statement handle */
  rc = SQLAllocHandle(SQL_HANDLE_STMT, Connection, &hstmt_pos);
  CHECK_DBC_RC(Connection, rc);

  /* set the cursor name as 'mysqlcur' on Stmt */
  rc = SQLSetCursorName(Stmt, (SQLCHAR *)"mysqlcur", SQL_NTS);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,15,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY ,(SQLPOINTER)SQL_CONCUR_ROWVER , 0);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE  ,(SQLPOINTER)1 , 0);
  CHECK_STMT_RC(Stmt, rc);

  /* Open the resultset of table 'my_demo_cursor' */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_dynamic_cursor");
  CHECK_STMT_RC(Stmt,rc);

  /* goto the last row */
  rc = SQLFetchScroll(Stmt, SQL_FETCH_LAST, 1L);
  CHECK_STMT_RC(Stmt,rc);

  /* now update the name field to 'update' using positioned cursor */
  OK_SIMPLE_STMT(hstmt_pos, "UPDATE my_dynamic_cursor SET id=300, name='updated' WHERE CURRENT OF mysqlcur");

  rc = SQLRowCount(hstmt_pos, &nRowCount);
  CHECK_STMT_RC(hstmt_pos, rc);

  diag(" total rows updated:%d\n",nRowCount);
  is_num(nRowCount, 1);

  /* Now delete the newly updated record */
  strcpy((char*)szData,"updated");
  nData = 300;

  rc = SQLSetPos(Stmt,1,SQL_DELETE,SQL_LOCK_UNLOCK);
  FAIL_IF(rc!=SQL_ERROR,"Error expected");

  rc = SQLSetPos(Stmt,1,SQL_DELETE,SQL_LOCK_EXCLUSIVE);
  FAIL_IF(rc!=SQL_ERROR,"Error expected")
    
  rc = SQLSetPos(Stmt,1,SQL_DELETE,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt, &nRowCount);
  CHECK_STMT_RC(Stmt, rc);

  diag(" total rows deleted:%d\n",nRowCount);
  is_num(nRowCount, 1);

  /* Free statement cursor resorces */
  rc = SQLFreeStmt(Stmt, SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(hstmt_pos, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  /* Free the statement 'hstmt_pos' */
  rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt_pos);
  CHECK_STMT_RC(hstmt_pos,rc);

  /* Now fetch and verify the data */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_dynamic_cursor");

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt,1,SQL_C_LONG,&nData,0,NULL);
  CHECK_STMT_RC(Stmt,rc);
  is_num(nData, 100);

  rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,50,NULL);
  CHECK_STMT_RC(Stmt,rc);
  IS_STR(szData,"venu", 5);

  rc = SQLFetch(Stmt);
  FAIL_IF(rc!=SQL_NO_DATA_FOUND,"Eof expected");

  SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
  SQLFreeStmt(Stmt, SQL_UNBIND);
  SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_dynamic_cursor");

  return OK;
}


/* perform positioned update and delete */
ODBC_TEST(my_dynamic_pos_cursor1)
{
    SQLRETURN   rc;
    SQLLEN      nRowCount;
    SQLHSTMT    hstmt_pos;
    SQLINTEGER  i,nData[15];
    char        data[30],szData[15][10]={0}, buff[10];

    if (ForwardOnly == TRUE && NoCache == TRUE)
    {
      skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
    }
    /* initialize data */
    OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_dynamic_cursor");

    OK_SIMPLE_STMT(Stmt, "create table my_dynamic_cursor(id int, name varchar(30))");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(1,'MySQL1')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(2,'MySQL2')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(3,'MySQL3')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(4,'MySQL4')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(5,'MySQL5')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(6,'MySQL6')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(7,'MySQL7')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(8,'MySQL8')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(9,'MySQL9')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(10,'MySQL10')");

    SQLFreeStmt(Stmt,SQL_CLOSE);

    /* create new statement handle */
    rc = SQLAllocHandle(SQL_HANDLE_STMT, Connection, &hstmt_pos);
    CHECK_DBC_RC(Connection, rc);

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
    CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                  (SQLPOINTER)SQL_CURSOR_STATIC, 0));

    /* set the cursor name as 'mysqlcur' on Stmt */
    CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"mysqlcur", SQL_NTS));

    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));

    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData[0]), NULL));

    CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)3,0));

    /* Open the resultset of table 'my_demo_cursor' */
    OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_dynamic_cursor");

    /* goto the last row
    TODO: This does not really go to the last row - it fetches 3 rows(SQL_ATTR_ROW_ARRAY_SIZE) starting from 5th, and then cursor should be on 5th after the command */
    CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, 5L));

    for (i= 5; i < 5 + 3; i++)
    {
      diag("i %d nData: %d szData: %s", i, nData[i - 5], szData[i - 5]);
      is_num(i, nData[i - 5]);
    }

    /* Cursor is supposed to stay on the 1st row of the rowset */
    IS_STR(my_fetch_str(Stmt, buff, 2), "MySQL5", sizeof("MySQL5"));

    /*rc = SQLSetPos(Stmt,SQL_POSITION,2,SQL_LOCK_NO_CHANGE);
    CHECK_STMT_RC(Stmt,rc); */

    /* now update the name field to 'update' using positioned cursor */
    OK_SIMPLE_STMT(hstmt_pos, "UPDATE my_dynamic_cursor SET id=999, name='updated' WHERE CURRENT OF mysqlcur");

    CHECK_STMT_RC(hstmt_pos, SQLRowCount(hstmt_pos, &nRowCount));

    diag(" total rows updated:%d\n",nRowCount);
    is_num(nRowCount, 1);
    strcpy(szData[1],"updated");
    nData[1] = 999;

    CHECK_STMT_RC(Stmt,SQLSetPos(Stmt,2,SQL_DELETE,SQL_LOCK_NO_CHANGE));
    CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nRowCount));

    diag(" total rows deleted:%d\n",nRowCount);
    is_num(nRowCount, 1);

    /* Free statement cursor resorces */
    rc = SQLFreeStmt(Stmt, SQL_UNBIND);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFreeStmt(hstmt_pos, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    /* commit the transaction */
    rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    /* Free the statement 'hstmt_pos' */
    rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt_pos);
    CHECK_STMT_RC(hstmt_pos,rc);

    /* Now fetch and verify the data */
    rc = SQLSetStmtAttr(Stmt,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)1,0);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_dynamic_cursor");

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&i,0,NULL);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,2,SQL_C_CHAR, data,20,NULL);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,4L);
    CHECK_STMT_RC(Stmt,rc);

    is_num(i, 4);
    IS_STR(data,"MySQL4", 7);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,1L);
    CHECK_STMT_RC(Stmt,rc);

    is_num(i, 999);
    IS_STR(data, "updated", 8);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,1L);
    CHECK_STMT_RC(Stmt,rc);

    is_num(i, 7);
    IS_STR(data, "MySQL7", 7);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,10L);
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"Eof expected");

    SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt, SQL_UNBIND);
    SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_dynamic_cursor");

  return OK;
}


/* CURSOR POSITION - rowset size 1 */
ODBC_TEST(my_position)
{
    SQLRETURN rc;
    SQLLEN    nlen;
    char      szData[255]= {0};
    SQLINTEGER nData;
    SQLLEN    nrow;

    if (ForwardOnly == TRUE && NoCache == TRUE)
    {
      skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
    }

    OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_position");
    OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_position(col1 INT, col2 VARCHAR(30))");

    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_position VALUES(100,'MySQL1')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_position VALUES(200,'MySQL2')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_position VALUES(300,'MySQL3')");
    OK_SIMPLE_STMT(Stmt, "INSERT INTO my_position VALUES(400,'MySQL4')");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY ,(SQLPOINTER)SQL_CONCUR_ROWVER , 0);
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE  ,(SQLPOINTER)1 , 0);
    CHECK_STMT_RC(Stmt, rc);

    OK_SIMPLE_STMT(Stmt,"SELECT * FROM my_position");

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,0,&nrow);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,10,&nlen);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,3);
    CHECK_STMT_RC(Stmt,rc);

    nData = 999; nrow = SQL_COLUMN_IGNORE;
    strcpy(szData,"update");

    rc = SQLSetPos(Stmt,1,SQL_POSITION,SQL_LOCK_NO_CHANGE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLSetPos(Stmt,1,SQL_UPDATE,SQL_LOCK_NO_CHANGE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLRowCount(Stmt,&nlen);
    CHECK_STMT_RC(Stmt,rc);

    diag(" rows affected:%d\n",nlen);
    is_num(nlen, 1);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_position");
    CHECK_STMT_RC(Stmt,rc);

    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

    rc = SQLGetData(Stmt,1,SQL_C_LONG,&nData,0,NULL);
    CHECK_STMT_RC(Stmt,rc);
    rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,10,NULL);
    CHECK_STMT_RC(Stmt,rc);
    is_num(nData, 300);
    IS_STR(szData, "update", 7);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetch(Stmt);
    FAIL_IF(rc!=SQL_NO_DATA_FOUND,"No data found expected");

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_position");

  return OK;
}


/* CURSOR POSITION - rowset size 3 */
ODBC_TEST(my_position1)
{
  SQLINTEGER nData[15];
  SQLLEN    nlen[15]= {0}, nrow[15]= {0};
  SQLCHAR   szData[15][15]= {0};

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_position");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_position (col1 INT, col2 VARCHAR(30))");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_position VALUES (1,'MySQL1'), (2,'MySQL2'),"
         "(3,'MySQL3'), (4,'MySQL4'), (5,'MySQL5'), (6,'MySQL6'), (7,'MySQL7'),"
         "(8,'MySQL8'), (9,'MySQL9'), (10,'MySQL10'), (11,'MySQL11'),"
         "(12,'MySQL12')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY,
                                (SQLPOINTER)SQL_CONCUR_ROWVER, 0));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)3, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_position");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, nrow));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData[0]),
                            nlen));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, 4));

  /* Small piece to test ODBC-56 */
  is_num(nlen[0], 6);
  is_num(nlen[1], 6);
  is_num(nlen[2], 6);
  is_num(nrow[0], nrow[1]);
  is_num(nrow[0], nrow[2]);
  is_num(nrow[2], sizeof(SQLINTEGER));

  nData[0]= 888;
  nData[1]= 999;
  nrow[1]= SQL_COLUMN_IGNORE;
  nData[2]= 1000;

  strcpy((char *)szData[0], "updatex");
  nlen[0]= 15;
  strcpy((char *)szData[1], "updatey");
  nlen[1]= 15;
  strcpy((char *)szData[2], "updatez");
  nlen[2]= 15;

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 2, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 3, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_position");

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, 4));

  is_num(nData[0], 4);
  IS_STR(szData[0], "MySQL4", 6);
  is_num(nData[1], 5);
  IS_STR(szData[1], "updatey", 7);
  is_num(nData[2], 1000);
  IS_STR(szData[2], "updatez", 7);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)1, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE my_position");

  return OK;
}


/* IROW VALUE - 0 */
ODBC_TEST(my_zero_irow_update)
{
  SQLRETURN rc;
  SQLLEN    nlen[15]= {0}, nrow[15]= {0};
  char      szData[15][15]={0};
  SQLINTEGER nData[15];

  /* Tests(this and others) will actually fail with FORWARDONLY only, since they do cursor scrolling beyond
   * possible with forward only cursors
   */
  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_zero_irow");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_zero_irow(col1 INT, col2 VARCHAR(30))");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(1,'MySQL1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(2,'MySQL2')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(3,'MySQL3')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(4,'MySQL4')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(5,'MySQL5')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(6,'MySQL6')");

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY ,(SQLPOINTER)SQL_CONCUR_ROWVER , 0);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE  ,(SQLPOINTER)3 , 0);
  CHECK_STMT_RC(Stmt, rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_zero_irow");
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,0,nrow);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,sizeof(szData[0]),nlen);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,2);
  CHECK_STMT_RC(Stmt,rc);

  nData[0] = 888;
  nData[1] = 999; nrow[1] = SQL_COLUMN_IGNORE;
  nData[2] = 1000;

  strcpy(szData[0],"updatex"); nlen[0] = 15;
  strcpy(szData[1],"updatey"); nlen[1] = 15;
  strcpy(szData[2],"updatez"); nlen[2] = 15;

  rc = SQLSetPos(Stmt,0,SQL_UPDATE,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_zero_irow");
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,2);
  CHECK_STMT_RC(Stmt,rc);

  is_num(nData[0], 888);
  IS_STR(szData[0], "updatex", 8);
  is_num(nData[1], 3);
  IS_STR(szData[1], "updatey", 8);
  is_num(nData[2], 1000);
  IS_STR(szData[2], "updatez", 8);

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE  ,(SQLPOINTER)1 , 0);
  CHECK_STMT_RC(Stmt, rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE my_zero_irow");

  return OK;
}


/* IROW VALUE - 0 - DELETE */
ODBC_TEST(my_zero_irow_delete)
{
  SQLRETURN rc;
  SQLLEN    nlen[15]= {0}, nrow[15]= {0};
  char      szData[15][15]={0};
  SQLINTEGER nData[15];

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_zero_irow");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_zero_irow(col1 INT, col2 VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(1,'MySQL1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(2,'MySQL2')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(3,'MySQL3')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(4,'MySQL4')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(5,'MySQL5')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_zero_irow VALUES(6,'MySQL6')");


  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY ,(SQLPOINTER)SQL_CONCUR_ROWVER , 0);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE  ,(SQLPOINTER)3 , 0);
  CHECK_STMT_RC(Stmt, rc);

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM my_zero_irow");
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,0,nrow);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,sizeof(szData[0]),nlen);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,2);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetPos(Stmt,0,SQL_DELETE,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_zero_irow");

  rc = SQLFetchScroll(Stmt,SQL_FETCH_ABSOLUTE,1);
  CHECK_STMT_RC(Stmt,rc);

  is_num(nData[0], 1);
  IS_STR(szData[0], "MySQL1", 7);
  is_num(nData[1], 5);
  IS_STR(szData[1], "MySQL5", 7);
  is_num(nData[2], 6);
  IS_STR(szData[2], "MySQL6", 7);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,1);
  FAIL_IF(rc!=SQL_NO_DATA_FOUND, "no data found expected");

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE  ,(SQLPOINTER)1 , 0);
  CHECK_STMT_RC(Stmt, rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE my_zero_irow");

  return OK;
}


/* DYNAMIC CURSOR TESTING */
ODBC_TEST(my_dynamic_cursor)
{
  SQLRETURN rc;
  SQLLEN nlen;
  SQLINTEGER nData = 500;
  SQLCHAR szData[255]={0};

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }
  /* initialize data */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_dynamic_cursor");

  OK_SIMPLE_STMT(Stmt, "create table my_dynamic_cursor(col1 int, col2 varchar(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(100,'venu')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_dynamic_cursor VALUES(200,'monty')");

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY , (SQLPOINTER)SQL_CONCUR_ROWVER , 0);
  CHECK_STMT_RC(Stmt, rc);

  /* Now, add a row of data */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_dynamic_cursor");
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,15,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFetchScroll(Stmt,SQL_FETCH_NEXT,1);
  CHECK_STMT_RC(Stmt,rc);

  nData = 300;
  strcpy((char *)szData , "mysql");

  rc = SQLSetPos(Stmt,3,SQL_ADD,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt,&nlen);
  CHECK_STMT_RC(Stmt,rc);

  diag("rows affected:%d\n",nlen);
  strcpy((char *)szData , "insert-new2");
  rc = SQLSetPos(Stmt,1,SQL_ADD,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt,&nlen);
  CHECK_STMT_RC(Stmt,rc);

  diag("rows affected:%d\n",nlen);

  strcpy((char *)szData , "insert-new3");
  rc = SQLSetPos(Stmt,0,SQL_ADD,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt,&nlen);
  CHECK_STMT_RC(Stmt,rc);

  diag("rows affected:%d\n",nlen);

  strcpy((char *)szData , "insert-new4");
  rc = SQLSetPos(Stmt,10,SQL_ADD,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt,&nlen);
  CHECK_STMT_RC(Stmt,rc);

  diag("rows affected:%d\n",nlen);

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_dynamic_cursor");

  is_num(myrowcount(Stmt), 6);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE my_dynamic_cursor");

  return OK;
}

MA_ODBC_TESTS my_tests[]=
{
  {my_dynamic_pos_cursor, "my_dynamic_pos_cursor",   NORMAL},
  {my_dynamic_pos_cursor1, "my_dynamic_pos_cursor1", NORMAL},
  {my_position, "my_position",                       NORMAL},
  { my_position1, "my_position1",                    NORMAL },
  { my_zero_irow_update, "my_zero_irow_update",      NORMAL },
  {my_zero_irow_delete, "my_zero_irow_delete",       NORMAL},
  {my_dynamic_cursor, "my_dynamic_cursor",           NORMAL},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  CHANGE_DEFAULT_OPTIONS(my_options | 32);
  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
