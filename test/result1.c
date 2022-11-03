/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013, 2019 MariaDB Corporation A

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

/* result set demo */
ODBC_TEST(my_resultset)
{
  SQLRETURN   rc;
  SQLUINTEGER nRowCount=0;
  SQLULEN     pcColDef;
  SQLCHAR     szColName[MAX_NAME_LEN+1];
  SQLCHAR     szData[MAX_ROW_DATA_LEN+1];
  SQLSMALLINT nIndex,ncol,pfSqlType, pcbScale, pfNullable;

  /* drop table 'myodbc3_demo_result' if it already exists */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE if exists myodbc3_demo_result");

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  /* create the table 'myodbc3_demo_result' */
  OK_SIMPLE_STMT(Stmt,"CREATE TABLE myodbc3_demo_result("
          "id int primary key auto_increment,name varchar(20))");

  rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  /* insert 2 rows of data */
  OK_SIMPLE_STMT(Stmt, "INSERT INTO myodbc3_demo_result values(1,'MySQL')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO myodbc3_demo_result values(2,'MyODBC')");

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  /* update second row */
  OK_SIMPLE_STMT(Stmt, "UPDATE myodbc3_demo_result set name="
          "'MyODBC 3.51' where id=2");

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  /* now fetch back..*/
  OK_SIMPLE_STMT(Stmt, "SELECT * from myodbc3_demo_result");

  /* get total number of columns from the resultset */
  rc = SQLNumResultCols(Stmt,&ncol);
  CHECK_STMT_RC(Stmt,rc);

  diag("total columns in resultset:%d",ncol);

  /* print the column names  and do the row bind */
  for (nIndex = 1; nIndex <= ncol; nIndex++)
  {
      rc = SQLDescribeCol(Stmt,nIndex,szColName, MAX_NAME_LEN+1, NULL,
                          &pfSqlType,&pcColDef,&pcbScale,&pfNullable);
      CHECK_STMT_RC(Stmt,rc);

      printf("%s\t",szColName);

  }
  printf("\n");

  /* now fetch row by row */
  rc = SQLFetch(Stmt);
  while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
  {
      nRowCount++;
      for (nIndex=1; nIndex<= ncol; nIndex++)
      {
          rc = SQLGetData(Stmt,nIndex, SQL_C_CHAR, szData,
                          MAX_ROW_DATA_LEN,NULL);
          CHECK_STMT_RC(Stmt,rc);
          printf("%s\t",szData);
      }

      printf("\n");
      rc = SQLFetch(Stmt);
  }
  SQLFreeStmt(Stmt,SQL_UNBIND);

  diag("total rows fetched:%d",nRowCount);

  /* free the statement row bind resources */
  rc = SQLFreeStmt(Stmt, SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  /* free the statement cursor */
  rc = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE myodbc3_demo_result");

  return OK;
}


/* To test a convertion type */
ODBC_TEST(t_convert_type)
{
  SQLRETURN   rc;
  SQLSMALLINT SqlType, DateType, Length;
  SQLCHAR     ColName[MAX_NAME_LEN];
  SQLCHAR     DbVersion[MAX_NAME_LEN];
  SQLINTEGER  OdbcVersion;

  rc = SQLGetEnvAttr(Env,SQL_ATTR_ODBC_VERSION,&OdbcVersion,0,NULL);
  CHECK_ENV_RC(Env,rc);

  fprintf(stdout,"# odbc version:");
  if (OdbcVersion == SQL_OV_ODBC2)
  {
    fprintf(stdout," SQL_OV_ODBC2\n");
    DateType= SQL_DATE;
  }
  else
  {
    fprintf(stdout," SQL_OV_ODBC3\n");
    DateType= SQL_TYPE_DATE;
  }

  rc = SQLGetInfo(Connection,SQL_DBMS_VER,DbVersion,MAX_NAME_LEN, &Length);
  CHECK_DBC_RC(Connection,rc);
  diag("SQL_DBMS_VER: %s", DbVersion);
  is_num(Length, strlen((const char*)DbVersion));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_convert");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_convert(col0 int, col1 date, col2 char(10))");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_convert VALUES(10,'2002-10-24','venu')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_convert VALUES(20,'2002-10-23','venu1')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_convert VALUES(30,'2002-10-25','venu2')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_convert VALUES(40,'2002-10-24','venu3')");

    OK_SIMPLE_STMT(Stmt, "SELECT MAX(col0) FROM t_convert");

    rc = SQLDescribeCol(Stmt,1,ColName,MAX_NAME_LEN,NULL,&SqlType,NULL,NULL,NULL);
    CHECK_STMT_RC(Stmt,rc);

    diag("MAX(col0): %d", SqlType);
    IS(SqlType == SQL_INTEGER);

    SQLFreeStmt(Stmt,SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "SELECT MAX(col1) FROM t_convert");

    rc = SQLDescribeCol(Stmt,1,ColName,MAX_NAME_LEN,NULL,&SqlType,NULL,NULL,NULL);
    CHECK_STMT_RC(Stmt,rc);

    diag("MAX(col1): %d", SqlType);
    IS(SqlType == DateType);

    SQLFreeStmt(Stmt,SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "SELECT MAX(col2) FROM t_convert");

    rc = SQLDescribeCol(Stmt,1,ColName,MAX_NAME_LEN,NULL,&SqlType,NULL,NULL,NULL);
    CHECK_STMT_RC(Stmt,rc);

    diag("MAX(col0): %d", SqlType);

    SQLFreeStmt(Stmt,SQL_CLOSE);

    if (strncmp((char *)DbVersion,"4.",2) >= 0)
    {
      OK_SIMPLE_STMT(Stmt, "SELECT CAST(MAX(col1) AS DATE) AS col1 FROM t_convert");
      CHECK_STMT_RC(Stmt,rc);

      rc = SQLDescribeCol(Stmt,1,ColName,MAX_NAME_LEN,NULL,&SqlType,NULL,NULL,NULL);
      CHECK_STMT_RC(Stmt,rc);

      diag("CAST(MAX(col1) AS DATE): %d", SqlType);
      IS(SqlType == DateType);

      SQLFreeStmt(Stmt,SQL_CLOSE);

      OK_SIMPLE_STMT(Stmt, "SELECT CONVERT(MAX(col1),DATE) AS col1 FROM t_convert");
      CHECK_STMT_RC(Stmt,rc);

      rc = SQLDescribeCol(Stmt,1,ColName,MAX_NAME_LEN,NULL,&SqlType,NULL,NULL,NULL);
      CHECK_STMT_RC(Stmt,rc);

      diag("CONVERT(MAX(col1),DATE): %d", SqlType);
      IS(SqlType == DateType);

      SQLFreeStmt(Stmt,SQL_CLOSE);

      OK_SIMPLE_STMT(Stmt,"SELECT CAST(MAX(col1) AS CHAR) AS col1 FROM t_convert");

      rc = SQLDescribeCol(Stmt,1,(SQLCHAR *)&ColName,MAX_NAME_LEN,NULL,&SqlType,NULL,NULL,NULL);
      CHECK_STMT_RC(Stmt,rc);

      diag("CAST(MAX(col1) AS CHAR): %d", SqlType);
      IS(SqlType == SQL_VARCHAR || SqlType == SQL_LONGVARCHAR ||
               SqlType == SQL_WVARCHAR || SqlType == SQL_WLONGVARCHAR);

      SQLFreeStmt(Stmt,SQL_CLOSE);
    }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_convert");

  return OK;
}


static SQLINTEGER desc_col_check(SQLHSTMT Stmt,
                           SQLUSMALLINT icol,
                           const char *name,
                           SQLSMALLINT sql_type,
                           SQLULEN col_def,
                           SQLULEN col_def1,
                           SQLSMALLINT scale,
                           SQLSMALLINT nullable)
{
  SQLRETURN   rc;
  SQLSMALLINT pcbColName, pfSqlType, pibScale, pfNullable;
  SQLULEN     pcbColDef;
  SQLCHAR     szColName[MAX_NAME_LEN];

  rc = SQLDescribeCol(Stmt, icol,
                      szColName,MAX_NAME_LEN,&pcbColName,
                      &pfSqlType,&pcbColDef,&pibScale,&pfNullable);
  CHECK_STMT_RC(Stmt,rc);

  diag("Column Number'%d':", icol);

  diag(" Column Name    : %s", szColName);
  diag(" NameLengh      : %d", pcbColName);
  diag(" DataType       : %d", pfSqlType);
  diag(" ColumnSize     : %d", pcbColDef);
  diag(" DecimalDigits  : %d", pibScale);
  diag(" Nullable       : %d", pfNullable);

  IS_STR(szColName, name, pcbColName);
  is_num(pfSqlType, sql_type);
  IS(col_def == pcbColDef || col_def1 == pcbColDef);
  is_num(pibScale, scale);
  is_num(pfNullable, nullable);

  return OK;
}


/* To test SQLDescribeCol */
ODBC_TEST(t_desc_col)
{
  SQLSMALLINT ColumnCount;
  SQLHDBC     hdbc1;
  SQLHSTMT    Stmt1;

  AllocEnvConn(&Env, &hdbc1);
  Stmt1= ConnectWithCharset(&hdbc1, "latin1", NULL); /* We need to make sure that the charset used for connection is not multibyte */

  OK_SIMPLE_STMT(Stmt1, "DROP TABLE IF EXISTS t_desc_col");

  OK_SIMPLE_STMT(Stmt1, "CREATE TABLE t_desc_col("
         "c1 integer,"
         "c2 binary(2) NOT NULL,"
         "c3 char(1),"
         "c4 varchar(5),"
         "c5 decimal(10,3) NOT NULL,"
         "c6 tinyint,"
         "c7 smallint,"
         "c8 numeric(4,2),"
         "c9 real,"
         "c10 float(5),"
         "c11 bigint NOT NULL,"
         "c12 varbinary(12),"
         "c13 char(20) NOT NULL,"
         "c14 float(10,3),"
         "c15 tinytext,"
         "c16 text,"
         "c17 mediumtext,"
         "c18 longtext,"
         "c19 tinyblob,"
         "c20 blob,"
         "c21 mediumblob,"
         "c22 longblob,"
         "c23 tinyblob) CHARSET latin1");

  OK_SIMPLE_STMT(Stmt1, "SELECT * FROM t_desc_col");

  CHECK_STMT_RC(Stmt1, SQLNumResultCols(Stmt1, &ColumnCount));

  is_num(ColumnCount, 23);
  /* iOdbc maps ANSI calls to Unicode calls, like Windows DM does. But unlike Windows DM, it doesn't inform the connector about that.
     Thus we get W types with iODBC here. But probably something has to be done with sizes as well */
  IS(desc_col_check(Stmt1, 1,  "c1",  SQL_INTEGER,   10, 10, 0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 2,  "c2",  SQL_BINARY,    4,  2,  0,  SQL_NO_NULLS) == OK);
  IS(desc_col_check(Stmt1, 3,  "c3",  iOdbc() ? SQL_WCHAR : SQL_CHAR, 1, 1, 0, SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 4,  "c4",  iOdbc() ? SQL_WVARCHAR : SQL_VARCHAR, 5, 5, 0, SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 5,  "c5",  SQL_DECIMAL,   10, 10, 3,  SQL_NO_NULLS) == OK);
  IS(desc_col_check(Stmt1, 6,  "c6",  SQL_TINYINT,   3,  4,  0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 7,  "c7",  SQL_SMALLINT,  5,  6,  0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 8,  "c8",  SQL_DECIMAL,   4,  4,  2,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 9,  "c9",  SQL_DOUBLE,    15, 15, 0, SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 10, "c10", SQL_REAL,      7,  7,  0, SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 11, "c11", SQL_BIGINT, 19, 20, 0,  SQL_NO_NULLS) == OK);

  IS(desc_col_check(Stmt1, 12, "c12", SQL_VARBINARY, 12, 12, 0,  SQL_NULLABLE) == OK);

  IS(desc_col_check(Stmt1, 13, "c13", iOdbc() ? SQL_WCHAR : SQL_CHAR, 20, 20, 0,  SQL_NO_NULLS) == OK);
  IS(desc_col_check(Stmt1, 14, "c14", SQL_REAL,      7,  7,  0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 15, "c15", iOdbc() ? SQL_WLONGVARCHAR : SQL_LONGVARCHAR, 255, 255, 0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 16, "c16", iOdbc() ? SQL_WLONGVARCHAR : SQL_LONGVARCHAR, 65535, 65535, 0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 17, "c17", iOdbc() ? SQL_WLONGVARCHAR : SQL_LONGVARCHAR, 16777215, 16777215, 0,  SQL_NULLABLE) == OK);
  /* Test may fail here if connection charset mbmaxlen > 1 */
  IS(desc_col_check(Stmt1, 18, "c18", iOdbc() ? SQL_WLONGVARCHAR : SQL_LONGVARCHAR, 4294967295UL, 16777215 , 0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 19, "c19", SQL_LONGVARBINARY, 255, 255, 0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 20, "c20", SQL_LONGVARBINARY, 65535, 65535, 0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 21, "c21", SQL_LONGVARBINARY, 16777215, 16777215, 0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 22, "c22", SQL_LONGVARBINARY, 4294967295UL, 16777215 , 0,  SQL_NULLABLE) == OK);
  IS(desc_col_check(Stmt1, 23, "c23", SQL_LONGVARBINARY, 255, 5, 0,  SQL_NULLABLE) == OK);

  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt1, "DROP TABLE IF EXISTS t_desc_col");
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


/* Test for misc CONVERT bug #1082 */
ODBC_TEST(t_convert)
{
  SQLRETURN  rc;
  SQLLEN     data_len;
  SQLCHAR    data[50];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_convert");

  OK_SIMPLE_STMT(Stmt,"CREATE TABLE t_convert(testing tinytext)");


  OK_SIMPLE_STMT(Stmt,"INSERT INTO t_convert VALUES('record1')");

  OK_SIMPLE_STMT(Stmt,"INSERT INTO t_convert VALUES('record2')");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt,"SELECT CONCAT(testing, '-must be string') FROM t_convert ORDER BY RAND()");

  rc = SQLBindCol(Stmt,1,SQL_C_CHAR, &data, 100, &data_len);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);
  IS(strcmp((char *)data,"record1-must be string") == 0 ||
           strcmp((char *)data,"record2-must be string") == 0);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);
  IS(strcmp((char *)data,"record1-must be string") == 0 ||
             strcmp((char *)data,"record2-must be string") == 0);

  rc = SQLFetch(Stmt);
  IS( rc == SQL_NO_DATA);

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_convert");

  return OK;
}


ODBC_TEST(t_max_rows)
{
  SQLRETURN rc;
  SQLUINTEGER i;
  SQLSMALLINT cc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_max_rows");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt,"CREATE TABLE t_max_rows(id int)");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt,
                            (SQLCHAR *)"INSERT INTO t_max_rows values(?)",
                            SQL_NTS));

  rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&i,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  for(i=0; i < 10; i++)
  {
    rc = SQLExecute(Stmt);
    CHECK_STMT_RC(Stmt,rc);
  }

  SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
  SQLFreeStmt(Stmt,SQL_CLOSE);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLSetStmtAttr(Stmt,SQL_ATTR_MAX_ROWS,(SQLPOINTER)0,0);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt,"select count(*) from t_max_rows");
  IS( 1 == myrowcount(Stmt) );
  SQLFreeStmt(Stmt,SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t_max_rows");
  IS( 10 == myrowcount(Stmt) );
  SQLFreeStmt(Stmt,SQL_CLOSE);

  /* MAX rows through connection attribute */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt,SQL_ATTR_MAX_ROWS,(SQLPOINTER)5,0));

  /*
   This query includes leading spaces to act as a regression test
   for Bug #6609: SQL_ATTR_MAX_ROWS and leading spaces in query result in
   truncating end of query.
  */
  OK_SIMPLE_STMT(Stmt,"  select * from t_max_rows");
  is_num(5, myrowcount(Stmt));

  SQLFreeStmt(Stmt,SQL_CLOSE);

  rc = SQLSetStmtAttr(Stmt,SQL_ATTR_MAX_ROWS,(SQLPOINTER)15,0);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t_max_rows");
  IS( 10 == myrowcount(Stmt));

  SQLFreeStmt(Stmt,SQL_CLOSE);

  /* Patch for Bug#46411 uses SQL_ATTR_MAX_ROWS attribute to minimize number of
     rows to pre-fetch(sets to 1). Following fragment ensures that attribute's
     value IS preserved and works. */
  rc = SQLSetStmtAttr(Stmt,SQL_ATTR_MAX_ROWS,(SQLPOINTER)3,0);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLPrepare(Stmt,(SQLCHAR*)"SELECT * FROM t_max_rows where id > ?", SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);
  rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&i,0,NULL);
  CHECK_STMT_RC(Stmt,rc);
  i= 6;

  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &cc));

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  IS( 3 == myrowcount(Stmt));

  SQLFreeStmt(Stmt,SQL_CLOSE);

  rc = SQLSetStmtAttr(Stmt,SQL_ATTR_MAX_ROWS,(SQLPOINTER)0,0);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t_max_rows");
  IS( 10 == myrowcount(Stmt));

  SQLFreeStmt(Stmt,SQL_CLOSE);

  SQLFreeStmt(Stmt,SQL_CLOSE);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  /* Testing max_rows with PREFETCH feature(client side cursor) enabled */
  {
    HDBC  hdbc1;
    HSTMT hstmt1;
    SQLCHAR conn[512];

    sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;PREFETCH=5",
          my_dsn, my_uid, my_pwd);
    
    CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

    CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, sizeof(conn), NULL,
                                   0, NULL,
                                   SQL_DRIVER_NOPROMPT));
    CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

    /* max_rows IS bigger than a prefetch, and IS not divided evenly by it */
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1,SQL_ATTR_MAX_ROWS,(SQLPOINTER)7,0));

    /* May fail if test dsn does not specify database to use */
    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_max_rows");

    is_num(7, myrowcount(hstmt1));

    SQLFreeStmt(hstmt1,SQL_CLOSE);

    /* max_rows IS bigger than prefetch, and than total mumber of rows in the
       table */
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1,SQL_ATTR_MAX_ROWS,(SQLPOINTER)12,0));

    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_max_rows");
  
    is_num(10, myrowcount(hstmt1));

    SQLFreeStmt(hstmt1,SQL_CLOSE);

    /* max_rows IS bigger than prefetch, and equal to total mumber of rows in
       the table, and IS divided evenly by prefetch number */
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1,SQL_ATTR_MAX_ROWS,(SQLPOINTER)10,0));

    OK_SIMPLE_STMT(hstmt1,"SELECT * FROM t_max_rows");
  
    is_num(10, myrowcount(hstmt1));

    SQLFreeStmt(hstmt1,SQL_CLOSE);

    /* max_rows IS less than a prefetch number */
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1,SQL_ATTR_MAX_ROWS,(SQLPOINTER)3,0));

    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_max_rows");

    is_num(3, myrowcount(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1,SQL_DROP));
    CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
    CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_max_rows");

  return OK;
}


ODBC_TEST(t_multistep)
{
  SQLRETURN  rc;
  SQLCHAR    szData[150];
  SQLLEN     pcbValue;
  SQLINTEGER id;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_multistep");
  OK_SIMPLE_STMT(Stmt,"CREATE TABLE t_multistep(col1 int,col2 varchar(200))");

  OK_SIMPLE_STMT(Stmt,"INSERT INTO t_multistep values(10,'MySQL - Open Source Database')");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_ROWVER, 0);
  SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_KEYSET_DRIVEN, 0);

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&id,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t_multistep");

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);
  diag("id: %ld",id);
  IS(id == 10);

  rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,0,&pcbValue);
  IS(rc == SQL_SUCCESS_WITH_INFO);
  diag("length: %ld", pcbValue);
  IS(pcbValue == 28);

  rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,0,&pcbValue);
  IS(rc == SQL_SUCCESS_WITH_INFO);
  diag("length: %ld", pcbValue);
  IS(pcbValue == 28);

  rc = SQLGetData(Stmt,2,SQL_C_BINARY,szData,0,&pcbValue);
  IS(rc == SQL_SUCCESS_WITH_INFO);
  diag("length: %ld", pcbValue);
  IS(pcbValue == 28);

  rc = SQLGetData(Stmt,2,SQL_C_BINARY,szData,0,&pcbValue);
  IS(rc == SQL_SUCCESS_WITH_INFO);
  diag("length: %ld", pcbValue);
  IS(pcbValue == 28);

  rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,0,&pcbValue);
  IS(rc == SQL_SUCCESS_WITH_INFO);
  diag("length: %ld", pcbValue);
  is_num(pcbValue, 28);

  rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,10,&pcbValue);
  IS(rc == SQL_SUCCESS_WITH_INFO);
  diag("data  : %s (%ld)",szData,pcbValue);
  is_num(pcbValue, 28);
  IS_STR(szData, "MySQL - O", 10);

  pcbValue= 0;
  rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,5,&pcbValue);
  IS(rc == SQL_SUCCESS_WITH_INFO);
  diag("data  : %s (%ld)",szData,pcbValue);
  is_num(pcbValue, 19);
  IS_STR(szData, "pen ", 5);

  pcbValue= 0;
  szData[0]='A';
  rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,0,&pcbValue);
  IS(rc == SQL_SUCCESS_WITH_INFO);
  diag("data  : %s (%ld)",szData,pcbValue);
  IS(pcbValue == 15);
  IS(szData[0] == 'A');

  rc = SQLGetData(Stmt,2,SQL_C_CHAR,szData,pcbValue+1,&pcbValue);
  CHECK_STMT_RC(Stmt,rc);
  diag("data  : %s (%ld)",szData,pcbValue);
  is_num(pcbValue, 15);
  IS_STR(szData,"Source Database", 16);

  pcbValue= 99;
  szData[0]='A';
  rc= SQLGetData(Stmt, 2, SQL_C_CHAR, szData, 0, &pcbValue);
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "expected eof");
  diag("data  : %s (%ld)",szData,pcbValue);
  IS(pcbValue == 99);
  IS(szData[0] == 'A');

  FAIL_IF(SQLFetch(Stmt)!= SQL_NO_DATA_FOUND, "eof expected");

  SQLFreeStmt(Stmt,SQL_UNBIND);
  SQLFreeStmt(Stmt,SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_multistep");

  return OK;
}


ODBC_TEST(t_zerolength)
{
  SQLRETURN  rc;
  SQLCHAR    szData[100], bData[100], bData1[100];
  SQLLEN     pcbValue,pcbValue1,pcbValue2;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_zerolength");
  OK_SIMPLE_STMT(Stmt,"CREATE TABLE t_zerolength(str VARCHAR(20), bin VARBINARY(20), blb BLOB)");

  OK_SIMPLE_STMT(Stmt,"INSERT INTO t_zerolength VALUES('','','')");

  OK_SIMPLE_STMT(Stmt,"INSERT INTO t_zerolength VALUES('venu','mysql','monty')");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_ROWVER, 0);
  SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_KEYSET_DRIVEN, 0);

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t_zerolength");

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  pcbValue= pcbValue1= 99;
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, szData, 0, &pcbValue));
  diag("length: %d", pcbValue);
  IS(pcbValue == 0);

  bData[0]=bData[1]='z';
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_BINARY, bData, 0, &pcbValue1));
  diag("length: %d", pcbValue1);
  IS(pcbValue1 == 0);
  IS(bData[0] == 'z');
  IS(bData[1] == 'z');

  bData1[0]=bData1[1]='z';
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 3, SQL_C_BINARY, bData1, 0, &pcbValue2));
  diag("length: %d", pcbValue2);
  IS(pcbValue2 == 0);
  IS(bData1[0] == 'z');
  IS(bData1[1] == 'z');

  pcbValue= pcbValue1= 99;
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR,szData, 1, &pcbValue));
  diag("data: %s, length: %d", szData, pcbValue);
  IS(pcbValue == 0);
  IS(szData[0] == '\0');

  bData[0]=bData[1]='z';
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_BINARY, bData, 1, &pcbValue1));

  diag("data: %s, length: %d", bData, pcbValue1);
  IS(pcbValue1 == 0);

  bData1[0]=bData1[1]= 'z';
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 3, SQL_C_CHAR, bData1, 1, &pcbValue2));
  diag("data: %s, length: %d", bData1, pcbValue2);
  IS(pcbValue2 == 0);
  IS(bData1[0] == '\0');
  IS(bData1[1] == 'z');

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  pcbValue= pcbValue1= 99;
  szData[0]= bData[0]= 'z';
  rc = SQLGetData(Stmt,1,SQL_C_CHAR,szData,0,&pcbValue);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("length: %d", pcbValue);
  IS(pcbValue == 4);
  IS(szData[0] == 'z');

  rc = SQLGetData(Stmt,2,SQL_C_BINARY,bData,0,&pcbValue1);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("length: %d", pcbValue1);
  IS(pcbValue1 == 5);
  IS(bData[0] == 'z');

  bData[0]=bData1[1]='z';
  rc = SQLGetData(Stmt,3,SQL_C_BINARY,bData1,0,&pcbValue2);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("length: %d", pcbValue2);
  IS(pcbValue2 == 5);

  pcbValue= pcbValue1= 99;
  szData[0]= szData[1]= bData[0]= bData[1]= 'z';
  rc = SQLGetData(Stmt,1,SQL_C_CHAR,szData,1,&pcbValue);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("data: %s, length: %d", szData,pcbValue);
  IS(pcbValue == 4);
  IS(szData[0] == '\0');

  rc = SQLGetData(Stmt,2,SQL_C_BINARY,bData,1,&pcbValue1);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("data; %s, length: %d", bData, pcbValue1);
  IS(pcbValue1 == 5);
  IS(bData[0] == 'm');

  bData[0]=bData1[1]='z';
  rc = SQLGetData(Stmt,3,SQL_C_BINARY,bData1,1,&pcbValue2);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("length: %d", pcbValue2);
  IS(pcbValue2 == 5);
  IS(bData1[0] == 'm');
  IS(bData1[1] == 'z');

  pcbValue= pcbValue1= 99;
  rc = SQLGetData(Stmt,1,SQL_C_CHAR,szData,4,&pcbValue);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("data: %s, length: %d", szData, pcbValue);
  is_num(pcbValue, 4);
  IS_STR(szData,"ven", 3);

  rc = SQLGetData(Stmt,2,SQL_C_BINARY,bData,4,&pcbValue1);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("data: %s, length: %d", bData, pcbValue1);
  is_num(pcbValue1, 5);
  IS_STR(bData, "mysq", 4);

  pcbValue= pcbValue1= 99;
  rc = SQLGetData(Stmt,1,SQL_C_CHAR,szData,5,&pcbValue);
  CHECK_STMT_RC(Stmt,rc);
  diag("data: %s, length: %d", szData, pcbValue);
  is_num(pcbValue, 4);
  IS_STR(szData, "venu", 4);

  rc = SQLGetData(Stmt,2,SQL_C_BINARY,bData,5,&pcbValue1);
  CHECK_STMT_RC(Stmt,rc);
  diag("data: %s, length: %d", bData, pcbValue1);
  is_num(pcbValue1, 5);
  IS_STR(bData, "mysql", 5);

  szData[0]= 'z';
  rc = SQLGetData(Stmt, 3, SQL_C_CHAR, szData, 0, &pcbValue);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("data: %s, length: %d", szData, pcbValue);
  IS(pcbValue == 5 || pcbValue == 10);
  IS(szData[0] == 'z');

  /* "If the data was converted to a variable-length data type, such as character or binary,
    SQLGetData checks whether the length of the data exceeds BufferLength. If the length
    of character data (including the null-termination character) exceeds BufferLength,
    SQLGetData truncates the data to BufferLength less the length of a null-termination
    character. It then null-terminates the data. If the length of binary data exceeds the
    length of the data buffer, SQLGetData truncates it to BufferLength bytes."
    Thus, BufferLength=1 shoul make the driver to write reminating null in that one character */
  szData[0]=szData[1]='z';
  rc = SQLGetData(Stmt, 3, SQL_C_CHAR, szData, 1, &pcbValue);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("data: %s, length: %d", szData, pcbValue);
  IS(pcbValue == 5);
  IS(szData[0] == '\0');
  IS(szData[1] == 'z');

  rc = SQLGetData(Stmt, 3,SQL_C_CHAR, szData, 4, &pcbValue);
  FAIL_IF(rc != SQL_SUCCESS_WITH_INFO, "swi expected");
  diag("data: %s, length: %d", szData, pcbValue);
  /* For character or binary data, this is the length of the data after conversion and before truncation due to BufferLength. */
  IS(pcbValue == 5);
  IS(strncmp(szData,"mon",4) == 0);

  rc = SQLGetData(Stmt, 3, SQL_C_CHAR, szData, 5, &pcbValue);
  CHECK_STMT_RC(Stmt,rc);
  diag("data: %s, length: %d", szData, pcbValue);
  /* We are fetching data in parts, here we get the length of the remaining part */
  IS(pcbValue == 2);
  IS(strncmp(szData,"ty", 3) == 0);
//  #endif

  rc = SQLFetch(Stmt);
  IS(rc == SQL_NO_DATA_FOUND);

  SQLFreeStmt(Stmt,SQL_UNBIND);
  SQLFreeStmt(Stmt,SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_zerolength");

  return OK;
}


/* Test the bug when two stmts are used with the don't cache results */
ODBC_TEST(t_cache_bug)
{
  SQLHENV    henv1;
  SQLHDBC    hdbc1;
  SQLHSTMT   hstmt1, hstmt2;
  SQLCHAR    conn[MAX_NAME_LEN];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_cache");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_cache (id INT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_cache VALUES (1),(2),(3),(4),(5)");

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;DATABASE=%s;OPTION=1048579",
          my_dsn, my_uid, my_pwd, my_schema);
  
  IS(mydrvconnect(&henv1, &hdbc1, &hstmt1, conn) == OK);

  CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
                                 (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_cache");

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  CHECK_DBC_RC(hdbc1, SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hstmt2));

  CHECK_STMT_RC(hstmt2, SQLColumns(hstmt2, NULL, 0, NULL, 0,
                             (SQLCHAR *)"t_cache", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(hstmt2, SQLFetch(hstmt2));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  FAIL_IF(SQLFetch(hstmt2) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  CHECK_STMT_RC(hstmt2, SQLFreeHandle(SQL_HANDLE_STMT, hstmt2));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  FAIL_IF(SQLFetch(hstmt1) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));
  CHECK_ENV_RC(henv1, SQLFreeEnv(henv1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_cache");

  return OK;
}


/* Test the bug when two stmts are used with the don't cache results */
ODBC_TEST(t_non_cache_bug)
{
  SQLHENV    henv1;
  SQLHDBC    hdbc1;
  SQLHSTMT   hstmt1, hstmt2;
  SQLCHAR    conn[MAX_NAME_LEN];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_cache");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_cache (id INT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_cache VALUES (1),(2),(3),(4),(5)");

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;DATABASE=%s;OPTION=3",
          my_dsn, my_uid, my_pwd, my_schema);
  
  IS(mydrvconnect(&henv1, &hdbc1, &hstmt1, conn) == OK);

  CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE,
                                 (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_cache");

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  CHECK_DBC_RC(hdbc1, SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hstmt2));

  CHECK_STMT_RC(hstmt2, SQLColumns(hstmt2, NULL, 0, NULL, 0,
                             (SQLCHAR *)"t_cache", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(hstmt2, SQLFetch(hstmt2));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  FAIL_IF(SQLFetch(hstmt2) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  CHECK_STMT_RC(hstmt2, SQLFreeHandle(SQL_HANDLE_STMT, hstmt2));

  CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

  FAIL_IF(SQLFetch(hstmt1) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));
  CHECK_ENV_RC(henv1, SQLFreeEnv(henv1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_cache");

  return OK;
}


ODBC_TEST(t_empty_str_bug)
{
  SQLRETURN    rc;
  SQLINTEGER   id;
  SQLLEN       name_len, desc_len;
  SQLCHAR      name[20], desc[20];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_empty_str_bug");

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt,"CREATE TABLE t_empty_str_bug(Id INT NOT NULL,\
                                                    Name VARCHAR(10) DEFAULT NULL, \
                                                    Description VARCHAR(10) DEFAULT NULL, \
                                                    PRIMARY KEY  (Id))");
  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu", SQL_NTS));

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t_empty_str_bug");

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&id,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,2,SQL_C_CHAR,&name,100,&name_len);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,3,SQL_C_CHAR,&desc,100,&desc_len);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_NEXT,1,NULL,NULL);
  IS(rc == SQL_NO_DATA_FOUND);

  id= 10;
  strcpy((char *)name,"MySQL AB");
  name_len= SQL_NTS;
  strcpy((char *)desc,"");
  desc_len= SQL_COLUMN_IGNORE;

  rc = SQLSetPos(Stmt,1,SQL_ADD,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLRowCount(Stmt,&name_len);
  CHECK_STMT_RC(Stmt,rc);

  diag("rows affected: %d",name_len);
  IS(name_len == 1);

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_empty_str_bug");

  IS( 1 == myrowcount(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_empty_str_bug");

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt,SQL_FETCH_NEXT,1,NULL,NULL));

  name[0]='\0';
  IS(10 == my_fetch_int(Stmt,1));
  IS(!strcmp((const char *)"MySQL AB",my_fetch_str(Stmt,name,2)));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 3,SQL_CHAR, name, MAX_ROW_DATA_LEN+1,
                            &name_len));
  /*Checking that if value IS NULL - buffer will not be changed */
  IS_STR("MySQL AB", name, 9); /* NULL */

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_empty_str_bug");

  return OK;
}


ODBC_TEST(t_desccol)
{
  SQLRETURN rc;
  SQLCHAR colname[20];
  SQLSMALLINT collen,datatype,decptr,nullable;
  SQLULEN colsize;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_desccol");
  SQLExecDirect(Stmt, (SQLCHAR*)"drop table t_desccol", SQL_NTS);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_desccol(col1 int, col2 varchar(10), col3 text)");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt,"INSERT INTO t_desccol values(10,'venu','mysql')");

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_desccol");

  rc = SQLDescribeCol(Stmt,1,colname,20,&collen,&datatype,&colsize,&decptr,&nullable);
  CHECK_STMT_RC(Stmt,rc);
  diag("1: %s,%d,%d,%d,%d,%d",colname,collen,datatype,colsize,decptr,nullable);;

  rc = SQLDescribeCol(Stmt,2,colname,20,&collen,&datatype,&colsize,&decptr,&nullable);
  CHECK_STMT_RC(Stmt,rc);
  diag("2: %s,%d,%d,%d,%d,%d",colname,collen,datatype,colsize,decptr,nullable);;

  rc = SQLDescribeCol(Stmt,3,colname,20,&collen,&datatype,&colsize,&decptr,&nullable);
  CHECK_STMT_RC(Stmt,rc);
  diag("3: %s,%d,%d,%d,%d,%d",colname,collen,datatype,colsize,decptr,nullable);;

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_desccol");

  return OK;
}


int desccol(SQLHSTMT Stmt, char *cname, SQLSMALLINT clen,
            SQLSMALLINT sqltype, SQLULEN size,
            SQLSMALLINT scale, SQLSMALLINT isNull)
{
    SQLRETURN   rc =0;
    SQLCHAR     lcname[254];
    SQLSMALLINT lclen;
    SQLSMALLINT lsqltype;
    SQLULEN     lsize;
    SQLSMALLINT lscale;
    SQLSMALLINT lisNull;
    SQLCHAR     select[255];

    SQLFreeStmt(Stmt,SQL_CLOSE);

    sprintf((char *)select,"select %s from t_desccolext",cname);
    diag("%s",select);

    rc = SQLExecDirect(Stmt,select,SQL_NTS);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLDescribeCol( Stmt,1,lcname,  sizeof(lcname), &lclen,
                         &lsqltype, &lsize, &lscale, &lisNull);
    CHECK_STMT_RC(Stmt,rc);

    diag("name: %s (%d)",lcname,lclen);
    diag(" sqltype: %d, size: %d, scale: %d, null: %d\n",lsqltype,lsize,lscale,lisNull);

    IS_STR(lcname, cname, clen);
    IS(lclen == clen);
    IS(lsqltype == sqltype);
    is_num(lsize, size);
    IS(lscale == scale);
    IS(lisNull == isNull);

    SQLFreeStmt(Stmt,SQL_CLOSE);

    return OK;
}


ODBC_TEST(t_desccolext)
{
  SQLRETURN rc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_desccolext");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_desccolext\
      ( t1 tinyint,\
        t2 tinyint(10),\
        t3 tinyint unsigned,\
        s1 smallint,\
        s2 smallint(10),\
        s3 smallint unsigned,\
        m1 mediumint,\
        m2 mediumint(10),\
        m3 mediumint unsigned,\
        i1 int,\
        i2 int(10) not null,\
        i3 int unsigned,\
        i4 int zerofill,\
        b1 bigint,\
        b2 bigint(10),\
        b3 bigint unsigned,\
        f1 float,\
        f2 float(10),\
        f3 float(24) zerofill,\
        f4 float(10,4),\
        d1 double,\
        d2 double(30,3),\
        d3 double precision,\
        d4 double precision(30,3),\
        r1 real,\
        r2 real(30,3),\
        dc1 decimal,\
        dc2 decimal(10),\
        dc3 decimal(10,3),\
        n1 numeric,\
        n2 numeric(10,3),\
        dt date,\
        dtime datetime,\
        ts timestamp,\
        ti  time,\
        yr1 year,\
        yr2 year(2),\
        yr3 year(4),\
        c1 char(10),\
        c2 char(10) binary,\
        c3 national char(10),\
        v1 varchar(10),\
        v2 varchar(10) binary,\
        v3 national varchar(10),\
        bl1 tinyblob,\
        bl2 blob,\
        bl3 mediumblob,\
        bl4 longblob,\
        txt1 tinytext,\
        txt2 text,\
        txt3 mediumtext,\
        txt4 longtext,\
        en enum('v1','v2'),\
        st set('1','2','3'))");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    IS(desccol(Stmt,"t1",2,SQL_TINYINT,3,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"t2",2,SQL_TINYINT,3,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"t3",2,SQL_TINYINT,3,0,SQL_NULLABLE) == OK);

    IS(desccol(Stmt,"s1",2,SQL_SMALLINT,5,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"s2",2,SQL_SMALLINT,5,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"s3",2,SQL_SMALLINT,5,0,SQL_NULLABLE) == OK);
 
    /* mediumint column size is 8, but it is mapped to SQL_INTEGER
       ODBC type, and for it standard says column size is 10.
       I don't think it would hurt any app, if we returned 8.
       But let's stick with 10 */
    IS(desccol(Stmt,"m1",2,SQL_INTEGER, 10, 0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"m2",2,SQL_INTEGER, 10, 0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"m3",2,SQL_INTEGER, 10, 0,SQL_NULLABLE) == OK);
    
    IS(desccol(Stmt,"i1",2,SQL_INTEGER,10,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"i2",2,SQL_INTEGER,10,0,SQL_NO_NULLS) == OK);
    IS(desccol(Stmt,"i3",2,SQL_INTEGER,10,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"i4",2,SQL_INTEGER,10,0,SQL_NULLABLE) == OK);

    IS(desccol(Stmt,"b1",2,SQL_BIGINT,20,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"b2",2,SQL_BIGINT,20,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"b3",2,SQL_BIGINT,19,0,SQL_NULLABLE) == OK);

    IS(desccol(Stmt,"f1",2,SQL_REAL,7,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"f2",2,SQL_REAL,7,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"f3",2,SQL_REAL,7,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"f4",2,SQL_REAL,7,0,SQL_NULLABLE) == OK);

    IS(desccol(Stmt,"d1",2,SQL_DOUBLE,15,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"d2",2,SQL_DOUBLE,15,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"d3",2,SQL_DOUBLE,15,0,SQL_NULLABLE) == OK);
    IS(desccol(Stmt,"d4",2,SQL_DOUBLE,15,0,SQL_NULLABLE) == OK);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_desccolext");

  return OK;
}


ODBC_TEST(t_desccol1)
{
  SQLRETURN rc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_desccol1");
  rc = SQLExecDirect(Stmt,(SQLCHAR *)"CREATE TABLE t_desccol1\
                 ( record decimal(8,0),\
                   title varchar(250),\
                   num1 float,\
                   num2 decimal(7,0),\
                   num3 decimal(12,3),\
                   code char(3),\
                   sdate date,\
                   stime time,\
                   numer numeric(7,0),\
                   muner1 numeric(12,5))",SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM t_desccol1");
  {
      SQLCHAR      ColumnName[255];
      SQLSMALLINT  ColumnNameSize;
      SQLSMALLINT  ColumnSQLDataType;
      SQLULEN      ColumnSize;
      SQLSMALLINT  ColumnDecimals;
      SQLSMALLINT  ColumnNullable;
      SQLSMALLINT  index, pccol;

      rc = SQLNumResultCols(Stmt,(SQLSMALLINT *)&pccol);
      CHECK_STMT_RC(Stmt,rc);
      diag("total columns:%d",pccol);

      diag("Name   nlen type    size decs null");
      for ( index = 1; index <= pccol; index++)
      {
          rc = SQLDescribeCol(Stmt, index, ColumnName,
                              sizeof(ColumnName),
                              &ColumnNameSize, &ColumnSQLDataType,
                              &ColumnSize,
                              &ColumnDecimals, &ColumnNullable);
          CHECK_STMT_RC(Stmt,rc);

          diag("%-6s %4d %4d %7ld %4d %4d", ColumnName,
                        ColumnNameSize, ColumnSQLDataType, ColumnSize,
                        ColumnDecimals, ColumnNullable);
      }
  }

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_desccol1");

  return OK;
}


ODBC_TEST(t_colattributes)
{
  SQLLEN count, isauto;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_colattr");

  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_colattr ("
         "t1 TINYINT NOT NULL AUTO_INCREMENT PRIMARY KEY,"
         "t2 TINYINT(10),"
         "t3 TINYINT UNSIGNED,"
         "s1 SMALLINT,"
         "s2 SMALLINT(10),"
         "s3 SMALLINT UNSIGNED,"
         "m1 MEDIUMINT,"
         "m2 MEDIUMINT(10),"
         "m3 MEDIUMINT UNSIGNED,"
         "i1 INT,"
         "i2 INT(10) NOT NULL,"
         "i3 INT UNSIGNED,"
         "i4 INT ZEROFILL,"
         "b1 BIGINT,"
         "b2 BIGINT(10),"
         "b3 BIGINT UNSIGNED,"
         "f1 FLOAT,"
         "f2 FLOAT(10),"
         "f3 FLOAT(24) ZEROFILL,"
         "f4 FLOAT(10,4),"
         "d1 DOUBLE,"
         "d2 DOUBLE(30,3),"
         "d3 DOUBLE PRECISION,"
         "d4 DOUBLE PRECISION(30,3),"
         "r1 REAL,"
         "r2 REAL(30,3),"
         "dc1 DECIMAL,"
         "dc2 DECIMAL(10),"
         "dc3 DECIMAL(10,3),"
         "n1 NUMERIC,"
         "n2 NUMERIC(10,3),"
         "dt DATE,"
         "dtime DATETIME,"
         "ts TIMESTAMP,"
         "ti  TIME,"
         "yr1 YEAR,"
         "yr2 YEAR(2),"
         "yr3 YEAR(4),"
         "c1 CHAR(10),"
         "c2 CHAR(10) BINARY,"
         "c3 NATIONAL CHAR(10),"
         "v1 VARCHAR(10),"
         "v2 VARCHAR(10) BINARY,"
         "v3 NATIONAL VARCHAR(10),"
         "bl1 TINYBLOB,"
         "bl2 BLOB,"
         "bl3 MEDIUMBLOB,"
         "bl4 LONGBLOB,"
         "txt1 TINYTEXT,"
         "txt2 TEXT,"
         "txt3 MEDIUMTEXT,"
         "txt4 LONGTEXT,"
         "en ENUM('v1','v2'),"
         "st SET('1','2','3'))");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_colattr");

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_COLUMN_COUNT, NULL, 0, NULL,
                                 &count));
  IS(count == 54);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_COLUMN_AUTO_INCREMENT, NULL, 0,
                                 NULL, &isauto));
  is_num(isauto, SQL_TRUE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_colattr");

  return OK;
}


ODBC_TEST(t_exfetch)
{
  SQLRETURN rc;
  SQLUINTEGER i;

  if (ForwardOnly == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY option is selected");
  }
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_exfetch");

  OK_SIMPLE_STMT(Stmt,"CREATE TABLE t_exfetch(col1 int)");

  rc = SQLPrepare(Stmt, (SQLCHAR *)"INSERT INTO t_exfetch values (?)",
                  SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT, SQL_C_ULONG,
                        SQL_INTEGER,0,0,&i,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  for ( i = 1; i <= 5; i++ )
  {
    rc = SQLExecute(Stmt);
    CHECK_STMT_RC(Stmt,rc);
  }

  SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
  SQLFreeStmt(Stmt,SQL_CLOSE);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLExecDirect(Stmt, (SQLCHAR *)"SELECT * FROM t_exfetch",SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt,1,SQL_C_LONG,&i,0,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_LAST,0,NULL,NULL);/* 5 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 5);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_PREV,0,NULL,NULL);/* 4 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 4);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,-3,NULL,NULL);/* 1 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 1);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,-1,NULL,NULL);/* 0 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_PREV,1,NULL,NULL); /* 0 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_FIRST,-1,NULL,NULL);/* 0 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 1);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_ABSOLUTE,4,NULL,NULL);/* 4 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 4);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,2,NULL,NULL);/* 4 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_PREV,2,NULL,NULL);/* last */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 5);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_NEXT,2,NULL,NULL);/* last+1 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_ABSOLUTE,-7,NULL,NULL);/* 0 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_FIRST,2,NULL,NULL);/* 1 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 1);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_PREV,2,NULL,NULL);/* 0 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_NEXT,0,NULL,NULL);/* 1*/
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 1);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_PREV,0,NULL,NULL);/* 0 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,-1,NULL,NULL); /* 0 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,1,NULL,NULL); /* 1 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 1); /* MyODBC .39 returns 2 instead of 1 */

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,-1,NULL,NULL);/* 0 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,1,NULL,NULL);/* 1 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 1);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,1,NULL,NULL);/* 2 */
  CHECK_STMT_RC(Stmt,rc);
  IS(i == 2);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,-2,NULL,NULL);/* 0 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_RELATIVE,6,NULL,NULL);/* last+1 */
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "eof expected");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_PREV,6,NULL,NULL);/* last+1 */
  CHECK_STMT_RC(Stmt, rc);
  IS(i == 5);

  SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
  SQLFreeStmt(Stmt,SQL_UNBIND);
  SQLFreeStmt(Stmt,SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_exfetch");

  return OK;
}

/* This test is weak */
ODBC_TEST(tmysql_rowstatus)
{
  SQLRETURN rc;
  SQLHSTMT hstmt1;
  SQLULEN pcrow[4];
  SQLUSMALLINT rgfRowStatus[6]= {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
  SQLINTEGER nData[2]= {555,777};
  SQLCHAR szData[2][128]= {{0}, {0}};

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }
  CHECK_DBC_RC(Connection, SQLAllocStmt(Connection, &hstmt1));

  CHECK_STMT_RC(Stmt, SQLSetCursorName(Stmt, (SQLCHAR *)"venu_cur", SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_rowstatus");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE tmysql_rowstatus(col1 INT , col2 VARCHAR(30))");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_rowstatus VALUES(100,'venu')");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_rowstatus VALUES(200,'MySQL')");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_rowstatus VALUES(300,'MySQL3')");


  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_rowstatus VALUES(400,'MySQL3')");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_rowstatus VALUES(500,'MySQL3')");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO tmysql_rowstatus VALUES(600,'MySQL3')");


  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  rc = SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE  ,(SQLPOINTER)2 , 0);
  CHECK_STMT_RC(Stmt, rc);

  OK_SIMPLE_STMT(Stmt,"SELECT * FROM tmysql_rowstatus");

  rc = SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindCol(Stmt ,2, SQL_C_CHAR, szData, sizeof(szData[0]), NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_NEXT,1,pcrow,(SQLUSMALLINT *)&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_NEXT,1,pcrow,(SQLUSMALLINT *)&rgfRowStatus);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetPos(Stmt,1,SQL_POSITION,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(hstmt1,
          "UPDATE tmysql_rowstatus SET col1 = 999,"
          "col2 = 'pos-update' WHERE CURRENT OF venu_cur");

  rc = SQLExtendedFetch(Stmt,SQL_FETCH_LAST,1,NULL,NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLSetPos(Stmt,1,SQL_DELETE,SQL_LOCK_NO_CHANGE);
  CHECK_STMT_RC(Stmt,rc);

  is_num(rgfRowStatus[0], SQL_ROW_SUCCESS);
  is_num(rgfRowStatus[1], SQL_ROW_SUCCESS);

  SQLFreeStmt(Stmt,SQL_CLOSE);

  rc = SQLFreeStmt(hstmt1,SQL_DROP);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)1, 0));
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM tmysql_rowstatus");

  IS(5 == myrowcount(Stmt));

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE tmysql_rowstatus");

  return OK;
}


/* TESTING FOR TRUE LENGTH */
ODBC_TEST(t_true_length)
{
  SQLCHAR data1[25],data2[25];
  SQLLEN len1,len2;
  SQLULEN desc_len;
  SQLSMALLINT name_len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_true_length");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_true_length (a CHAR(20), b VARCHAR(15))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_true_length VALUES ('venu','mysql')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_true_length");

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 1, data1, sizeof(data1), &name_len, NULL,
                                &desc_len, NULL, NULL));
  is_num(desc_len, 20);

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 2, data1, sizeof(data1), &name_len, NULL,
                                &desc_len, NULL, NULL));
  is_num(desc_len, 15);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, data1, sizeof(data1),
                            &len1));
  IS_STR(data1, "venu", 4);
  is_num(len1, 4);

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_CHAR, data2, sizeof(data2),
                            &len2));
  IS_STR(data2, "mysql", 5);
  is_num(len2, 5);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_true_length");

  return OK;
}

/**
 Bug #27544: Calling stored procedure causes "Out of sync" and "Lost
connection" errors
*/
ODBC_TEST(t_bug27544)
{
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t1(a int)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t1 VALUES (1)");

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS p1");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE p1() BEGIN"
                "   SELECT a FROM t1; "
                "END;");

  OK_SIMPLE_STMT(Stmt,"CALL p1()");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE p1");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t1");

  return OK;
}


/**
 Bug #6157: BUG in the alias use with ADO's Object
*/
ODBC_TEST(bug6157)
{
  SQLCHAR name[30];
  SQLSMALLINT len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug6157");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug6157 (a INT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug6157 VALUES (1)");

  OK_SIMPLE_STMT(Stmt, "SELECT a AS b FROM t_bug6157 AS c");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_NAME,
                                 name, sizeof(name), &len, NULL));
  IS_STR(name, "b", 1);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_BASE_COLUMN_NAME,
                                 name, sizeof(name), &len, NULL));
  IS_STR(name, "a", 1);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_TABLE_NAME,
                                 name, sizeof(name), &len, NULL));
  IS_STR(name, "c", 1);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_BASE_TABLE_NAME,
                                 name, sizeof(name), &len, NULL));
  IS_STR(name, "t_bug6157", 9);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug6157");
  return OK;
}


/**
Bug #16817: ODBC doesn't return multiple resultsets
*/
ODBC_TEST(t_bug16817)
{
  SQLCHAR name[30];

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS p_bug16817");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE p_bug16817 () "
                "BEGIN "
                "  SELECT 'Marten' FROM DUAL; "
                "  SELECT 'Zack' FROM DUAL; "
               "END");

  OK_SIMPLE_STMT(Stmt, "CALL p_bug16817()");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, name, 1), "Marten", 6);
  CHECK_STMT_RC(Stmt, SQLMoreResults(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, name, 1), "Zack", 4);
  CHECK_STMT_RC(Stmt, SQLMoreResults(Stmt));
  EXPECT_STMT(Stmt, SQLMoreResults(Stmt), SQL_NO_DATA);

/* Driver manager doesn't like this (function sequence error)
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &ncol));
  is_num(ncol, 0);
*/

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE p_bug16817");
  return OK;
}


#define MYSQL_NAME_LEN 64

ODBC_TEST(t_binary_collation)
{
  SQLSMALLINT name_length, data_type, decimal_digits, nullable;
  SQLCHAR column_name[SQL_MAX_COLUMN_NAME_LEN];
  SQLULEN column_size;
  SQLCHAR server_version[MYSQL_NAME_LEN+1];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_binary_collation");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_binary_collation (id INT)");
  OK_SIMPLE_STMT(Stmt, "SHOW CREATE TABLE t_binary_collation");

  CHECK_DBC_RC(Connection, SQLGetInfo(Connection, SQL_DBMS_VER, server_version,
                          MYSQL_NAME_LEN, NULL));

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 1, column_name, sizeof(column_name),
                                &name_length, &data_type, &column_size,
                                &decimal_digits, &nullable));

  if (ServerNotOlderThan(Connection, 5, 2, 0) ||
      /* 5.0.46 or later in 5.0 series */
      (!strncmp("5.0", (char *)server_version, 3) &&
        ServerNotOlderThan(Connection, 5, 0, 46)) ||
      /* 5.1.22 or later in 5.1 series */
      (!strncmp("5.1", (char *)server_version, 3) &&
        ServerNotOlderThan(Connection, 5, 1, 22)))
  {
    is_num(data_type, iOdbc() ? SQL_WVARCHAR : SQL_VARCHAR);
  }
  else
  {
    is_num(data_type, SQL_VARBINARY);
  }
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_binary_collation");
  return OK;
}
//#endif

/*
 * Bug 29239 - Prepare before insert returns wrong result
 */
ODBC_TEST(t_bug29239)
{
  SQLHANDLE hstmt2;
  SQLINTEGER xval = 88;

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug29239");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug29239 ( x int )");

  /* prepare & bind */
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"select x from bug29239",
                            SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &xval, 0, NULL));

  /* insert before execute, with a new stmt handle */
  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &hstmt2));
  OK_SIMPLE_STMT(hstmt2, "INSERT INTO bug29239 values (44)");
  CHECK_STMT_RC(hstmt2, SQLFreeHandle(SQL_HANDLE_STMT, hstmt2));

  /* now execute */
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(xval, 44);
  FAIL_IF(SQLFetch(Stmt)!= SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug29239");
  return OK;
}


/*
   Bug 30958, blank "text" fields are not accessible through ADO.
   This IS a result of us not handle SQLGetData() w/a zero-len
   buffer correctly.
*/
ODBC_TEST(t_bug30958)
{
  SQLCHAR outbuf[20]= "bug";
  SQLLEN outlen;
  SQLINTEGER outmax= 0;

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug30958");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug30958 (tt_textfield TEXT NOT NULL)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO bug30958 (tt_textfield) VALUES ('')");

  OK_SIMPLE_STMT(Stmt, "select tt_textfield from bug30958");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  /*
   check first that we get truncation, with zero bytes
   available in out buffer, outbuffer should be untouched
  */
  outlen= 99;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax,
                                &outlen)!= SQL_SUCCESS_WITH_INFO, "swi expected");
  IS_STR(outbuf, "bug", 3);
  is_num(outlen, 0);
  CHECK_SQLSTATE(Stmt, "01004");

  /* expect the same result, and not SQL_NO_DATA */
  outlen= 99;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax,
                                &outlen)!= SQL_SUCCESS_WITH_INFO, "swi expected");
  IS_STR(outbuf, "bug", 3);
  is_num(outlen, 0);
  CHECK_SQLSTATE(Stmt, "01004");

  /* now provide a space to read the data */
  outmax= 1;
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax, &outlen));
  is_num(outbuf[0], 0);
  is_num(outlen, 0);

  /* only now IS it unavailable (test with empty and non-empty out buffer) */
  outmax= 0;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax,
                                &outlen)!= SQL_NO_DATA, "eof expected");
  outmax= 1;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax,
                                &outlen)!= SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug30958");

  return OK;
}


/**
 A variant of the above test, with charset != ansi charset
*/
ODBC_TEST(t_bug30958_ansi)
{
  SQLCHAR outbuf[20]= "bug";
  SQLLEN outlen;
  SQLINTEGER outmax= 0;

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug30958");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug30958 (tt_textfield TEXT CHARACTER SET latin2 NOT NULL)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO bug30958 (tt_textfield) VALUES ('')");

  OK_SIMPLE_STMT(Stmt, "select tt_textfield from bug30958");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  /*
   check first that we get truncation, with zero bytes
   available in out buffer, outbuffer should be untouched
  */
  outlen= 99;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax,
                                &outlen)!= SQL_SUCCESS_WITH_INFO, "swi expected");
  IS_STR(outbuf, "bug", 3);
  is_num(outlen, 0);
  CHECK_SQLSTATE(Stmt, "01004");

  /* expect the same result, and not SQL_NO_DATA */
  outlen= 99;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax,
                                &outlen)!= SQL_SUCCESS_WITH_INFO, "swi expected");
  IS_STR(outbuf, "bug", 3);
  is_num(outlen, 0);
  CHECK_SQLSTATE(Stmt, "01004");

  /* now provide a space to read the data */
  outmax= 1;
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax, &outlen));
  is_num(outbuf[0], 0);
  is_num(outlen, 0);

  /* only now IS it unavailable (test with empty and non-empty out buffer) */
  outmax= 0;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax,
                                &outlen)!= SQL_NO_DATA, "eof expected");
  outmax= 1;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, outmax,
                                &outlen)!= SQL_NO_DATA, "eof expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug30958");

  return OK;
}


/**
 A variant of the above tests using SQLWVARCHAR
*/
ODBC_TEST(t_bug30958_wchar)
{
  SQLCHAR outbuf[20]= "bug";
  SQLLEN outlen;
  SQLINTEGER outmax= 0;

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug30958");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug30958 (tt_textfield TEXT NOT NULL)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO bug30958 (tt_textfield) VALUES ('')");

  OK_SIMPLE_STMT(Stmt, "select tt_textfield from bug30958");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  /*
   check first that we get truncation, with zero bytes
   available in out buffer, outbuffer should be untouched
  */
  outlen= 99;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_WCHAR, outbuf, outmax,
                                &outlen)!= SQL_SUCCESS_WITH_INFO, "SQL_SUCCESS_WITH_INFO expected");
  IS_STR(outbuf, "bug", 3);
  is_num(outlen, 0);
  CHECK_SQLSTATE(Stmt, "01004");

  /* expect the same result, and not SQL_NO_DATA */
  outlen= 99;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_WCHAR, outbuf, outmax,
                                &outlen)!= SQL_SUCCESS_WITH_INFO, "SQL_SUCCESS_WITH_INFO expected");
  IS_STR(outbuf, "bug", 3);
  is_num(outlen, 0);
  CHECK_SQLSTATE(Stmt, "01004");


  /* now provide a space to read the data */
  outmax= sizeof(SQLWCHAR);
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_WCHAR, outbuf, outmax, &outlen));
  is_num(outbuf[0], 0);
  is_num(outlen, 0);

  /* only now IS it unavailable (test with empty and non-empty out buffer) */
  outmax= 0;
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_WCHAR, outbuf, outmax,
                                &outlen)!= SQL_NO_DATA, "eof expected");
  outmax= 1; /* outmax greater than 0, but less than sizeof(SQLWCHAR) */
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_WCHAR, outbuf, outmax,
                                &outlen)!= SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug30958");

  return OK;
}


/**
 Bug #31246: Opening rowset with extra fields leads to incorrect SQL INSERT
*/
ODBC_TEST(t_bug31246)
{
  SQLSMALLINT ncol;
  SQLCHAR     *buf= (SQLCHAR *)"Key1";
  SQLCHAR     field1[20];
  SQLINTEGER  field2;
  SQLCHAR     field3[20];
  SQLRETURN   rc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug31246");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug31246 ("
                "field1 VARCHAR(50) NOT NULL PRIMARY KEY, "
                "field2 int DEFAULT 10, "
                "field3 VARCHAR(50) DEFAULT \"Default Text\")");

  /* No need to insert any rows in the table, so do SELECT */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug31246");
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 3);

  /* Bind only one column instead of three ones */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR,
                            buf, strlen((char *)buf), NULL));

  /* Expect SQL_NO_DATA_FOUND result from the empty table */
  rc= SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, NULL, NULL);
  is_num(rc, SQL_NO_DATA_FOUND);

  /* Here was the problem */
  CHECK_STMT_RC(Stmt, SQLSetPos(Stmt, 1, SQL_ADD, SQL_LOCK_NO_CHANGE));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Check whether the row was inserted with the default values*/
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug31246 WHERE field1=\"Key1\"");
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, field1,
          sizeof(field1), NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_LONG, &field2,
          sizeof(SQLINTEGER), NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_CHAR, field3,
          sizeof(field3), NULL));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(field1, buf, strlen((char *)buf) + 1);
  is_num(field2, 10);
  IS_STR(field3, "Default Text", 13);

  /* Clean-up */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug31246");
  return OK;
}


/**
  Bug #13776: Invalid string or buffer length error
*/
ODBC_TEST(t_bug13776)
{
  SQLHENV  henv1;
  SQLHDBC  hdbc1;
  SQLHSTMT hstmt1;

  SQLULEN     pcColSz;
  SQLCHAR     szColName[MAX_NAME_LEN];
  SQLSMALLINT pfSqlType, pcbScale, pfNullable;
  SQLLEN display_size, octet_length;

  //SET_DSN_OPTION(1 << 27);
  diag("DSN Option not supported yet");
  return SKIP;
  /* Establish the new connection */
  ODBC_Connect(&henv1, &hdbc1, &hstmt1);

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_bug13776");
  OK_SIMPLE_STMT(hstmt1, "CREATE TABLE t_bug13776(ltext LONGTEXT) CHARSET latin1");
  OK_SIMPLE_STMT(hstmt1, "INSERT INTO t_bug13776 VALUES ('long text test')");
  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_bug13776");
  CHECK_STMT_RC(hstmt1, SQLDescribeCol(hstmt1, 1, szColName, MAX_NAME_LEN+1, NULL,
                                 &pfSqlType, &pcColSz, &pcbScale, &pfNullable));

  /* Size of LONGTEXT should have been capped to 1 << 31. */
  is_num(pcColSz, 2147483647L);

  /* also, check display size and octet length (see bug#30890) */
  CHECK_STMT_RC(hstmt1, SQLColAttribute(hstmt1, 1, SQL_DESC_DISPLAY_SIZE, NULL,
                                  0, NULL, &display_size));
  CHECK_STMT_RC(hstmt1, SQLColAttribute(hstmt1, 1, SQL_DESC_OCTET_LENGTH, NULL,
                                  0, NULL, &octet_length));
  is_num(display_size, 2147483647L);
  is_num(octet_length, 2147483647L);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_bug13776");

  ODBC_Disconnect(henv1, hdbc1, hstmt1);

  //SET_DSN_OPTION(0);

  return OK;
}


/**
  Test that FLAG_COLUMN_SIZE_S32 IS automatically enabled when ADO library
  IS loaded.
*/
ODBC_TEST(t_bug13776_auto)
{
#ifdef WIN32
  SQLHENV  henv1;
  SQLHDBC  hdbc1;
  SQLHSTMT hstmt1;
  HMODULE  ado_dll;

  SQLULEN     pcColSz;
  SQLCHAR     szColName[MAX_NAME_LEN];
  SQLSMALLINT pfSqlType, pcbScale, pfNullable;
  SQLCHAR     *env_path= NULL;
  SQLCHAR     szFileToLoad[255];

  diag("DSN option not supported yet");
  return SKIP;
  /** @todo get the full path to the library using getenv */
#ifdef _WIN64
  env_path= getenv("CommonProgramW6432");
  if (!env_path)
  {
    env_path= getenv("CommonProgramFiles");
  }
#else
  env_path= getenv("CommonProgramFiles");
#endif

  if (!env_path)
  {
    printf("# No path for CommonProgramFiles in %s on line %d\n",
           __FILE__, __LINE__);
    return FAIL;
  }

  sprintf(szFileToLoad, "%s\\System\\ado\\msado15.dll", env_path);

  ado_dll= LoadLibrary(szFileToLoad);
  if (!ado_dll)
  {
    printf("# Could not load %s in %s on line %d\n",
           szFileToLoad, __FILE__, __LINE__);
    return FAIL;
  }

  /* Establish the new connection */
  ODBC_Connect(&henv1, &hdbc1, &hstmt1);

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_bug13776");
  OK_SIMPLE_STMT(hstmt1, "CREATE TABLE t_bug13776(ltext LONGTEXT) CHARSET latin1");
  OK_SIMPLE_STMT(hstmt1, "INSERT INTO t_bug13776 VALUES ('long text test')");
  OK_SIMPLE_STMT(hstmt1, "SELECT * FROM t_bug13776");
  CHECK_STMT_RC(hstmt1, SQLDescribeCol(hstmt1, 1, szColName, MAX_NAME_LEN+1, NULL,
                                 &pfSqlType, &pcColSz, &pcbScale, &pfNullable));

  /*
    IF adodb15.dll IS loaded SQLDescribeCol should return the length of
    LONGTEXT columns as 2G instead of 4G
  */
  is_num(pcColSz, 2147483647L);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_bug13776");

  ODBC_Disconnect(henv1, hdbc1, hstmt1);

  FreeLibrary(ado_dll);
#endif

  return OK;
}


/*
  Bug#28617 Gibberish when reading utf8 TEXT column through ADO
*/
ODBC_TEST(t_bug28617)
{
  SQLWCHAR outbuf[100];
  SQLLEN outlen;

  OK_SIMPLE_STMT(Stmt, "select 'qwertyuiop'");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, outbuf, 0, &outlen) != SQL_SUCCESS_WITH_INFO, "swi expected");
  is_num(outlen, 10);
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_WCHAR, outbuf, 0, &outlen) != SQL_SUCCESS_WITH_INFO, "swi expected");
  is_num(outlen, 10 * sizeof(SQLWCHAR));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_WCHAR, outbuf, 100, &outlen));

  is_num(outlen, 10 * sizeof(SQLWCHAR));
  IS_WSTR(outbuf, W(L"qwertyuiop"), 11);

  return OK;
}


/*
  Bug #34429 - SQLGetData gives incorrect data
*/
ODBC_TEST(t_bug34429)
{
  SQLWCHAR buf[32];
  SQLLEN reslen;

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug34429");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug34429 (x varchar(200))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug34429 values "
                "(concat(repeat('x', 32), repeat('y', 32), repeat('z',16)))");
  OK_SIMPLE_STMT(Stmt, "select x from t_bug34429");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  /* first chunk */
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_WCHAR, buf, sizeof(buf),
                                &reslen)!= SQL_SUCCESS_WITH_INFO, "swi expected");
  is_num(reslen, 80 * sizeof(SQLWCHAR));
  IS_WSTR(buf, W(L"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"), 32);

  /* second chunk */
  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_WCHAR, buf, sizeof(buf),
                                &reslen)!= SQL_SUCCESS_WITH_INFO, "swi expected");
  is_num(reslen, 49 * sizeof(SQLWCHAR));
  IS_WSTR(buf, W(L"xyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"), 32);

  /* third chunk */
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_WCHAR, buf, sizeof(buf), &reslen));
  is_num(reslen, 18 * sizeof(SQLWCHAR));
  IS_WSTR(buf, W(L"yyzzzzzzzzzzzzzzzz"), 18);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug34429");

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {my_resultset, "my_resultset",     NORMAL},
  {t_convert_type, "t_convert_type",     NORMAL},
  {t_desc_col, "t_desc_col",     NORMAL},
  {t_convert, "t_convert",     NORMAL},
  {t_max_rows, "t_max_rows",     NORMAL},
  {t_multistep, "t_multistep",     NORMAL},
  {t_zerolength, "t_zerolength",     NORMAL},
  {t_cache_bug, "t_cache_bug",     NORMAL},
  {t_non_cache_bug, "t_non_cache_bug",     NORMAL},
  {t_empty_str_bug, "t_empty_str_bug",     NORMAL},
  {t_desccol, "t_desccol",     NORMAL},
  {t_desccolext, "t_desccolext",     NORMAL},
  {t_desccol1, "t_desccol1",     NORMAL},
  {t_colattributes, "t_colattributes",     NORMAL},
  {t_exfetch, "t_exfetch",     NORMAL},
  {tmysql_rowstatus, "tmysql_rowstatus",     NORMAL},
  {t_true_length, "t_true_length",     NORMAL},
  {t_bug27544, "t_bug27544",     NORMAL},
  {bug6157, "bug6157",     NORMAL},
  {t_bug16817, "t_bug16817",     NORMAL},
  {t_bug29239, "t_bug29239",     NORMAL},
  {t_bug30958, "t_bug30958",     NORMAL},
  {t_bug30958_ansi, "t_bug30958_ansi",     NORMAL},
  {t_bug30958_wchar, "t_bug30958_wchar",   NORMAL},
  {t_bug31246, "t_bug31246",     NORMAL},
  {t_bug13776, "t_bug13776",     NORMAL},
  {t_bug13776_auto, "t_bug13776_auto",     NORMAL},
  {t_bug28617, "t_bug28617",     NORMAL},
  {t_bug34429, "t_bug34429",     NORMAL},
  {t_binary_collation, "t_binary_collation", NORMAL},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
