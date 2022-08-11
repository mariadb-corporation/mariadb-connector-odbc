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

#include <sqlucode.h>

/* perform positioned update and delete */
ODBC_TEST(my_positioned_cursor)
{
  SQLLEN      nRowCount;
  SQLHSTMT    hstmt_pos;
  SQLCHAR     data[10];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_demo_cursor");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_demo_cursor (id INT, name VARCHAR(20))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_demo_cursor VALUES (0,'MySQL0'),(1,'MySQL1'),"
         "(2,'MySQL2'),(3,'MySQL3'),(4,'MySQL4')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  SKIP_IF_FORWARDONLY(Stmt, SQL_CURSOR_DYNAMIC);

  /* set the cursor name as 'mysqlcur' on Stmt */
  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR*)"mysqlcur", SQL_NTS));

  /* Open the resultset of table 'my_demo_cursor' */
  OK_SIMPLE_STMT(Stmt,"SELECT * FROM my_demo_cursor WHERE 1");

  /* goto the last row */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_LAST, 1L));

  /* create new statement handle */
  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &hstmt_pos));

  /* now update the name field to 'updated' using positioned cursor */
  OK_SIMPLE_STMT(hstmt_pos, "UPDATE my_demo_cursor SET name='updated' "
         "WHERE CURRENT OF mysqlcur");

  CHECK_STMT_RC(Stmt, SQLRowCount(hstmt_pos, &nRowCount));
  is_num(nRowCount, 1);

  CHECK_STMT_RC(hstmt_pos, SQLFreeStmt(hstmt_pos, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Now delete 2nd row */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_demo_cursor");

  /* goto the second row */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, 2L));

  /* now delete the current row */
  OK_SIMPLE_STMT(hstmt_pos, "DELETE FROM my_demo_cursor WHERE CURRENT OF mysqlcur");

  CHECK_STMT_RC(Stmt, SQLRowCount(hstmt_pos, &nRowCount));
  is_num(nRowCount, 1);

  /* free the statement cursor */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Free the statement 'hstmt_pos' */
  CHECK_STMT_RC(hstmt_pos, SQLFreeHandle(SQL_HANDLE_STMT, hstmt_pos));

  /* Now fetch and verify the data */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_demo_cursor");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 0);
  IS_STR(my_fetch_str(Stmt, data, 2), "MySQL0", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 2);
  IS_STR(my_fetch_str(Stmt, data, 2), "MySQL2", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 3);
  IS_STR(my_fetch_str(Stmt, data, 2), "MySQL3", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 4);
  IS_STR(my_fetch_str(Stmt, data, 2), "updated", 7);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_demo_cursor");

  return OK;
}


/* perform delete and update using SQLSetPos */
ODBC_TEST(my_setpos_cursor)
{
  SQLLEN      nRowCount;
  SQLINTEGER  id= 0;
  SQLCHAR     name[50]= {0};
  SQLRETURN   rc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_demo_cursor");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_demo_cursor (id INT, name VARCHAR(20))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_demo_cursor VALUES (0,'MySQL0'),(1,'MySQL1'),"
         "(2,'MySQL2'),(3,'MySQL3'),(4,'MySQL4')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_demo_cursor");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &id, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, name, sizeof(name),NULL));

  if (ForwardOnly == FALSE)
  {
    CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1L));
  }
  else
  {
    if (NoCache == TRUE)
    {
      skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
    }
    /* if we have forced FORWARD_ONLY - technically at this poing SQL_FETCH_FIRST and SQL_FETCH_NEXT is the same, but
     * for SQL_FETCH_FIRST SQLFetchScroll must return error
     */
    CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1L));
  }

  strcpy((char *)name, "first-row");

  /* now update the name field to 'first-row' using SQLSetPos */
  rc= SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE);

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nRowCount));
  is_num(nRowCount, 1);

  /* position to second row and delete it ..*/
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, 2L));

  /* now delete the current, second row */
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nRowCount));
  is_num(nRowCount, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Now fetch and verify the data */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_demo_cursor");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 0);
  IS_STR(my_fetch_str(Stmt, name, 2), "first-row", 9);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 2);
  IS_STR(my_fetch_str(Stmt, name, 2), "MySQL2", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 3);
  IS_STR(my_fetch_str(Stmt, name, 2), "MySQL3", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 4);
  IS_STR(my_fetch_str(Stmt, name, 2), "MySQL4", 6);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE my_demo_cursor");

  return OK;
}


/**
 Bug #5853: Using Update with 'WHERE CURRENT OF' with binary data crashes
*/
ODBC_TEST(t_bug5853)
{
  SQLRETURN rc;
  SQLHSTMT  hstmt_pos;
  SQLCHAR   nData[4]= {0};
  SQLLEN    nLen= SQL_DATA_AT_EXEC;
  int       i= 0;

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }
  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &hstmt_pos));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug5853");

  OK_SIMPLE_STMT(Stmt,"CREATE TABLE t_bug5853 (id INT AUTO_INCREMENT PRIMARY KEY, a VARCHAR(3))");

  OK_SIMPLE_STMT(Stmt,"INSERT INTO t_bug5853 (a) VALUES ('abc'),('def')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_DYNAMIC,0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"bug5853", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug5853");

  CHECK_STMT_RC(hstmt_pos, SQLPrepare(hstmt_pos, (SQLCHAR *)
                     "UPDATE t_bug5853 SET a = ? WHERE CURRENT OF bug5853",
                     SQL_NTS));
  CHECK_STMT_RC(hstmt_pos, SQLBindParameter(hstmt_pos, 1, SQL_PARAM_INPUT,
                                      SQL_C_CHAR, SQL_VARCHAR, 0, 0, NULL,
                                      0, &nLen));

  while ((rc= SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0)) != SQL_NO_DATA_FOUND)
  {
    char data[2][3] = { "uvw", "xyz" };

    FAIL_IF(SQLExecute(hstmt_pos)!= SQL_NEED_DATA, "SQL_NEED_DATA expected");
    rc= SQL_NEED_DATA;

    while (rc == SQL_NEED_DATA)
    {
      SQLPOINTER token;
     
      rc= SQLParamData(hstmt_pos, &token);
      if (rc == SQL_NEED_DATA)
      {
        CHECK_STMT_RC(hstmt_pos, SQLPutData(hstmt_pos, data[i++ % 2],
                                      sizeof(data[0])));
      }
    }
  }

  CHECK_STMT_RC(hstmt_pos, SQLFreeStmt(hstmt_pos, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t_bug5853");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, nData, sizeof(nData), &nLen));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));
  IS_STR(nData, "uvw", 3);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));
  IS_STR(nData, "xyz", 3);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug5853");

  return OK;
}


ODBC_TEST(t_setpos_del_all)
{
  SQLINTEGER nData[4]= {0};
  SQLCHAR szData[4][10]= {{0}, {0}, {0}, {0}};
  SQLUSMALLINT rgfRowStatus[4];
  SQLLEN nlen;

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_setpos_del_all");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_setpos_del_all (a INT NOT NULL PRIMARY KEY,"
         "b VARCHAR(20))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_setpos_del_all VALUES (100,'MySQL1'),"
         "(200,'MySQL2'),(300,'MySQL3'),(400,'MySQL4')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLSetStmtOption(Stmt, SQL_ROWSET_SIZE, 4));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_setpos_del_all ORDER BY a ASC");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData[0]),
                            NULL));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_FIRST, 1, NULL,
                                  rgfRowStatus));

  is_num(nData[0], 100);
  IS_STR(szData[0], "MySQL1", 6);
  is_num(nData[1], 200);
  IS_STR(szData[1], "MySQL2", 6);
  is_num(nData[2], 300);
  IS_STR(szData[2], "MySQL3", 6);
  is_num(nData[3], 400);
  IS_STR(szData[3], "MySQL4", 6);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 0, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));
  is_num(nlen, 4);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_setpos_del_all");

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtOption(Stmt, SQL_ROWSET_SIZE, 1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_setpos_del_all");

  return OK;
}


ODBC_TEST(t_setpos_upd_decimal)
{
  SQLINTEGER   rec= 0;
  SQLUSMALLINT status;

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_setpos_upd_decimal");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_setpos_upd_decimal (record DECIMAL(3,0),"
         "num1 FLOAT, num2 DECIMAL(6,0), num3 DECIMAL(10,3))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_setpos_upd_decimal VALUES (1,12.3,134,0.100)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT record FROM t_setpos_upd_decimal");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &rec, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, NULL, &status));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  rec= 100;

  FAIL_IF(SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE) != SQL_ERROR, "Error expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_setpos_upd_decimal");

  return OK;
}


ODBC_TEST(t_setpos_position)
{
  SQLINTEGER nData;
  SQLLEN nlen;
  SQLCHAR szData[255];
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_setpos_position");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_setpos_position (a INT, b VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_setpos_position VALUES (100,'MySQL1'),"
         "(200,'MySQL2'),(300,'MySQL3')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY,
                                (SQLPOINTER)SQL_CONCUR_ROWVER, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_setpos_position");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
                            &nlen));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, &pcrow,
                                  &rgfRowStatus));

  is_num(rgfRowStatus,SQL_ROW_SUCCESS);
  is_num(nData, 100);
  is_num(nlen, 6);
  IS_STR(szData, "MySQL1", 6);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  nData= 1000;
  strcpy((char *)szData, "updated");
  nlen= 7;

  FAIL_IF(SQLSetPos(Stmt, 3, SQL_UPDATE, SQL_LOCK_NO_CHANGE) != SQL_ERROR, "Error expected");

  FAIL_IF(SQLSetPos(Stmt, 2, SQL_UPDATE, SQL_LOCK_NO_CHANGE) != SQL_ERROR, "Error expected");

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));
  is_num(nlen, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_setpos_position");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1000);
  IS_STR(my_fetch_str(Stmt, szData, 2), "updated", 7);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 200);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL2", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 300);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL3", 6);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DELETE FROM t_setpos_position WHERE b = 'updated'");

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));
  is_num(nlen, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_setpos_position");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 200);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL2", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 300);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL3", 6);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_setpos_position");

  return OK;
}


ODBC_TEST(t_pos_column_ignore)
{
  SQLCHAR szData[20];
  SQLINTEGER nData;
  SQLLEN  pcbValue, nlen;
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;
  SQLLEN Rows;

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_column_ignore");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_pos_column_ignore "
         "(col1 INT NOT NULL PRIMARY KEY, col2 VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt,
         "INSERT INTO t_pos_column_ignore VALUES (10,'venu'),(100,'MySQL')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY,
                                (SQLPOINTER)SQL_CONCUR_ROWVER, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0));

  /* ignore all columns */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_column_ignore ORDER BY col1 ASC");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, &pcbValue));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
                            &pcbValue));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, &pcrow,
                                  &rgfRowStatus));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  nData= 99;
  strcpy((char *)szData , "updated");

  pcbValue= SQL_COLUMN_IGNORE;
  /* We have all columns ignored - connector should return error and 21S02 */
  EXPECT_STMT(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE), SQL_ERROR);
  CHECK_SQLSTATE(Stmt, "21S02");

  /* Affected rows should be 0 !! */
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &Rows));
  FAIL_IF(Rows != 0, "Expected 0 rows");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_column_ignore ORDER BY col1 ASC");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 10);
  IS_STR(my_fetch_str(Stmt, szData, 2), "venu", 4);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* ignore only one column */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_column_ignore ORDER BY col1 ASC");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
                            &pcbValue));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, &pcrow,
                                  &rgfRowStatus));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  nData= 99;
  strcpy((char *)szData , "updated");

  pcbValue= SQL_COLUMN_IGNORE;
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));
  is_num(nlen, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_column_ignore ORDER BY col1 ASC");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 99);
  IS_STR(my_fetch_str(Stmt, szData, 2), "venu", 4);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_column_ignore");

  return OK;
}


ODBC_TEST(t_pos_datetime_delete)
{
  SQLHSTMT     hstmt1;
  SQLINTEGER   int_data;
  SQLLEN       row_count;
  SQLUSMALLINT rgfRowStatus;

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_datetime_delete");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_pos_datetime_delete (a INT NOT NULL DEFAULT 0,"
         "b VARCHAR(20) NOT NULL DEFAULT '', c DATETIME NOT NULL DEFAULT '2000-01-01')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_datetime_delete VALUES"
         "(1,'venu','2003-02-10 14:45:39')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_datetime_delete (b) VALUES ('')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_datetime_delete (a) VALUES (2)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY,
                                (SQLPOINTER)SQL_CONCUR_ROWVER, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu_cur", 8));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_datetime_delete");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &int_data, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, NULL,
                                  &rgfRowStatus));
  is_num(int_data, 1);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &hstmt1));
  CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CONCURRENCY,
                                 (SQLPOINTER)SQL_CONCUR_ROWVER, 0));
  CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
                                 (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0));

  OK_SIMPLE_STMT(hstmt1, "DELETE FROM t_pos_datetime_delete WHERE CURRENT OF venu_cur");

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &row_count));
  is_num(row_count, 1);

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, NULL,
                                  &rgfRowStatus));
  is_num(int_data, 0);

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, NULL, NULL));
  is_num(int_data, 2);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));
  is_num(int_data, 2);

  OK_SIMPLE_STMT(hstmt1, "DELETE FROM t_pos_datetime_delete WHERE CURRENT OF venu_cur");

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &row_count));
  is_num(row_count, 1);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_datetime_delete");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 0);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_datetime_delete");

  return OK;
}


ODBC_TEST(t_pos_datetime_delete1)
{
  SQLRETURN rc;
  SQLHSTMT hstmt1;
  SQLINTEGER int_data;
  SQLLEN row_count, cur_type;
  SQLUSMALLINT rgfRowStatus;


  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_delete");

  rc = SQLAllocStmt(Connection,&hstmt1);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_pos_delete(id int not null default '0',\
                                                  name varchar(20) NOT NULL default '',\
                                                  created datetime NOT NULL default '2000-01-01')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_delete VALUES(1,'venu','2003-02-10 14:45:39')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_delete(name) VALUES('')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_delete(id) VALUES(2)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_delete(id) VALUES(3)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_delete(id) VALUES(4)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_delete(id) VALUES(5)");

  rc = SQLTransact(NULL, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_delete");

  IS(6 == myrowcount(Stmt));

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_ROWVER, 0);
  SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_DYNAMIC, 0);
  SQLSetStmtOption(Stmt,SQL_SIMULATE_CURSOR,SQL_SC_NON_UNIQUE);

  SQLSetStmtAttr(hstmt1, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_ROWVER, 0);
  SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_DYNAMIC, 0);
  SQLSetStmtOption(hstmt1,SQL_SIMULATE_CURSOR,SQL_SC_NON_UNIQUE);

  rc = SQLSetCursorName(Stmt, (SQLCHAR *)"venu_cur",8);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, &cur_type, 0, NULL);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_delete");

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&int_data,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_ABSOLUTE,3,NULL,&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);
  fprintf(stdout,"current_row: %d\n", int_data);
  IS(int_data == 2);

  rc = SQLSetPos(Stmt,1,SQL_POSITION,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(hstmt1, "DELETE FROM t_pos_delete WHERE CURRENT OF venu_cur");

  rc = SQLRowCount(hstmt1,&row_count);
  CHECK_STMT_RC(hstmt1,rc);
  fprintf(stdout, "rows affected: %d\n", (int)row_count);
  IS(row_count == 1);

  rc = SQLExtendedFetch(Stmt, SQL_FETCH_NEXT,1,NULL,&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);
  fprintf(stdout,"current_row: %d\n", int_data);

  rc = SQLExtendedFetch(Stmt, SQL_FETCH_NEXT,1,NULL,&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);
  fprintf(stdout,"current_row: %d\n", int_data);

  /*rc = SQLExtendedFetch(Stmt,SQL_FETCH_NEXT,1,NULL,NULL);
  CHECK_STMT_RC(Stmt,rc);*/

  rc = SQLSetPos(Stmt,1,SQL_POSITION,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(hstmt1, "DELETE FROM t_pos_delete WHERE CURRENT OF venu_cur");

  rc = SQLRowCount(hstmt1,&row_count);
  CHECK_STMT_RC(hstmt1,rc);
  fprintf(stdout, "rows affected: %d\n", (int)row_count);
  IS(row_count == 1);

  SQLFreeStmt(Stmt,SQL_UNBIND);
  SQLFreeStmt(Stmt,SQL_CLOSE);
  SQLFreeStmt(hstmt1,SQL_CLOSE);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_delete");

  IS(4 == myrowcount(Stmt));

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(hstmt1,SQL_DROP);
  CHECK_STMT_RC(hstmt1,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_delete");

  return OK;
}


ODBC_TEST(t_getcursor)
{
  SQLRETURN rc;
  SQLHSTMT hstmt1,hstmt2,hstmt3;
  SQLCHAR curname[50];
  SQLSMALLINT nlen;

  rc = SQLAllocHandle(SQL_HANDLE_STMT,Connection,&hstmt1);
  CHECK_DBC_RC(Connection, rc);
  rc = SQLAllocHandle(SQL_HANDLE_STMT,Connection,&hstmt2);
  CHECK_DBC_RC(Connection, rc);
  rc = SQLAllocHandle(SQL_HANDLE_STMT,Connection,&hstmt3);
  CHECK_DBC_RC(Connection, rc);

  rc = SQLGetCursorName(hstmt1, curname, 50, &nlen);
  if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
  {
    fprintf(stdout,"default cursor name  : %s(%d)\n", curname, nlen);
    is_num(nlen, 8);
    IS_STR(curname,"SQL_CUR0", 9);

    rc = SQLGetCursorName(hstmt3, curname, 50, &nlen);
    CHECK_STMT_RC(hstmt1, rc);
    fprintf(stdout,"default cursor name  : %s(%d)\n", curname, nlen);

    rc = SQLGetCursorName(hstmt1,curname, 4, &nlen);
    FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "expected success with info");
    fprintf(stdout,"truncated cursor name: %s(%d)\n", curname, nlen);
    is_num(nlen, 8);
    IS_STR(curname, "SQL", 4);

    rc = SQLGetCursorName(hstmt1, curname, 0, &nlen);
    FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "expected success with info");
    fprintf(stdout, "untouched cursor name: %s(%d)\n", curname, nlen);
    IS(nlen == 8);

    FAIL_IF(SQLGetCursorName(hstmt1, curname, 8, &nlen) != SQL_SUCCESS_WITH_INFO, "success with info expected");
    fprintf(stdout, "truncated cursor name: %s(%d)\n", curname, nlen);
    is_num(nlen, 8);
    IS_STR(curname, "SQL_CUR", 8);

    rc = SQLGetCursorName(hstmt1,curname, 9, &nlen);
    fprintf(stdout, "full cursor name     : %s(%d)\n", curname, nlen);
    is_num(nlen, 8);
    IS_STR(curname, "SQL_CUR0", 9);
  }

  rc = SQLSetCursorName(hstmt1, (SQLCHAR *)"venucur123", 7);
  CHECK_STMT_RC(hstmt1, rc);

  rc = SQLGetCursorName(hstmt1, curname, 8, &nlen);
  CHECK_STMT_RC(hstmt1, rc);
  is_num(nlen, 7);
  IS_STR(curname, "venucur", 8);

  rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt1);
  CHECK_STMT_RC(hstmt1, rc);

  return OK;
}


ODBC_TEST(t_getcursor1)
{
  SQLHSTMT hstmt1;
  SQLCHAR curname[50];
  SQLSMALLINT nlen,index;

  for(index=0; index < 100; index++)
  {
    CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT,Connection,&hstmt1));
    CHECK_STMT_RC(hstmt1, SQLGetCursorName(hstmt1,curname,50,&nlen));
    fprintf(stdout,"%s(%d) \n",curname,nlen);
    CHECK_STMT_RC(hstmt1, SQLFreeHandle(SQL_HANDLE_STMT, hstmt1));
  }

  return OK;
}


ODBC_TEST(t_acc_crash)
{
  SQLINTEGER  id;
  SQLCHAR     name[20], data[30];
  /* Hasn't that actually to be a SQL_DATE_STRUCT */
  SQL_TIMESTAMP_STRUCT ts;
  SQLLEN      ind_strlen;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_acc_crash");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_acc_crash (a INT NOT NULL AUTO_INCREMENT,"
         "b CHAR(20), c DATE, PRIMARY KEY (a))");
  OK_SIMPLE_STMT(Stmt,
         "INSERT INTO t_acc_crash (b) VALUES ('venu'),('monty'),('mysql')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtOption(Stmt, SQL_ROWSET_SIZE, 1));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_acc_crash ORDER BY a ASC");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &id, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, name, sizeof(name), NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_DATE, &ts, 0, &ind_strlen));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1));
  
  id= 9;
  strcpy((char *)name, "updated");
  ts.year= 2010;
  ts.month= 9;
  ts.day= 25;

  ind_strlen= 0;
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_acc_crash ORDER BY a DESC");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(my_fetch_int(Stmt, 1), 9);
  IS_STR(my_fetch_str(Stmt, data, 2), "updated", 7);
  IS_STR(my_fetch_str(Stmt, data, 3), "2010-09-25", 10);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_acc_crash");

  return OK;
}


ODBC_TEST(tmysql_setpos_del)
{
  SQLINTEGER nData;
  SQLLEN nlen;
  SQLCHAR szData[255];
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos_del");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_setpos_del (a INT, b VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_del VALUES (100,'MySQL1'),"
         "(200,'MySQL2'),(300,'MySQL3'),(400,'MySQL4'),(300,'MySQL5'),"
         "(300,'MySQL6'),(300,'MySQL7')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos_del");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
                            &nlen));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 5, &pcrow,
                                  &rgfRowStatus));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));
  is_num(nlen, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos_del");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 100);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL1", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 200);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL2", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 300);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL3", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 400);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL4", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 300);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL6", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 300);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL7", 6);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos_del");

  return OK;
}


ODBC_TEST(tmysql_setpos_del1)
{
  SQLINTEGER nData;
  SQLLEN nlen;
  SQLCHAR szData[255];
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos_del1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_setpos_del1 (a INT, b VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_del1 VALUES (100,'MySQL1'),"
         "(200,'MySQL2'),(300,'MySQL3'),(400,'MySQL4')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));
  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos_del1");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
                            &nlen));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 3, &pcrow,
                                  &rgfRowStatus));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 0, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));
  is_num(nlen, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos_del1");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 100);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL1", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 200);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL2", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 400);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL4", 6);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos_del1");

  return OK;
}


ODBC_TEST(tmysql_setpos_upd)
{
  SQLRETURN rc;
  SQLINTEGER nData = 500;
  SQLLEN nlen;
  SQLCHAR szData[255]={0};
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_setpos(col1 int, col2 varchar(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos VALUES(100,'MySQL1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos VALUES(300,'MySQL3')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos VALUES(200,'MySQL2')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos VALUES(300,'MySQL3')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos VALUES(400,'MySQL4')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos VALUES(300,'MySQL3')");

  rc = SQLTransact(NULL, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  rc = SQLSetCursorName(Stmt, (SQLCHAR *)"venu",SQL_NTS);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos");

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,100,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,100,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE,3,&pcrow,&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);

  diag(" pcrow:%d\n",pcrow);

  diag(" row1:%d,%s\n",nData,szData);

  rc = SQLSetPos(Stmt,1,SQL_POSITION,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  nData = 1000;
  strcpy((char *)szData , "updated");

  rc = SQLSetPos(Stmt,3,SQL_UPDATE,SQL_LOCK_NO_CHANGE);
  FAIL_IF(rc != SQL_ERROR, "expected error");
    
  rc = SQLSetPos(Stmt,1,SQL_UPDATE,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt,&nlen);
  CHECK_STMT_RC(Stmt,rc);

  diag(" rows affected:%d\n",nlen);

  rc = SQLFreeStmt(Stmt, SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos");

  myrowcount(Stmt);

  rc = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DELETE FROM tmysql_setpos WHERE col2 = 'updated'");

  rc = SQLRowCount(Stmt,&nlen);
  CHECK_STMT_RC(Stmt,rc);
  diag("\n total rows affected:%d",nlen);
  IS(nlen == 1);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos");

  IS(5 == myrowcount(Stmt));

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos");

  return OK;
}


ODBC_TEST(tmysql_setpos_add)
{
  SQLRETURN rc;
  SQLINTEGER nData= 500;
  SQLLEN nlen;
  SQLCHAR szData[255]={0};
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos_add");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_setpos_add(col1 int, col2 varchar(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_add VALUES(100,'MySQL1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_add VALUES(300,'MySQL3')");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetCursorName(Stmt, (SQLCHAR *)"venu",SQL_NTS);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos_add");

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,100,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,100,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_NEXT,1,&pcrow,&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);

  nData = 1000;
  strcpy((char *)szData , "insert-new1");

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

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM tmysql_setpos_add");

  IS(6 == myrowcount(Stmt));

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos_add");

  return OK;
}


ODBC_TEST(tmysql_pos_delete)
{
  SQLHSTMT hstmt1;
  SQLLEN rows;
  SQLCHAR buff[10];

  CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &hstmt1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_pos_delete");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_pos_delete (a INT, b VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_pos_delete VALUES (1,'venu'),(2,'MySQL')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu_cur", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_pos_delete");

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, NULL, NULL));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  FAIL_IF(SQLExecDirect(hstmt1, (SQLCHAR*)"   DfffELETE FROM tmysql_pos_delete WHERE CURRENT OF venu_cur", SQL_NTS) != SQL_ERROR, "error expected");

  FAIL_IF(SQLExecDirect(hstmt1, (SQLCHAR*)"   DELETE FROM tmysql_pos_delete WHERE CURRENT OF venu_cur curs", SQL_NTS) != SQL_ERROR, "error expected");
             
  FAIL_IF(SQLExecDirect(hstmt1, (SQLCHAR*)"   DELETE FROM tmysql_pos_delete WHERE ONE CURRENT OF venu_cur", SQL_NTS) != SQL_ERROR, "error expected");
             
  OK_SIMPLE_STMT(hstmt1, "   DELETE FROM tmysql_pos_delete WHERE CURRENT OF venu_cur");

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rows));
  is_num(rows, 1);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_pos_delete");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(my_fetch_int(Stmt, 1), 2);
  IS_STR(my_fetch_str(Stmt, buff, 2), "MySQL", 5);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_pos_delete");

  return OK;
}


ODBC_TEST(t_pos_update)
{
  SQLHSTMT hstmt1;
  SQLCHAR  szData[10];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_update");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_pos_update (col1 INT, col2 VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_update VALUES (100,'venu'),(200,'MySQL')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu_cur", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_update");

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 2, NULL, NULL));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &hstmt1));

  FAIL_IF(SQLExecDirect(hstmt1, (SQLCHAR*)"  UPerrDATE t_pos_update SET col1 = 999, col2 = 'update' "
             "WHERE CURRENT OF venu_cur", SQL_NTS) != SQL_ERROR, "Error expected");

  FAIL_IF(SQLExecDirect(hstmt1,
   (SQLCHAR*)"  UPDATE t_pos_update SET col1 = 999, col2 = 'update' "
             "WHERE CURRENT OF", SQL_NTS) != SQL_ERROR, "Error expected");

  OK_SIMPLE_STMT(hstmt1,
    (SQLCHAR*)"  UPDATE t_pos_update SET col1 = 999, col2 = 'update' "
         "WHERE CURRENT OF venu_cur");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
  CHECK_STMT_RC(hstmt1, SQLFreeHandle(SQL_HANDLE_STMT, hstmt1));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_update");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 100);
  IS_STR(my_fetch_str(Stmt, szData, 2), "venu", 4);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 999);
  IS_STR(my_fetch_str(Stmt, szData, 2), "update", 5);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_update");

  return OK;
}


ODBC_TEST(tmysql_pos_update_ex)
{
  SQLHSTMT hstmt1;
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;
  SQLLEN rows;
  SQLCHAR cursor[30], sql[255], data[]= "tmysql_pos_update_ex";

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_updex");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_pos_updex (a INT PRIMARY KEY, b VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_updex VALUES (100,'venu'),(200,'MySQL')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_updex");

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 2,
                                  &pcrow, &rgfRowStatus));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLGetCursorName(Stmt, cursor, sizeof(cursor), NULL));

  CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                   SQL_CHAR, 0, 0, data, sizeof(data), NULL));

  sprintf((char *)sql,
          "UPDATE t_pos_updex SET a = 999, b = ? WHERE CURRENT OF %s",
          cursor);

  CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, sql, SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rows));
  is_num(rows, 1);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_updex");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 100);
  IS_STR(my_fetch_str(Stmt, sql, 2), "venu", 4);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 999);
  IS_STR(my_fetch_str(Stmt, sql, 2), "tmysql_pos_update_ex", 20);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_updex");

  return OK;
}


ODBC_TEST(tmysql_pos_update_ex1)
{
  SQLHSTMT hstmt1;
  SQLULEN pcrow;
  SQLLEN rows;
  SQLUSMALLINT rgfRowStatus;
  SQLCHAR cursor[30], sql[100], data[]= "tmysql_pos_update_ex1";

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_updex1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_pos_updex1  (a INT, b VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_updex1 VALUES (100,'venu'),(200,'MySQL')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_updex1");

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 2, &pcrow,
                                  &rgfRowStatus));
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLGetCursorName(Stmt, cursor, sizeof(cursor), NULL));

  CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                   SQL_CHAR, 0, 0, data, sizeof(data), NULL));

  sprintf((char *)sql,
          "UPDATE t_pos_updex1 SET a = 999, b = ? WHERE CURRENT OF %s", cursor);

  CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, sql, SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rows));
  is_num(rows, 1);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_updex1");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 100);
  IS_STR(my_fetch_str(Stmt, sql, 2), "venu", 4);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 999);
  IS_STR(my_fetch_str(Stmt, sql, 2), "tmysql_pos_update_ex1", 21);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_updex1");

  return OK;
}


ODBC_TEST(tmysql_pos_update_ex3)
{
  SQLHSTMT hstmt1;
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;
  SQLCHAR cursor[30], sql[255];
  SQLCHAR *test= (SQLCHAR*)"test";

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_updex3");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_pos_updex3 (a INT NOT NULL PRIMARY KEY,"
        " b VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_updex3 VALUES (100,'venu'),(200,'MySQL')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt,  "SELECT a, b FROM t_pos_updex3");

  if (ForwardOnly)
  {
    CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 2, &pcrow,
      &rgfRowStatus));
  }
  else
  {
    CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 2, &pcrow,
      &rgfRowStatus));
  }
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLGetCursorName(Stmt, cursor, sizeof(cursor), NULL));

  CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &hstmt1));

  sprintf((char *)sql,
          "UPDATE t_pos_updex3 SET a = 999, b = ? WHERE CURRENT OF %s", cursor);

  /* In case of RS streaming this returns not the error this test expects. Thus the error should be verified to be
     a  right error */
  ERR_SIMPLE_STMT(hstmt1, sql);
  odbc_print_error(SQL_HANDLE_STMT, hstmt1);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_updex3");

  return OK;
}


ODBC_TEST(tmysql_pos_update_ex4)
{
  SQLULEN pcrow;
  SQLLEN nlen= SQL_NTS;
  SQLCHAR data[]= "venu", szData[20];
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_updex4");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_pos_updex4 (a VARCHAR(20) NOT NULL,"
         "b VARCHAR(20) NOT NULL, c VARCHAR(5), PRIMARY KEY (b))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_pos_updex4 (a,b) VALUES ('Monty','Widenius')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_pos_updex4");

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 1, &pcrow,
                                  &rgfRowStatus));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &nlen));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));
  is_num(nlen, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT a FROM t_pos_updex4");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, szData, 1), "venu", 4);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_pos_updex4");

  return OK;
}


ODBC_TEST(tmysql_pos_dyncursor)
{
  SQLHSTMT hstmt1;
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;
  SQLCHAR buff[100];
  SQLLEN rows;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_pos_dyncursor");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_pos_dyncursor (a INT, b VARCHAR(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_pos_dyncursor VALUES (1,'foo'),(2,'bar')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu_cur", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_pos_dyncursor");

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 2, &pcrow,
                                  &rgfRowStatus));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "UPDATE tmysql_pos_dyncursor SET a = 9, b = 'update' "
         "WHERE CURRENT OF venu_cur");

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rows));
  is_num(rows, 1);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_pos_dyncursor");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  IS_STR(my_fetch_str(Stmt, buff, 2), "foo", 3);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 9);
  IS_STR(my_fetch_str(Stmt, buff, 2), "update", 6);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_pos_dyncursor");

  return OK;
}


ODBC_TEST(tmysql_mtab_setpos_del)
{
  SQLRETURN rc;
  SQLINTEGER nData= 500;
  SQLLEN nlen;
  SQLCHAR szData[255]={0};
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_t1, tmysql_t2");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_t1(col1 int, col2 varchar(20))");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_t2(col1 int, col2 varchar(20))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_t1 VALUES(1,'t1_one')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_t1 VALUES(2,'t1_two')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_t1 VALUES(3,'t1_three')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_t2 VALUES(2,'t2_one')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_t2 VALUES(3,'t2_two')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_t2 VALUES(4,'t2_three')");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  rc = SQLSetCursorName(Stmt, (SQLCHAR *)"venu",SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  /* FULL JOIN */
  OK_SIMPLE_STMT(Stmt, "select tmysql_t1.*,tmysql_t2.* from tmysql_t1,tmysql_t2");

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,100,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,100,&nlen);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_ABSOLUTE,3,&pcrow,&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);

  diag(" pcrow:%d\n",pcrow);

  diag(" row1:%d,%s\n",nData,szData);

  rc = SQLSetPos(Stmt,1,SQL_POSITION,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  /* not yet supported..*/
  rc = SQLSetPos(Stmt,2,SQL_DELETE,SQL_LOCK_NO_CHANGE);
  FAIL_IF(rc != SQL_ERROR, "error expected");
  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_t1, tmysql_t2");

  return OK;
}


ODBC_TEST(tmysql_setpos_pkdel)
{
  SQLRETURN rc;
  SQLINTEGER nData= 500;
  SQLLEN nlen;
  SQLCHAR szData[255]={0};
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos1");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_setpos1(col1 int primary key, col2 varchar(30))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos1 VALUES(100,'MySQL1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos1 VALUES(200,'MySQL2')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos1 VALUES(300,'MySQL3')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos1 VALUES(400,'MySQL4')");
 
  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  rc = SQLSetCursorName(Stmt, (SQLCHAR *)"venu",SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos1");

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,100,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,100,&nlen);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_ABSOLUTE,4,&pcrow,&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);

  diag(" pcrow:%d\n",pcrow);

  diag(" row1:%d,%s\n",nData,szData);

  rc = SQLSetPos(Stmt,1,SQL_POSITION,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetPos(Stmt,1,SQL_DELETE,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt,&nlen);
  CHECK_STMT_RC(Stmt,rc);

  diag(" rows affected:%d\n",nlen);

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos1");

  IS( 3 == myrowcount(Stmt));

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos1");

  return OK;
}


ODBC_TEST(t_alias_setpos_pkdel)
{
  SQLINTEGER nData= 500;
  SQLLEN nlen;
  SQLCHAR szData[255]= {0};
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_alias_setpos_pkdel");

  CHECK_DBC_RC(Connection, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));

  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_alias_setpos_pkdel (col1 INT PRIMARY KEY,"
        " col2 VARCHAR(30))");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_alias_setpos_pkdel VALUES (100, 'MySQL1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_alias_setpos_pkdel VALUES (200, 'MySQL2')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_alias_setpos_pkdel VALUES (300, 'MySQL3')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_alias_setpos_pkdel VALUES (400, 'MySQL4')");

  CHECK_DBC_RC(Connection, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu", SQL_NTS));

  OK_SIMPLE_STMT(Stmt,"SELECT col1 AS id, col2 AS name FROM t_alias_setpos_pkdel");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
                            &nlen));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 1,
                                  &pcrow, &rgfRowStatus));

  diag("pcrow:%d, rgfRowStatus:%d", pcrow, rgfRowStatus);
  diag(" row1:%d, %s", nData, szData);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));

  diag("rows affected:%d",nlen);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_alias_setpos_pkdel");

  IS(3 == myrowcount(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_alias_setpos_pkdel");

  return OK;
}


ODBC_TEST(t_alias_setpos_del)
{
  SQLINTEGER nData= 500;
  SQLLEN nlen;
  SQLCHAR szData[255]= {0};
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_alias_setpos_del");

  CHECK_DBC_RC(Connection, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_alias_setpos_del (col1 INT, col2 VARCHAR(30))");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_alias_setpos_del VALUES (100, 'MySQL1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_alias_setpos_del VALUES (200, 'MySQL2')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_alias_setpos_del VALUES (300, 'MySQL3')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_alias_setpos_del VALUES (400, 'MySQL4')");

  CHECK_DBC_RC(Connection, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu", SQL_NTS));

  OK_SIMPLE_STMT(Stmt,"SELECT col1 AS id, col2 AS name FROM t_alias_setpos_del");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
                            &nlen));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 1,
                                  &pcrow, &rgfRowStatus));

  diag("pcrow:%d, rgfRowStatus:%d", pcrow, rgfRowStatus);
  diag(" row1:%d, %s", nData, szData);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));

  diag("rows affected:%d",nlen);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_alias_setpos_del");

  IS(3 == myrowcount(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_alias_setpos_del");

  return OK;
}


ODBC_TEST(tmysql_setpos_pkdel2)
{
  SQLINTEGER nData= 500;
  SQLLEN nlen;
  SQLCHAR szData[255]= {0};
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos_pkdel2");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_setpos_pkdel2 (a INT, b INT,"
         "c VARCHAR(30) PRIMARY KEY)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_pkdel2 VALUES (100,10,'MySQL1'),"
         "(200,20,'MySQL2'),(300,30,'MySQL3'),(400,40,'MySQL4')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT b,c FROM tmysql_setpos_pkdel2");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
                            &nlen));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 4,
                                  &pcrow, &rgfRowStatus));
  is_num(pcrow, 1);
  is_num(nData, 40);
  IS_STR(szData, "MySQL4", 6);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));
  is_num(nlen, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos_pkdel2");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 100);
  is_num(my_fetch_int(Stmt, 2), 10);
  IS_STR(my_fetch_str(Stmt, szData, 3), "MySQL1", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 200);
  is_num(my_fetch_int(Stmt, 2), 20);
  IS_STR(my_fetch_str(Stmt, szData, 3), "MySQL2", 6);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 300);
  is_num(my_fetch_int(Stmt, 2), 30);
  IS_STR(my_fetch_str(Stmt, szData, 3), "MySQL3", 6);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos_pkdel2");

  return OK;
}


ODBC_TEST(t_setpos_upd_bug1)
{
  SQLRETURN rc;
  SQLINTEGER id;
  SQLLEN len,id_len,f_len,l_len,ts_len;
  SQLCHAR fname[21],lname[21],szTable[256];
  SQL_TIMESTAMP_STRUCT ts;
  SQLSMALLINT pccol;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_setpos_upd_bug1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_setpos_upd_bug1(id int(11) NOT NULL auto_increment,\
                                                          fname char(20) NOT NULL default '',\
                                                          lname char(20) NOT NULL default '',\
                                                          last_modi timestamp,\
                                                          PRIMARY KEY(id)) ENGINE=MyISAM");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_setpos_upd_bug1(fname,lname) VALUES('joshua','kugler')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_setpos_upd_bug1(fname,lname) VALUES('monty','widenius')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_setpos_upd_bug1(fname,lname) VALUES('mr.','venu')");

  rc = SQLTransact(NULL, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_setpos_upd_bug1 order by id asc");

  rc = SQLNumResultCols(Stmt,&pccol);
  CHECK_STMT_RC(Stmt,rc);

  diag(" total columns:%d\n",pccol);

  rc = SQLBindCol(Stmt,1,SQL_C_SLONG,&id,4,&id_len);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,fname,6,&f_len);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,3,SQL_C_CHAR,lname,20,&l_len);
  CHECK_STMT_RC(Stmt,rc);

  diag("type: %d", SQL_C_TIMESTAMP);
  rc = SQLBindCol(Stmt,4,SQL_C_TIMESTAMP,&ts,16,&ts_len);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLColAttribute(Stmt,1,SQL_COLUMN_TABLE_NAME,szTable,sizeof(szTable),NULL,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_FIRST,0,NULL,&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetStmtOption(Stmt,SQL_QUERY_TIMEOUT,30);
  CHECK_STMT_RC(Stmt,rc);

  strcpy((char *)fname , "updated");
  strcpy((char *)lname , "updated01234567890");

  rc = SQLSetPos(Stmt,1,SQL_UPDATE,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt,&len);
  CHECK_STMT_RC(Stmt,rc);

  diag(" rows affected:%d\n",len);

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_setpos_upd_bug1");

  myrowcount(Stmt);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DELETE FROM t_setpos_upd_bug1 WHERE fname = 'update'");

  rc = SQLRowCount(Stmt,&len);
  CHECK_STMT_RC(Stmt,rc);
  diag("\n total rows affceted:%d",len);
  IS(len == 1);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_setpos_upd_bug1");

  IS(2 == myrowcount(Stmt));

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_setpos_upd_bug1");

  return OK;
}


ODBC_TEST(my_setpos_upd_pk_order)
{
  SQLRETURN rc;
  SQLINTEGER nData= 500;
  SQLLEN nlen;
  SQLCHAR szData[255]={0};
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_setpos_upd_pk_order");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_setpos_upd_pk_order(col1 int not null, col2 varchar(30) NOT NULL, primary key(col2,col1))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_setpos_upd_pk_order VALUES(100,'MySQL1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_setpos_upd_pk_order VALUES(200,'MySQL2')");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  rc = SQLSetCursorName(Stmt, (SQLCHAR *)"venu",SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_setpos_upd_pk_order");

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nData,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szData,sizeof(szData),NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_ABSOLUTE,2,&pcrow,&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);

  diag(" row1:%d,%s\n",nData,szData);

  nData = 1000;
  strcpy((char *)szData , "updated");

  rc = SQLSetPos(Stmt,1,SQL_UPDATE,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt,&nlen);
  CHECK_STMT_RC(Stmt,rc);

  is_num(1, nlen);

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_setpos_upd_pk_order");

  myrowcount(Stmt);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DELETE FROM my_setpos_upd_pk_order WHERE col2 = 'updated'");

  rc = SQLRowCount(Stmt,&nlen);
  CHECK_STMT_RC(Stmt,rc);
  diag("\n total rows affceted:%d",nlen);
  IS(nlen == 1);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_setpos_upd_pk_order");

  return OK;
}


/**
  In this test, we prove that we can update a row in a table with a
  multi-part primary key even though we're only updating two parts of
  the key.
 */
ODBC_TEST(my_setpos_upd_pk_order1)
{
  SQLINTEGER nData;
  SQLCHAR szData[255];
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;
  SQLLEN rows;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_setpos_upd_pk_order1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_setpos_upd_pk_order1 (a INT NOT NULL,"
         "b VARCHAR(30) NOT NULL, c INT NOT NULL, PRIMARY KEY (a,b,c))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_setpos_upd_pk_order1 VALUES (100,'MySQL1',1),"
         "(200,'MySQL2',2)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_setpos_upd_pk_order1");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
                            NULL));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, &pcrow,
                                  &rgfRowStatus));
  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, &pcrow,
                                  &rgfRowStatus));

  nData= 1000;
  strcpy((char *)szData, "updated");

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rows));
  is_num(rows, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_setpos_upd_pk_order1");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 100);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL1", 6);
  is_num(my_fetch_int(Stmt, 3), 1);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1000);
  IS_STR(my_fetch_str(Stmt, szData, 2), "updated", 7);
  is_num(my_fetch_int(Stmt, 3), 2);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_setpos_upd_pk_order1");

  return OK;
}


ODBC_TEST(tmy_cursor1)
{
  SQLCHAR getCurName[20];
  SQLSMALLINT getLen;

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"MYSQL", 5));
  CHECK_STMT_RC(Stmt, SQLGetCursorName(Stmt, getCurName, 20, &getLen));
  IS_STR(getCurName, "MYSQL", 6);

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"MYSQL", 10));
  CHECK_STMT_RC(Stmt, SQLGetCursorName(Stmt, getCurName, 20, &getLen));
  IS_STR(getCurName, "MYSQL", 6);

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"MYSQL", 2));
  CHECK_STMT_RC(Stmt, SQLGetCursorName(Stmt, getCurName, 20, &getLen));
  IS_STR(getCurName, "MY", 3);

  return OK;
}


ODBC_TEST(tmy_cursor2)
{
  SQLCHAR     getCursor[50]= {0};
  SQLSMALLINT getLen;

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"MYODBC", 6));

  FAIL_IF(SQLGetCursorName(Stmt, getCursor, 0, &getLen) != SQL_SUCCESS_WITH_INFO, "success with info expected");
  IS_STR(getCursor, "", 1);
  is_num(getLen, 6);

  FAIL_IF(SQLGetCursorName(Stmt, getCursor, -1, &getLen) != SQL_ERROR, "Error expected");

  FAIL_IF(SQLGetCursorName(Stmt, getCursor, 4, &getLen) != SQL_SUCCESS_WITH_INFO, "success with info expected");
              
  IS_STR(getCursor, "MYO", 4);
  is_num(getLen, 6);

  FAIL_IF(SQLGetCursorName(Stmt, getCursor, 6, &getLen)!= SQL_SUCCESS_WITH_INFO, "success with info expected");
              
  IS_STR(getCursor, "MYODB", 6);
  is_num(getLen, 6);

  CHECK_STMT_RC(Stmt, SQLGetCursorName(Stmt, getCursor, 7, &getLen));
  IS_STR(getCursor, "MYODBC", 7);
  is_num(getLen, 6);

  return OK;
}


ODBC_TEST(tmy_cursor3)
{
#if IODBC_BUG_FIXED
  /*
    iODBC has a bug that forces the ODBCv2 behavior of throwing an error
    when SQLSetCursorName() has not bee called and there is no open cursor.
  */
  SQLCHAR     getCursor[50];
  SQLSMALLINT getLen= -1;
  SQLHSTMT    hstmt1;

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"MYSQLODBC", 9));

  /* New statement should get its own (generated) cursor name. */
  CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &hstmt1));
  CHECK_STMT_RC(hstmt1, SQLGetCursorName(hstmt1, getCursor, 20, &getLen));
  IS_STR(getCursor, "SQL_CUR", 7);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
#endif

  return OK;
}


ODBC_TEST(tmysql_pcbvalue)
{
  SQLCHAR    szdata[20], sztdata[100];
  SQLINTEGER nodata;
  SQLLEN     nlen, slen, tlen;
  SQLUSMALLINT rgfRowStatus[20];

  OK_SIMPLE_STMT(Stmt, "SET SESSION sql_mode=''");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_pcbvalue");

  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE tmysql_pcbvalue (col1 INT PRIMARY KEY,"
         "                              col2 VARCHAR(1), col3 TEXT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_pcbvalue VALUES (100,'venu','mysql')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_pcbvalue VALUES (200,'monty','mysql')");

  CHECK_DBC_RC(Connection, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM tmysql_pcbvalue");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nodata, 0, &nlen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szdata, sizeof(szdata),
                            &slen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_CHAR, sztdata, sizeof(sztdata),
                            &tlen));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_FIRST, 1, NULL,
                                  rgfRowStatus));

  diag("row1: %d(%d), %s(%d),%s(%d)\n",
               nodata, nlen, szdata, slen, sztdata, tlen);

  strcpy((char *)szdata, "updated-one");

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, NULL, rgfRowStatus));

  diag("row2: %d(%d), %s(%d),%s(%d)\n",
               nodata, nlen, szdata, slen, sztdata, tlen);

  FAIL_IF(SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, NULL,
                                      rgfRowStatus)!=  SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_DBC_RC(Connection, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_pcbvalue");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_CHAR, szdata, sizeof(szdata),
                            &slen));

  diag("updated data:%s(%d)\n",szdata,slen);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_pcbvalue");

  CHECK_DBC_RC(Connection, SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT));

  return OK;
}


/**
 Bug #28255: Cursor operations on result sets containing only part of a key
 are incorrect
*/
ODBC_TEST(t_bug28255)
{
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug28255");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug28255 (a INT, b INT, PRIMARY KEY (a,b))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug28255 VALUES (1,3),(1,4),(1,5)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"bug", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT a FROM t_bug28255 WHERE b > 3");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));
  FAIL_IF(SQLSetPos(Stmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE) != SQL_ERROR, "Error expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug28255");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  is_num(my_fetch_int(Stmt, 2), 3);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  is_num(my_fetch_int(Stmt, 2), 4);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  is_num(my_fetch_int(Stmt, 2), 5);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug28255");

  return OK;
}


/**
 Bug #10563: Update using multicolumn primary key with duplicate indexes fails
*/
ODBC_TEST(bug10563)
{
  SQLLEN nlen;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug10563");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug10563 (a INT, b INT, PRIMARY KEY (a,b), UNIQUE (b))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug10563 VALUES (1,3),(1,4)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"bug", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT b FROM t_bug10563 WHERE b > 3");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 4);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));
  is_num(nlen, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug10563");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  is_num(my_fetch_int(Stmt, 2), 3);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug10563");

  return OK;
}


/*
 * Bug 6741 - SQL_ATTR_ROW_BIND_OFFSET_PTR is not supported
 * It was supported for use in some batch operations, but not
 * standard cursor operations.
 */
#define BUG6741_VALS 5

ODBC_TEST(bug6741)
{
  int i;
  SQLLEN offset;
  struct {
    SQLINTEGER xval;
    SQLINTEGER dummy;
    SQLLEN ylen;
  } results[BUG6741_VALS];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug6741");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug6741 (x int, y int)");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug6741 values (0,0),(1,NULL),(2,2),(3,NULL),(4,4)");
  OK_SIMPLE_STMT(Stmt, "select x,y from t_bug6741 order by x");

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_BIND_OFFSET_PTR,
          &offset, SQL_IS_POINTER));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &results[0].xval, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_LONG, &results[0].dummy, 0, &results[0].ylen));

  /* fetch all the data */
  for(i = 0; i < BUG6741_VALS; ++i)
  {
    offset = i * sizeof(results[0]);
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  }
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  /* verify it */
  for(i = 0; i < BUG6741_VALS; ++i)
  {
    printf("xval[%d] = %d\n", i, results[i].xval);
    printf("ylen[%d] = %lld\n", i, (unsigned long long)results[i].ylen);
    is_num(results[i].xval, i);
    if(i % 2)
    {
      is_num(results[i].ylen, SQL_NULL_DATA);
    }
    else
    {
      is_num(results[i].ylen, sizeof(SQLINTEGER));
    }
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug6741");

  return OK;
}


/*
  Test that the ARD (bound) type is used for the update and not
  the IRD (server-given) type.
*/
ODBC_TEST(t_update_type)
{
  SQLUSMALLINT *val= malloc(sizeof(SQLUSMALLINT));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_update_no_strlen");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_update_no_strlen (x int not null)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_update_no_strlen values (0xaaaa)");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_update_no_strlen");
  /* server will use SQL_C_LONG, but we use short */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_USHORT, val, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(*val, 0xaaaa);

  *val= 0xcccc;
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* verify the right value was updated */
  *val= 0;
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_update_no_strlen");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(*val, 0xcccc);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_update_no_strlen");

  free(val);

  return OK;
}


/*
  Test bind offset ptr and bind type for cursor update operations.
*/
ODBC_TEST(t_update_offsets)
{
  SQLULEN rowcnt= 3;
  SQLINTEGER row_offset1= 5;
  /*
    TODO we should prob allow changing SQL_ATTR_ROW_BIND_OFFSET_PTR
    between SQLFetch() and SQLSetPos(). Setting a different value
    here will fail. (must be lower than row_offset1 anyways)
  */
  SQLINTEGER row_offset2= 5;
  struct {
    SQLINTEGER id;
    SQLCHAR name[24];
    SQLLEN namelen;
  } rows[8];
  size_t row_size= sizeof(rows[0]);
  SQLLEN bind_offset= -100000;
  SQLUINTEGER i;
  SQLCHAR buf[50];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_update_offsets");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_update_offsets (id INT NOT NULL, "
                "name VARCHAR(50), PRIMARY KEY (id))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_update_offsets VALUES "
                "(0, 'name0'),(1,'name1'),(2,'name2')");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)rowcnt, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_BIND_TYPE,
                                (SQLPOINTER)row_size, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_BIND_OFFSET_PTR,
                                &bind_offset, 0));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &rows[0].id, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR,
                            &rows[0].name, 24, &rows[0].namelen));

  /* get the first block and verify it */
  OK_SIMPLE_STMT(Stmt, "SELECT id,name FROM t_update_offsets ORDER BY id");

  bind_offset= row_size * row_offset1;
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  for (i= 0; i < rowcnt; ++i)
  {
    sprintf((char *)buf, "name%d", i);
    is_num(rows[row_offset1+i].id, i);
    IS_STR(rows[row_offset1+i].name, buf, strlen((char *)buf) + 1);
    is_num(rows[row_offset1+i].namelen, strlen((char *)buf));

    /* change the values here */
    rows[row_offset2+i].id= i * 10;
    sprintf((char *)rows[row_offset2+i].name, "name_%d_%d", i, i * 10);
    rows[row_offset2+i].namelen= strlen((char *)rows[row_offset2+i].name);
  }

  /* update all rows */
  bind_offset= row_size * row_offset2;
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 0, SQL_UPDATE, SQL_LOCK_NO_CHANGE));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* verify updates */
  memset(rows, 0, sizeof(rows));
  is_num(rows[0].id, 0);
  OK_SIMPLE_STMT(Stmt, "SELECT id,name FROM t_update_offsets ORDER by id");

  bind_offset= row_size;
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  for (i= 0; i < rowcnt; ++i)
  {
    sprintf((char *)buf, "name_%d_%d", i, i * 10);
    is_num(rows[i+1].id, i * 10);
    IS_STR(rows[i+1].name, buf, strlen((char *)buf) + 1);
    is_num(rows[i+1].namelen, strlen((char *)buf));
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_update_offsets");

  return OK;
}


/**
 Bug #6157: BUG in the alias use with ADO's Object
*/
ODBC_TEST(t_bug6157)
{
  SQLINTEGER data;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug6157");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug6157(a INT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug6157 VALUES (1)");

  OK_SIMPLE_STMT(Stmt, "SELECT a AS b FROM t_bug6157");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &data, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  data= 6157;
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  data= 9999;
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_ADD, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT a FROM t_bug6157 ORDER BY a");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &data, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(data, 6157);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(data, 9999);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug6157");

  return OK;
}


/**
 Bug #32420: Don't cache results and SQLExtendedFetch ignore SQL_ROWSET_SIZE
 option
*/
ODBC_TEST(t_bug32420)
{
  HDBC hdbc1;
  HSTMT hstmt1;

  SQLINTEGER nData[4];
  SQLCHAR szData[4][16];
  SQLUSMALLINT rgfRowStatus[4];
  SQLCHAR   conn[512], conn_out[512];
  SQLSMALLINT conn_out_len;
  SQLULEN row_count;

  /* Don't cache result + force forward only cursors option in the connection string */
  sprintf((char *)conn, "DSN=%s;UID=%s;PASSWORD=%s;"
          "DATABASE=%s;SERVER=%s;OPTION=3145728",
          my_dsn, my_uid, my_pwd, my_schema, my_servername);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, (SQLSMALLINT)sizeof(conn), conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS bug32420");
  OK_SIMPLE_STMT(hstmt1, "CREATE TABLE bug32420 ("\
                "tt_int INT PRIMARY KEY auto_increment,"\
                "tt_varchar VARCHAR(128) NOT NULL)");
  OK_SIMPLE_STMT(hstmt1, "INSERT INTO bug32420 VALUES "\
                "(100, 'string 1'),"\
                "(200, 'string 2'),"\
                "(300, 'string 3'),"\
                "(400, 'string 4'),"\
                "(500, 'string 5'),"\
                "(600, 'string 6'),"\
                "(700, 'string 7'),"\
                "(800, 'string 8'),"\
                "(900, 'string 9'),"\
                "(910, 'string A'),"\
                "(920, 'string B')");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLSetStmtOption(hstmt1, SQL_ROWSET_SIZE, 4));

  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM bug32420");
  CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, nData, 0, NULL));
  CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_CHAR, szData, sizeof(szData[0]),
                            NULL));
  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_NEXT, 0, &row_count,
                                   rgfRowStatus));

  is_num(row_count, 4);
  is_num(nData[0], 100);
  IS_STR(szData[0], "string 1", 8);
  is_num(nData[1], 200);
  IS_STR(szData[1], "string 2", 8);
  is_num(nData[2], 300);
  IS_STR(szData[2], "string 3", 8);
  is_num(nData[3], 400);
  IS_STR(szData[3], "string 4", 8);

  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_NEXT, 0, &row_count,
                                   rgfRowStatus));

  is_num(row_count, 4);
  is_num(nData[0], 500);
  IS_STR(szData[0], "string 5", 8);
  is_num(nData[1], 600);
  IS_STR(szData[1], "string 6", 8);
  is_num(nData[2], 700);
  IS_STR(szData[2], "string 7", 8);
  is_num(nData[3], 800);
  IS_STR(szData[3], "string 8", 8);

  /*
     Now checking the last records when the result is shorter than
     ROWSET_SIZE
  */
  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_NEXT, 0, &row_count,
                                   rgfRowStatus));

  is_num(row_count, 3);
  is_num(nData[0], 900);
  IS_STR(szData[0], "string 9", 8);
  is_num(nData[1], 910);
  IS_STR(szData[1], "string A", 8);
  is_num(nData[2], 920);
  IS_STR(szData[2], "string B", 8);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
  OK_SIMPLE_STMT(hstmt1, "DROP TABLE bug32420");
  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  /*
     Result cache is enabled. Need to check that cached results are not
     broken
  */
  sprintf((char *)conn,"DRIVER=%s;UID=%s;PASSWORD=%s;"
          "DATABASE=%s;SERVER=%s;%s;%s;FORWARDONLY=0", /* Making sure FORWARDONLY is not enabled in DSN and/or additional parameters */
          my_drivername, my_uid, my_pwd, my_schema, my_servername, ma_strport, add_connstr);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, (SQLSMALLINT)strlen(conn), conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));
  CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
                                 (SQLPOINTER) SQL_CURSOR_DYNAMIC, 0));
  OK_SIMPLE_STMT(hstmt1, "CREATE TABLE bug32420 ("\
                "tt_int INT PRIMARY KEY auto_increment,"\
                "tt_varchar VARCHAR(128) NOT NULL)");
  OK_SIMPLE_STMT(hstmt1, "INSERT INTO bug32420 VALUES "\
                "(100, 'string 1'),"\
                "(200, 'string 2'),"\
                "(300, 'string 3'),"\
                "(400, 'string 4'),"\
                "(500, 'string 5'),"\
                "(600, 'string 6'),"\
                "(700, 'string 7'),"\
                "(800, 'string 8'),"\
                "(900, 'string 9'),"\
                "(910, 'string A'),"\
                "(920, 'string B')");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLSetStmtOption(hstmt1, SQL_ROWSET_SIZE, 4));

  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM bug32420");
  CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, nData, 0, NULL));
  CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_CHAR, szData, sizeof(szData[0]),
                            NULL));
  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_NEXT, 0, &row_count,
                                   rgfRowStatus));

  is_num(row_count, 4);
  is_num(nData[0], 100);
  IS_STR(szData[0], "string 1", 8);
  is_num(nData[1], 200);
  IS_STR(szData[1], "string 2", 8);
  is_num(nData[2], 300);
  IS_STR(szData[2], "string 3", 8);
  is_num(nData[3], 400);
  IS_STR(szData[3], "string 4", 8);

  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_NEXT, 0, &row_count,
                                   rgfRowStatus));

  is_num(row_count, 4);
  is_num(nData[0], 500);
  IS_STR(szData[0], "string 5", 8);
  is_num(nData[1], 600);
  IS_STR(szData[1], "string 6", 8);
  is_num(nData[2], 700);
  IS_STR(szData[2], "string 7", 8);
  is_num(nData[3], 800);
  IS_STR(szData[3], "string 8", 8);

  /*
     Now checking the last records when the result is shorter than
     ROWSET_SIZE
  */
  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_NEXT, 0, &row_count,
                                   rgfRowStatus));

  is_num(row_count, 3);
  is_num(nData[0], 900);
  IS_STR(szData[0], "string 9", 8);
  is_num(nData[1], 910);
  IS_STR(szData[1], "string A", 8);
  is_num(nData[2], 920);
  IS_STR(szData[2], "string B", 8);

  /* Dynamic cursor allows fetching first records */
  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_FIRST, 0, &row_count,
                                   rgfRowStatus));

  is_num(row_count, 4);
  is_num(nData[0], 100);
  IS_STR(szData[0], "string 1", 8);
  is_num(nData[1], 200);
  IS_STR(szData[1], "string 2", 8);
  is_num(nData[2], 300);
  IS_STR(szData[2], "string 3", 8);
  is_num(nData[3], 400);
  IS_STR(szData[3], "string 4", 8);

  /* Fetching last records */
  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_LAST, 0, &row_count,
                                   rgfRowStatus));

  is_num(row_count, 4);
  is_num(nData[0], 800);
  IS_STR(szData[0], "string 8", 8);
  is_num(nData[1], 900);
  IS_STR(szData[1], "string 9", 8);
  is_num(nData[2], 910);
  IS_STR(szData[2], "string A", 8);
  is_num(nData[3], 920);
  IS_STR(szData[3], "string B", 8);

  /* Fetching with absolute offset */
  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_ABSOLUTE, 3, &row_count,
                                   rgfRowStatus));

  is_num(row_count, 4);
  is_num(nData[0], 300);
  IS_STR(szData[0], "string 3", 8);
  is_num(nData[1], 400);
  IS_STR(szData[1], "string 4", 8);
  is_num(nData[2], 500);
  IS_STR(szData[2], "string 5", 8);
  is_num(nData[3], 600);
  IS_STR(szData[3], "string 6", 8);

   CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS bug32420");
  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


/*
  Shared between t_cursor_pos_static and t_cursor_pos_dynamic.
  Tests all the cursor position handling.
  Cursor type is setup by caller.
*/
int t_cursor_pos(SQLHANDLE Stmt)
{
  SQLINTEGER i;
  SQLINTEGER x[3];
  SQLINTEGER y[3];
  SQLINTEGER remaining_rows[]= {1, 5, 6, 7, 8};
  SQLINTEGER remaining_row_count= 5;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_cursor_pos");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_cursor_pos (x int not null, "
                "y int, primary key (x))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_cursor_pos values (0,0),(1,1),"
                "(2,2),(3,3),(4,4),  (5,5),(6,6),(7,7),  (8,8)");

  OK_SIMPLE_STMT(Stmt, "select x,y from t_cursor_pos order by 1");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, x, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_LONG, y, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));

  /* this covers bug#29765 and bug#33388 */
  is_num(x[0], 0);
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 0, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));
  is_num(x[0], 1);

  y[0]++;
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 0, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));
  is_num(x[0], 2);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_PRIOR, 0));
  is_num(x[0], 1);

  /* and rowset tests */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)3, 0));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));
  for (i= 0; i < 3; ++i)
    is_num(x[i], 2 + i);

  /* delete 2,3,4 */
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 0, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));
  for (i= 0; i < 3; ++i)
  {
    is_num(x[i], 5 + i);
    y[i]++;
  }

  /* update 5,6,7 */
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 0, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  /* set rowset_size back to 1 */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)1, 0));

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));
  is_num(x[0], 8);
  y[0]++;
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 0, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  /* check all rows were updated correctly */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "select x,y from t_cursor_pos order by 1");
  for (i= 0; i < remaining_row_count; ++i)
  {
    CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));
    is_num(x[0], remaining_rows[i]);
    is_num(y[0], x[0] + 1);
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_cursor_pos");
  return OK;
}


/*
  Wrapper for t_cursor_pos using static cursor.
*/
ODBC_TEST(t_cursor_pos_static)
{
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));
  return t_cursor_pos(Stmt);
}


/*
  Wrapper for t_cursor_pos using dynamic cursor.
*/
ODBC_TEST(t_cursor_pos_dynamic)
{
  SQLHANDLE henv1, hdbc1, hstmt1;
 // SET_DSN_OPTION(32);
  ODBC_Connect(&henv1, &hdbc1, &hstmt1);
  CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
                                 (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0));
  is_num(t_cursor_pos(hstmt1), OK);
  (void) ODBC_Disconnect(henv1, hdbc1, hstmt1);
//  SET_DSN_OPTION(0);
  return OK;
}


/*
  Bug#11846 - DIAG [S1T00] Driver Failed to set the internal dynamic result
  Dynamic cursors on statements with parameters wasn't supported.
*/
ODBC_TEST(t_bug11846)
{
  SQLINTEGER val_in= 4, val_out= 99;
  SQLHANDLE henv1, hdbc1, hstmt1;
  //SET_DSN_OPTION(32);
  ODBC_Connect(&henv1, &hdbc1, &hstmt1);

  CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
                                 (SQLPOINTER)SQL_CURSOR_DYNAMIC,0));
  CHECK_STMT_RC(hstmt1, SQLBindParameter(hstmt1, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                   SQL_INTEGER, 0, 0, &val_in, 0, NULL));
  OK_SIMPLE_STMT(hstmt1, "select ?");

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_C_LONG, &val_out, 0, NULL));
  is_num(val_out, val_in);

  ODBC_Disconnect(henv1, hdbc1, hstmt1);
  //SET_DSN_OPTION(0);
  return OK;
}


/*
  Basic test of data-at-exec with SQLSetPos() insert.
*/
typedef struct {
  SQLINTEGER x, z;
  SQLCHAR y[11];
  SQLLEN ylen;
} t_dae_row;
ODBC_TEST(t_dae_setpos_insert)
{
  SQLPOINTER holder= (SQLPOINTER) 0xcfcdceccLL;
  SQLPOINTER paramptr;
  SQLLEN     offset= 0;
  t_dae_row  data[2];

  memset(data, 0, 2 * sizeof(t_dae_row));
  data[1].x= 20;
  data[1].z= 40;
  sprintf((char*)(data[1].y), "1234567890");
  data[1].ylen= SQL_LEN_DATA_AT_EXEC(10);
  
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_dae");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_dae (x INT NOT NULL, y VARCHAR(5000), z INT, "
                       "PRIMARY KEY (x) )");
  OK_SIMPLE_STMT(Stmt, "SELECT x, y, z FROM t_dae");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &data[0].x, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, holder, 10, &data[0].ylen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_LONG, &data[0].z, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_BIND_OFFSET_PTR,
          &offset, SQL_IS_POINTER));

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0) != SQL_NO_DATA, "eof expected");

  offset= sizeof(t_dae_row);
  FAIL_IF(SQLSetPos(Stmt, 0, SQL_ADD, SQL_LOCK_NO_CHANGE)!= SQL_NEED_DATA, "SQL_NEED_DATA expected");
  FAIL_IF(SQLParamData(Stmt, &paramptr) != SQL_NEED_DATA, "SQL_NEED_DATA expected");
  is_num(paramptr, ((SQLCHAR *)holder + offset));
  CHECK_STMT_RC(Stmt, SQLPutData(Stmt, data[1].y, 10));
  CHECK_STMT_RC(Stmt, SQLParamData(Stmt, &paramptr));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  offset= 0;
  OK_SIMPLE_STMT(Stmt, "SELECT x, y, z FROM t_dae");
  
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, data[0].y, 11, &data[0].ylen));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(20, data[0].x);
  is_num(40, data[0].z);
  IS_STR(data[0].y, data[1].y, 11);
  is_num(10, data[0].ylen);
  
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_dae");
  return OK;
}


/*
  Basic test of data-at-exec with SQLSetPos() update.
*/
ODBC_TEST(t_dae_setpos_update)
{
  SQLINTEGER x= 20;
  SQLINTEGER z= 40;
  SQLCHAR *yval= (SQLCHAR *) "1234567890";
  SQLCHAR yout[11];
  SQLLEN ylen= SQL_LEN_DATA_AT_EXEC(9);
  SQLPOINTER holder= (SQLPOINTER) 0xcfcdceccLL;
  SQLPOINTER paramptr;
  /* setup */
  
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_dae");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_dae (x INT NOT NULL, y VARCHAR(5000), z INT, "
                "PRIMARY KEY (x) )");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_dae VALUES (10, '9876', 30)");
  /* create cursor and get first row */
  OK_SIMPLE_STMT(Stmt, "SELECT x, y, z FROM t_dae");

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0));

  /* bind values for positioned update */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &x, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_BINARY, holder, 10, &ylen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_LONG, &z, 0, NULL));

  /* perform update and provide data */
  FAIL_IF(SQLSetPos(Stmt, 0, SQL_UPDATE, SQL_LOCK_NO_CHANGE) != SQL_NEED_DATA, "SQL_NEED_DATA expected");
  FAIL_IF(SQLParamData(Stmt, &paramptr) != SQL_NEED_DATA, "SQL_NEED_DATA expected");
  is_num(paramptr, holder);
  CHECK_STMT_RC(Stmt, SQLPutData(Stmt, yval, 10));
  CHECK_STMT_RC(Stmt, SQLParamData(Stmt, &paramptr));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  x= 0;
  z= 0;
  ylen= 0;
  OK_SIMPLE_STMT(Stmt, "SELECT x, y, z FROM t_dae");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, yout, 11, &ylen));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(20, x);
  is_num(40, z);
  IS_STR(yval, yout, 11);
  is_num(10, ylen);
  
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_dae");
  return OK;
}


/*
  Bug #39951 - Positioned update with SQL_C_NUMERIC loses prec/scale values
*/
ODBC_TEST(t_bug39961)
{
  SQL_NUMERIC_STRUCT num;
  SQLHANDLE ard;
  SQLCHAR buf[10];
  SQLINTEGER id;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug39961");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug39961(id INT NOT NULL, m1 DECIMAL(19, 4), "
	 "PRIMARY KEY (id))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug39961 VALUES (1, 987)");
  OK_SIMPLE_STMT(Stmt, "SELECT id, m1 FROM t_bug39961");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &id, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC, &ard, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLSetDescField(ard, 2, SQL_DESC_CONCISE_TYPE,
				 (SQLPOINTER) SQL_C_NUMERIC, SQL_IS_INTEGER));
  CHECK_STMT_RC(Stmt, SQLSetDescField(ard, 2, SQL_DESC_PRECISION, (SQLPOINTER) 19,
				 SQL_IS_INTEGER));
  CHECK_STMT_RC(Stmt, SQLSetDescField(ard, 2, SQL_DESC_SCALE, (SQLPOINTER) 2,
				 SQL_IS_INTEGER));
  CHECK_STMT_RC(Stmt, SQLSetDescField(ard, 2, SQL_DESC_DATA_PTR, &num,
				 SQL_IS_POINTER));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  /* set value to .1 */
  num.val[0]= 10;
  num.val[1]= 0;
  num.val[2]= 0;
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  /* add a new row */
  id++;
  CHECK_STMT_RC(Stmt, SQLBulkOperations(Stmt, SQL_ADD));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* fetch both rows, they should have the same decimal value */
  OK_SIMPLE_STMT(Stmt, "SELECT m1 FROM t_bug39961");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buf, 1), "0.1000", 4);
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buf, 1), "0.1000", 4);
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug39961");
  return OK;
}


/*
Bug #41946: Inserting a row using SQLSetPos does not correspond to
DB name in SELECT
*/
ODBC_TEST(t_bug41946)
{
	SQLINTEGER nData= 500;
	SQLCHAR szData[255]={0};

	OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS other_test_db");
	OK_SIMPLE_STMT(Stmt, "CREATE DATABASE other_test_db");

	OK_SIMPLE_STMT(Stmt, "CREATE TABLE other_test_db.t_41946(Id int NOT NULL,\
				  Name varchar(32),\
				  PRIMARY KEY  (Id))");

	OK_SIMPLE_STMT(Stmt, "SELECT * FROM other_test_db.t_41946");

	CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 5, NULL));

	CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, 11, NULL));

	FAIL_IF(SQLFetchScroll(Stmt,SQL_FETCH_NEXT,1)!= SQL_NO_DATA, "eof expected");

	nData= 33;
	strcpy((char *)szData , "insert-new");

	CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_ADD, SQL_LOCK_NO_CHANGE));

	nData= 0;
	strcpy((char *)szData , "something else");

  /* We have to close the cursor before issuing next sql query */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
    
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM other_test_db.t_41946");

	CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

	is_num(nData, 33);
	IS_STR(szData, "insert-new", 11);

	CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

	OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS other_test_db");

	return OK;
}


/*
ODBC-251 - Updating row with mediumblob field
*/
ODBC_TEST(odbc251)
{
  SQLCHAR Data[16]= "Updated";
  SQLLEN Len= 8;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc251");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc251 (`id` int(11) NOT NULL PRIMARY KEY AUTO_INCREMENT, `file` mediumblob DEFAULT NULL)");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc251 VALUES(1, '')");

  OK_SIMPLE_STMT(Stmt, "SELECT file FROM t_odbc251 WHERE id=1");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, &Data, sizeof(Data), &Len));

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT file FROM t_odbc251 WHERE id=1");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  Data[0]= 0;
  IS_STR(my_fetch_str(Stmt, Data, 1), "Updated", 8);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc251");

  return OK;
}


ODBC_TEST(odbc276)
{
  SQLINTEGER nData= 500;
  SQLLEN nlen;
  SQLCHAR szData[255]= { 0 };
  SQLULEN pcrow;
  SQLUSMALLINT rgfRowStatus;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_setpos_bin");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_setpos_bin(col1 INT, col2 BINARY(6))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_bin VALUES(100,'MySQL1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_bin VALUES(300,'MySQL3')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_bin VALUES(200,'My\\0QL2')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_bin VALUES(300,'MySQL3')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_setpos_bin VALUES(400,'MySQL4')");

  CHECK_DBC_RC(Connection, SQLTransact(NULL, Connection, SQL_COMMIT));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
    (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos_bin");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 100, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_BINARY, szData, 100, NULL));

  // Fetch Row 2 (That does not have a null) and update it
  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 2, &pcrow, &rgfRowStatus));

  diag(" pcrow:%d\n", pcrow);

  diag(" row2:%d,%s\n", nData, szData);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  nData = 1000;
  strcpy((char*)szData, "updat1");

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));

  diag(" rows affected:%d\n", nlen);
  is_num(nlen, 1);

  // Fetch Row 3 (That does have a null) and update it
  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_ABSOLUTE, 3, &pcrow, &rgfRowStatus));

  diag(" pcrow:%d\n", pcrow);
  diag(" row3:%d,%s\n", nData, szData);

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  nData = 1001;
  strcpy((char*)szData, "updat2");

  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nlen));

  diag(" rows affected:%d\n", nlen);
  is_num(nlen, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_setpos_bin");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 100);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL1", sizeof("MySQL1"));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1000);
  IS_STR(my_fetch_str(Stmt, szData, 2), "updat1", sizeof("updat1"));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1001);
  IS_STR(my_fetch_str(Stmt, szData, 2), "updat2", sizeof("updat2"));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 300);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL3", sizeof("MySQL3"));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 400);
  IS_STR(my_fetch_str(Stmt, szData, 2), "MySQL4", sizeof("MySQL4"));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE tmysql_setpos_bin");

  return OK;
}


/*
 * ODBC-289 - Crash on new use of previously closed cursor with SQL_ATTR_ROW_ARRAY_SIZE > 1
 * Putting it here cuz it's mainly about closing the cursor
 */
ODBC_TEST(odbc289)
{
  SQLLEN i, rowsToInsert= 3, rowsToFetch= 2;
  SQLINTEGER value[2]= {0, 0};

  SQLCHAR value2[60];
  SQLLEN length[2];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc289");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc289 (`LAST_NAME` VARCHAR(29) NOT NULL)");
  for (i = 0; i < rowsToInsert; ++i)
  {
    OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc289 VALUES('whatever')");
  }
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "SELECT LAST_NAME FROM t_odbc289", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowsToFetch, SQL_IS_INTEGER));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_CHAR, value2, 30, length));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  printf("fetch: %s\n", value2);

  printf("fetch: %s\n", value2 + 30);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  printf("fetch: %s\n", value2);
  printf("fetch: %s\n", value2 + 30);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc289");

  return OK;
}


/* Positioned operations with pri/unique index */
ODBC_TEST(odbc356)
{
  SQLLEN      RowCount;
  SQLHSTMT    posStmt, cursorOnUni;
  SQLCHAR     data[32];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc356");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc356_1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc356 (id INT PRIMARY KEY AUTO_INCREMENT NOT NULL, ukey INT NOT NULL, val VARCHAR(15), UNIQUE INDEX onUkey(ukey))");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc356_1(upart1 INT NOT NULL, upart2 INT NOT NULL, val VARCHAR(15), UNIQUE INDEX onUkey(upart1, upart2))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc356 VALUES (1, 2,'Right PRI'),(2, 3, 'Wrong'), (3, 1, 'Right UNI')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc356_1 VALUES (1, 2, 'Wrong'),(1, 3,'Right')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
    (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR*)"macursor", SQL_NTS));
  OK_SIMPLE_STMT(Stmt, "SELECT id, val FROM t_odbc356 ORDER BY 1");
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 1L));

  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &cursorOnUni));
  CHECK_STMT_RC(cursorOnUni, SQLSetCursorName(cursorOnUni, (SQLCHAR*)"maunicursor", SQL_NTS));
  OK_SIMPLE_STMT(cursorOnUni, "SELECT ukey, val FROM t_odbc356 ORDER BY 1");
  CHECK_STMT_RC(cursorOnUni, SQLFetchScroll(cursorOnUni, SQL_FETCH_FIRST, 1L));

  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &posStmt));
  OK_SIMPLE_STMT(posStmt, "UPDATE t_odbc356 SET val= CONCAT(val, '!') "
                          "WHERE CURRENT OF macursor");
  CHECK_STMT_RC(Stmt, SQLRowCount(posStmt, &RowCount));
  is_num(RowCount, 1);

  OK_SIMPLE_STMT(posStmt, "UPDATE t_odbc356 SET val= CONCAT(val, '?') "
                          "WHERE CURRENT OF maunicursor");
  CHECK_STMT_RC(Stmt, SQLRowCount(posStmt, &RowCount));
  is_num(RowCount, 1);

  CHECK_STMT_RC(posStmt, SQLFreeStmt(posStmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  OK_SIMPLE_STMT(posStmt, "DELETE FROM t_odbc356 WHERE CURRENT OF macursor");
  CHECK_STMT_RC(Stmt, SQLRowCount(posStmt, &RowCount));
  is_num(RowCount, 1);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(posStmt, SQLFreeHandle(SQL_HANDLE_STMT, cursorOnUni));

  OK_SIMPLE_STMT(Stmt, "SELECT id, ukey, val FROM t_odbc356 ORDER BY 1");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  is_num(my_fetch_int(Stmt, 2), 2);
  IS_STR(my_fetch_str(Stmt, data, 3), "Right PRI!", sizeof("Right PRI!"));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 3);
  is_num(my_fetch_int(Stmt, 2), 1);
  IS_STR(my_fetch_str(Stmt, data, 3), "Right UNI?", sizeof("Right UNI?"));

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "SQL_NO_DATA expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* This test is to verify, that if stmt handle is used for a new query, wrong old key datat is not used for the postioned operation */
  OK_SIMPLE_STMT(Stmt, "SELECT upart1, upart2, val FROM t_odbc356_1 ORDER BY 1,2");
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_LAST, 1L));

  OK_SIMPLE_STMT(posStmt, "DELETE FROM t_odbc356_1 WHERE CURRENT OF macursor");
  CHECK_STMT_RC(Stmt, SQLRowCount(posStmt, &RowCount));
  is_num(RowCount, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "SELECT upart1, upart2, val FROM t_odbc356_1 ORDER BY 1,2");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  is_num(my_fetch_int(Stmt, 2), 2);
  IS_STR(my_fetch_str(Stmt, data, 3), "Wrong", sizeof("Wrong")); /* Means the right row has been deleted */

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "SQL_NO_DATA expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(posStmt, SQLFreeHandle(SQL_HANDLE_STMT, posStmt));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc356");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc356_1");

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {my_positioned_cursor, "my_positioned_cursor",     NORMAL},
  {my_setpos_cursor, "my_setpos_cursor",     NORMAL},
  {t_bug5853, "t_bug5853",     NORMAL},
  {t_setpos_del_all, "t_setpos_del_all",     NORMAL},
  {t_setpos_upd_decimal, "t_setpos_upd_decimal",     NORMAL},
  {t_setpos_position, "t_setpos_position",     NORMAL},
  {t_pos_column_ignore, "t_pos_column_ignore",     NORMAL},
  {t_pos_datetime_delete, "t_pos_datetime_delete",   NORMAL},
  {t_pos_datetime_delete1, "t_pos_datetime_delete1", NORMAL, SkipIfRsStreming},
  {t_getcursor, "t_getcursor",     NORMAL},
  {t_getcursor1, "t_getcursor1",   NORMAL},
  {t_acc_crash, "t_acc_crash",     NORMAL, SkipIfRsStreming},
  {tmysql_setpos_del, "tmysql_setpos_del",     NORMAL, SkipIfRsStreming},
  {tmysql_setpos_del1, "tmysql_setpos_del1",   NORMAL, SkipIfRsStreming},
  {tmysql_setpos_upd, "tmysql_setpos_upd",     NORMAL, SkipIfRsStreming},
  {tmysql_setpos_add, "tmysql_setpos_add",     NORMAL, SkipIfRsStreming},
  {tmysql_pos_delete, "tmysql_pos_delete",     NORMAL, SkipIfRsStreming},
  {t_pos_update,      "t_pos_update",          NORMAL, SkipIfRsStreming},
  {tmysql_pos_update_ex, "tmysql_pos_update_ex",   NORMAL, SkipIfRsStreming},
  {tmysql_pos_update_ex1, "tmysql_pos_update_ex1", NORMAL, SkipIfRsStreming},
  {tmysql_pos_update_ex3, "tmysql_pos_update_ex3", NORMAL},
  {tmysql_pos_update_ex4, "tmysql_pos_update_ex4", NORMAL, SkipIfRsStreming},
  {tmysql_pos_dyncursor,  "tmysql_pos_dyncursor",  NORMAL, SkipIfRsStreming},
  {tmysql_mtab_setpos_del,"tmysql_mtab_setpos_del",NORMAL, SkipIfRsStreming},
  {tmysql_setpos_pkdel,   "tmysql_setpos_pkdel",   NORMAL, SkipIfRsStreming},
  {t_alias_setpos_pkdel,  "t_alias_setpos_pkdel",  NORMAL, SkipIfRsStreming},
  {t_alias_setpos_del,    "t_alias_setpos_del",    NORMAL, SkipIfRsStreming},
  {tmysql_setpos_pkdel2,  "tmysql_setpos_pkdel2",  NORMAL, SkipIfRsStreming},
  {t_setpos_upd_bug1,     "t_setpos_upd_bug1",     NORMAL, SkipIfRsStreming},
  {my_setpos_upd_pk_order,"my_setpos_upd_pk_order",NORMAL, SkipIfRsStreming},
  {my_setpos_upd_pk_order1, "my_setpos_upd_pk_order1", NORMAL, SkipIfRsStreming},
  {tmy_cursor1, "tmy_cursor1",     NORMAL},
  {tmy_cursor2, "tmy_cursor2",     NORMAL},
  {tmysql_pcbvalue, "tmysql_pcbvalue", NORMAL, SkipIfRsStreming},
  {t_bug28255, "t_bug28255", NORMAL},
  {bug10563, "bug10563",     NORMAL, SkipIfRsStreming},
  {bug6741, "bug6741",     NORMAL},
  {t_update_type,    "t_update_type",    NORMAL, SkipIfRsStreming},
  {t_update_offsets, "t_update_offsets", NORMAL, SkipIfRsStreming},
  {t_bug6157, "t_bug6157",   NORMAL, SkipIfRsStreming},
  {t_bug32420, "t_bug32420", NORMAL},
  {t_cursor_pos_static, "t_cursor_pos_static",   NORMAL, SkipIfRsStreming},
  {t_cursor_pos_dynamic, "t_cursor_pos_dynamic", NORMAL, SkipIfRsStreming},
  {t_bug11846, "t_bug11846",     NORMAL},
  {t_dae_setpos_insert, "t_dae_setpos_insert", NORMAL},
  {t_dae_setpos_update, "t_dae_setpos_update", NORMAL, SkipIfRsStreming},
  {t_bug39961, "t_bug39961",        NORMAL, SkipIfRsStreming},
  {t_bug41946, "t_bug41946",        NORMAL},
  {odbc251, "odbc251-mblob_update", TO_FIX},
  {odbc276, "odbc276-bin_update", NORMAL, SkipIfRsStreming},
  {odbc289, "odbc289-fech_after_close", NORMAL},
  {odbc356, "odbc356-key_cursor", NORMAL, SkipIfRsStreming},
  {NULL, NULL, 0, NULL}
};


int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  //DoNotSkipTests(my_tests);
  return run_tests(my_tests);
}
