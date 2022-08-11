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

ODBC_TEST(my_ts)
{
  SQLCHAR          szTs[50];
  TIMESTAMP_STRUCT ts;
  SQLLEN           len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_ts");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_ts (ts TIMESTAMP)");

  /* insert using SQL_C_CHAR to SQL_TIMESTAMP */
  strcpy((char *)szTs, "2002-01-07 10:20:49.06");
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT,
                                  SQL_C_CHAR, SQL_TIMESTAMP,
                                  0, 0, szTs, sizeof(szTs), NULL));
  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_ts (ts) VALUES (?)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_RESET_PARAMS));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0));

  /* insert using SQL_C_TIMESTAMP to SQL_TIMESTAMP */
  ts.year= 2002;
  ts.month= 1;
  ts.day= 7;
  ts.hour= 19;
  ts.minute= 47;
  ts.second= 59;
  ts.fraction= 4;
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT,
                                  SQL_C_TIMESTAMP, SQL_TIMESTAMP,
                                  0, 0, &ts, sizeof(ts), NULL));

  OK_SIMPLE_STMT(Stmt, "INSERT INTO my_ts (ts) VALUES (?)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_RESET_PARAMS));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  /* now fetch and verify the results .. */
  OK_SIMPLE_STMT(Stmt, "SELECT ts, ts as ts2 FROM my_ts");

  /* now fetch first row */
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, szTs, sizeof(szTs), &len));
  IS_STR(szTs, "2002-01-07 10:20:49", len);
  diag("# row1 using SQL_C_CHAR: %s (%ld)\n", szTs, len);

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_TIMESTAMP, &ts, sizeof(ts), &len));
  is_num(ts.year,  2002);
  is_num(ts.month, 1);
  is_num(ts.day,   7);
  is_num(ts.hour,  10);
  is_num(ts.minute,20);
  is_num(ts.second,49);
  is_num(len, sizeof(SQL_TIMESTAMP_STRUCT));
  diag("# row1 using SQL_C_TIMESTAMP: %d-%d-%d %d:%d:%d.%d (%ld)\n",
         ts.year, ts.month,ts.day, ts.hour, ts.minute, ts.second, ts.fraction,
         len);

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, szTs, sizeof(szTs), &len));
  IS_STR(szTs, "2002-01-07 19:47:59", len);
  diag("# row2 using SQL_C_CHAR: %s(%ld)\n", szTs, len);

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_TIMESTAMP, &ts, sizeof(ts), &len));
  is_num(ts.year,  2002);
  is_num(ts.month, 1);
  is_num(ts.day,   7);
  is_num(ts.hour,  19);
  is_num(ts.minute,47);
  is_num(ts.second,59);
  diag("# row2 using SQL_C_TIMESTAMP: %d-%d-%d %d:%d:%d.%d (%ld)\n",
         ts.year, ts.month,ts.day, ts.hour, ts.minute, ts.second, ts.fraction,
         len);


  FAIL_IF(SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_UNBIND));
  CHECK_STMT_RC(Stmt,  SQLFreeStmt(Stmt,SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_ts");

  /* Test of 2-digits year(YYMMDD) format */
  OK_SIMPLE_STMT(Stmt, "select cast('910825' as date)"); 
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_TIMESTAMP, &ts, sizeof(ts), &len));

  is_num(1991, ts.year);
  is_num(8, ts.month);
  is_num(25, ts.day);

  return OK;
}


ODBC_TEST(t_tstotime)
{
  SQLRETURN rc;
  SQL_TIMESTAMP_STRUCT ts, ts1, ts2;

  ts.day    = 02;
  ts.month  = 8;
  ts.year   = 2001;
  ts.hour   = 18;
  ts.minute = 20;
  ts.second = 45;
  ts.fraction = 05;   

  memcpy(&ts1, (void*) &ts, sizeof(SQL_TIMESTAMP_STRUCT));
  /* For SQL_TIME fraction is truncated and that would cause error */
  ts1.fraction= 0;

  memcpy(&ts2, (void*) &ts1, sizeof(SQL_TIMESTAMP_STRUCT));
  /* Same for SQL_DATE - time is truncated -> error */
  ts2.hour= ts2.minute= ts2.second= 0;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_tstotime");

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLExecDirect(Stmt,"create table t_tstotime(col1 date, col2 time, col3 timestamp)", SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  /* TIMESTAMP TO DATE, TIME and TS CONVERSION */
  rc = SQLPrepare(Stmt, (SQLCHAR *)"insert into t_tstotime(col1, col2, col3) values(?,?,?)",SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);   

  rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_TIMESTAMP,
                        SQL_DATE,0,0,&ts2,sizeof(ts2),NULL);

  CHECK_STMT_RC(Stmt,rc);

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt,2,SQL_PARAM_INPUT,SQL_C_TIMESTAMP,
                        SQL_TIME,0,0,&ts1,sizeof(ts1),NULL));

  rc = SQLBindParameter(Stmt,3,SQL_PARAM_INPUT,SQL_C_TIMESTAMP,
                        SQL_TIMESTAMP,0,0,&ts,sizeof(ts),NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
  CHECK_STMT_RC(Stmt,rc);  

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);  

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "select * from t_tstotime");  

  IS( 1 == myrowcount(Stmt));

  rc = SQLFreeStmt(Stmt, SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIMESTAMP,
                        SQL_TIME, 0, 0, &ts1, sizeof(ts1), NULL));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_tstotime WHERE col2= ?"); 

  IS(1 == myrowcount(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_tstotime");

  return OK;
}


ODBC_TEST(t_tstotime1)
{
  SQLCHAR ts[40]= "2001-08-02 18:20:45.05";

  OK_SIMPLE_STMT(Stmt,"DROP TABLE IF EXISTS t_tstotime1");

  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_tstotime1(col1 DATE, col2 TIME, col3 TIMESTAMP)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  /* TIMESTAMP TO DATE, TIME and TS CONVERSION */
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt,
                            (SQLCHAR *)"INSERT INTO t_tstotime1 VALUES (?,?,?)",
                            SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_DATE, 0, 0, &ts, sizeof(ts), NULL));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_TIME, 0, 0, &ts, sizeof(ts), NULL));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_TIMESTAMP, 0, 0, &ts, sizeof(ts), NULL));

  /* Here currently it's supposed to fail - when binding strings as date/time types, connector doesn't parse them,
     and thus does not detect and report error. Created ODBC-43 for it */
  FAIL_IF(SQLExecute(Stmt) != SQL_ERROR, "Error expected");

  CHECK_SQLSTATE(Stmt, "22008");

  /* Taking only date part */
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_DATE, 0, 0, &ts, 10, NULL));

  FAIL_IF(SQLExecute(Stmt) != SQL_ERROR, "Error expected");

  CHECK_SQLSTATE(Stmt, "22008");

  /* are not taking fractional part */
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_TIME, 0, 0, &ts, 19, NULL));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_DBC_RC(Connection, SQLTransact(NULL,Connection,SQL_COMMIT));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_tstotime1");

  IS(1 == myrowcount(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_tstotime1");

  return OK;
}


ODBC_TEST(t_bug25846)
{
  SQLSMALLINT          column_count;
  SQLLEN               my_time_cb;
  SQLLEN               my_date_cb;
  SQL_TIMESTAMP_STRUCT my_time_ts;
  SQL_TIMESTAMP_STRUCT my_date_ts;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug25846");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug25846 (a TIME, b DATE)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug25846 VALUES ('02:56:30', '1969-07-21')");

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug25846");

  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &column_count));
  is_num(column_count, 2);

  /* Bind the TIMESTAMP buffer for TIME column */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_TIMESTAMP, &my_time_ts,
                            sizeof(my_time_ts), &my_time_cb));

  /* Bind the TIMESTAMP buffer for DATE column */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_TIMESTAMP, &my_date_ts,
                            sizeof(my_date_ts), &my_date_cb));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(my_time_ts.hour,   2);
  is_num(my_time_ts.minute, 56);
  is_num(my_time_ts.second, 30);
  is_num(my_time_cb, sizeof(SQL_TIMESTAMP_STRUCT));

  is_num(my_date_ts.year,   1969);
  is_num(my_date_ts.month,  7);
  is_num(my_date_ts.day,    21);
  is_num(my_date_cb, sizeof(SQL_TIMESTAMP_STRUCT));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug25846");

  return OK;
}


ODBC_TEST(t_time)
{
  SQLRETURN       rc;
  SQL_TIME_STRUCT tm;
  SQLCHAR         str[20];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_time");
  rc = SQLExecDirect(Stmt,"create table t_time(tm time, ts timestamp)", SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt,
                            (SQLCHAR *)"insert into t_time values (?,?)",
                            SQL_NTS));

  rc = SQLBindParameter( Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIME,
                         SQL_TIME, 0, 0, &tm, 0, NULL );
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindParameter( Stmt, 2, SQL_PARAM_INPUT, SQL_C_TIME,
                         SQL_TIMESTAMP, 0, 0, &tm, 15, NULL );
  CHECK_STMT_RC(Stmt,rc);

  tm.hour = 20;
  tm.minute = 59;
  tm.second = 45;

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt, rc);

  rc = SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "select tm from t_time");

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt,1,SQL_C_CHAR,&str,100,NULL);
  CHECK_STMT_RC(Stmt,rc);
  IS_STR(str, "20:59:45", 9);

  rc = SQLFetch(Stmt);
  IS(rc == SQL_NO_DATA_FOUND);

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_time");

  return OK;
}


/* Test for a simple time struct */
ODBC_TEST(t_time1)
{
  SQLRETURN       rc;
  SQL_TIME_STRUCT tt;
  SQLCHAR         data[30];
  SQLLEN          length;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_time");
  OK_SIMPLE_STMT(Stmt, "create table t_time(t time, t1 timestamp, t2 datetime, t3 date)");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt,
                            (SQLCHAR *)"insert into t_time(t) values(?)",
                            SQL_NTS));

  rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_TYPE_TIME,
                        SQL_TIME,0,0,&tt,0,NULL);


  tt.hour= 00;
  tt.minute= 00;
  tt.second= 03;

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  tt.hour= 01;
  tt.minute= 00;
  tt.second= 00;

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  tt.hour= 19;
  tt.minute= 00;
  tt.second= 00;

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt, rc);

  tt.hour= 01;
  tt.minute= 01;
  tt.second= 00;

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt, rc);

  tt.hour= 01;
  tt.minute= 00;
  tt.second= 01;

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt, rc);

  tt.hour= 00;
  tt.minute= 01;
  tt.second= 00;

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt, rc);

  tt.hour= 00;
  tt.minute= 11;
  tt.second= 12;

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt, rc);

  tt.hour= 01;
  tt.minute= 01;
  tt.second= 01;

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt, rc);

  tt.hour= 00;
  tt.minute= 00;
  tt.second= 00;

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt, rc);

  tt.hour= 10;
  tt.minute= 11;
  tt.second= 12;

  rc = SQLExecute(Stmt);
  CHECK_STMT_RC(Stmt, rc);

  SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
  SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "select t from t_time");

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "00:00:03", 9);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "01:00:00", 9);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "19:00:00", 9);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "01:01:00", 9);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "01:00:01", 9);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "00:01:00", 9);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "00:11:12", 9);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "01:01:01", 9);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "00:00:00", 9);

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(length, 8);
  IS_STR(data, "10:11:12", 9);

  rc = SQLFetch(Stmt);
  IS(rc == SQL_NO_DATA);

  SQLFreeStmt(Stmt, SQL_UNBIND);
  SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt,"delete from t_time");

  OK_SIMPLE_STMT(Stmt, "insert into t_time(t1) values('2003-05-12 10:11:12')");

  OK_SIMPLE_STMT(Stmt, "select t1 from t_time");

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_TIME, &tt, sizeof(tt), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(tt.hour, 10);
  is_num(tt.minute, 11);
  is_num(tt.second, 12);
  is_num(length, sizeof(SQL_TIME_STRUCT));

  rc = SQLFetch(Stmt);
  IS(rc == SQL_NO_DATA);

  SQLFreeStmt(Stmt, SQL_UNBIND);
  SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt,"delete from t_time");
  OK_SIMPLE_STMT(Stmt,"insert into t_time(t2) values('03-12-28 05:59:59')");
  OK_SIMPLE_STMT(Stmt,"select t2 from t_time");

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_TIME, &tt, sizeof(tt), &length);
  CHECK_STMT_RC(Stmt,rc);
  is_num(tt.hour, 05);
  is_num(tt.minute, 59);
  is_num(tt.second, 59);
  is_num(length, sizeof(SQL_TIME_STRUCT));

  rc = SQLFetch(Stmt);
  is_num(rc, SQL_NO_DATA);

  SQLFreeStmt(Stmt, SQL_UNBIND);
  SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt,"delete from t_time");

  OK_SIMPLE_STMT(Stmt,"insert into t_time(t3) values('2003-05-12 10:11:12')");

  OK_SIMPLE_STMT(Stmt,"select t3 from t_time");

  rc = SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetData(Stmt, 1, SQL_C_TIME, &tt, sizeof(tt), &length);
  CHECK_STMT_RC(Stmt,rc);
//   is(tt.hour == 00 || tt.minute == 00 || tt.second == 00);
  is_num(length, sizeof(SQL_TIME_STRUCT));

  rc = SQLFetch(Stmt);
  IS(rc == SQL_NO_DATA);

  SQLFreeStmt(Stmt, SQL_UNBIND);
  SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_time");

  return OK;
}


/**
 Bug #12520: DATETIME Default Value 0000-00-00 00:00:00 not returning
 correct thru ODBC
*/
ODBC_TEST(t_bug12520)
{
  SQL_TIMESTAMP_STRUCT my_time_ts;
  SQLLEN len, my_time_cb;
  SQLCHAR datetime[50];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug12520");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_bug12520 (a DATETIME DEFAULT '0000-00-00 00:00',"
         "b DATETIME DEFAULT '0000-00-00 00:00', c INT)");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug12520 (c) VALUES (1)");

  OK_SIMPLE_STMT(Stmt, "SELECT a, b FROM t_bug12520");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, datetime, sizeof(datetime),
                            &len));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_TIMESTAMP, &my_time_ts, 0,
                            &my_time_cb));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(datetime, "0000-00-00 00:00:00", 19);
  is_num(my_time_cb, SQL_NULL_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug12520");

  return OK;
}

/**
 Bug #15773: Wrong between results
*/
ODBC_TEST(t_bug15773)
{
  SQL_DATE_STRUCT a,b,c,d;
  SQLLEN len1;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug15773");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug15773("
         "`a` varchar(255) NOT NULL default '',"
         "`b` datetime NOT NULL default '0000-00-00 00:00:00',"
         "`c` datetime NOT NULL default '0000-00-00 00:00:00'"
         ") ENGINE=InnoDB DEFAULT CHARSET=latin1");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug15773 VALUES ('a', '2005-12-24 00:00:00', '2008-05-12 00:00:00')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug15773 VALUES ('b', '2004-01-01 00:00:00', '2005-01-01 00:00:00')");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug15773 VALUES ('c', '2004-12-12 00:00:00', '2005-12-12 00:00:00')");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"SELECT * FROM t_bug15773"
                           " WHERE (?) BETWEEN b AND c", SQL_NTS));

  d.day= 15;
  d.month= 12;
  d.year = 2005;

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_DATE,
                                  SQL_TYPE_DATE, 0, 0, &d, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, &a, 255, &len1));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_TYPE_DATE, &b, 0, &len1));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_TYPE_DATE, &c, 0, &len1));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug15773");
  return OK;
}


/**
 Bug #9927: Updating datetime columns
*/
ODBC_TEST(t_bug9927)
{
  SQLCHAR col[10];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug9927");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_bug9927 (a TIMESTAMP DEFAULT 0,"
        "b TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)");

  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt,SQL_ROWVER,  NULL, 0,
                                   NULL, 0, (SQLCHAR *)"t_bug9927", SQL_NTS,
                                   0, SQL_NO_NULLS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, col, 2), "b", 1);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug9927");

  return OK;
}


/**
 Bug #30081: Can't distinguish between auto-set TIMESTAMP and auto-updated
 TIMESTAMP
*/
ODBC_TEST(t_bug30081)
{
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug30081");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_bug30081 (a TIMESTAMP DEFAULT 0,"
        "b TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt,SQL_ROWVER,  NULL, 0,
                                   NULL, 0, (SQLCHAR *)"t_bug30081", SQL_NTS,
                                   0, SQL_NO_NULLS));

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug30081");

  return OK;
}


/**
  Verify that we get correct data for SQL_DATA_TYPE and SQL_DATETIME_SUB
  from SQLColumns(). Also check SQL_DESC_TYPE from SQLColAttribute().
*/
ODBC_TEST(t_datecolumns)
{
  SQLCHAR col[10];
  SQLLEN type;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_datecolumns");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_datecolumns(a TIMESTAMP, b DATETIME, c DATE, d TIME)");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                            (SQLCHAR *)"t_datecolumns", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, col, 4), "a", 1);
  is_num(my_fetch_int(Stmt, 14), SQL_DATETIME);
  is_num(my_fetch_int(Stmt, 15), SQL_CODE_TIMESTAMP);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, col, 4), "b", 1);
  is_num(my_fetch_int(Stmt, 14), SQL_DATETIME);
  is_num(my_fetch_int(Stmt, 15), SQL_CODE_TIMESTAMP);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, col, 4), "c", 1);
  is_num(my_fetch_int(Stmt, 14), SQL_DATETIME);
  is_num(my_fetch_int(Stmt, 15), SQL_CODE_DATE);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, col, 4), "d", 1);
  is_num(my_fetch_int(Stmt, 14), SQL_DATETIME);
  is_num(my_fetch_int(Stmt, 15), SQL_CODE_TIME);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_datecolumns");

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_TYPE, NULL, 0, NULL,
                                 &type));
  is_num(type, SQL_DATETIME);
  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 2, SQL_DESC_TYPE, NULL, 0, NULL,
                                 &type));
  is_num(type, SQL_DATETIME);
  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 3, SQL_DESC_TYPE, NULL, 0, NULL,
                                 &type));
  is_num(type, SQL_DATETIME);
  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 4, SQL_DESC_TYPE, NULL, 0, NULL,
                                 &type));
  is_num(type, SQL_DATETIME);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_datecolumns");
  return OK;
}


/**
  Bug #14414: SQLColumn() does not return timestamp nullable attribute correctly
*/
ODBC_TEST(t_bug14414)
{
  SQLCHAR col[10];
  SQLSMALLINT nullable;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug14414");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug14414(a TIMESTAMP, b TIMESTAMP NOT NULL,"
        "c TIMESTAMP NULL)");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                            (SQLCHAR *)"t_bug14414", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, col, 4), "a", 1);
  is_num(my_fetch_int(Stmt, 11), SQL_NULLABLE);
  IS_STR(my_fetch_str(Stmt, col, 18), "YES", 3);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, col, 4), "b", 1);
  is_num(my_fetch_int(Stmt, 11), SQL_NULLABLE);
  IS_STR(my_fetch_str(Stmt, col, 18), "YES", 3);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, col, 4), "c", 1);
  is_num(my_fetch_int(Stmt, 11), SQL_NULLABLE);
  IS_STR(my_fetch_str(Stmt, col, 18), "YES", 3);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /**
    Bug #26108  MyODBC ADO field attributes reporting adFldMayBeNull for
    not null columns
  */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug14414");

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 1, col, sizeof(col), NULL, NULL, NULL,
                                NULL, &nullable));
  is_num(nullable, SQL_NULLABLE);

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 2, col, sizeof(col), NULL, NULL, NULL,
                                NULL, &nullable));
  is_num(nullable, SQL_NULLABLE);

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 3, col, sizeof(col), NULL, NULL, NULL,
                                NULL, &nullable));
  is_num(nullable, SQL_NULLABLE);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug14414");
  return OK;
}


/**
 Bug #30939: SQLGetTypeInfo returns 6 instead of 8 for COLUMN_SIZE for
 SQL_TYPE_TIME
*/
ODBC_TEST(t_bug30939)
{
  CHECK_STMT_RC(Stmt, SQLGetTypeInfo(Stmt, SQL_TYPE_TIME));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(my_fetch_int(Stmt, 3), 8);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "eof expected");

  return OK;
}


/**
 Bug #31009: Wrong SQL_DESC_LITERAL_PREFIX for date-time types
*/
ODBC_TEST(t_bug31009)
{
  SQLCHAR data[20];
  SQLSMALLINT len;
  SQLLEN dlen;

  OK_SIMPLE_STMT(Stmt, "SELECT CAST('2007-01-13' AS DATE) AS col1");

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_LITERAL_PREFIX,
                                 data, sizeof(data), &len, NULL));
  is_num(len, 1);
  IS_STR(data, "'", 2);

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_LITERAL_SUFFIX,
                                 data, sizeof(data), &len, NULL));
  is_num(len, 1);
  IS_STR(data, "'", 2);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &dlen));
  is_num(dlen, 10);
  IS_STR(data, "2007-01-13", 11);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "eof expected");

  return OK;
}


/**
 Bug #37342: ODBC TIMESTAMP string format not handled properly by ODBC driver
*/
ODBC_TEST(t_bug37342)
{
  SQLCHAR *date= (SQLCHAR *)"{dt '2007-01-13'}";
  SQLCHAR *time= (SQLCHAR *)"194759";
  SQLCHAR out[30];
  TIMESTAMP_STRUCT ts;
  SQLLEN len= SQL_NTS;

  diag("{dt ts not supported}");
  return SKIP;

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_TIMESTAMP, 0, 0, date, 0, &len));

  OK_SIMPLE_STMT(Stmt, "SELECT ? AS foo");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, out, 1), "2007-01-13", 11);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_TYPE_TIME, 0, 0, time, 0, &len));

  OK_SIMPLE_STMT(Stmt, "SELECT ? AS foo");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, out, 1), "19:47:59", 9);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIMESTAMP,
                                  SQL_TYPE_TIME, 0, 0, &ts, sizeof(ts), NULL));

  ts.hour= 19;
  ts.minute= 47;
  ts.second= 59;
  ts.fraction= 4;

  /* Fractional truncation */
  FAIL_IF(SQLExecDirect(Stmt, "SELECT ? AS foo", SQL_NTS) != SQL_ERROR, "Error expected");
  ts.fraction= 0;

  OK_SIMPLE_STMT(Stmt, "SELECT ? AS foo");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, out, 1), "19:47:59", 9);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


/**
  Bug#60646 - Fractions of seconds ignored in results
*/
ODBC_TEST(t_bug60646)
{
  SQLCHAR buff[128];
  TIMESTAMP_STRUCT ts;
  SQLLEN len;
  const char *expected= "2012-01-01 01:01:01.000001";

  OK_SIMPLE_STMT(Stmt,
        "SELECT timestamp('2012-01-01 01:01:01.000001')"  /*1*/
        " ,timestamp('2012-01-01 01:01:01.100002')"       /*2*/
        " ,'2011-07-29 17:52:15.0000000009'"              /*3*/
        " ,'1000-01-01 12:00:00.000001'"                  /*4*/
        " ,time('2011-12-31 23:59:59.999999')"            /*5*/
        " ,time('2011-12-31 23:59:59.999999')"            /*6 - same as 5th for */
        );
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1));

  /* Fields 1-4 checking conversions from date as a string
   * 1) just to be sure that everything is fine with string */
  IS_STR(my_fetch_str(Stmt, buff, 1), expected, sizeof(expected));

  /* 2) testing if fractional part is converted to nanoseconds correctly */
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_TYPE_TIMESTAMP, &ts, sizeof(ts),
                            &len));

  is_num(ts.fraction, 100002000);

  /* 3) fractional part is less than we care (less than nanosecond).
        Test using string as MySQL does not support units less than a microsecond */
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 3, SQL_C_TYPE_TIMESTAMP, &ts, sizeof(ts),
                            &len));
  is_num(ts.fraction, 0);

  /* 4) testing if min fraction detected
        Again - mysql supports microseconds only. thus using string
   */
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 4, SQL_C_TYPE_TIMESTAMP, &ts, sizeof(ts),
                            &len));
  is_num(ts.fraction, 1000);

  /* 5) if time is converted to timestamp - checking if current date is set
        and if fractional part in place. former can actually fail if day is
        being changed */
  {
    time_t sec_time= time(NULL);
    struct tm * cur_tm;

    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 5, SQL_C_TYPE_TIMESTAMP, &ts, sizeof(ts),
                              &len));
    cur_tm= localtime(&sec_time);

    is_num(ts.year, 1900 + cur_tm->tm_year);
    is_num(ts.month, 1 + cur_tm->tm_mon);
    is_num(ts.day, cur_tm->tm_mday);
  }

  is_num(ts.fraction, 0);

  /* 5th col once again(but from 6th column, that has to be the same as 5th)
     This time we get it in time struct. Thus we are
     loosing fractioanl part. Thus the state has to be 01S07 and
     SQL_SUCCESS_WITH_INFO returned */
  {
    SQL_TIME_STRUCT timestruct;

    EXPECT_STMT(Stmt, SQLGetData(Stmt, 6, SQL_C_TYPE_TIME, &timestruct,
      sizeof(timestruct), &len), SQL_SUCCESS_WITH_INFO);

    CHECK_SQLSTATE(Stmt, "01S07");
  }

  EXPECT_STMT(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1L), SQL_NO_DATA);

  return OK;
}


/* Bug#60648 ODBC prepared statements ignore fractional part of temporal data
   types */
ODBC_TEST(t_bug60648)
{
  SQL_TIMESTAMP_STRUCT param, result;

  param.year=     2011;
  param.month=    8;
  param.day=      6;
  param.hour=     1;
  param.minute=   2;
  param.second=   3;
  param.fraction= 1000;
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"select ?", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP,
    SQL_TYPE_DATE, 0, 0, &param, 0, NULL));

  FAIL_IF(SQLExecute(Stmt) != SQL_ERROR, "Error expected");
  CHECK_SQLSTATE(Stmt, "22008");

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP,
    SQL_TYPE_TIME, 0, 0, &param, 0, NULL));

  FAIL_IF(SQLExecute(Stmt) != SQL_ERROR, "Error expected");
  CHECK_SQLSTATE(Stmt, "22008");

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP,
    SQL_TYPE_TIMESTAMP, 0, 0, &param, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_TYPE_TIMESTAMP, &result, 0,
                            NULL));

  is_num(1000, result.fraction);

  return OK;
}


ODBC_TEST(t_b13975271)
{
  SQLCHAR ts[27];
  SQLLEN len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_b13975271");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_b13975271 (ts TIMESTAMP(6), dt DATETIME(6),\
                  t TIME(6))");

  strcpy((char *)ts, "2012-04-25 10:20:49.0194");

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_TIMESTAMP,0, 0, ts, sizeof(ts), NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_CHAR,0, 0, ts, sizeof(ts), NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_CHAR,0, 0, ts, sizeof(ts), NULL));
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "INSERT INTO t_b13975271(ts,dt,t) \
                                    VALUES (?,?,?)",
                            SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  /* now fetch and verify the results .. */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_b13975271");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, ts, sizeof(ts), &len));
  IS_STR(ts, "2012-04-25 10:20:49.019400", 26);

  /*To make sure that for next test we compare not the data from prev call */
  ts[0]='\0';
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_CHAR, ts, sizeof(ts), &len));
  IS_STR(ts, "2012-04-25 10:20:49.0194", 24);
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 3, SQL_C_CHAR, ts, sizeof(ts), &len));
  IS_STR(ts, "10:20:49.0194", 13);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_b13975271");

  return OK;
}


ODBC_TEST(t_odbc82)
{
  SQL_TIME_STRUCT ts, ts2;
  SQLLEN  ind, ind2;

  OK_SIMPLE_STMT(Stmt, "SELECT '24:00:00', '00:00:00'");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_TYPE_TIME, &ts2,
    sizeof(ts2), &ind2));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_TYPE_TIME, &ts,
    sizeof(ts), &ind));

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_ERROR);

  CHECK_SQLSTATE(Stmt, "22007");

  is_num(ts.hour, 0);
  is_num(ts.minute, 0);
  is_num(ts.second, 0);
  FAIL_IF(ind==SQL_NULL_DATA, "00:00:00 is valid time, indicator shouldn't be SQL_NULL_DATA");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


ODBC_TEST(t_odbc70)
{
  SQL_TIMESTAMP_STRUCT ts= { 0 }, ts1= { 0 }, ts2= { 0 };
  char * ZeroDate= "0000-00-00 00:00:00";

  ts.year= 1970;
  ts.month= 1;
  ts.day= 2;

  ts1.year= ts1.month= ts1.day= 1;
  ts2.year= ts2.month= ts2.day= 1;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc70");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc70 (ts TIMESTAMP, dt DATETIME, d DATE)");

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIMESTAMP, SQL_TIMESTAMP,
    0, 0, &ts, sizeof(ts), NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_TIMESTAMP,
    SQL_TIMESTAMP, 0, 0, &ts1, sizeof(ts1), NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_TIMESTAMP,
    SQL_TYPE_DATE, 0, 0, &ts2, sizeof(ts2), NULL));
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "INSERT INTO t_odbc70(ts, dt, d) \
                                        VALUES (?, ?, ?)", SQL_NTS));
  /* First valid values */
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  ts2.year= ts2.month= ts2.day= 0;

  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_ERROR);
  CHECK_SQLSTATE(Stmt, "22007");

  ts1.year= ts1.month= ts1.day= 0;
  ts2.year= ts2.month= ts2.day= 1;

  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_ERROR);
  CHECK_SQLSTATE(Stmt, "22007");

  /* Not sure why is it here 2nd time, but won't hurt */
  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_ERROR);
  CHECK_SQLSTATE(Stmt, "22007");

  ts1.year= ts1.month= ts1.day= 1;
  ts.year= ts.month= ts.day= 0;

  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_ERROR);
  CHECK_SQLSTATE(Stmt, "22007");

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_TIMESTAMP,
    0, 0, ZeroDate, 0, NULL));

  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_ERROR);
    CHECK_SQLSTATE(Stmt, "22018");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc70");

  return OK;
}


ODBC_TEST(t_17613161)
{
  SQL_TIME_STRUCT ts, result;
  SQL_INTERVAL_STRUCT h2s, interval;

  h2s.intval.day_second.hour=   ts.hour  = 100;
  h2s.intval.day_second.minute= ts.minute= 20;
  h2s.intval.day_second.second= ts.second= 45;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_17613161");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_17613161(col1 time)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"INSERT INTO t_17613161 "
    "(col1) VALUES (?)", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIME,
    SQL_TIME, 0, 0, &ts, sizeof(ts), NULL));
  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_ERROR);
  is_num(check_sqlstate(Stmt, "22007"), OK);

  /* Such conversion is not supported */
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIME,
    SQL_INTERVAL_HOUR_TO_SECOND, 0, 0, &ts, sizeof(ts), NULL));
  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_ERROR);
  is_num(check_sqlstate(Stmt, "07006"), OK);

  /* For interval types big hours should work fine */
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_INTERVAL_HOUR_TO_SECOND,
    SQL_INTERVAL_HOUR_TO_SECOND, 0, 0, &h2s, sizeof(h2s), NULL));
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_17613161");

  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1));

  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_C_TYPE_TIME, &result, 0,
    NULL), SQL_ERROR);
  is_num(check_sqlstate(Stmt, "22007"), OK);

  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_C_INTERVAL_HOUR_TO_MINUTE, &interval, 0,
    NULL), SQL_SUCCESS_WITH_INFO);
  CHECK_SQLSTATE(Stmt, "01S07");
  is_num(interval.intval.day_second.second, 0);
  is_num(interval.intval.day_second.minute, 20);
  is_num(interval.intval.day_second.hour, 100);

  memset(&interval, 0, sizeof(interval));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_INTERVAL_HOUR_TO_SECOND, &interval, 0,
    NULL));
  is_num(interval.intval.day_second.second, 45);
  is_num(interval.intval.day_second.minute, 20);
  is_num(interval.intval.day_second.hour, 100);

  memset(&interval, 0, sizeof(interval));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_INTERVAL_HOUR_TO_MINUTE, &interval, 0, NULL));
  if (ForwardOnly)
  {
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_17613161");
    EXPECT_STMT(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1), SQL_SUCCESS_WITH_INFO);
  }
  else
  {
    EXPECT_STMT(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 0), SQL_SUCCESS_WITH_INFO);
  }
  CHECK_SQLSTATE(Stmt, "01S07");
  is_num(interval.intval.day_second.second, 0);
  is_num(interval.intval.day_second.minute, 20);
  is_num(interval.intval.day_second.hour, 100);

  memset(&interval, 0, sizeof(interval));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_INTERVAL_HOUR_TO_SECOND, &interval, 0, NULL));
  if (ForwardOnly)
  {
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
    OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_17613161");
    CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1));
  }
  else
  {
    CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 0));
  }
  is_num(interval.intval.day_second.second, 45);
  is_num(interval.intval.day_second.minute, 20);
  is_num(interval.intval.day_second.hour, 100);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_17613161");

  return OK;
}


ODBC_TEST(t_bug67793)
{
  SQL_INTERVAL_STRUCT h2s;
  SQLLEN outlen= 0;
  const SQLCHAR *query= "SELECT '123456789:45:07', '99999:42:09', CAST('-800:12:17' AS TIME)";

  /* check situations with sec and min overflow */
  OK_SIMPLE_STMT(Stmt, query);
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1));

  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_INTERVAL_HOUR_TO_SECOND, &h2s, sizeof(h2s), &outlen), SQL_ERROR);
  /* leading precision is 5, i.e. hours max value is 99999 */
  CHECK_SQLSTATE(Stmt, "22015")

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_INTERVAL_HOUR_TO_SECOND, &h2s, sizeof(h2s), &outlen));
  is_num(outlen, sizeof(h2s));
  is_num(h2s.intval.day_second.hour, 99999);
  is_num(h2s.intval.day_second.minute, 42);
  is_num(h2s.intval.day_second.second, 9);

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 3, SQL_INTERVAL_HOUR_TO_SECOND, &h2s, sizeof(h2s), &outlen));
  is_num(outlen, sizeof(h2s));
  is_num(h2s.intval.day_second.hour, 800);
  is_num(h2s.intval.day_second.minute, 12);
  is_num(h2s.intval.day_second.second, 17);
  is_num(h2s.interval_sign, SQL_TRUE);

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_INTERVAL_HOUR_TO_SECOND, &h2s, sizeof(h2s), NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_INTERVAL_HOUR_TO_SECOND, &h2s, sizeof(h2s), &outlen));

  if (ForwardOnly)
  {
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
    OK_SIMPLE_STMT(Stmt, query);
    EXPECT_STMT(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1), SQL_ERROR);
  }
  else
  {
    EXPECT_STMT(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_FIRST, 0), SQL_ERROR);
  }
  CHECK_SQLSTATE(Stmt, "22015");

  is_num(outlen, sizeof(h2s));
  is_num(h2s.intval.day_second.hour, 99999);
  is_num(h2s.intval.day_second.minute, 42);
  is_num(h2s.intval.day_second.second, 9);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  return OK;
}


ODBC_TEST(t_odbc138)
{
  SQL_TIMESTAMP_STRUCT ts= { 0 };
  SQLLEN ind;
  unsigned int i;

  for (i= 0; i < 100; ++i)
  {
    OK_SIMPLE_STMT(Stmt, "SELECT DATE_ADD('2018-02-01', INTERVAL -188 DAY)");

    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_TIMESTAMP, &ts, 0, &ind));

    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

    if (ts.year != 2017)
    {
      diag("Failure of the Iteration #%d: Year %hd != 2017", i, ts.year);
    }
    if (ts.month != 7)
    {
      diag("Failure of the Iteration #%d: Month %hd != 7", i, ts.month);

    }
    if (ts.day != 28)
    {
      diag("Failure of the Iteration #%d: Day %hd != 28", i, ts.day);
    }
    CHECK_STMT_RC(Stmt, SQLCloseCursor(Stmt));
  }

  return OK;
}

/* ODBC- 148 DATE and DATETIME values are NULL in Crystal Reports.
   Connector didn't set length of returned data in the length buffer for those types */
ODBC_TEST(t_odbc148)
{
  SQL_TIMESTAMP_STRUCT ts= { 0 };
  SQL_DATE_STRUCT      d=  { 0 };
  SQL_TIME_STRUCT      t=  { 0 };
  SQLLEN tsLen, dLen, tLen;

  OK_SIMPLE_STMT(Stmt, "SELECT CAST('2018-06-17 23:45:01' AS DATETIME), CAST('2018-07-18' AS DATE), CAST('11:30:27' AS TIME)");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_TIMESTAMP, &ts, 0, &tsLen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_DATE,      &d,  0, &dLen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_TIME,      &t,  0, &tLen));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(ts.year, 2018);
  is_num(ts.month, 6);
  is_num(ts.day, 17);
  is_num(ts.hour, 23);
  is_num(ts.minute, 45);
  is_num(ts.second, 1);
  is_num(ts.fraction, 0);
  is_num(tsLen, sizeof(SQL_TIMESTAMP_STRUCT));
  is_num(d.year, 2018);
  is_num(d.month, 7);
  is_num(d.day, 18);
  is_num(dLen, sizeof(SQL_DATE_STRUCT));
  is_num(t.hour, 11);
  is_num(t.minute, 30);
  is_num(t.second, 27);
  is_num(tLen, sizeof(SQL_TIME_STRUCT));

  tsLen= dLen= tLen= 0;
  memset(&ts, 0, sizeof(ts));
  memset(&d, 0, sizeof(d));
  memset(&t, 0, sizeof(t));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_TIMESTAMP, &ts, 0, &tsLen));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_DATE,      &d,  0, &dLen));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 3, SQL_C_TIME,      &t,  0, &tLen));

  is_num(ts.year, 2018);
  is_num(ts.month, 6);
  is_num(ts.day, 17);
  is_num(ts.hour, 23);
  is_num(ts.minute, 45);
  is_num(ts.second, 1);
  is_num(ts.fraction, 0);
  is_num(tsLen, sizeof(SQL_TIMESTAMP_STRUCT));
  is_num(d.year, 2018);
  is_num(d.month, 7);
  is_num(d.day, 18);
  is_num(dLen, sizeof(SQL_DATE_STRUCT));
  is_num(t.hour, 11);
  is_num(t.minute, 30);
  is_num(t.second, 27);
  is_num(tLen, sizeof(SQL_TIME_STRUCT));

  CHECK_STMT_RC(Stmt, SQLCloseCursor(Stmt));

  return OK;
}


ODBC_TEST(t_odbc199_time2timestamp)
{
  SQL_TIME_STRUCT t;
  SQL_TIMESTAMP_STRUCT ts= {0}, ts1= {0};
  time_t sec_time;
  struct tm * cur_tm;

  sec_time= time(NULL);
  cur_tm= localtime(&sec_time);

  ts.year= 1900 + cur_tm->tm_year;
  ts.month= cur_tm->tm_mon + 1;
  ts.day= cur_tm->tm_mday;
  ts.fraction= 0;

  ts1.year= ts1.month= ts1.day= 1;
  t.hour= 11;
  t.minute= 21;
  t.second= 33;

  ts.hour= t.hour;
  ts.minute= t.minute;
  ts.second= t.second;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_time2timestamp");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_time2timestamp (ts TIMESTAMP)");

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIME, SQL_TIMESTAMP,
    0, 0, &t, sizeof(t), NULL));

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "INSERT INTO t_time2timestamp(ts) \
                                        VALUES (?)", SQL_NTS));
  /* First valid values */
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  OK_SIMPLE_STMT(Stmt, "SELECT ts FROM t_time2timestamp");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_TYPE_TIMESTAMP, &ts1, 0, NULL));

  diag("This test may fail if run at the moment when day changes");
  is_num(ts.year, ts1.year);
  is_num(ts.month, ts1.month);
  is_num(ts.day, ts1.day);
  is_num(ts.hour, ts1.hour);
  is_num(ts.minute, ts1.minute);
  is_num(ts.second, ts1.second);
  is_num(ts.fraction, ts1.fraction);

  CHECK_STMT_RC(Stmt, SQLCloseCursor(Stmt));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_time2timestamp");

  return OK;
}

/* ODBC-345 connector doesn't allow to enter into datetime field dates beyond timestamp allowed dates */
ODBC_TEST(t_odbc345)
{
  SQL_TIMESTAMP_STRUCT ts = { 0 }, ts1 = { 0 }, r1, r2;
  char asStr[32];

  ts.year= 1968;
  ts.month= 1;
  ts.day= 2;
  ts.fraction= 0;
  ts.hour= 3;
  ts.minute= 4;
  ts.second= 5;
  ts1.year= 2039;
  ts1.month= 12;
  ts1.day= 11;
  ts1.fraction = 1000;
  ts1.hour = 10;
  ts1.minute = 9;
  ts1.second = 8;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc345");
  /* Also checking that fractional seconds are not affected */
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc345 (dt DATETIME NOT NULL, dtf DATETIME(6) NOT NULL)");

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP,
    0, 0, &ts, sizeof(ts), NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP,
    0, 0, &ts1, sizeof(ts), NULL));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "INSERT INTO t_odbc345(dt, dtf) \
                                           VALUES (?,?)", SQL_NTS));;

  OK_SIMPLE_STMT(Stmt, "SELECT dt, dtf FROM t_odbc345");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_TIMESTAMP, &r1,
    sizeof(r1), NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_TIMESTAMP, &r2,
    sizeof(r2), NULL));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(ts.year, r1.year);
  is_num(ts.month, r1.month);
  is_num(ts.day, r1.day);
  is_num(ts.hour, r1.hour);
  is_num(ts.minute, r1.minute);
  is_num(ts.second, r1.second);
  is_num(ts.fraction, r1.fraction);

  is_num(ts1.year, r2.year);
  is_num(ts1.month, r2.month);
  is_num(ts1.day, r2.day);
  is_num(ts1.hour, r2.hour);
  is_num(ts1.minute, r2.minute);
  is_num(ts1.second, r2.second);
  is_num(ts1.fraction, r2.fraction);

  IS_STR("2039-12-11 10:09:08.000001", my_fetch_str(Stmt, asStr, 2), sizeof("2039-12-11 10:09:08.000001"));

  CHECK_STMT_RC(Stmt, SQLCloseCursor(Stmt));
  
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc345");
  
  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {my_ts,         "my_ts",       NORMAL},
  {t_tstotime,    "t_tstotime",  NORMAL},
  {t_tstotime1,   "t_tstotime1", NORMAL},
  {t_bug25846,    "t_bug25846",  NORMAL},
  {t_time,        "t_time",      NORMAL},
  {t_time1,       "t_time1",     NORMAL},
  {t_bug12520,    "t_bug12520",  NORMAL},
  {t_bug15773,    "t_bug15773",  NORMAL},
  {t_bug9927,     "t_bug9927",   NORMAL},
  {t_bug30081,    "t_bug30081",  NORMAL},
  {t_datecolumns, "t_datecolumns", NORMAL},
  {t_bug14414,    "t_bug14414",  NORMAL},
  {t_bug30939,    "t_bug30939",  NORMAL},
  {t_bug31009,    "t_bug31009",  NORMAL},
  {t_bug37342,    "t_bug37342",  NORMAL},
  {t_bug60646,    "t_bug60646",  NORMAL},
  {t_bug60648,    "t_bug60648",  NORMAL},
  {t_b13975271,   "t_b13975271", NORMAL},
  {t_odbc82,      "t_odbc82_zero_time_vals", NORMAL},
  {t_odbc70,      "t_odbc70_zero_datetime_vals", NORMAL},
  {t_17613161,    "t_17613161",  NORMAL},
  {t_bug67793,    "t_bug67793",  NORMAL},
  {t_odbc138,     "t_odbc138_dateadd_negative", NORMAL},
  {t_odbc148,     "t_odbc148_datatypes_values_len", NORMAL},
  {t_odbc199_time2timestamp, "t_odbc199_time2timestamp", NORMAL},
  {t_odbc345,     "t_odbc345", NORMAL},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);

  return run_tests(my_tests);
}
