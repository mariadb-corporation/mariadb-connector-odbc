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

SQLRETURN rc;

/* Basic prepared statements - binary protocol test */
ODBC_TEST(t_prep_basic)
{
  SQLINTEGER id;
  SQLLEN length1, length2, pcrow;
  char       name[20];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_basic");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_prep_basic(a INT, b CHAR(4))");

  CHECK_STMT_RC(Stmt,
          SQLPrepare(Stmt,
                     (SQLCHAR *)"INSERT INTO t_prep_basic VALUES(?,'venu')",
                     SQL_NTS));

    rc = SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id, 0, NULL);
    mystmt(Stmt,rc);

    id = 100;
    rc = SQLExecute(Stmt);
    mystmt(Stmt,rc);

    rc = SQLRowCount(Stmt, &pcrow);
    mystmt(Stmt,rc);

    diag( "affected rows: %ld\n", pcrow);
    IS(pcrow == 1);

    SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt,SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_prep_basic");

    rc = SQLBindCol(Stmt, 1, SQL_C_LONG, &id, 0, &length1);
    mystmt(Stmt,rc);

    rc = SQLBindCol(Stmt, 2, SQL_C_CHAR, name, 5, &length2);
    mystmt(Stmt,rc);

    id = 0;
    rc = SQLFetch(Stmt);
    mystmt(Stmt,rc);

    diag( "outdata: %d(%d), %s(%d)\n",id,length1,name,length2);
    IS(id == 100 && length1 == sizeof(SQLINTEGER));
    IS(strcmp(name,"venu")==0 && length2 == 4);

    rc = SQLFetch(Stmt);
    IS(rc == SQL_NO_DATA_FOUND);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_basic");

  return OK;
}


/* to test buffer length */
ODBC_TEST(t_prep_buffer_length)
{
  SQLLEN length;
  SQLCHAR buffer[20];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_buffer_length");

  OK_SIMPLE_STMT(Stmt, "create table t_prep_buffer_length(a varchar(20))");

  CHECK_STMT_RC(Stmt,
          SQLPrepare(Stmt,
                     (SQLCHAR *)"insert into t_prep_buffer_length values(?)",
                     SQL_NTS));

  length= 0;
  strcpy((char *)buffer, "abcdefghij");

  rc = SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 15, 10, buffer, 4, &length);
  mystmt(Stmt,rc);

  rc = SQLExecute(Stmt);
  mystmt(Stmt,rc);

  length= 3;

  rc = SQLExecute(Stmt);
  mystmt(Stmt,rc);

  length= 10;    

  rc = SQLExecute(Stmt);
  mystmt(Stmt,rc);

  length= 9;    

  rc = SQLExecute(Stmt);
  mystmt(Stmt,rc);

  length= SQL_NTS;

  rc = SQLExecute(Stmt);
  mystmt(Stmt,rc);

  SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
  SQLFreeStmt(Stmt,SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "select * from t_prep_buffer_length");

  rc = SQLBindCol(Stmt, 1, SQL_C_CHAR, buffer, 15, &length);
  mystmt(Stmt,rc);

  rc = SQLFetch(Stmt);
  mystmt(Stmt,rc);

  diag( "outdata: %s (%ld)\n", buffer, length);
  is_num(length, 0);
  is_num(buffer[0], '\0');

  rc = SQLFetch(Stmt);
  mystmt(Stmt,rc);

  diag("outdata: %s (%ld)\n", buffer, length);
  is_num(length, 3);
  IS_STR(buffer, "abc", 10);

  rc = SQLFetch(Stmt);
  mystmt(Stmt,rc);

  diag("outdata: %s (%ld)\n", buffer, length);
  is_num(length, 10);
  IS_STR(buffer, "abcdefghij", 10);

  rc = SQLFetch(Stmt);
  mystmt(Stmt,rc);

  diag("outdata: %s (%ld)\n", buffer, length);
  is_num(length, 9);
  IS_STR(buffer, "abcdefghi", 9);

  rc = SQLFetch(Stmt);
  mystmt(Stmt,rc);

  diag("outdata: %s (%ld)\n", buffer, length);
  is_num(length, 10);
  IS_STR(buffer, "abcdefghij", 10);

  rc = SQLFetch(Stmt);
  IS(rc == SQL_NO_DATA_FOUND);

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  mystmt(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  mystmt(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_buffer_length");

  return OK;
}


/* For data truncation */
ODBC_TEST(t_prep_truncate)
{
    SQLLEN length, length1, pcrow;
    SQLCHAR    name[20], bin[10];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_truncate");

  OK_SIMPLE_STMT(Stmt, "create table t_prep_truncate(a int, b char(4), c binary(4))");

  CHECK_STMT_RC(Stmt,
          SQLPrepare(Stmt,
                     (SQLCHAR *)"insert into t_prep_truncate "
                     "values(500,'venu','venu')", SQL_NTS));

    strcpy((char *)name,"venu");
    rc = SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 10, 10, name, 5, NULL);
    mystmt(Stmt,rc);

    rc = SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY,SQL_BINARY, 10, 10, name, 5, NULL);
    mystmt(Stmt,rc);

    rc = SQLExecute(Stmt);
    mystmt(Stmt,rc);

    rc = SQLRowCount(Stmt, &pcrow);
    mystmt(Stmt,rc);

    diag( "affected rows: %ld\n", pcrow);
    IS(pcrow == 1);

    SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt,SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "select b,c from t_prep_truncate");

    rc = SQLBindCol(Stmt, 1, SQL_C_CHAR, name, 2, &length);
    mystmt(Stmt,rc);

    rc = SQLBindCol(Stmt, 2, SQL_C_BINARY, bin, 4, &length1);
    mystmt(Stmt,rc);

    rc = SQLFetch(Stmt);
    mystmt(Stmt,rc);

    diag("str outdata: %s(%d)\n",name,length);
    is_num(length, 4);
    IS_STR(bin, "v", 1);

    bin[4]='M';
    diag("bin outdata: %s(%d)\n",bin,length1);
    is_num(length, 4);
    IS_STR(bin, "venuM", 5);

    rc = SQLFetch(Stmt);
    IS(rc == SQL_NO_DATA_FOUND);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_truncate");

  return OK;
}


/* For scrolling */
ODBC_TEST(t_prep_scroll)
{
  SQLINTEGER i, data, max_rows= 5;

  if (ForwardOnly == TRUE)
  {
    skip("This test cannot be run with FORWARDONLY option selected");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_scroll");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_prep_scroll (a TINYINT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_prep_scroll VALUES (1),(2),(3),(4),(5)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_prep_scroll");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &data, 0, NULL));

  for (i= 1; ; i++)
  {
    SQLRETURN rc= SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 0);
    if (rc == SQL_NO_DATA)
        break;

    is_num(i, data);
  }

  is_num(i, max_rows + 1);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_ABSOLUTE, 3));
  is_num(data, 3);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_PREV, 3));
  is_num(data, 2);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 3));
  is_num(data, 1);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_PREV, 3) != SQL_NO_DATA, "eof expected");

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -2) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt,  SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 2));
  is_num(data, 2);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_LAST, 3));
  is_num(data, 5);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -2));
  is_num(data, 3);

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, 3) != SQL_NO_DATA, "eof expected");

  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 3) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_RELATIVE, -2));
  is_num(data, 4);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_scroll");

  return OK;
}


/* For SQLGetData */
ODBC_TEST(t_prep_getdata)
{
    SQLCHAR    name[10];
    SQLINTEGER data;
    SQLLEN     length;
    SQLCHAR    tiny;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_getdata");

  OK_SIMPLE_STMT(Stmt, "create table t_prep_getdata(a tinyint, b int, c char(4))");

  CHECK_STMT_RC(Stmt,
          SQLPrepare(Stmt,
                     (SQLCHAR *)"insert into t_prep_getdata values(?,?,?)",
                     SQL_NTS));

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_TINYINT,
                          0,0,&data,0,NULL);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,
                          0,0,&data,0,NULL);

    rc = SQLBindParameter(Stmt,3,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_CHAR,
                          10,10,name,6,NULL);
    mystmt(Stmt,rc);

    sprintf((char *)name,"venu"); data = 10;

    rc = SQLExecute(Stmt);
    mystmt(Stmt,rc);

    rc = SQLExecute(Stmt);
    mystmt(Stmt,rc);

    SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt,SQL_CLOSE);

    data= 0;
    OK_SIMPLE_STMT(Stmt, "select * from t_prep_getdata");
    mystmt(Stmt,rc);

    rc = SQLBindCol(Stmt, 1,SQL_C_TINYINT, &tiny, 0, NULL);
    mystmt(Stmt, rc);

    rc = SQLFetch(Stmt);
    mystmt(Stmt, rc);

    rc = SQLFetch(Stmt);
    mystmt(Stmt, rc);

    diag("record 1 : %d\n", tiny);
    IS( tiny == 10);

    rc = SQLGetData(Stmt,2,SQL_C_LONG,&data,0,NULL);
    mystmt(Stmt,rc);

    diag("record 2 : %ld\n", data);
    IS( data == 10);

    name[0]= '\0';
    rc = SQLGetData(Stmt,3,SQL_C_CHAR,name,5,&length);
    mystmt(Stmt,rc);

    diag("record 3 : %s(%ld)\n", name, (long)length);

    is_num(length, 4);
    IS_STR(name, "venu", 4);

    data = 0;
    rc = SQLGetData(Stmt,1,SQL_C_LONG,&data,0,NULL);
    mystmt(Stmt,rc);

    diag("record 1 : %ld", data);
    IS( data == 10);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_getdata");

  return OK;
}


/* For SQLGetData in truncation */
ODBC_TEST(t_prep_getdata1)
{
    SQLCHAR     data[11];
    SQLLEN length;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_getdata");

  OK_SIMPLE_STMT(Stmt, "create table t_prep_getdata(a char(10), b int)");

  OK_SIMPLE_STMT(Stmt, "insert into t_prep_getdata values('abcdefghij',12345)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "select * from t_prep_getdata");

    rc = SQLFetch(Stmt);
    mystmt(Stmt, rc);

    data[0]= 'M'; data[1]= '\0';
    rc = SQLGetData(Stmt,1,SQL_C_CHAR,data,0,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 10);
    IS_STR(data, "M", 1);

    rc = SQLGetData(Stmt,1,SQL_C_CHAR,data,4,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 10);
    IS_STR(data, "abc", 3);

    rc = SQLGetData(Stmt,1,SQL_C_CHAR,data,4,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 7);
    IS_STR(data, "def", 3);

    rc = SQLGetData(Stmt,1,SQL_C_CHAR,data,4,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 4);
    IS_STR(data, "ghi", 3);

    data[0]= 'M';
    rc = SQLGetData(Stmt,1,SQL_C_CHAR,data,0,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 1);
    IS_STR(data, "M", 1);

    rc = SQLGetData(Stmt,1,SQL_C_CHAR,data,1,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 1);
    is_num(data[0], '\0');

    rc = SQLGetData(Stmt,1,SQL_C_CHAR,data,2,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 1);
    IS_STR(data, "j", 1);

    rc = SQLGetData(Stmt,1,SQL_C_CHAR,data,2,&length);
    IS(rc == SQL_NO_DATA);

    data[0]= 'M'; data[1]= '\0';
    rc = SQLGetData(Stmt,2,SQL_C_CHAR,data,0,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 5);
    IS_STR(data, "M", 2);

    rc = SQLGetData(Stmt,2,SQL_C_CHAR,data,3,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 5);
    IS_STR(data, "12", 2);

    rc = SQLGetData(Stmt,2,SQL_C_CHAR,data,2,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 3);
    IS_STR(data, "3", 1);

    rc = SQLGetData(Stmt,2,SQL_C_CHAR,data,2,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 2);
    IS_STR(data, "4", 1);

    data[0]= 'M';
    rc = SQLGetData(Stmt,2,SQL_C_CHAR,data,0,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 1);
    IS_STR(data, "M", 1);

    rc = SQLGetData(Stmt,2,SQL_C_CHAR,data,1,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    IS(data[0] == '\0' && length == 1);
    is_num(length, 1);
    is_num(data[0], '\0');

    rc = SQLGetData(Stmt,2,SQL_C_CHAR,data,2,&length);
    mystmt(Stmt,rc);

    diag("data: %s (%ld)\n", data, length);
    is_num(length, 1);
    IS_STR(data, "5", 1);

    rc = SQLGetData(Stmt,2,SQL_C_CHAR,data,2,&length);
    IS(rc == SQL_NO_DATA);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_getdata");

  return OK;
}


ODBC_TEST(t_prep_catalog)
{
    SQLCHAR     table[20];
    SQLLEN      length;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_catalog");

  OK_SIMPLE_STMT(Stmt, "create table t_prep_catalog(a int default 100)");

    rc = SQLTables(Stmt,NULL,0,NULL,0,(SQLCHAR *)"t_prep_catalog",14,
                   (SQLCHAR *)"BASE TABLE",10);
    mystmt(Stmt,rc);

    rc = SQLFetch(Stmt);
    mystmt(Stmt,rc);

    rc = SQLGetData(Stmt,3,SQL_C_CHAR,table,0,&length);
    mystmt(Stmt,rc);
    IS(length == 14);

    rc = SQLGetData(Stmt,3,SQL_C_CHAR,table,15,&length);
    mystmt(Stmt,rc);
    is_num(length, 14);
    IS_STR(table, "t_prep_catalog", 14);

    rc = SQLFetch(Stmt);
    IS(rc == SQL_NO_DATA);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

    rc = SQLColumns(Stmt,NULL,0,NULL,0,(SQLCHAR *)"t_prep_catalog",14,NULL,0);
    mystmt(Stmt,rc);

    rc = SQLFetch(Stmt);
    mystmt(Stmt,rc);

    rc = SQLGetData(Stmt,3,SQL_C_CHAR,table,15,&length);
    mystmt(Stmt,rc);
    is_num(length, 14);
    IS_STR(table, "t_prep_catalog", 14);

    rc = SQLGetData(Stmt,4,SQL_C_CHAR,table,0,&length);
    mystmt(Stmt,rc);
    is_num(length, 1);

    rc = SQLGetData(Stmt,4,SQL_C_CHAR,table,2,&length);
    mystmt(Stmt,rc);
    is_num(length, 1);
    IS_STR(table, "a", 1);

    rc = SQLGetData(Stmt,13,SQL_C_CHAR,table,10,&length);
    mystmt(Stmt,rc);
    diag("table: %s(%d)\n", table, length);
    is_num(length, 3);
    IS_STR(table, "100", 3);

    rc = SQLFetch(Stmt);
    IS(rc == SQL_NO_DATA);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prep_catalog");

  return OK;
}


ODBC_TEST(t_sps)
{
    SQLINTEGER a, a1;
    SQLLEN length, length1;
    char b[]= "abcdefghij", b1[10];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_tabsp");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS t_sp");

  OK_SIMPLE_STMT(Stmt, "create table t_tabsp(a int, b varchar(10))");

  OK_SIMPLE_STMT(Stmt,"create procedure t_sp(x int, y char(10)) "
         "begin insert into t_tabsp values(x, y); end;");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"call t_sp(?,?)", SQL_NTS));

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,
                          0,0,&a,0,NULL);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_CHAR,
                          0,0,b,0,&length);


    for (length= 0, a= 0; a < 10; a++, length++)
    {
        rc = SQLExecute(Stmt);
        mystmt(Stmt, rc);
    }

    SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "select * from t_tabsp");

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&a,0,NULL);
    mystmt(Stmt,rc);

    rc = SQLBindCol(Stmt,2,SQL_C_CHAR,b1,11,&length);
    mystmt(Stmt,rc);

    for (length1= 0, a1= 0; a1 < 10; a1++, length1++)
    {
        rc = SQLFetch(Stmt);
        mystmt(Stmt, rc);

        diag( "data: %d, %s(%d)\n", a, b1, length);
        IS( a == a1);
        IS(strncmp(b1,b,length1) == 0 && length1 == length);
    }

    SQLFreeStmt(Stmt, SQL_UNBIND);
    SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS t_sp");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_tabsp");

  return OK;
}


ODBC_TEST(t_prepare)
{
  SQLRETURN rc;
  SQLINTEGER nidata= 200, nodata;
  SQLLEN    nlen;
  char      szodata[20],szidata[20]="MySQL";
  SQLSMALLINT pccol;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prepare");

    rc = SQLExecDirect(Stmt,"create table t_prepare(col1 int primary key, col2 varchar(30), col3 set('one', 'two'))", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"insert into t_prepare values(100,'venu','one')", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"insert into t_prepare values(200,'MySQL','two')", SQL_NTS);
    mystmt(Stmt,rc);

    CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"select * from t_prepare "
                              "where col2 = ? AND col1 = ?",SQL_NTS));

    rc = SQLNumResultCols(Stmt,&pccol);
    mystmt(Stmt,rc);
    is_num(pccol, 3);

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nodata,0,&nlen);
    mystmt(Stmt,rc);

    rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szodata,200,&nlen);
    mystmt(Stmt,rc);

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT, SQL_C_CHAR,SQL_VARCHAR,
                          0,0,szidata,20,&nlen);
    mystmt(Stmt,rc);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT, SQL_C_LONG,SQL_INTEGER,
                          0,0,&nidata,20,NULL);
    mystmt(Stmt,rc);

    nlen= strlen(szidata);
    rc = SQLExecute(Stmt);
    mystmt(Stmt,rc);

    rc = SQLFetch(Stmt);
    mystmt(Stmt,rc);

    fprintf(stdout," outdata: %d, %s(%lld)\n", nodata, szodata, (unsigned long long)nlen);
    IS(nodata == 200);

    rc = SQLFetch(Stmt);
    IS(rc == SQL_NO_DATA_FOUND);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    mystmt(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prepare");

  return OK;
}


ODBC_TEST(t_prepare1)
{
  SQLRETURN rc;
  SQLINTEGER nidata= 1000;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prepare1");

    rc = SQLExecDirect(Stmt,"create table t_prepare1(col1 int primary key, col2 varchar(30))", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"insert into t_prepare1 values(100,'venu')", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"insert into t_prepare1 values(200,'MySQL')", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

    rc = SQLPrepare(Stmt,"insert into t_prepare1(col1) values(?)", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT, SQL_C_LONG,SQL_INTEGER,
                          0,0,&nidata,0,NULL);
    mystmt(Stmt,rc);

    rc = SQLExecute(Stmt);
    mystmt(Stmt,rc);

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    mystmt(Stmt,rc);

    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_prepare1");

    IS(3 == myrowcount(Stmt));/* unless prepare is supported..*/

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_prepare1");

  return OK;
}


ODBC_TEST(tmysql_bindcol)
{
    SQLRETURN rc;
    SQLINTEGER nodata, nidata = 200;
    SQLLEN     nlen;
    SQLCHAR   szodata[20],szidata[20]="MySQL";

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_bindcol");

    rc = SQLExecDirect(Stmt,"create table tmysql_bindcol(col1 int primary key, col2 varchar(30))", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"insert into tmysql_bindcol values(100,'venu')", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"insert into tmysql_bindcol values(200,'MySQL')", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

    rc = SQLPrepare(Stmt,"select * from tmysql_bindcol where col2 = ? AND col1 = ?", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nodata,0,&nlen);
    mystmt(Stmt,rc);

    rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szodata,200,&nlen);
    mystmt(Stmt,rc);

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT, SQL_C_CHAR,SQL_VARCHAR,
                          0,0,szidata,5,NULL);
    mystmt(Stmt,rc);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT, SQL_C_LONG,SQL_INTEGER,
                          0,0,&nidata,20,NULL);
    mystmt(Stmt,rc);

    rc = SQLExecute(Stmt);
    mystmt(Stmt,rc);

    rc = SQLFetch(Stmt);
    mystmt(Stmt,rc);

    diag(" outdata: %d, %s(%d)\n", nodata,szodata,nlen);
    IS(nodata == 200);

    rc = SQLFetch(Stmt);

    IS(rc == SQL_NO_DATA_FOUND);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    mystmt(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_bindcol");

  return OK;
}


ODBC_TEST(tmysql_bindparam)
{
    SQLRETURN rc;
    SQLINTEGER nodata, nidata= 200;
    SQLLEN    nlen;
    SQLCHAR   szodata[20],szidata[20]="MySQL";
    SQLSMALLINT pccol;

    SQLExecDirect(Stmt,"drop table tmysql_bindparam", SQL_NTS);

    rc = SQLExecDirect(Stmt,"create table tmysql_bindparam(col1 int primary key, col2 varchar(30))", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"insert into tmysql_bindparam values(100,'venu')", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"insert into tmysql_bindparam values(200,'MySQL')", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

    rc = SQLPrepare(Stmt,"select * from tmysql_bindparam where col2 = ? AND col1 = ?", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLNumResultCols(Stmt,&pccol);
    mystmt(Stmt,rc);

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&nodata,0,&nlen);
    mystmt(Stmt,rc);

    rc = SQLBindCol(Stmt,2,SQL_C_CHAR,szodata,200,&nlen);
    mystmt(Stmt,rc);

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT, SQL_C_CHAR,SQL_VARCHAR,
                          0,0,szidata,5,NULL);
    mystmt(Stmt,rc);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT, SQL_C_LONG,SQL_INTEGER,
                          0,0,&nidata,20,NULL);
    mystmt(Stmt,rc);

    rc = SQLExecute(Stmt);
    mystmt(Stmt,rc);

    rc = SQLFetch(Stmt);
    mystmt(Stmt,rc);

    diag(" outdata: %d, %s(%d)\n", nodata,szodata,nlen);
    IS(nodata == 200);

    rc = SQLFetch(Stmt);

    IS(rc == SQL_NO_DATA_FOUND);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
    mystmt(Stmt,rc);

    rc = SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"drop table tmysql_bindparam", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

  return OK;
}


ODBC_TEST(t_acc_update)
{
    SQLRETURN rc;
    SQLINTEGER id,id1;
    SQLLEN pcrow;
    SQLHSTMT hstmt1;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_acc_update");

    rc = SQLExecDirect(Stmt,"create table t_acc_update(id int)", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLExecDirect(Stmt,"insert into t_acc_update values(1)", SQL_NTS);
    mystmt(Stmt,rc);
    rc = SQLExecDirect(Stmt,"insert into t_acc_update values(2)", SQL_NTS);
    mystmt(Stmt,rc);

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);

    mystmt(Stmt,rc);

    CHECK_STMT_RC(Stmt,
            SQLPrepare(Stmt,
                       (SQLCHAR *)"select id from t_acc_update where id = ?",
                       SQL_NTS));

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_DEFAULT,SQL_INTEGER,11,0,&id,0,NULL);
    mystmt(Stmt,rc);

    id = 2;
    rc = SQLExecute(Stmt);
    mystmt(Stmt,rc);

    rc = SQLFetch(Stmt);
    mystmt(Stmt,rc);

    rc = SQLGetData(Stmt,1,SQL_C_LONG,&id1,512,NULL);
    mystmt(Stmt,rc);
    diag("outdata:%d\n",id1);

    SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt,SQL_UNBIND);
    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    mystmt(Stmt,rc);


    rc = SQLSetConnectOption(Connection,SQL_AUTOCOMMIT,0L);
    CHECK_DBC_RC(Connection,rc);

    rc = SQLAllocStmt(Connection,&hstmt1);
    CHECK_DBC_RC(Connection,rc);

    id = 2;
    id1=2;
    rc = SQLBindParameter(hstmt1,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,10,0,&id,0,NULL);
    mystmt(hstmt1,rc);

    rc = SQLBindParameter(hstmt1,2,SQL_PARAM_INPUT,SQL_C_DEFAULT,SQL_INTEGER,11,0,&id1,0,NULL);
    mystmt(hstmt1,rc);

    OK_SIMPLE_STMT(hstmt1, "UPDATE t_acc_update SET id = ?  WHERE id = ?");

    rc = SQLRowCount(hstmt1,&pcrow);
    mystmt(hstmt1,rc);
    diag("rows affected:%d\n",pcrow);

    SQLFreeStmt(hstmt1,SQL_RESET_PARAMS);
    rc = SQLFreeStmt(hstmt1,SQL_DROP);
    mystmt(hstmt1,rc);

    rc = SQLTransact(NULL,Connection,0);
    CHECK_DBC_RC(Connection,rc);

    rc = SQLSetConnectOption(Connection,SQL_AUTOCOMMIT,1L);
    CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_acc_update");

  return OK;
}


/**
  Bug #29871: MyODBC problem with MS Query ('Memory allocation error')
*/
ODBC_TEST(t_bug29871)
{
  SQLCHAR *param= (SQLCHAR *)"1";

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug29871");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug29871 (a INT)");

  /* The bug is related to calling deprecated SQLSetParam */
  CHECK_STMT_RC(Stmt, SQLSetParam(Stmt, 1, SQL_C_CHAR, SQL_INTEGER, 10, 0,
                             param, 0));
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug29871 VALUES (?)");
  CHECK_STMT_RC(Stmt, SQLSetParam(Stmt, 1, SQL_C_CHAR, SQL_INTEGER, 10, 0,
                             param, 0));
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug29871 WHERE a=?");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug29871");
  return OK;
}


/**
  Bug #67340: Memory leak in 5.2.2(w) ODBC driver
              causes Prepared_stmt_count to grow
*/
ODBC_TEST(t_bug67340)
{
  SQLCHAR *param= (SQLCHAR *)"1";
  SQLCHAR     data[255]= "abcdefg";
  SQLLEN paramlen= 7;
  int i, stmt_count= 0;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug67340");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug67340(id INT AUTO_INCREMENT PRIMARY KEY,"\
                "vc VARCHAR(32))");

  /* get the initial numnber of Prepared_stmt_count */
  OK_SIMPLE_STMT(Stmt, "SHOW STATUS LIKE 'Prepared_stmt_count'");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  stmt_count= my_fetch_int(Stmt, 2);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  for(i=0; i < 100; i++)
  {
    CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "INSERT INTO t_bug67340(id, vc) "\
                                     "VALUES (NULL, ?)", SQL_NTS));
    CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                    SQL_CHAR, 0, 0, data, sizeof(data), 
                                    &paramlen));
    CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
  }

  /* get the new numnber of Prepared_stmt_count */
  OK_SIMPLE_STMT(Stmt, "SHOW STATUS LIKE 'Prepared_stmt_count'");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  /* check how much Prepared_stmt_count has increased */
  FAIL_IF((my_fetch_int(Stmt, 2) - stmt_count > 1), "comparison failed");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug67340");
  return OK;
}


/**
  Bug #67702: Problem with BIT columns in MS Access
*/
ODBC_TEST(t_bug67702)
{
  SQLCHAR data1[5]= "abcd";
  char c1 = 1, c2= 0;
  int id= 1;

  SQLLEN paramlen= 0;

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "drop table if exists bug67702", 
                                      SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "create table bug67702"\
                                      "(id int auto_increment primary key,"\
                                      "vc varchar(32), yesno bit(1))", 
                                      SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "INSERT INTO bug67702(id, vc, yesno)"\
                                      "VALUES (1, 'abcd', 1)", 
                                      SQL_NTS));

  /* Set parameter values here to make it clearer where each one goes */
  c1= 0;
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_BIT, 
                                  SQL_BIT, 1, 0, &c1, 0, NULL));

  id= 1;
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, sizeof(id), 0, &id, 0, NULL));

  paramlen= 4;
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_VARCHAR, 4, 0, data1, 0, &paramlen));

  c2= 1;
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 4, SQL_PARAM_INPUT, SQL_C_BIT,
                                  SQL_BIT, 1, 0, &c2, 0, NULL));

  /* The prepared query looks exactly as MS Access does it */
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "UPDATE `bug67702` SET `yesno`=?  "\
                                      "WHERE `id` = ? AND `vc` = ? AND "\
                                      "`yesno` = ?", SQL_NTS));
  SQLFreeStmt(Stmt, SQL_CLOSE);

  /* Now check the result of update the bit field should be set to 0 */
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "SELECT `yesno` FROM `bug67702`", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  FAIL_IF(my_fetch_int(Stmt, 1) != 0, "comparison failed");
  
  SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "drop table if exists bug67702", SQL_NTS));
  return OK;
}

/**
ODBC-57: MS Access adds parenthesis around each "SELECT" in a "UNION" query. Connector had problems preparing it.
But in general any (returning result) query in parenthesis caused the same problem
*/
ODBC_TEST(t_odbc57)
{
  int value= 0;

  OK_SIMPLE_STMT(Stmt, "(SELECT 1)");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_INTEGER, &value, 0, NULL));
  
  is_num(value, 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  return OK;
}


ODBC_TEST(t_odbc141)
{
  SQLCHAR SQLState[6];
  SQLINTEGER NativeError;
  SQLCHAR SQLMessage[SQL_MAX_MESSAGE_LENGTH];
  SQLSMALLINT TextLengthPtr;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc141");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE odbc141(id int not null)");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "LOAD DATA INFILE '/nonexistent/non_existent' INTO TABLE odbc141\
                                        CHARACTER SET latin1 FIELDS TERMINATED BY ';'\
                                        ENCLOSED BY '\"' LINES TERMINATED BY '\n'", SQL_NTS));

  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_ERROR);

  SQLGetDiagRec(SQL_HANDLE_STMT, Stmt, 1, SQLState, &NativeError, SQLMessage, SQL_MAX_MESSAGE_LENGTH, &TextLengthPtr);
  diag("%s(%d) %s", SQLState, NativeError, SQLMessage);

  if (NativeError == 1045)
  {
    skip("Test user doesn't have enough privileges to run this test");
  }
  FAIL_IF(NativeError!=29 && NativeError != 13, "Expected 13 or 29 native error"); /* File not found or No such file or directory... */

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


ODBC_TEST(t_odbc269)
{
  unsigned int i;

  /* Not sure when "BEGIN NOT ATOMIC" was introduced, but it's not supported in 5.5, and 10.0 is already not supported */
  if (ServerNotOlderThan(Connection, 10, 1, 0) == FALSE)
  {
    skip("The test requires min 10.5.0 version");
  }

  OK_SIMPLE_STMT(Stmt, "BEGIN NOT ATOMIC SET @SOME_ODBC267= 'someinfo';  SELECT 1, @SOME_ODBC267; SELECT 127, 'value',2020; END");

  for (i= 0; i < 2; ++i)
  {
    IS(my_print_non_format_result_ex(Stmt, FALSE) == 1);
    CHECK_STMT_RC(Stmt, SQLMoreResults(Stmt)/* == SQL_SUCCESS*/);
  }

  EXPECT_STMT(Stmt, SQLMoreResults(Stmt), SQL_NO_DATA_FOUND);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "BEGIN NOT ATOMIC SET @SOME_ODBC267= 'someinfo';  SELECT 1, @SOME_ODBC267; SELECT 127, 'value',2020; END", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  for (i= 0; i < 2; ++i)
  {
    IS(my_print_non_format_result_ex(Stmt, FALSE) == 1);
    CHECK_STMT_RC(Stmt, SQLMoreResults(Stmt)/* == SQL_SUCCESS*/);
  }

  EXPECT_STMT(Stmt, SQLMoreResults(Stmt), SQL_NO_DATA_FOUND);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


ODBC_TEST(t_mdev16708)
{
  SQLCHAR query[5/*USE */+ MAX_NAME_LEN];

  /*  */
  if (ServerNotOlderThan(Connection, 10, 6, 0) == FALSE)
  {
    skip("The test requires min 10.6.0 version");
  }
  _snprintf(query, sizeof(query), "USE %s", my_schema);
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, query, SQL_NTS));
  /* USE in particular won't support parameters */
  /* CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, my_schema, strlen(dbname), NULL)); */
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {t_prep_basic, "t_prep_basic"},
  {t_prep_buffer_length, "t_prep_buffer_length"},
  {t_prep_truncate, "t_prep_truncate"},
  {t_prep_scroll, "t_prep_scroll"},
  {t_prep_getdata, "t_prep_getdata"},
  {t_prep_getdata1, "t_prep_getdata1"},
  {t_prep_catalog, "t_prep_catalog"},
  {t_sps, "t_sps"},
  {t_prepare, "t_prepare"},
  {t_prepare1, "t_prepare1"},
  {tmysql_bindcol, "tmysql_bindcol"},
  {tmysql_bindparam, "tmysql_bindparam"},
  {t_acc_update, "t_acc_update"},
  {t_bug29871, "t_bug29871"},
  {t_bug67340, "t_bug67340"},
  {t_bug67702, "t_bug67702"},
  {t_odbc57, "odbc-57-query_in_parenthesis"},
  {t_odbc141, "odbc-141-load_data_infile"},
  {t_odbc269, "odbc-269-begin_not_atomic"},
  {t_mdev16708, "t_mdev16708-new_supported_statements"},
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
