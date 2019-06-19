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

ODBC_TEST(t_longlong1)
{
#if SQLBIGINT_MADE_PORTABLE || defined(_WIN32)
  SQLBIGINT session_id, ctn;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_longlong");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_longlong (a BIGINT, b BIGINT)");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY,
                                (SQLPOINTER)SQL_CONCUR_ROWVER, 0));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CONCURRENCY,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLPrepare(Stmt,
                            (SQLCHAR *)"INSERT INTO t_longlong VALUES (?,?)",
                            SQL_NTS));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_UBIGINT,
                                  SQL_BIGINT, 20, 0, &session_id, 20, NULL));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_UBIGINT,
                                  SQL_BIGINT, 20, 0, &ctn, 20, NULL));

  for (session_id= 50, ctn= 0; session_id < 100; session_id++)
  {
    ctn+= session_id;
    CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecute(Stmt));
  }

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_longlong");

  FAIL_IF(50 != myrowcount(Stmt), "Expexted 50 rows");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_longlong");
#endif /* SQLBIGINT_MADE_PORTABLE || defined(_WIN32) */

  return OK;
}


ODBC_TEST(t_decimal)
{
  SQLCHAR         str[20],s_data[]="189.4567";
  SQLDOUBLE       d_data=189.4567;
  SQLINTEGER      i_data=189, l_data=-23;
  SQLRETURN       rc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_decimal");
  OK_SIMPLE_STMT(Stmt,"create table t_decimal(d1 decimal(10,6))");
    
  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection, rc);
  
  rc = SQLPrepare(Stmt, (SQLCHAR *)"insert into t_decimal values (?),(?),(?),(?)",SQL_NTS);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);
    
  rc = SQLBindParameter( Stmt, 1, SQL_PARAM_INPUT, SQL_C_DOUBLE,
                           SQL_DECIMAL, 10, 4, &d_data, 0, NULL );
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  rc = SQLBindParameter( Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG,
                           SQL_DECIMAL, 10, 4, &i_data, 0, NULL );
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  rc = SQLBindParameter( Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR,
                           SQL_DECIMAL, 10, 4, &s_data, 9, NULL );
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);
    
  rc = SQLBindParameter( Stmt, 4, SQL_PARAM_INPUT, SQL_C_LONG,
                           SQL_DECIMAL, 10, 4, &l_data, 0, NULL );
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  rc = SQLExecute(Stmt);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  rc = SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);
  
  rc = SQLTransact(NULL,Connection,SQL_COMMIT);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  OK_SIMPLE_STMT(Stmt, "select d1 from t_decimal");

  rc = SQLFetch(Stmt);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  rc = SQLGetData(Stmt,1,SQL_C_CHAR,&str,19,NULL);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);
  
  fprintf(stdout,"decimal(SQL_C_DOUBLE) : %s\n",str);
  FAIL_IF(strcmp((const char*)str, "189.456700") != 0, "expected str=189.456700");

  rc = SQLFetch(Stmt);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);
    
  rc = SQLGetData(Stmt,1,SQL_C_CHAR,&str,19,NULL);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);
    
  fprintf(stdout,"decimal(SQL_C_INTEGER): %s\n",str);
  FAIL_IF(strcmp((const char*)str,"189.000000")!=0,"expected 189.000000");

  rc = SQLFetch(Stmt);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  rc = SQLGetData(Stmt,1,SQL_C_CHAR,&str,19,NULL);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);
  fprintf(stdout,"decimal(SQL_C_CHAR)   : %s\n",str);
  FAIL_IF(strcmp((const char*)str,"189.456700")!=0,"expected 189.456700");

  rc = SQLFetch(Stmt);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  rc = SQLGetData(Stmt,1,SQL_C_CHAR,&str,19,NULL);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);
  fprintf(stdout,"decimal(SQL_C_LONG)   : %s\n",str);
  FAIL_IF(strcmp((const char*)str, "-23.000000") != 0, "expected -23.00000");

  rc = SQLFetch(Stmt);
  FAIL_IF(rc != SQL_NO_DATA_FOUND, "expected eof");

  rc = SQLFreeStmt(Stmt,SQL_UNBIND);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_decimal");

  return OK;
}


ODBC_TEST(t_bigint)
{
#if SQLBIGINT_MADE_PORTABLE || defined(_WIN32)
    SQLRETURN rc;
    SQLLEN nlen = 4;
    union {                    /* An union to get 4 byte alignment */
      SQLCHAR buf[20];
      SQLINTEGER dummy;
    } id = {"99998888"};       /* Just to get a binary pattern for some 64 bit big int */

    OK_SIMPLE_STMT(Stmt,"drop table if exists t_bigint");

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    OK_SIMPLE_STMT(Stmt,"create table t_bigint(id int(20) NOT NULL auto_increment,name varchar(20), primary key(id))");
   

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    /* TIMESTAMP TO DATE, TIME and TS CONVERSION */
    rc = SQLPrepare(Stmt,"insert into t_bigint values(?,'venuxyz')", SQL_NTS);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_LONG,
                          SQL_BIGINT,0,0,&id.buf,sizeof(id.buf),&nlen);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLExecute(Stmt);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    OK_SIMPLE_STMT(Stmt,"insert into t_bigint values(10,'mysql1')");


   OK_SIMPLE_STMT(Stmt,"insert into t_bigint values(20,'mysql2')");


    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    rc = SQLSpecialColumns(Stmt,SQL_ROWVER,NULL,SQL_NTS,NULL,SQL_NTS,
                           "t_bigint",SQL_NTS,SQL_SCOPE_TRANSACTION,SQL_NULLABLE);

    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    FAIL_IF( 0 != myrowcount(Stmt), "expected 0 rows");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLColumns(Stmt,NULL,SQL_NTS,NULL,SQL_NTS,"t_bigint",SQL_NTS,NULL,SQL_NTS);

    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    FAIL_IF( 2 != myrowcount(Stmt), "expected 2 rows");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLStatistics(Stmt,NULL,SQL_NTS,NULL,SQL_NTS,"t_bigint",SQL_NTS,SQL_INDEX_ALL,SQL_QUICK);

    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    FAIL_IF( 1 != myrowcount(Stmt), "Expected 1 row");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

#if CATALOG_FUNCTIONS_FIXED
    rc = SQLGetTypeInfo(Stmt,SQL_BIGINT);
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    FAIL_IF( 4 != myrowcount(Stmt), "expected 4 rows");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLGetTypeInfo(Stmt,SQL_BIGINT);
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    FAIL_IF( 4 != myrowcount(Stmt), "Expected 4 rows");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);
#endif

    OK_SIMPLE_STMT(Stmt,"select * from t_bigint");

    rc = SQLFetch(Stmt);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLGetData(Stmt,1,SQL_C_DEFAULT,&id.buf,sizeof(id.buf),&nlen);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

#endif /* SQLBIGINT_MADE_PORTABLE || defined(_WIN32) */
  return OK;
}


ODBC_TEST(t_enumset)
{
    SQLRETURN rc;
    SQLCHAR szEnum[40]="MYSQL_E1";
    SQLCHAR szSet[40]="THREE,ONE,TWO";

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_enumset");

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    OK_SIMPLE_STMT(Stmt,"create table t_enumset(col1 enum('MYSQL_E1','MYSQL_E2'),col2 set('ONE','TWO','THREE'))");
   
    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    OK_SIMPLE_STMT(Stmt, "insert into t_enumset values('MYSQL_E2','TWO,THREE')");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLPrepare(Stmt, (SQLCHAR *)
                              "insert into t_enumset values(?,?)", SQL_NTS));

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_CHAR,0,0,&szEnum,sizeof(szEnum),NULL);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_CHAR,0,0,&szSet,sizeof(szSet),NULL);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLExecute(Stmt);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLTransact(NULL,Connection,SQL_COMMIT);
    CHECK_HANDLE_RC(SQL_HANDLE_DBC, Connection,rc);

    OK_SIMPLE_STMT(Stmt,"select * from t_enumset");
  
    FAIL_IF( 2 != myrowcount(Stmt), "expected 2 rows");

    rc = SQLFreeStmt(Stmt,SQL_UNBIND);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
   CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_enumset");

  return OK;
}


/**
 Bug #16917: MyODBC doesn't return ASCII 0 characters for TEXT columns
*/
ODBC_TEST(t_bug16917)
{
  SQLCHAR buff[255];
  SQLLEN  len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug16917");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug16917 (a TEXT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug16917 VALUES ('a\\0b')");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT a FROM t_bug16917");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  /* This SQLSetPos() causes the field lengths to get lost. */
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLSetPos(Stmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, buff, 0, &len));
  is_num(len, 3);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, buff, sizeof(buff), &len));
  is_num(buff[0], 'a');
  is_num(buff[1], 0);
  is_num(buff[2], 'b');
  is_num(len, 3);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug16917");
  return OK;
}


/**
 Bug #16235: ODBC driver doesn't format parameters correctly
*/
ODBC_TEST(t_bug16235)
{
  SQLCHAR varchar[]= "a'b", text[]= "c'd", buff[10];
  SQLLEN varchar_len= SQL_NTS, text_len= SQL_NTS;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug16235");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug16235 (a NVARCHAR(20), b TEXT)");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLPrepare(Stmt, (SQLCHAR *)
                            "INSERT INTO t_bug16235 VALUES (?,?)", SQL_NTS));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_WVARCHAR, 0, 0, varchar, sizeof(varchar),
                                  &varchar_len));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_WLONGVARCHAR, 0, 0, text, sizeof(text),
                                  &text_len));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecute(Stmt));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug16235");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 1), "a'b", 3);
  IS_STR(my_fetch_str(Stmt, buff, 2), "c'd", 3);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected EOF");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug16235");

  return OK;
}


/**
 Bug #27862: Function return incorrect SQL_COLUMN_SIZE
*/
ODBC_TEST(t_bug27862_1)
{
  SQLHDBC  hdbc1;
  SQLHSTMT hstmt1;
  SQLLEN   len;

  AllocEnvConn(&Env, &hdbc1);
  hstmt1= ConnectWithCharset(&hdbc1, "latin1", NULL); /* We need to make sure that the charset used for connection is not multibyte */

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_bug27862");
  OK_SIMPLE_STMT(hstmt1, "CREATE TABLE t_bug27862 (a VARCHAR(2), b VARCHAR(2)) charset latin1");
  OK_SIMPLE_STMT(hstmt1, "INSERT INTO t_bug27862 VALUES ('a','b')");

  OK_SIMPLE_STMT(hstmt1, "SELECT CONCAT(a,b) FROM t_bug27862");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, hstmt1, SQLColAttribute(hstmt1, 1, SQL_DESC_DISPLAY_SIZE, NULL, 0,
                                 NULL, &len));
  is_num(len, 4);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, hstmt1, SQLColAttribute(hstmt1, 1, SQL_DESC_LENGTH, NULL, 0,
                                 NULL, &len));
  is_num(len, 4);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, hstmt1, SQLColAttribute(hstmt1, 1, SQL_DESC_OCTET_LENGTH, NULL, 0,
                                 NULL, &len));
  /* Octet length should *not* include terminanting null character according to ODBC specs. This check may fail if multibyte charset is used for connection */
  is_num(len, 4);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug27862");

  return OK;
}


/**
 Because integers are given the charset 63 (binary) when they are
 used as strings, functions like CONCAT() return a binary string.
 This is a server bug that we do not try to work around.
*/
ODBC_TEST(t_bug27862_2)
{
  SQLLEN len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug27862");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug27862 (c DATE, d INT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug27862 VALUES ('2007-01-13',5)");

  OK_SIMPLE_STMT(Stmt, "SELECT CONCAT_WS(' - ', DATE_FORMAT(c, '%b-%d-%y'), d) "
         "FROM t_bug27862");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_DISPLAY_SIZE, NULL, 0,
                                 NULL, &len));
  is_num(len, 52);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_LENGTH, NULL, 0,
                                 NULL, &len));
  is_num(len, 13);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_OCTET_LENGTH, NULL, 0,
                                 NULL, &len));
  is_num(len, 14);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug27862");

  return OK;
}


/**
  SQL_DESC_FIXED_PREC_SCALE was wrong for new DECIMAL types.
*/
ODBC_TEST(decimal_scale)
{
  SQLLEN fixed= SQL_FALSE;
  SQLLEN prec, scale;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_decscale");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_decscale (a DECIMAL(5,3))");

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_decscale");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_FIXED_PREC_SCALE,
                                 NULL, 0, NULL, &fixed));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_PRECISION,
                                 NULL, 0, NULL, &prec));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_SCALE,
                                 NULL, 0, NULL, &scale));

  is_num(fixed, SQL_FALSE);
  is_num(prec, 5);
  is_num(scale, 3);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_decscale");
  return OK;
}


/**
  Wrong value returned for SQL_DESC_LITERAL_SUFFIX for binary field.
*/
ODBC_TEST(binary_suffix)
{
  SQLCHAR suffix[10];
  SQLSMALLINT len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_binarysuffix");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_binarysuffix (a BINARY(10))");

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_binarysuffix");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_LITERAL_SUFFIX,
                                 suffix, 10, &len, NULL));

  is_num(len, 0);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_binarysuffix");
  return OK;
}


/**
  Wrong value returned for SQL_DESC_SCALE for float and double.
*/
ODBC_TEST(float_scale)
{
  SQLLEN scale;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_floatscale");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_floatscale(a FLOAT, b DOUBLE, c DECIMAL(3,2))");

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_floatscale");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_SCALE,
                                 NULL, 0, NULL, &scale));

  is_num(scale, 0);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 2, SQL_DESC_SCALE,
                                 NULL, 0, NULL, &scale));

  is_num(scale, 0);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 3, SQL_DESC_SCALE,
                                 NULL, 0, NULL, &scale));

  is_num(scale, 2);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_floatscale");
  return OK;
}


/**
  Test the BIT type, which has different behavior for BIT(1) and BIT(n > 1).
*/
ODBC_TEST(bit)
{
  SQLCHAR col[10];
  SQLLEN type, len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bit");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bit (a BIT(1), b BIT(17))");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                            (SQLCHAR *)"t_bit", SQL_NTS, NULL, 0));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, col, 4), "a", 1);
  is_num(my_fetch_int(Stmt, 5), SQL_BIT); /* DATA_TYPE */
  is_num(my_fetch_int(Stmt, 7), 1); /* COLUMN_SIZE */
  is_num(my_fetch_int(Stmt, 8), 1); /* BUFFER_LENGTH */
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 16, SQL_C_LONG, &type, 0, &len));
  is_num(len, SQL_NULL_DATA); /* CHAR_OCTET_LENGTH */

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, col, 4), "b", 1);
  is_num(my_fetch_int(Stmt, 5), SQL_BINARY); /* DATA_TYPE */
  is_num(my_fetch_int(Stmt, 7), 3); /* COLUMN_SIZE */
  is_num(my_fetch_int(Stmt, 8), 3); /* BUFFER_LENGTH */
  is_num(my_fetch_int(Stmt, 16), 3); /* CHAR_OCTET_LENGTH */

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected EOF");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bit");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 1, SQL_DESC_TYPE, NULL, 0, NULL,
                                 &type));
  is_num(type, SQL_BIT);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLColAttribute(Stmt, 2, SQL_DESC_TYPE, NULL, 0, NULL,
                                 &type));
  is_num(type, SQL_BINARY);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bit");
  return OK;
}


/**
 Bug #32171: ODBC Driver incorrectly parses large Unsigned Integers
*/
ODBC_TEST(t_bug32171)
{
  SQLUINTEGER in= 4255080020UL, out;
  SQLCHAR buff[128];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug32171");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug32171 (a INT UNSIGNED)");

  sprintf((char *)buff, "INSERT INTO t_bug32171 VALUES ('%u')", in);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecDirect(Stmt, buff, SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug32171");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindCol(Stmt, 1, SQL_C_ULONG, &out, 0, NULL));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  is_num(out, in);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected EOF");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug32171");

  return OK;
}


/** Test passing an SQL_C_CHAR to a SQL_WCHAR field. */
ODBC_TEST(sqlwchar)
{
  /* Note: this is an SQLCHAR, so it is 'ANSI' data. */
  SQLCHAR data[]= "S\xe3o Paulo", buff[30];
  SQLWCHAR wbuff[30]= {0};
  SQLWCHAR wcdata[]= {'S','\x00e3', 'o', 'P', 'a', 'o', 'l', 'o'};

  diag((const char*)data);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_sqlwchar");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_sqlwchar (a VARCHAR(30)) DEFAULT CHARSET utf8");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLPrepare(Stmt, (SQLCHAR *)
                            "INSERT INTO t_sqlwchar VALUES (?)", SQL_NTS));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_WVARCHAR, 0, 0, data, sizeof(data),
                                  NULL));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecute(Stmt));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR,
                                  SQL_WVARCHAR, 0, 0, wcdata,
                                  sizeof(wcdata), NULL));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecute(Stmt));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT HEX(a) FROM t_sqlwchar");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 1), "53C3A36F205061756C6F", 20);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 1), "53C3A36F205061756C6F", 20);
  
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected EOF");
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT a FROM t_sqlwchar");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 1), data, sizeof(data));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 1), data, sizeof(data));

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected EOF");
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "SELECT a FROM t_sqlwchar");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  IS_WSTR(my_fetch_wstr(Stmt, wbuff, 1, 30), wcdata, 9);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  IS_WSTR(my_fetch_wstr(Stmt, wbuff, 1, 30), wcdata, 9);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected EOF"); 
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_sqlwchar");
  return OK;
}


/*
   This is from the canonical example of retrieving a SQL_NUMERIC_STRUCT:
   http://support.microsoft.com/default.aspx/kb/222831
*/
ODBC_TEST(t_sqlnum_msdn)
{
  SQLHANDLE ard;
  SQL_NUMERIC_STRUCT *sqlnum= malloc(sizeof(SQL_NUMERIC_STRUCT));
  SQLCHAR exp_data[SQL_MAX_NUMERIC_LEN]=
          {0x7c, 0x62, 0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  sqlnum->sign= sqlnum->precision= sqlnum->scale= (SQLCHAR)128;

  OK_SIMPLE_STMT(Stmt, "select 25.212");
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC, &ard, 0, NULL));

  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_TYPE,
                               (SQLPOINTER) SQL_C_NUMERIC, SQL_IS_INTEGER));
  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_SCALE,
                               (SQLPOINTER) 3, SQL_IS_INTEGER));
  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_DATA_PTR,
                               sqlnum, SQL_IS_POINTER));  

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  is_num(sqlnum->sign, 1);
  is_num(sqlnum->precision, 38);
  is_num(sqlnum->scale, 3);
  IS(!memcmp(sqlnum->val, exp_data, SQL_MAX_NUMERIC_LEN));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  free(sqlnum);
  return OK;
}


/**
  Internal function to test retrieving a SQL_NUMERIC_STRUCT value.

  @todo Printing some additional output (sqlnum->val as hex, dec)

  @param[in]  hstmt       Statement handle
  @param[in]  numstr      String to retrieve as SQL_NUMERIC_STRUCT
  @param[in]  prec        Precision to retrieve
  @param[in]  scale       Scale to retrieve
  @param[in]  sign        Expected sign (1=+,0=-)
  @param[in]  expdata     Expected byte array value (need this or expnum)
  @param[in]  expnum      Expected numeric value (if it fits)
  @param[in]  overflow    Whether to expect a retrieval failure (22003)
  @return OK/FAIL just like a test.
*/
int sqlnum_test_from_str(SQLHANDLE Stmt,
                         const char *numstr, SQLCHAR prec, SQLSCHAR scale,
                         SQLCHAR sign, SQLCHAR *expdata, int expnum,
                         int overflow)
{
  SQL_NUMERIC_STRUCT *sqlnum= malloc(sizeof(SQL_NUMERIC_STRUCT));
  SQLCHAR buf[512];
  SQLHANDLE ard;
  unsigned long numval;

  sprintf((char *)buf, "select %s", numstr);
  /* OK_SIMPLE_STMT(Stmt, buf); */
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecDirect(Stmt, buf, SQL_NTS));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC, &ard, 0, NULL));

  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_TYPE,
                               (SQLPOINTER) SQL_C_NUMERIC, SQL_IS_INTEGER));
  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_PRECISION,
                               (SQLPOINTER)(SQLLEN) prec, SQL_IS_INTEGER));
  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_SCALE,
                               (SQLPOINTER)(SQLLEN) scale, SQL_IS_INTEGER));
  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_DATA_PTR,
                               sqlnum, SQL_IS_POINTER));

  if (overflow)
  {
    FAIL_IF(SQLFetch(Stmt) != SQL_ERROR, "expected SQL_ERROR");
    CHECK_SQLSTATE(Stmt, "22003");
  }
  else
    CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  is_num(sqlnum->precision, prec);
  is_num(sqlnum->scale, scale);
  is_num(sqlnum->sign, sign);
  if (expdata)
  {
    IS(!memcmp(sqlnum->val, expdata, SQL_MAX_NUMERIC_LEN));
  }
  else
  {
    /* only use this for <=32bit values */
    int i;
    numval= 0;
    for (i= 0; i < 8; ++i)
      numval += sqlnum->val[7 - i] << (8 * (7 - i));
    
    if (numval != expnum)
      diag("compare %d %d", numval, expnum);
    is_num(numval, expnum);
  }

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  free(sqlnum);
  return OK;
}


/*
   Testing of retrieving SQL_NUMERIC_STRUCT

   Make sure to use little-endian in SQLCHAR arrays.
*/
ODBC_TEST(t_sqlnum_from_str)
{
  char *num1= "25.212";
  char *num2= "-101234.0010";
  char *num3= "-101230.0010";

  diag("rounding not implemented yet");
  return SKIP;

  /* some basic tests including min-precision and scale changes */
  IS(sqlnum_test_from_str(Stmt, num1, 5, 3, 1, NULL, 25212, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num1, 4, 3, 1, NULL, 0, 1) == OK);
  IS(sqlnum_test_from_str(Stmt, num1, 4, 2, 1, NULL, 2521, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num1, 2, 0, 1, NULL, 25, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num1, 2, -1, 1, NULL, 0, 1) == OK);

  /* more comprehensive testing of scale and precision */
  {SQLCHAR expdata[]= {0x2a, 0x15, 0x57, 0x3c, 0,0,0,0,0,0,0,0,0,0,0,0};
//   IS(sqlnum_test_from_str(Stmt, num2, 9, 4, 0, expdata, 0, 0) == OK);
//  IS(sqlnum_test_from_str(Stmt, num2, 9, 4, 0, NULL, 1012340010, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num2, 9, 3, 0, NULL, 101234001, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num2, 8, 3, 0, NULL, 0, 1) == OK);
  IS(sqlnum_test_from_str(Stmt, num2, 8, 2, 0, NULL, 10123400, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num2, 7, 2, 0, NULL, 10123400, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num2, 6, 2, 0, NULL, 10123400, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num2, 6, 1, 0, NULL, 1012340, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num2, 6, 0, 0, NULL, 101234, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num2, 6, -1, 0, NULL, 0, 1) == OK);}

  IS(sqlnum_test_from_str(Stmt, num3, 6, -1, 0, NULL, 101230, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, num3, 5, -1, 0, NULL, 10123, 0) == OK);

  /* Bug#35920 */
  IS(sqlnum_test_from_str(Stmt, "8000.00", 30, 2, 1, NULL, 800000, 0) == OK);
  IS(sqlnum_test_from_str(Stmt, "1234567.00", 30, 2, 1, NULL, 123456700, 0) == OK);

  /* some larger numbers */
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "1234456789", 10, 0, 1, expdata, 0, 0));}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0x7c, 0x62, 0,0, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "25.212", 5, 3, 1, expdata, 0, 0));}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0xaa, 0x86, 0x1, 0, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "10.0010", 6, 4, 1, expdata, 0, 0));}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0x2a, 0x15, 0x57, 0x3c, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "-101234.0010", 10, 4, 0, expdata, 0, 0));}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0x72, 0x8b, 0x1, 0, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "101234", 6, 0, 1, expdata, 0, 0));}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0x97, 0x03, 0x7C, 0xE3, 0x76, 0x5E, 0xF0, 0x00, 0x24, 0x1A, 0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "123445678999123445678999", 24, 0, 1, expdata, 0, 0));}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0x95, 0xFA, 0x0B, 0xF1, 0xED, 0x3C, 0x7C, 0xE4, 0x1B, 0x5F, 0x80, 0x1A, 0x16, 0x06, 0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "123445678999123445678999543216789", 33, 0, 1, expdata, 0, 0));}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "12344567899912344567899954321678909876543212", 44, 0, 1, expdata, 0, 1));}
  /* overflow with dec pt after the overflow */
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "1234456789991234456789995432167890987654321.2", 44, 1, 1, expdata, 0, 1));}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
   IS(OK == sqlnum_test_from_str(Stmt, "340282366920938463463374607431768211455", 39, 0, 1, expdata, 0, 0)); /* MAX */}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "340282366920938463463374607431768211456", 39, 0, 1, expdata, 0, 1)); /* MAX+1 */}
  {SQLCHAR expdata[SQL_MAX_NUMERIC_LEN]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_from_str(Stmt, "0", 1, 0, 1, expdata, 0, 0));}

  return OK;
}


/*
   Basic test of binding a SQL_NUMERIC_STRUCT as a query parameter
*/
ODBC_TEST(t_bindsqlnum_basic)
{
  SQL_NUMERIC_STRUCT *sqlnum= malloc(sizeof(SQL_NUMERIC_STRUCT));
  SQLCHAR outstr[20];
  memset(sqlnum, 0, sizeof(SQL_NUMERIC_STRUCT));

  sqlnum->sign= 1;
  sqlnum->scale= 3;
  sqlnum->val[0]= 0x7c;
  sqlnum->val[1]= 0x62;

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLPrepare(Stmt, (SQLCHAR *)"select ?", SQL_NTS));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_NUMERIC,
                                  SQL_DECIMAL, 5, 3,
                                  sqlnum, 0, NULL));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecute(Stmt));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, outstr, 20, NULL));
  IS_STR(outstr, "25.212", 6);
  is_num(sqlnum->sign, 1);
  is_num(sqlnum->precision, 5);
  is_num(sqlnum->scale, 3);

  free(sqlnum);

  return OK;
}


/*
   Test of binding a SQL_NUMERIC_STRUCT as a query parameter
   with more precision than is really needed
*/
ODBC_TEST(t_bindsqlnum_wide)
{
  SQL_NUMERIC_STRUCT sqlnum;
  SQLCHAR outstr[20];
  memset(sqlnum.val, 0, SQL_MAX_NUMERIC_LEN);

  sqlnum.sign= 1;
  sqlnum.scale= 3;
  sqlnum.val[0]= 0x7c;
  sqlnum.val[1]= 0x62;

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLPrepare(Stmt, (SQLCHAR *)"select ?", SQL_NTS));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_NUMERIC,
                                  SQL_DECIMAL, 15, 3,
                                  &sqlnum, 0, NULL));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecute(Stmt));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, outstr, 20, NULL));
  IS_STR(outstr, "25.212", 6);
  is_num(sqlnum.sign, 1);
  is_num(sqlnum.precision, 15);
  is_num(sqlnum.scale, 3);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  sqlnum.sign= 0;
  sqlnum.scale= 3;
  sqlnum.val[0]= 0x7c;
  sqlnum.val[1]= 0x62;

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLExecute(Stmt));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, outstr, 20, NULL));
  IS_STR(outstr, "-25.212", 6);
  is_num(sqlnum.sign, 0);
  is_num(sqlnum.precision, 15);
  is_num(sqlnum.scale, 3);

  return OK;
}


/**
  Internal function to test sending a SQL_NUMERIC_STRUCT value.

  @todo Printing some additional output (sqlnum->val as hex, dec)

  @param[in]  hstmt       Statement handle
  @param[in]  numdata     Numeric data
  @param[in]  prec        Precision to send
  @param[in]  scale       Scale to send
  @param[in]  sign        Sign (1=+,0=-)
  @param[in]  outstr      Expected result string
  @param[in]  exptrunc    Expected truncation failure
  @return OK/FAIL just like a test.
*/
int sqlnum_test_to_str(SQLHANDLE Stmt, SQLCHAR *numdata, SQLCHAR prec,
                       SQLSCHAR scale, SQLCHAR sign, char *outstr,
                       char *exptrunc)
{
  SQL_NUMERIC_STRUCT *sqlnum= malloc(sizeof(SQL_NUMERIC_STRUCT));
  SQLCHAR obuf[30];
  SQLRETURN exprc= SQL_SUCCESS;

  /* TODO until sqlnum errors are supported */
  /*
  if (!strcmp("01S07", exptrunc))
    exprc= SQL_SUCCESS_WITH_INFO;
  else if (!strcmp("22003", exptrunc))
    exprc= SQL_ERROR;
  */

  sqlnum->sign= sign;
  memcpy(sqlnum->val, numdata, SQL_MAX_NUMERIC_LEN);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_NUMERIC,
                                  SQL_DECIMAL, prec, scale, sqlnum, 0, NULL));

  OK_SIMPLE_STMT(Stmt, "select ?");

  exprc= SQLFetch(Stmt);
  if (exprc != SQL_SUCCESS)
  {
    CHECK_SQLSTATE(Stmt, (char *)exptrunc);
  }
  if (exprc == SQL_ERROR)
    return OK;
  is_num(sqlnum->precision, prec);
  is_num(sqlnum->scale, scale);
  is_num(sqlnum->sign, sign);
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, obuf, sizeof(obuf), NULL));
  IS_STR(obuf, outstr, strlen(outstr));
  FAIL_IF(memcmp(sqlnum->val, numdata, SQL_MAX_NUMERIC_LEN), "memcmp failed");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  free(sqlnum);
  return OK;
}


/*
   Testing of passing SQL_NUMERIC_STRUCT as query parameters
*/
ODBC_TEST(t_sqlnum_to_str)
{
  {SQLCHAR numdata[]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_to_str(Stmt, numdata, 10, 4, 1, "123445.6789", ""));}

  /* fractional truncation */
  {SQLCHAR numdata[]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_to_str(Stmt, numdata, 9, 2, 1, "12344567.8", "01S07"));}
  {SQLCHAR numdata[]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_to_str(Stmt, numdata, 8, 2, 1, "12344567", "01S07"));}

  /* whole number truncation - error */
  /* TODO need err handling for this test
  {SQLCHAR numdata[]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_to_str(Stmt, numdata, 7, 2, 1, "1234456", "22003"));}
  */

  /* negative scale */
  {SQLCHAR numdata[]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_to_str(Stmt, numdata, 10, -2, 1, "123445678900", ""));}
  {SQLCHAR numdata[]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_to_str(Stmt, numdata, 10, -2, 0, "-123445678900", ""));}

  /* scale > prec */
  {SQLCHAR numdata[]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_to_str(Stmt, numdata, 10, 11, 1, "0.01234456789", ""));}
  {SQLCHAR numdata[]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_to_str(Stmt, numdata, 10, 11, 0, "-0.01234456789", ""));}
  {SQLCHAR numdata[]= {0xD5, 0x50, 0x94, 0x49, 0,0,0,0,0,0,0,0,0,0,0,0};
   IS(OK == sqlnum_test_to_str(Stmt, numdata, 10, 20, 1, "0.00000000001234456789", ""));}

  return OK;
}


/*
  Bug #31220 - SQLFetch or SQLFetchScroll returns negative data length
               when using SQL_C_WCHAR
*/
ODBC_TEST(t_bug31220)
{
  SQLLEN outlen= 999;
  SQLWCHAR outbuf[5];

  /* the call sequence of this test is not allowed under a driver manager */
  if (using_dm(Connection))
    return OK;

  OK_SIMPLE_STMT(Stmt, "select 1");
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindCol(Stmt, 1, 999 /* unknown type */,
                            outbuf, 5, &outlen));
  FAIL_IF(SQLFetch(Stmt) != SQL_ERROR, "SQL_ERROR expected");
  CHECK_SQLSTATE(Stmt, "07006");
  is_num(outlen, 999);
  return OK;  
}


/**
  Bug #29402: field type charset 63 problem
*/
ODBC_TEST(t_bug29402)
{
  SQLSMALLINT name_length, data_type, decimal_digits, nullable;
  SQLCHAR     column_name[SQL_MAX_COLUMN_NAME_LEN];
  SQLULEN     column_size;
  //SQLCHAR     buf[80]= {0};
  SQLTCHAR    wbuf[80];
  SQLLEN      buflen= 0;
  SQLHDBC     hdbc1;
  SQLHSTMT    hstmt1;
  const SQLCHAR *expected= (const SQLCHAR*)"\x80""100";

  IS(AllocEnvConn(&Env, &hdbc1));

  /* We don't have NO_BINARY_RESULT option, and not clear atm if we need it */
  hstmt1= ConnectWithCharset(&hdbc1, "cp1250", NULL);

  FAIL_IF(hstmt1 == NULL, "");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, hstmt1, SQLExecDirectW(hstmt1, CW("SELECT CONCAT(_cp1250 0x80, 100) concated"), SQL_NTS));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, hstmt1, SQLDescribeCol(hstmt1, 1, column_name, sizeof(column_name),
                                &name_length, &data_type, &column_size,
                                &decimal_digits, &nullable));

  IS(data_type == SQL_VARCHAR || data_type == SQL_WVARCHAR);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, hstmt1, SQLFetch(hstmt1));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, hstmt1, SQLGetData(hstmt1, 1, SQL_C_TCHAR, wbuf, sizeof(wbuf), &buflen));

  is_num(buflen, 4);

  if (strncmp((const char*)wbuf, (const char*)expected, buflen) != 0)
  {
    /* Because of this
       http://msdn.microsoft.com/en-us/library/ms716540%28v=vs.85%29.aspx
       test can fail. Rather test problem.
       Upd: Hopefully the test is fixed, but keeping this message so far */
    diag("%s != %s(%#x!=%#x) - this test may fail on some "
                 "platforms - TODO", wbuf, "\x80""100", wbuf[0], expected[0]);
    return FAIL;
  }

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_HANDLE_RC(SQL_HANDLE_DBC, hdbc1, SQLDisconnect(hdbc1));
  CHECK_HANDLE_RC(SQL_HANDLE_DBC, hdbc1, SQLFreeConnect(hdbc1));

#ifdef FLAG_NO_BINARY_RESULT
  /* Check without FLAG_NO_BINARY_RESULT */
  OK_SIMPLE_STMT(Stmt, "SELECT CONCAT('\x80', 100) concated");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLDescribeCol(Stmt, 1, column_name, sizeof(column_name),
                                &name_length, &data_type, &column_size,
                                &decimal_digits, &nullable));

  /* Depending on server default charset it can be either SQL_VARCHAR or
      SQL_WVARCHAR. Wee are fine to know if the data_type is one of those */
  if(data_type != SQL_VARCHAR && data_type != SQL_WVARCHAR)
  {
    return FAIL;
  }
#endif

  return OK;
}


/*
   Test that SQL_C_NUMERIC returns scale correctly when it is
   different to the requested scale.
   NB: We now always return numeric with requested scale. Thus the testcase is rather redundant.
       But I am leaving it in place, with according changes.
 */ 
ODBC_TEST(t_sqlnum_truncate)
{
  SQLHANDLE ard;
  SQL_NUMERIC_STRUCT sqlnum;
  SQLCHAR exp_data[SQL_MAX_NUMERIC_LEN]=
          {0x60, 0xb4, 0x80, 1,0,0,0,0,0,0,0,0,0,0,0,0};

  sqlnum.sign= sqlnum.precision= sqlnum.scale= (SQLCHAR)128;

  OK_SIMPLE_STMT(Stmt, "select 25.212");
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC, &ard, 0, NULL));

  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_TYPE,
                               (SQLPOINTER) SQL_C_NUMERIC, SQL_IS_INTEGER));
  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_SCALE,
                               (SQLPOINTER) 6, SQL_IS_INTEGER));
  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_DATA_PTR,
                               &sqlnum, SQL_IS_POINTER));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  is_num(sqlnum.sign, 1);
  is_num(sqlnum.precision, 38);
  is_num(sqlnum.scale, 6);
  IS(!memcmp(sqlnum.val, exp_data, SQL_MAX_NUMERIC_LEN));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "select 25.212000");
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC, &ard, 0, NULL));

  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_TYPE,
                               (SQLPOINTER) SQL_C_NUMERIC, SQL_IS_INTEGER));
  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_SCALE,
                               (SQLPOINTER) 6, SQL_IS_INTEGER));
  CHECK_HANDLE_RC(SQL_HANDLE_DESC, ard, SQLSetDescField(ard, 1, SQL_DESC_DATA_PTR,
                               &sqlnum, SQL_IS_POINTER));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  is_num(sqlnum.sign, 1);
  is_num(sqlnum.precision, 38);
  is_num(sqlnum.scale, 6);
  IS(!memcmp(sqlnum.val, exp_data, SQL_MAX_NUMERIC_LEN));

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}

/* ODBC-158 QUery with some of aggregate functions returns error on execution. The reason is that those functions type is
            MYSQL_TYPE_LONGLONG, Access gets them as SQL_C_LONG, and connector returned data length 8.
            Checking also if correct length returned is the opposite case - field is smaller than the buffer */
ODBC_TEST(t_odbc158)
{
  SQLINTEGER Val;
  SQLLEN     Len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc158");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc158(bi bigint not null, si smallint not null)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc158(bi, si) VALUES(1, 2)");

  OK_SIMPLE_STMT(Stmt, "SELECT bi, si FROM t_odbc158");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_LONG, &Val, 0, &Len));
  is_num(Len, 4);
  is_num(Val, 1);

  Len= 0;

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_LONG, &Val, 0, &Len));
  is_num(Len, 4);
  is_num(Val, 2);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc158");

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {t_longlong1,        "t_longlong1",       NORMAL},
  {t_decimal,          "t_decimal",         NORMAL},
  {t_bigint,           "t_bigint",          NORMAL},
  {t_enumset,          "t_enumset",         NORMAL},
  {t_bug16917,         "t_bug16917",        NORMAL},
  {t_bug16235,         "t_bug16235",        NORMAL},
  {t_bug27862_1,       "t_bug27862_1",      NORMAL},
  {t_bug27862_2,       "t_bug27862_2",      KNOWN_FAILURE},
  {decimal_scale,      "decimal_scale",     NORMAL},
  {binary_suffix,      "binary_suffix",     NORMAL},
  {float_scale,        "float_scale",       NORMAL},
  {bit,                "bit",               NORMAL},
  {t_bug32171,         "t_bug32171",        NORMAL},
  {sqlwchar,           "sqlwchar",          KNOWN_FAILURE},
  {t_sqlnum_msdn,      "t_sqlnum_msdn",     NORMAL},
  {t_sqlnum_from_str,  "t_sqlnum_from_str", NORMAL},
  {t_bindsqlnum_basic, "t_bindsqlnum_basic",NORMAL},
  {t_bindsqlnum_wide,  "t_bindsqlnum_wide", NORMAL},
  {t_sqlnum_to_str,    "t_sqlnum_to_str",   NORMAL},
  {t_bug31220,         "t_bug31220",        NORMAL},
  {t_bug29402,         "t_bug29402",        NORMAL},
  {t_sqlnum_truncate,  "t_sqlnum_truncate", NORMAL},
  {t_odbc158,          "odbc158_bigintcolumn_as_c_long", NORMAL},
  {NULL, NULL, NORMAL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
