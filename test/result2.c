/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013, 2016 MariaDB Corporation AB

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


/*
  Bug #32420 - Don't cache results and SQLExtendedFetch work badly together
*/
ODBC_TEST(t_bug32420)
{
  SQLHANDLE henv1, hdbc1, hstmt1;
  SQLINTEGER nData[4];
  SQLCHAR szData[4][16];
  SQLUSMALLINT rgfRowStatus[4];

  //SET_DSN_OPTION(1048576);

  ODBC_Connect(&henv1, &hdbc1, &hstmt1);

  OK_SIMPLE_STMT(hstmt1, "drop table if exists bug32420");
  OK_SIMPLE_STMT(hstmt1, "CREATE TABLE bug32420 ("\
                "tt_int INT PRIMARY KEY auto_increment,"\
                "tt_varchar VARCHAR(128) NOT NULL)");
  OK_SIMPLE_STMT(hstmt1, "INSERT INTO bug32420 VALUES "\
                "(100, 'string 1'),"\
                "(200, 'string 2'),"\
                "(300, 'string 3'),"\
                "(400, 'string 4')");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLSetStmtOption(hstmt1, SQL_ROWSET_SIZE, 4));

  OK_SIMPLE_STMT(hstmt1, "select * from bug32420 order by 1");
  CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, nData, 0, NULL));
  CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_CHAR, szData, sizeof(szData[0]),
                            NULL));
  CHECK_STMT_RC(hstmt1, SQLExtendedFetch(hstmt1, SQL_FETCH_NEXT, 0, NULL, 
                                   rgfRowStatus));

  is_num(nData[0], 100);
  IS_STR(szData[0], "string 1", 8);
  is_num(nData[1], 200);
  IS_STR(szData[1], "string 2", 8);
  is_num(nData[2], 300);
  IS_STR(szData[2], "string 3", 8);
  is_num(nData[3], 400);
  IS_STR(szData[3], "string 4", 8);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
  OK_SIMPLE_STMT(hstmt1, "drop table if exists bug32420");

  ODBC_Disconnect(henv1, hdbc1, hstmt1);

  //SET_DSN_OPTION(1048576);

  return OK;
}


/**
 Bug #34575: SQL_C_CHAR value type and numeric parameter type causes trouble
*/
ODBC_TEST(t_bug34575)
{
  SQLCHAR buff[10];
  SQLLEN len= 0;

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *) "SELECT ?", SQL_NTS));
  strcpy((char *)buff, "2.0");
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_DECIMAL, 10, 0, buff, sizeof(buff),
                                  &len));

  /* Note: buff has '2.0', but len is still 0! */
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 1), "", 1);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "eof expected");

  strcpy((char *)buff, "2.0");
  len= 3;

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 1), "2.0", 4);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


/*
Bug #24131 SHOW CREATE TABLE result truncated with mysql 3.23 and ODBC driver 3.51.12.00
*/
ODBC_TEST(t_bug24131)
{
  SQLCHAR buff[1024];
  SQLLEN boundLen= 0;
  SQLULEN count;
  UWORD status;
  SQLULEN colSize;

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug24131");

  /* Table definition should be long enough. */
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE `bug24131` ("
    "`Codigo` int(10) unsigned NOT NULL auto_increment,"
    "`Nombre` varchar(255) default NULL,"
    "`Telefono` varchar(255) default NULL,"
    "`Observaciones` longtext,"
    "`Direccion` varchar(255) default NULL,"
    "`Dni` varchar(255) default NULL,"
    "`CP` int(11) default NULL,"
    "`Provincia` varchar(255) default NULL,"
    "`Poblacion` varchar(255) default NULL,"
    "PRIMARY KEY  (`Codigo`)"
    ") ENGINE=MyISAM AUTO_INCREMENT=11 DEFAULT CHARSET=utf8");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"show create table bug24131", SQL_NTS));



  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt,2,SQL_C_BINARY, buff, 1024, &boundLen));

  /* Note: buff has '2.0', but len is still 0! */
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 2, buff, sizeof(buff), NULL, NULL,
                                &colSize, NULL, NULL));

  CHECK_STMT_RC(Stmt, SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, &count, &status));

  if (sizeof(SQLLEN) == 4)
    diag("colSize: %lu, boundLen: %ld", colSize, boundLen);
  else
    diag("colSize: %llu, boundLen: %lld", colSize, boundLen);
  IS(colSize >= (SQLULEN)boundLen);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists bug24131");

  return OK;
}


/*
  Bug #36069 - SQLProcedures followed by a SQLFreeStmt causes a crash
 */
ODBC_TEST(t_bug36069)
{
  SQLSMALLINT size;

  CHECK_STMT_RC(Stmt, SQLProcedures(Stmt, NULL, 0, NULL, 0,
                               (SQLCHAR *)"non-existing", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &size));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"select ?", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &size));

  return OK;
}


/*
  Bug #41942 - SQLDescribeCol() segfault with non-zero name length
  and null buffer
*/
ODBC_TEST(t_bug41942)
{
  SQLSMALLINT len;
  OK_SIMPLE_STMT(Stmt, "select 1 as name");
  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 1, NULL, 10, &len,
                                NULL, NULL, NULL, NULL));
  is_num(len, 4);
  return OK;
}


/*
  Bug 39644 - Binding SQL_C_BIT to an integer column is not working
 */
ODBC_TEST(t_bug39644)
{
  char col1 = 0x3f;
  char col2 = 0xff;
  char col3 = 0x0;
  char col4 = 0x1;

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug39644");
  OK_SIMPLE_STMT(Stmt, "create table t_bug39644(col1 INT, col2 INT,"\
	            "col3 BIT, col4 BIT)");

  OK_SIMPLE_STMT(Stmt, "insert into t_bug39644 VALUES (5, 0, 1, 0)");

  /* Do SELECT */
  OK_SIMPLE_STMT(Stmt, "SELECT * from t_bug39644");

  /* Now bind buffers */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_BIT, &col1, sizeof(char), 0));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_BIT, &col2, sizeof(char), 0));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_BIT, &col3, sizeof(char), 0));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 4, SQL_C_BIT, &col4, sizeof(char), 0));

  /* Fetch and check results */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS( col1 == 1 );
  IS( col2 == 0 );
  IS( col3 == 1 );
  IS( col4 == 0 );

  /* Clean-up */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table t_bug39644");

  return OK;
}


/*
Bug#32821(it might be duplicate though): Wrong value if bit field is bound to
other than SQL_C_BIT variable
*/
ODBC_TEST(t_bug32821)
{
  SQLRETURN     rc;
  SQLUINTEGER   b;
  SQLUSMALLINT  c;
  SQLLEN        a_ind, b_ind, c_ind, i, j, k;
  unsigned char a;

  SQL_NUMERIC_STRUCT b_numeric;

  SQLUINTEGER par=  sizeof(SQLUSMALLINT)*8+1;
  SQLUINTEGER beoyndShortBit= 1<<(par-1);
  SQLLEN      sPar= sizeof(SQLUINTEGER);

  /* 131071 = 0x1ffff - all 1 for field c*/
  SQLCHAR * insStmt= "insert into t_bug32821 values (0,0,0),(1,1,1)\
                      ,(1,255,131071),(1,258,?)";
  const unsigned char expected_a[]= {'\0', '\1', '\1', '\1'};
  const SQLUINTEGER   expected_b[]= {0L, 1L, 255L, 258L};
  const SQLUSMALLINT  expected_c[]= {0, 1, 65535, 0};

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug32821");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug32821 (a BIT(1), b BIT(16), c BIT(17))");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, insStmt, SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG
    , SQL_INTEGER, 0, 0, &beoyndShortBit, 0
    , &sPar ));
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT a,b,c FROM t_bug32821");

  CHECK_STMT_RC( Stmt, SQLBindCol( Stmt, 1, SQL_C_BIT,    &a, 0, &a_ind ) );
  CHECK_STMT_RC( Stmt, SQLBindCol( Stmt, 2, SQL_C_ULONG,  &b, 0, &b_ind ) );
  /*CHECK_STMT_RC( Stmt, SQLBindCol( Stmt, 1, SQL_C_TYPE_DATE,  &d, 0, &b_ind ) );*/
  CHECK_STMT_RC( Stmt, SQLBindCol( Stmt, 3, SQL_C_USHORT, &c, 0, &c_ind ) );

  i= 0;
  while( (rc= SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0)) != SQL_NO_DATA_FOUND)
  {
    diag("testing row #%d", i+1);
    is_num(a, expected_a[i]);
    is_num(b, expected_b[i]);
    is_num(c, expected_c[i]);

    /*SQLGetData(Stmt, 1, SQL_C_BIT, &a, 0, &a_ind);
    is_num(a, expected_a[i]);
    SQLGetData(Stmt, 1, SQL_C_ULONG, &b, 0, &b_ind);
    is_num(b, expected_b[i]);
    SQLGetData(Stmt, 1, SQL_C_USHORT, &c, 0, &c_ind);
    is_num(c, expected_c[i]);*/
    
    /* Test of binding to numeric - currently is disabled. I think I it's better not to support this conversion at all */
    for (k= 1; k < 0; ++k)
    {
      b_ind= sizeof(SQL_NUMERIC_STRUCT);
      SQLGetData(Stmt, (SQLUSMALLINT)k, SQL_C_NUMERIC, &b_numeric, 0, &b_ind);

      b= 0;
      for(j= 0; j < b_numeric.precision; ++j)
      {
        b+= (0xff & b_numeric.val[j]) << 8*j;
      }

      switch (k)
      {
      case 1: is_num(b, expected_a[i]); break;
      case 2: is_num(b, expected_b[i]); break;
      }
      
    }
 
    ++i;
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug32821");
  return OK;
}


/*
  Bug #34271 - C/ODBC 5.1 does not list table fields in MSQRY32
*/
ODBC_TEST(t_bug34271)
{
  SQLINTEGER x1= 0, x2= 0;

  /* execute the query, but bind only the first column */
  OK_SIMPLE_STMT(Stmt, "select 1,2");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &x1, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(x1, 1);
  is_num(x2, 0);
  x1= 0;

  /* unbind */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));

  /* execute the query, but bind only the second column */
  OK_SIMPLE_STMT(Stmt, "select 1,2");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_LONG, &x2, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(x1, 0);
  is_num(x2, 2);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


/*
  Bug#32684 - chunked retrieval of SQL_C_WCHAR fails
*/
ODBC_TEST(t_bug32684)
{
  SQLWCHAR wbuf[20];
  SQLCHAR abuf[20];
  SQLLEN wlen, alen;
  OK_SIMPLE_STMT(Stmt, "select repeat('x', 100), repeat('y', 100)");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  do
  {
    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, abuf,
                              20, &alen));
    diag("data= %s, len=%d\n", abuf, alen);
  } while(alen > 20);
  /* Small addition to ensure that connector returns SQL_NO_DATA after all data fetched */
  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, abuf, 20, &alen), SQL_NO_DATA);
  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, abuf, 20, &alen), SQL_NO_DATA);

  do
  {
    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_WCHAR, wbuf,
                              20 * sizeof(SQLWCHAR), &wlen));
    wprintf(L"# data= %s, len=%d\n\n", wbuf, wlen);
  } while(wlen > 20 * sizeof(SQLWCHAR));

  return OK;
}


/*
  Bug 55024 - Wrong type returned by SQLColAttribute(SQL_DESC_PRECISION...) in 64-bit Linux
 */
ODBC_TEST(t_bug55024)
{
  SQLSMALLINT len;
  SQLLEN      res;

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "DROP TABLE IF EXISTS t_test55024", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "CREATE TABLE t_test55024(col01 LONGTEXT, "\
                                                                  "col02 BINARY(16),"\
                                                                  "col03 VARBINARY(16),"\
                                                                  "col04 LONGBLOB,"\
                                                                  "col05 BIGINT,"\
                                                                  "col06 TINYINT,"\
                                                                  "col07 BIT, col08 DOUBLE"\
                                                                  ") CHARSET latin1", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "INSERT INTO t_test55024 VALUES ('a', 'b', 'c', 'd', 999, 111, 1, 3.1415)", SQL_NTS));


  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "SELECT * FROM t_test55024", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_TYPE, NULL, 0, &len, &res));
  is_num(res, SQL_LONGVARCHAR);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 2, SQL_DESC_TYPE, NULL, 0, &len, &res));
  is_num(res, SQL_BINARY);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 3, SQL_DESC_TYPE, NULL, 0, &len, &res));
  is_num(res, SQL_VARBINARY);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 4, SQL_DESC_TYPE, NULL, 0, &len, &res));
  is_num(res, SQL_LONGVARBINARY);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 5, SQL_DESC_TYPE, NULL, 0, &len, &res));
  is_num(res, SQL_BIGINT);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 6, SQL_DESC_TYPE, NULL, 0, &len, &res));
  is_num(res, SQL_TINYINT);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 7, SQL_DESC_TYPE, NULL, 0, &len, &res));
  is_num(res, SQL_BIT);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug55024");
  return OK;
}


/*
Bug #56677 - SQLNumResultCols() causes the driver to return 
only first row in the resultset
*/
ODBC_TEST(t_bug56677)
{
  SQLINTEGER  nData;
  SQLCHAR     szData[16];
  SQLSMALLINT colCount;

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug56677");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug56677 ("\
    "tt_int INT PRIMARY KEY auto_increment,"\
    "tt_varchar VARCHAR(128) NOT NULL)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO bug56677 VALUES "\
    "(100, 'string 1'),"\
    "(200, 'string 2'),"\
    "(300, 'string 3'),"\
    "(400, 'string 4')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "select * from bug56677", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &colCount));

  is_num(colCount, 2);

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
    NULL));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(nData, 100);
  IS_STR(szData, "string 1", 8);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(nData, 200);
  IS_STR(szData, "string 2", 8);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(nData, 300);
  IS_STR(szData, "string 3", 8);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(nData, 400);
  IS_STR(szData, "string 4", 8);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists bug56677");

  return OK;
}

#define MYSQL_NAME_LEN 64

/* Test of SQLDescribeCol and SQLColAttribute if they are called before SQLExecute.
   Bug#56717 */
ODBC_TEST(t_desccol_before_exec)
{
  SQLINTEGER  nData= 200;
  SQLCHAR     szData[128];
  SQLSMALLINT colCount;
  char        colname[MYSQL_NAME_LEN];
  SQLULEN     collen;
  SQLLEN      coltype;

  diag("desccol before exec not supported");
  return SKIP;

  OK_SIMPLE_STMT(Stmt, "drop table if exists desccol_before_exec");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE desccol_before_exec ("\
    "tt_int INT PRIMARY KEY auto_increment,"\
    "tt_varchar VARCHAR(128) CHARACTER SET latin1 NOT NULL)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO desccol_before_exec VALUES "\
    "(100, 'string 1'),"\
    "(200, 'string 2'),"\
    "(300, 'string 3'),"\
    "(400, 'string 4')");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "select tt_varchar from desccol_before_exec where tt_int > ?", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 1, colname, sizeof(colname), NULL,
    NULL, &collen, NULL, NULL));

  IS_STR(colname, "tt_varchar", 11);
  is_num(collen, 128);

  /* Just to make sure that SQLNumResultCols still works fine */
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &colCount));

  is_num(colCount, 1);

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, szData, sizeof(szData),
    NULL));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(szData, "string 3", 8);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(szData, "string 4", 8);

  /* Now doing all the same things with SQLColAttribute */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "select tt_int, tt_varchar "
                                   "from desccol_before_exec "
                                   "where tt_int <= ?", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 2, SQL_DESC_TYPE, NULL, 0, NULL, &coltype));
  is_num(coltype, SQL_VARCHAR);

  /* Just to make sure that SQLNumResultCols still works fine */
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &colCount));

  is_num(colCount, 2);

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &nData, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, szData, sizeof(szData),
    NULL));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(szData, "string 1", 8);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(szData, "string 2", 8);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists desccol_before_exec");

  return OK;
}


/* Bug #62657 	A failure on one stmt causes another stmt to fail */
ODBC_TEST(t_bug62657)
{
  SQLHSTMT hstmt1;

  OK_SIMPLE_STMT(Stmt, "DROP table IF EXISTS b62657");

  OK_SIMPLE_STMT(Stmt, "CREATE table b62657(i int)");

  OK_SIMPLE_STMT(Stmt, "insert into b62657 values(1),(2)");


  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "select * from b62657", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  /* Any failing  query would do the job here */
  CHECK_DBC_RC(hstmt1, SQLAllocStmt(Connection, &hstmt1));

  FAIL_IF(SQLExecDirect(hstmt1, "select * from some_ne_rubbish", SQL_NTS) != SQL_ERROR, "Error expected");

  /* Error of other query before all rows fetched causes next fetch
     to fail */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP table b62657");

  return OK;
}


ODBC_TEST(t_row_status)
{
  SQLHANDLE ird, ard;
  SQLUSMALLINT arr1[2], arr2[2], i, j;
  const SQLUSMALLINT expectedRow1[]= {SQL_ROW_SUCCESS, SQL_ROW_NOROW},
    expectedRow2[][2]= { {SQL_ROW_SUCCESS, SQL_ROW_SUCCESS},
                      {SQL_ROW_SUCCESS_WITH_INFO, SQL_ROW_ERROR}
                    };
  SQLSMALLINT expectedFunction2[2]= {SQL_SUCCESS, SQL_ERROR};

  SQLCHAR res[5*2];

  diag("Test is buggy: No indicator ptr for NULL value provided");
  return SKIP;

  OK_SIMPLE_STMT(Stmt, "DROP table IF EXISTS b_row_status");

  OK_SIMPLE_STMT(Stmt, "CREATE table b_row_status(i int)");

  OK_SIMPLE_STMT(Stmt, "insert into b_row_status values(4),(2),(1),(NULL)");

  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_IMP_ROW_DESC,
                                &ird, SQL_IS_POINTER, NULL));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC,
                                &ard, SQL_IS_POINTER, NULL));

  CHECK_DESC_RC(ird, SQLSetDescField(ird, 0, SQL_DESC_ARRAY_STATUS_PTR,
                                (SQLPOINTER)arr1, SQL_IS_POINTER));
  CHECK_DESC_RC(ird, SQLSetDescField(ard, 0, SQL_DESC_ARRAY_SIZE,
                                (SQLPOINTER)2, SQL_IS_INTEGER));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "select * from b_row_status\
                                       where i=1", SQL_NTS));

  /* it has to be SQL_SUCCESS here */
  FAIL_IF(SQLExtendedFetch(Stmt, SQL_FETCH_NEXT, 1, NULL,
                                  (SQLUSMALLINT*)&arr2) != SQL_SUCCESS, "success expected");

  /*FAIL_IF(SQLFetch(Stmt), SQL_SUCCESS);*/
  for (i= 0; i<2; ++i)
  {
    diag("Row %d, Desc %d, Parameter %d", i+1, arr1[i], arr2[i]);
    is_num(expectedRow1[i], arr1[i]);
    is_num(arr1[i], arr2[i]);
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "select if(i is NULL,NULL,repeat(char(64+i),8/i))\
                                       from b_row_status\
                                       order by i desc", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, res, 5, NULL));  

  for (i= 0; i<2; ++i)
  {
    FAIL_IF(SQLFetch(Stmt) != expectedFunction2[i], "wrong return value");
    for (j= 0; j<2; ++j)
    {
      diag("Set %d Row %d, desc %d, parameter %d", i+1, j+1, arr1[j],
                    arr2[j]);
      is_num(expectedRow2[i][j], arr1[j]);
    }
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP table b_row_status");

  return OK;
}


ODBC_TEST(t_prefetch)
{
    HDBC  hdbc1;
    HSTMT hstmt1;
    SQLCHAR conn[512];

    diag("Multi statements not supported");
    return SKIP;

    sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;PREFETCH=5",
          my_dsn, my_uid, my_pwd);
    
    CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

    CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, sizeof(conn), NULL,
                                   0, NULL,
                                   SQL_DRIVER_NOPROMPT));
    CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

    OK_SIMPLE_STMT(Stmt, "DROP table IF EXISTS b_prefecth");
    OK_SIMPLE_STMT(Stmt, "CREATE table b_prefecth(i int)");

    OK_SIMPLE_STMT(Stmt, "insert into b_prefecth values(1),(2),(3),(4),(5),(6),(7)");

    CHECK_STMT_RC(hstmt1, SQLPrepare(hstmt1, "select* from b_prefecth;    ", SQL_NTS));
    CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));
    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1,SQL_DROP));

    CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

    sprintf((char *)conn+strlen(conn), ";MULTI_STATEMENTS=1");

    CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, sizeof(conn), NULL,
                                   0, NULL,
                                   SQL_DRIVER_NOPROMPT));
    CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

    CHECK_STMT_RC(hstmt1, SQLPrepare(hstmt1, "select* from b_prefecth;\
                                        select * from b_prefecth where i < 7; ",
                              SQL_NTS));

    CHECK_STMT_RC(hstmt1, SQLExecute(hstmt1));

    is_num(7, myrowcount(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLMoreResults(hstmt1));

    is_num(6, myrowcount(hstmt1));

    FAIL_IF(SQLMoreResults(hstmt1) != SQL_NO_DATA, "eof expected");

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1,SQL_DROP));
    CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
    CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

    OK_SIMPLE_STMT(Stmt, "DROP table IF EXISTS b_prefecth");

    return OK;
}


ODBC_TEST(t_outparams)
{
  SQLSMALLINT ncol, i;
  SQLINTEGER  par[3]= {10, 20, 30}, val;
  SQLLEN      len;

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS p_outparams");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE p_outparams("
                "  IN p_in INT, "
                "  OUT p_out INT, "
                "  INOUT p_inout INT) "
                "BEGIN "
                "  SELECT p_in, p_out, p_inout; "
                "  SET p_in = 100, p_out = 200, p_inout = 300; "
                "  SELECT p_inout, p_in, p_out;"
                "END");


  for (i=0; i < sizeof(par)/sizeof(SQLINTEGER); ++i)
  {
    CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, i+1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
      0, &par[i], 0, NULL));
  }

  OK_SIMPLE_STMT(Stmt, "CALL p_outparams(?, ?, ?)");

  /* rs-1 */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 3);

  is_num(my_fetch_int(Stmt, 1), 10);
  /* p_out does not have value at the moment */
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_INTEGER, &val, 0, &len));
  is_num(len, SQL_NULL_DATA);
  is_num(my_fetch_int(Stmt, 3), 30);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "eof expected");

  /* rs-2 */
  CHECK_STMT_RC(Stmt, SQLMoreResults(Stmt));

  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 3);
  
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 300);
  is_num(my_fetch_int(Stmt, 2), 100);
  is_num(my_fetch_int(Stmt, 3), 200);

  /* rs-3 out params */
  CHECK_STMT_RC(Stmt, SQLMoreResults(Stmt));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 2);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 200);
  is_num(my_fetch_int(Stmt, 2), 300);
  /* Only 1 row always */
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "eof expected");

  /* SP execution status */
  FAIL_IF(SQLMoreResults(Stmt) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE p_outparams");
  return OK;
}


/*
  Bug #11766437: Incorrect increment(increments in multiple of SQLLEN) of 
  pointer to the length/indicator buffer(last parameter of SQLBindCol), 
  which gives incorrrect result when SQL_ATTR_ROW_BIND_TYPE is set to 
  size of data inserted which is not not multiple of 8 on 64 bit 
  system where sizeof SQLLEN is 8.
  Tests for data fetched with SQL_ATTR_ROW_BIND_TYPE size set to 
  multiple of 2, binded buffers are checked for proper data fetch.
*/
ODBC_TEST(t_bug11766437)
{
  SQLULEN     rowcnt= 3;
  SQLINTEGER  i, incr;
  SQLCHAR     tbuf[50];
  char        *ptr;
  char        rows[500]= {0};
  SQLINTEGER  MAX_CHAR_SIZE= 7; /*max size for character name*/ 

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug11766437");
  OK_SIMPLE_STMT(Stmt, "create table t_bug11766437 (id int not null, "
                "name varchar(7))");
  OK_SIMPLE_STMT(Stmt, "insert into t_bug11766437 values "
                "(0, 'name0'),(1,'name1'),(2,'name2')");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)rowcnt, 0));

  /*
    With same text inserted we change binding orientation 
    to verify our changes
  */ 
  for (incr= 0; incr <= 24; incr += 2)
  {
    size_t row_size= sizeof(SQLINTEGER) + sizeof(SQLLEN) + 
              sizeof(SQLLEN) + MAX_CHAR_SIZE + incr;

    /*
      Set SQL_ATTR_ROW_BIND_TYPE to the size of the data inserted 
      with multiple of 2 increment 
    */
    CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_ROW_BIND_TYPE,
                                  (SQLPOINTER)row_size, 0));

    /*
      Binding all parameters with same buffer to test proper 
      increment of last parameter of SQLBindCol
    */
    ptr= rows;
    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, 
      (SQLPOINTER) ptr,
      (SQLLEN) sizeof(SQLINTEGER),
      (SQLLEN *) (ptr + sizeof(SQLINTEGER))));

    /*
      Incrementing pointer position by sizeof(SQLINTEGER) i.e. size of id 
      and sizeof(SQLLEN) bytes required to store length of id
    */
    ptr += sizeof(SQLINTEGER) + sizeof(SQLLEN);
    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, 
      (SQLPOINTER) ptr,
      (SQLLEN) MAX_CHAR_SIZE,
      (SQLLEN *) (ptr + MAX_CHAR_SIZE)));
   
    OK_SIMPLE_STMT(Stmt, "select id,name from t_bug11766437 order by id");

    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

    ptr= rows;
    for (i= 0; (SQLUINTEGER)i < rowcnt; ++i)
    {
      /* Verifying inserted id field */
      is_num(*((SQLINTEGER *)ptr), i);
      /* Incrementing ptr by sizeof(SQLINTEGER) i.e. size of id column */
      ptr += sizeof(SQLINTEGER);

      /* Verifying length of id field which should be sizeof(SQLINTEGER) */
      is_num(*((SQLLEN *)ptr), sizeof(SQLINTEGER));
      /* Incrementing ptr by sizeof(SQLLEN) last parameter of SQLBindCol  */
      ptr += sizeof(SQLLEN);

      sprintf((char *)tbuf, "name%d", i);
      /* Verifying inserted name field */
      IS_STR(ptr, tbuf, strlen(tbuf));
      /* Incrementing ptr by MAX_CHAR_SIZE (max size kept for name column) */
      ptr+=MAX_CHAR_SIZE;

      /* Verifying length of name field */
      is_num(*((SQLLEN *)ptr), strlen(tbuf));
      /* Incrementing ptr by sizeof(SQLLEN) last parameter of SQLBindCol  */
      ptr += sizeof(SQLLEN);

      /* Incrementing ptr by incr to test multiples of 2 */
      ptr += incr;
    }
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug11766437");

  return OK;
}


/*
  Bug ODBC-29 - simple query fails if to add leading spaces
*/
ODBC_TEST(t_odbc29)
{
  SQLSMALLINT cols_count;

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug_odbc29");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug_odbc29 (id INT PRIMARY KEY auto_increment, value VARCHAR(100) NOT NULL)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO bug_odbc29(value) VALUES ('value')");

  /* The problem was that connector trimmed extra spaces, but used initial statement length. Adding garbage at the end to verify
     that trimmed string length is calculated correctly */
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "  select * from bug_odbc29somegarbageafterendofstatement", sizeof("  select * from bug_odbc29") - 1));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &cols_count));

  is_num(cols_count, 2);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists bug_odbc29");

  return OK;
}


ODBC_TEST(t_odbc41)
{
  SQLSMALLINT cols_count;

  OK_SIMPLE_STMT(Stmt, "DROP table if exists t_odbc41");

  OK_SIMPLE_STMT(Stmt, "SELECT 1, 2, 3, 4");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &cols_count));
  is_num(cols_count, 4);
  /* Testing without SQLFreeStmt(Stmt, SQL_CLOSE). If it works - will work with it as well */
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc41 (id INT PRIMARY KEY auto_increment)");

  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &cols_count));

  is_num(cols_count, 0);

  OK_SIMPLE_STMT(Stmt, "DROP table if exists t_odbc41");

  return OK;
}


ODBC_TEST(t_odbc58)
{
  SQLLEN      len1, len2, len3;
  SQLCHAR     text_col[96001];
  SQLINTEGER  int_col;
  SQLSMALLINT smint_col;

  OK_SIMPLE_STMT(Stmt, "DROP table if exists t_odbc58");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc58 (text_col TEXT, smint_col SMALLINT, int_col INT)");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc58 VALUES('data01', 21893, 1718038908), ('data2', -25734, -1857802040)");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, (SQLPOINTER)text_col, 96000, &len1));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_SHORT, (SQLPOINTER)&smint_col, 4, &len2));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_LONG, (SQLPOINTER)&int_col,  4, &len3));

  OK_SIMPLE_STMT(Stmt, "SELECT text_col, smint_col, int_col FROM t_odbc58");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(len1, 6);
  IS_STR(text_col, "data01", len1 + 1);
  is_num(len2, 2);
  is_num(smint_col, 21893);
  is_num(len3, 4);
  is_num(int_col, 1718038908);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(len1, 5);
  IS_STR(text_col, "data2", len1 + 1);
  is_num(len2, 2);
  is_num(smint_col, -25734);
  is_num(len3, 4);
  is_num(int_col, -1857802040);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP table if exists t_odbc58");

  return OK;
}


ODBC_TEST(t_odbc77)
{
  OK_SIMPLE_STMT(Stmt, "ANALYZE TABLE non_existent");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  if (ServerNewerThan(Connection, 10, 2, 5))
  {
    OK_SIMPLE_STMT(Stmt, "ANALYZE SELECT 1");
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }
  OK_SIMPLE_STMT(Stmt, "EXPLAIN SELECT 1");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /*CHECK is not preparable */
  /*OK_SIMPLE_STMT(Stmt, "CHECK TABLE non_existent");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));*/

  /* Just to test some more exotic commands */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc77");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE odbc77(id INT NOT NULL)");
  OK_SIMPLE_STMT(Stmt, "TRUNCATE TABLE odbc77");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE odbc77");

  OK_SIMPLE_STMT(Stmt, "SELECT 1");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


ODBC_TEST(t_odbc78)
{
  SQLLEN      len;
  SQLCHAR     val[16];

  OK_SIMPLE_STMT(Stmt, "SELECT 'abc'");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, val, sizeof(val), &len));
  is_num(len, 3);
  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, val, sizeof(val), &len), SQL_NO_DATA);
  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, val, sizeof(val), &len), SQL_NO_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT 1");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_LONG, val, 0, 0));
  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_C_LONG, val, 0, 0), SQL_NO_DATA);
  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_C_LONG, val, 0, 0), SQL_NO_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


ODBC_TEST(t_odbc73)
{
  SQLSMALLINT data_type;

  OK_SIMPLE_STMT(Stmt, "DROP table if exists t_odbc73");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc73 (binvc VARCHAR(64) COLLATE utf8_bin)");

  OK_SIMPLE_STMT(Stmt, "SELECT binvc FROM t_odbc73");

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 1, NULL, 0, NULL, &data_type, NULL, NULL, NULL));

  FAIL_IF(data_type == SQL_VARBINARY || data_type == SQL_BINARY || data_type == SQL_LONGVARBINARY,
          "The field shouldn't be described as binary");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP table if exists t_odbc73");

  return OK;
}

MA_ODBC_TESTS my_tests[]=
{
  {t_bug32420, "t_bug32420"},
  {t_bug34575, "t_bug34575"},
  {t_bug24131, "t_bug24131"},
  {t_bug36069, "t_bug36069"},
  {t_bug41942, "t_bug41942"},
  {t_bug39644, "t_bug39644"},
  {t_bug32821, "t_bug32821"},
  {t_bug34271, "t_bug34271"},
  {t_bug32684, "t_bug32684"},
  {t_bug55024, "t_bug55024"},
  {t_bug56677, "t_bug56677"},
  {t_bug62657, "t_bug62657"},
  {t_row_status, "t_row_status"},
  {t_prefetch, "t_prefetch"},
  {t_outparams, "t_outparams"},
  {t_bug11766437, "t_bug11766437"},
  {t_odbc29, "t_odbc-29"},
  {t_odbc41, "t_odbc-41-nors_after_rs"},
  {t_odbc58, "t_odbc-58-numeric_after_blob"},
  {t_odbc77, "t_odbc-77-analyze_table"},
  {t_odbc78, "t_odbc-78-sql_no_data"},
  {t_odbc73, "t_odbc-73-bin_collation"},
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
