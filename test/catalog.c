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

ODBC_TEST(my_columns_null)
{
  SQLLEN rowCount= 0;
  /* initialize data */
  OK_SIMPLE_STMT(Stmt, "drop table if exists my_column_null");

  OK_SIMPLE_STMT(Stmt, "create table my_column_null(id int not null, name varchar(30))");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
    (SQLCHAR *)"my_column_null", SQL_NTS,
    NULL, SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowCount));

  is_num(rowCount, 2);

  is_num(2, my_print_non_format_result(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS my_column_null");

  return OK;
}


ODBC_TEST(my_drop_table)
{
  OK_SIMPLE_STMT(Stmt, "drop table if exists my_drop_table");
  OK_SIMPLE_STMT(Stmt, "create table my_drop_table(id int not null)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                            (SQLCHAR *)"my_drop_table", SQL_NTS, NULL, 0));

  is_num(1, my_print_non_format_result(Stmt));

  OK_SIMPLE_STMT(Stmt, "drop table my_drop_table");

  return OK;
}


#define TODBC_BIND_CHAR(n,buf) SQLBindCol(Stmt,n,SQL_C_CHAR,&buf,sizeof(buf),NULL);


ODBC_TEST(my_table_dbs)
{
    SQLCHAR    database[100];
    SQLRETURN  rc;
    SQLINTEGER nrows= 0 ;
    SQLLEN lenOrNull, rowCount= 0;

    diag("fix me");
    return SKIP;
    
    OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS my_all_db_test1");
    OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS my_all_db_test2");
    OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS my_all_db_test3");
    OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS my_all_db_test4");

    /* This call caused problems when database names returned as '%' */
    CHECK_STMT_RC(Stmt, SQLTables(Stmt,(SQLCHAR*)SQL_ALL_CATALOGS,1,NULL,0,NULL,0,NULL,0));

    while (SQLFetch(Stmt) == SQL_SUCCESS)
    {
      ++nrows;
      CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, database,
                               sizeof(database), NULL));
      /* the table catalog in the results must not be '%' */
      FAIL_IF(database[0] == '%', "table catalog can't be '%'");
    }
    /* we should have got rows... */
    FAIL_IF(nrows=0, "nrows should be > 0");

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

    CHECK_STMT_RC(Stmt, SQLTables(Stmt,(SQLCHAR *)SQL_ALL_CATALOGS,1,"",0,"",0,NULL,0));

    /* Added calls to SQLRowCount just to have tests of it with SQLTAbles. */
    CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowCount));
    nrows = my_print_non_format_result(Stmt);

    is_num(rowCount, nrows)
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLTables(Stmt,(SQLCHAR *)SQL_ALL_CATALOGS,SQL_NTS,"",0,"",0,
                   NULL,0);
    CHECK_STMT_RC(Stmt,rc);

    is_num(nrows, my_print_non_format_result(Stmt));
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLTables(Stmt,(SQLCHAR *)my_schema,4,NULL,0,NULL,0,NULL,0);
    CHECK_STMT_RC(Stmt,rc);

    CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowCount));
    is_num(rowCount, my_print_non_format_result(Stmt));

    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    /* test fails on Win2003 x86 w/DM if len=5, SQL_NTS is used instead */
    rc = SQLTables(Stmt,(SQLCHAR *)"mysql",SQL_NTS,NULL,0,NULL,0,NULL,0);
    CHECK_STMT_RC(Stmt,rc);

    FAIL_IF(my_print_non_format_result(Stmt) == 0, "0");
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLTables(Stmt,(SQLCHAR *)"%",1,"",0,"",0,NULL,0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    memset(database,0,100);
    rc = SQLGetData(Stmt,1,SQL_C_CHAR,database,100,NULL);
    CHECK_STMT_RC(Stmt,rc);
    diag("catalog: %s", database);

    memset(database,0,100);
    rc = SQLGetData(Stmt,2,SQL_C_CHAR,database,100,&lenOrNull);
    CHECK_STMT_RC(Stmt,rc);
    diag("schema: %s", database);
    IS(lenOrNull == SQL_NULL_DATA);

    memset(database,0,100);
    rc = SQLGetData(Stmt,3,SQL_C_CHAR,database,100,&lenOrNull);
    CHECK_STMT_RC(Stmt,rc);
    diag("table: %s", database);
;

    memset(database,0,100);
    rc = SQLGetData(Stmt,4,SQL_C_CHAR,database,100,&lenOrNull);
    CHECK_STMT_RC(Stmt,rc);
    diag("type: %s", database);


    memset(database,0,100);
    rc = SQLGetData(Stmt,5,SQL_C_CHAR, database,100,&lenOrNull);
    CHECK_STMT_RC(Stmt,rc);
    diag("database remark: %s", database);

    SQLFreeStmt(Stmt,SQL_UNBIND);
    SQLFreeStmt(Stmt,SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "CREATE DATABASE my_all_db_test1");
    OK_SIMPLE_STMT(Stmt, "CREATE DATABASE my_all_db_test2");
    OK_SIMPLE_STMT(Stmt, "CREATE DATABASE my_all_db_test3");
    OK_SIMPLE_STMT(Stmt, "CREATE DATABASE my_all_db_test4");

    rc = SQLTables(Stmt, (SQLCHAR *)SQL_ALL_CATALOGS, 1, "", 0, "", 0, "", 0);
    CHECK_STMT_RC(Stmt,rc);

    nrows += 4;
    is_num(nrows, my_print_non_format_result(Stmt));
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLTables(Stmt,(SQLCHAR *)SQL_ALL_CATALOGS, SQL_NTS,
                   "", 0, "", 0, "", 0);
    CHECK_STMT_RC(Stmt,rc);

    is_num(my_print_non_format_result(Stmt), nrows);
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLTables(Stmt, (SQLCHAR *)"my_all_db_test", SQL_NTS,
                   "", 0, "", 0, "", 0);
    CHECK_STMT_RC(Stmt,rc);

    CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowCount));
    is_num(rowCount, 0);

    is_num(my_print_non_format_result(Stmt), 0);
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLTables(Stmt, (SQLCHAR *)"my_all_db_test%", SQL_NTS,
                   "", 0, "", 0, NULL, 0);
    CHECK_STMT_RC(Stmt,rc);
    CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowCount));
    is_num(my_print_non_format_result(Stmt), 4);
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    /* unknown table should be empty */
    rc = SQLTables(Stmt, (SQLCHAR *)"my_all_db_test%", SQL_NTS,
                   NULL, 0, (SQLCHAR *)"xyz", SQL_NTS, NULL, 0);
    CHECK_STMT_RC(Stmt,rc);

    is_num(my_print_non_format_result(Stmt), 0);
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE my_all_db_test1");
  OK_SIMPLE_STMT(Stmt, "DROP DATABASE my_all_db_test2");
  OK_SIMPLE_STMT(Stmt, "DROP DATABASE my_all_db_test3");
  OK_SIMPLE_STMT(Stmt, "DROP DATABASE my_all_db_test4");

  return OK;
}


ODBC_TEST(my_colpriv)
{
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS test_colprev1");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS test_colprev2");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS test_colprev3");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE test_colprev1(a INT,b INT,c INT, d INT)");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE test_colprev2(a INT,b INT,c INT, d INT)");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE test_colprev3(a INT,b INT,c INT, d INT)");

  (void)SQLExecDirect(Stmt, (SQLCHAR *)"DROP USER my_colpriv", SQL_NTS);
  OK_SIMPLE_STMT(Stmt, "CREATE USER my_colpriv");

  OK_SIMPLE_STMT(Stmt, "GRANT SELECT(a,b),INSERT(d),UPDATE(c) ON test_colprev1 TO my_colpriv");
  OK_SIMPLE_STMT(Stmt, "GRANT SELECT(c,a),UPDATE(a,b) ON test_colprev3 TO my_colpriv");

  OK_SIMPLE_STMT(Stmt, "FLUSH PRIVILEGES");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLColumnPrivileges(Stmt,
                                     NULL, SQL_NTS, NULL, SQL_NTS,
                                     (SQLCHAR *)"test_colprev1", SQL_NTS,
                                     (SQLCHAR *)/*NULL*/"%", SQL_NTS));

  diag("1) Privileges on all columns from test_colprev1");
  is_num(4, my_print_non_format_result(Stmt));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLColumnPrivileges(Stmt,
                                     NULL, SQL_NTS, NULL, SQL_NTS,
                                     (SQLCHAR *)"test_colprev1", SQL_NTS,
                                     (SQLCHAR *)"a", SQL_NTS));

  diag("2) Privileges on column 'a' from test_colprev1");
  is_num(my_print_non_format_result(Stmt), 1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLColumnPrivileges(Stmt,
                                     NULL, SQL_NTS, NULL, SQL_NTS,
                                     (SQLCHAR *)"test_colprev2", SQL_NTS,
                                     (SQLCHAR *)"%", SQL_NTS));

  diag("3) Privileges on all columns from test_colprev2");
  is_num(my_print_non_format_result(Stmt), 0);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLColumnPrivileges(Stmt,
                                     NULL, SQL_NTS, NULL, SQL_NTS,
                                     (SQLCHAR *)"test_colprev3", SQL_NTS,
                                     (SQLCHAR *)"%", SQL_NTS));

  diag("4) Privileges on all columns from test_colprev3");
  is_num(my_print_non_format_result(Stmt), 4);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLColumnPrivileges(Stmt,
                                     NULL, SQL_NTS, NULL, SQL_NTS,
                                     (SQLCHAR *)"test_%", SQL_NTS,
                                     (SQLCHAR *)"%", SQL_NTS));

 // is_num(my_print_non_format_result(Stmt), 0);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLColumnPrivileges(Stmt,
                                     (SQLCHAR *)"mysql", SQL_NTS, NULL, SQL_NTS,
                                     (SQLCHAR *)"columns_priv", SQL_NTS,
                                     NULL, SQL_NTS));

  my_print_non_format_result(Stmt);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP USER my_colpriv");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE test_colprev1, test_colprev2, test_colprev3");

  return OK;
}


ODBC_TEST(t_sqlprocedures)
{


  /* avoid errors in case binary log is activated */
  OK_SIMPLE_STMT(Stmt, "SET GLOBAL log_bin_trust_function_creators = 1");

  OK_SIMPLE_STMT(Stmt, "DROP FUNCTION IF EXISTS t_sqlproc_func");
  OK_SIMPLE_STMT(Stmt,
         "CREATE FUNCTION t_sqlproc_func (a INT) RETURNS INT RETURN SQRT(a)");

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS t_sqlproc_proc");
  OK_SIMPLE_STMT(Stmt,
         "CREATE PROCEDURE t_sqlproc_proc (OUT a INT) BEGIN"
         " SELECT COUNT(*) INTO a FROM t_sqlproc;"
         "END;");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Try without specifying a catalog. */
  CHECK_STMT_RC(Stmt, SQLProcedures(Stmt, NULL, 0, NULL, 0,
                               (SQLCHAR *)"t_sqlproc%", SQL_NTS));

  is_num(my_print_non_format_result(Stmt), 2);

  /* And try with specifying a catalog.  */
  CHECK_STMT_RC(Stmt, SQLProcedures(Stmt, (SQLCHAR *)my_schema, SQL_NTS, NULL, 0,
                               (SQLCHAR *)"t_sqlproc%", SQL_NTS));

  is_num(my_print_non_format_result(Stmt), 2);

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE t_sqlproc_proc");
  OK_SIMPLE_STMT(Stmt, "DROP FUNCTION t_sqlproc_func");

  return OK;
}

#define MYSQL_NAME_LEN 192

ODBC_TEST(t_catalog)
{
    SQLRETURN rc;
    SQLCHAR      name[MYSQL_NAME_LEN+1];
    SQLUSMALLINT i;
    SQLSMALLINT  ncols, len;

    SQLCHAR colnames[19][20]= {
        "TABLE_CAT","TABLE_SCHEM","TABLE_NAME","COLUMN_NAME",
        "DATA_TYPE","TYPE_NAME","COLUMN_SIZE","BUFFER_LENGTH",
        "DECIMAL_DIGITS","NUM_PREC_RADIX","NULLABLE","REMARKS",
        "COLUMN_DEF","SQL_DATA_TYPE","SQL_DATETIME_SUB",
        "CHAR_OCTET_LENGTH","ORDINAL_POSITION","IS_NULLABLE"
    };
    SQLSMALLINT collengths[18]= {
        9,11,10,11,9,9,11,13,14,14,8,7,10,13,16,17,16,11
    };

    OK_SIMPLE_STMT(Stmt, "drop table if exists t_catalog");

    OK_SIMPLE_STMT(Stmt,"create table t_catalog(abc tinyint, bcdefghijklmno char(4), uifield int unsigned not null)");

    CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                              (SQLCHAR *)"t_catalog", 9, NULL, 0));

    rc = SQLNumResultCols(Stmt, &ncols);
    CHECK_STMT_RC(Stmt,rc);

    diag("total columns: %d", ncols);
    IS(ncols == 18);
    IS(myresult(Stmt) == 3);

    SQLFreeStmt(Stmt, SQL_UNBIND);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    rc = SQLColumns(Stmt, NULL, 0, NULL, 0,
                    (SQLCHAR *)"t_catalog", 9, NULL, 0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLNumResultCols(Stmt,&ncols);
    CHECK_STMT_RC(Stmt,rc);

    for (i= 1; i <= (SQLUINTEGER) ncols; i++)
    {
        rc = SQLDescribeCol(Stmt, i, name, MYSQL_NAME_LEN+1, &len, NULL, NULL, NULL, NULL);
        CHECK_STMT_RC(Stmt,rc);

        diag("column %d: %s (%d)", i, name, len);
        is_num(len, collengths[i - 1]);
        IS_STR(name, colnames[i - 1], len);
    }
    SQLFreeStmt(Stmt,SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_catalog");

  return OK;
}


ODBC_TEST(tmysql_specialcols)
{
  OK_SIMPLE_STMT(Stmt,"drop table if exists tmysql_specialcols");
  OK_SIMPLE_STMT(Stmt,"create table tmysql_specialcols(col1 int primary key, col2 varchar(30), col3 int)");
    

  OK_SIMPLE_STMT(Stmt,"create index tmysql_ind1 on tmysql_specialcols(col1)");
    
  OK_SIMPLE_STMT(Stmt,"insert into tmysql_specialcols values(100,'venu',1)");

  OK_SIMPLE_STMT(Stmt,"insert into tmysql_specialcols values(200,'MySQL',2)");
    
  CHECK_DBC_RC(Connection, SQLTransact(NULL,Connection,SQL_COMMIT));

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt,"select * from tmysql_specialcols");

  myresult(Stmt);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt, SQL_BEST_ROWID, NULL,0, NULL,0,
                                   (SQLCHAR *)"tmysql_specialcols",SQL_NTS,
                                   SQL_SCOPE_SESSION, SQL_NULLABLE));

  myresult(Stmt);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt,"drop table tmysql_specialcols");

  CHECK_DBC_RC(Connection, SQLTransact(NULL,Connection,SQL_COMMIT));

  return OK;
}


/* To test SQLColumns misc case */
ODBC_TEST(t_columns)
{
  SQLSMALLINT   NumPrecRadix, DataType, Nullable, DecimalDigits;
  SQLLEN        cbColumnSize, cbDecimalDigits, cbNumPrecRadix,
                cbDataType, cbNullable;
  SQLINTEGER    cbDatabaseName;
  SQLUINTEGER   ColumnSize, i;
  SQLUINTEGER   ColumnCount= 7;
  SQLCHAR       ColumnName[MAX_NAME_LEN], DatabaseName[MAX_NAME_LEN];
  SQLINTEGER    Values[7][5][2]=
  {
    { {5,2},  {5,4}, {0,2},  {10,2},  {1,2}},
    { {1,2},  {5,4},  {0,-1}, {10,-1}, {1,2}},
    { {12,2}, {20,4}, {0,-1}, {10,-1}, {0,2}},
    { {3,2},  {10,4}, {2,2},  {10,2},  {1,2}},
    { {-6,2},  {3,4}, {0,2},  {10,2},  {0,2}},
    { {4,2}, {10,4}, {0,2},  {10,2},  {0,2}},
    { {-6,2}, {3,4}, {0,2},  {10,2},  {1,2}}
  };

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_columns");

  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_columns (col0 SMALLINT,"
         "col1 CHAR(5), col2 VARCHAR(20) NOT NULL, col3 DECIMAL(10,2),"
         "col4 TINYINT NOT NULL, col5 INTEGER PRIMARY KEY,"
         "col6 TINYINT NOT NULL UNIQUE AUTO_INCREMENT) CHARSET latin1");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_METADATA_ID,
                                (SQLPOINTER)SQL_FALSE, SQL_IS_UINTEGER));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG,
                                 DatabaseName, MAX_NAME_LEN,
                                 &cbDatabaseName)); /* Current Catalog */

  for (i= 0; i < ColumnCount; i++)
  {
    sprintf((char *)ColumnName, "col%d", (int)i);
    diag("checking column `%s`", (char *)ColumnName);

    CHECK_STMT_RC(Stmt, SQLColumns(Stmt,
                              DatabaseName, (SQLSMALLINT)cbDatabaseName,
                              (SQLCHAR *)"", SQL_NTS,
                              (SQLCHAR *)"t_columns", SQL_NTS,
                              ColumnName, SQL_NTS));

    /* 5 -- Data type */
    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 5, SQL_C_SSHORT, &DataType, 0,
                              &cbDataType));

    /* 7 -- Column Size */
    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 7, SQL_C_ULONG, &ColumnSize, 0,
                              &cbColumnSize));

    /* 9 -- Decimal Digits */
    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 9, SQL_C_SSHORT, &DecimalDigits, 0,
                              &cbDecimalDigits));

    /* 10 -- Num Prec Radix */
    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 10, SQL_C_SSHORT, &NumPrecRadix, 0,
                              &cbNumPrecRadix));

    /* 11 -- Nullable */
    CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 11, SQL_C_SSHORT, &Nullable, 0,
                              &cbNullable));

    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

	/* if you get -8 for col1 here - that's fine. depends on setup. the test probably needs
	   to be changed accordingly */
    is_num(DataType,   Values[i][0][0]);
    is_num(cbDataType, Values[i][0][1]);

    is_num(ColumnSize,   Values[i][1][0]);
    is_num(cbColumnSize, Values[i][1][1]);

    is_num(DecimalDigits,   Values[i][2][0]);
    is_num(cbDecimalDigits, Values[i][2][1]);

    is_num(NumPrecRadix,   Values[i][3][0]);
    is_num(cbNumPrecRadix, Values[i][3][1]);

    is_num(Nullable,   Values[i][4][0]);
    is_num(cbNullable, Values[i][4][1]);

    FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_columns");

  return OK;
}


/* Test the bug SQLTables */
typedef struct t_table_bug
{
  SQLCHAR     szColName[MAX_NAME_LEN];
  SQLSMALLINT pcbColName;
  SQLSMALLINT pfSqlType;
  SQLUINTEGER pcbColDef;
  SQLSMALLINT pibScale;
  SQLSMALLINT pfNullable;
} t_describe_col;


t_describe_col t_tables_bug_data[5] =
{
#ifdef MYODBC_UNICODEDRIVER
  {"TABLE_CAT",   9, SQL_WVARCHAR, MYSQL_NAME_LEN, 0, SQL_NO_NULLS},
  {"TABLE_SCHEM",11, SQL_WVARCHAR, MYSQL_NAME_LEN, 0, SQL_NULLABLE},
  {"TABLE_NAME", 10, SQL_WVARCHAR, MYSQL_NAME_LEN, 0, SQL_NO_NULLS},
  {"TABLE_TYPE", 10, SQL_WVARCHAR, MYSQL_NAME_LEN, 0, SQL_NO_NULLS},
  {"REMARKS",     7, SQL_WVARCHAR, MYSQL_NAME_LEN, 0, SQL_NO_NULLS}
#else
  {"TABLE_CAT",   9, SQL_VARCHAR, MYSQL_NAME_LEN, 0, SQL_NO_NULLS},
  {"TABLE_SCHEM",11, SQL_VARCHAR, MYSQL_NAME_LEN, 0, SQL_NULLABLE},
  {"TABLE_NAME", 10, SQL_VARCHAR, MYSQL_NAME_LEN, 0, SQL_NO_NULLS},
  {"TABLE_TYPE", 10, SQL_VARCHAR, MYSQL_NAME_LEN, 0, SQL_NO_NULLS},
  {"REMARKS",     7, SQL_VARCHAR, MYSQL_NAME_LEN, 0, SQL_NO_NULLS}
#endif
};


ODBC_TEST(t_tables_bug)
{
  SQLSMALLINT i, ColumnCount, pcbColName, pfSqlType, pibScale, pfNullable;
  SQLULEN     pcbColDef;
  SQLCHAR     szColName[MAX_NAME_LEN];

  diag("MariaDB ODBC Driver doesn't set column types for NULL columns");
  return SKIP;

  CHECK_STMT_RC(Stmt,  SQLTables(Stmt, NULL, 0, NULL, 0, NULL, 0,
                            (SQLCHAR *)"TABLE", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &ColumnCount));
  is_num(ColumnCount, 5);

  for (i= 1; i <= ColumnCount; ++i)
  {
    CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, (SQLUSMALLINT)i, szColName,
                                 MAX_NAME_LEN, &pcbColName, &pfSqlType,
                                 &pcbColDef, &pibScale, &pfNullable));

    fprintf(stdout, "# Column '%d':\n", i);
    fprintf(stdout, "#  Column Name   : %s\n", szColName);
    fprintf(stdout, "#  NameLengh     : %d\n", pcbColName);
    fprintf(stdout, "#  DataType      : %d\n", pfSqlType);
    fprintf(stdout, "#  ColumnSize    : %lu\n", pcbColDef);
    fprintf(stdout, "#  DecimalDigits : %d\n", pibScale);
    fprintf(stdout, "#  Nullable      : %d\n", pfNullable);

    IS_STR(t_tables_bug_data[i-1].szColName, szColName, pcbColName);
    is_num(t_tables_bug_data[i-1].pcbColName, pcbColName);
    is_num(t_tables_bug_data[i-1].pfSqlType, pfSqlType);
    /* This depends on NAME_LEN in mysql_com.h */

    is_num(t_tables_bug_data[i-1].pibScale, pibScale);
    is_num(t_tables_bug_data[i-1].pfNullable, pfNullable);
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


ODBC_TEST(t_current_catalog_unicode)
{
  SQLWCHAR    db[255];
  SQLWCHAR    cur_db[255];
  SQLRETURN   rc;
  SQLINTEGER  len;

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetConnectAttrW(Connection, SQL_ATTR_CURRENT_CATALOG, db, sizeof(db), &len);
  CHECK_DBC_RC(Connection,rc);

  is_num(len, strlen(my_schema) * sizeof(SQLWCHAR));
  is_wstr(db, LW("test"), 5);

  rc = SQLSetConnectAttrW(Connection, SQL_ATTR_CURRENT_CATALOG, db, SQL_NTS);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS test_odbc_current");

  wcscpy(cur_db, L"test_odbc_current");
  rc = SQLSetConnectAttrW(Connection, SQL_ATTR_CURRENT_CATALOG, cur_db, SQL_NTS);
  FAIL_IF(rc == SQL_SUCCESS, "Error expected");

  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE test_odbc_current");
  rc = SQLFreeStmt(Stmt,SQL_CLOSE);

  latin_as_sqlwchar("test_odbc_current", cur_db);
  rc = SQLSetConnectAttrW(Connection, SQL_ATTR_CURRENT_CATALOG, cur_db, SQL_NTS);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLGetConnectAttrW(Connection, SQL_ATTR_CURRENT_CATALOG, db, 255, &len);
  CHECK_DBC_RC(Connection,rc);

  is_num(len, strlen("test_odbc_current") * sizeof(SQLWCHAR));
  is_wstr(db, cur_db, 18);

  /* reset for further tests */
  rc = SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, my_schema, SQL_NTS);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE test_odbc_current");

  return OK;
}


ODBC_TEST(t_current_catalog_ansi)
{
  SQLCHAR     cur_db[255], db[255];
  SQLRETURN   rc;
  SQLINTEGER len;

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, db, sizeof(db), &len);
  CHECK_DBC_RC(Connection,rc);

  is_num(len, strlen(my_schema));
  IS_STR(db, my_schema, len);

  rc = SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, db, SQL_NTS);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS test_odbc_current");

  strcpy((char *)cur_db, "test_odbc_current");
  rc = SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, cur_db, SQL_NTS);
  FAIL_IF(rc == SQL_SUCCESS, "Error expected");

  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE test_odbc_current");
  rc = SQLFreeStmt(Stmt,SQL_CLOSE);

  strcpy((char *)cur_db, "test_odbc_current");
  rc = SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, cur_db, SQL_NTS);
  CHECK_DBC_RC(Connection,rc);

  rc = SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, db, 255, &len);
  CHECK_DBC_RC(Connection,rc);

  is_num(len, 17);
  IS_STR(db, cur_db, 18);



  /* reset for further tests */
  rc = SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, (SQLCHAR *)my_schema, SQL_NTS);
  CHECK_DBC_RC(Connection,rc);

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE test_odbc_current");

  return OK;
}


ODBC_TEST(tmysql_showkeys)
{
  SQLRETURN rc;

  OK_SIMPLE_STMT(Stmt,"drop table if exists tmysql_spk");

  OK_SIMPLE_STMT(Stmt,"create table tmysql_spk(col1 int primary key)");
   
  CHECK_DBC_RC(Connection, SQLTransact(NULL,Connection,SQL_COMMIT));
    
  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt,"SHOW KEYS FROM tmysql_spk");
    

  IS(1 == myresult(Stmt));

  rc = SQLFreeStmt(Stmt,SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_spk");

  return OK;
}


ODBC_TEST(t_sqltables)
{
  SQLRETURN   r;
  SQLINTEGER  rows;
  SQLLEN      rowCount;

  char query[128];

  sprintf(query, "DROP SCHEMA %s", my_schema);
  OK_SIMPLE_STMT(Stmt, query);
  sprintf(query, "CREATE SCHEMA %s", my_schema);
  OK_SIMPLE_STMT(Stmt, query);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, my_schema, SQL_NTS));

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t1 (a int)");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t2 LIKE t1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t3 LIKE t1");

  CHECK_STMT_RC(Stmt, SQLTables(Stmt,NULL,0,NULL,0,NULL,0,NULL,0));

  myresult(Stmt);

  r = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,r);

  r  = SQLTables(Stmt, NULL, 0, NULL, 0, NULL, 0,
                 (SQLCHAR *)"system table", SQL_NTS);
  CHECK_STMT_RC(Stmt,r);

  is_num(myresult(Stmt), 0);

  r = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,r);

  r  = SQLTables(Stmt, NULL, 0, NULL, 0, NULL, 0,
                 (SQLCHAR *)"TABLE", SQL_NTS);
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowCount));

  CHECK_STMT_RC(Stmt,r);

  is_num(myresult(Stmt), rowCount);

  r = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,r);

  r  = SQLTables(Stmt, (SQLCHAR *)"TEST", SQL_NTS,
                 (SQLCHAR *)"TEST", SQL_NTS, NULL, 0,
                 (SQLCHAR *)"TABLE", SQL_NTS);
  CHECK_STMT_RC(Stmt,r);

  myresult(Stmt);

  r = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,r);

  r = SQLTables(Stmt, (SQLCHAR *)"%", SQL_NTS, NULL, 0, NULL, 0, NULL, 0);
  CHECK_STMT_RC(Stmt,r);

  myresult(Stmt);

  r = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,r);

  r = SQLTables(Stmt, NULL, 0, (SQLCHAR *)"%", SQL_NTS, NULL, 0, NULL, 0);
  CHECK_STMT_RC(Stmt,r);

  myresult(Stmt);

  r = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,r);

  r = SQLTables(Stmt, "", 0, "", 0, "", 0, (SQLCHAR *)"%", SQL_NTS);
  CHECK_STMT_RC(Stmt,r);

  rows= myresult(Stmt);
  is_num(rows, 3);

  r = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,r);

  return OK;
}


ODBC_TEST(my_information_schema)
{
  SQLCHAR   conn[512], conn_out[512];
  SQLHDBC Connection1;
  SQLHSTMT Stmt1;
  SQLSMALLINT conn_out_len;
  SQLRETURN rc;

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS istest__");
  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS istest_1");
  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS istest_2");

  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE istest__");
  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE istest_1");
  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE istest_2");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE istest__.istab_(a INT,b INT,c INT, d INT)");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE istest_1.istab1(a INT,b INT,c INT, d INT)");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE istest_2.istab2(a INT,b INT,c INT, d INT)");

  /* We need to have istest__ as the default DB */
  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;DATABASE=istest__;OPTION=0", my_dsn, my_uid, my_pwd);
  
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &Connection1));

  CHECK_DBC_RC(Connection1, SQLDriverConnect(Connection1, NULL, conn, sizeof(conn), conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(Connection1, SQLAllocStmt(Connection1, &Stmt1));

  rc = SQLTables(Stmt1, "istest__", SQL_NTS, "", 0, "istab%", SQL_NTS, NULL, 0);
  CHECK_STMT_RC(Stmt1,rc);

  /* all tables from all databases should be displayed */
  is_num(my_print_non_format_result(Stmt1), 3);
  rc = SQLFreeStmt(Stmt1, SQL_CLOSE);

  rc = SQLTables(Stmt1, NULL, 0, NULL, 0, "istab%", SQL_NTS, NULL, 0);
  CHECK_STMT_RC(Stmt1,rc);

  is_num(my_print_non_format_result(Stmt1), 1);
  rc = SQLFreeStmt(Stmt1, SQL_CLOSE);
  CHECK_STMT_RC(Stmt1,rc);

  CHECK_DBC_RC(Connection1, SQLDisconnect(Connection1));
  CHECK_DBC_RC(Connection1, SQLFreeHandle(SQL_HANDLE_DBC, Connection1));

  return OK;
}


/**
 Bug #4518: SQLForeignKeys returns too many foreign key
 Bug #27723: SQLForeignKeys does not escape _ and % in the table name arguments

 The original test case was extended to have a table that would inadvertently
 get included because of the poor escaping.
*/
ODBC_TEST(t_bug4518)
{
  SQLCHAR buff[255];
  SQLLEN len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug4518_c, t_bug4518_c2, t_bug4518ac, "
                "                     t_bug4518_p");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug4518_p (id INT PRIMARY KEY) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug4518_c (id INT, parent_id INT,"
                "                          FOREIGN KEY (parent_id)"
                "                           REFERENCES"
                "                             t_bug4518_p(id)"
                "                           ON DELETE SET NULL)"
                " ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug4518_c2 (id INT, parent_id INT,"
                "                           FOREIGN KEY (parent_id)"
                "                            REFERENCES"
                "                              t_bug4518_p(id)"
                "                          )"
                " ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug4518ac (id INT, parent_id INT,"
                "                          FOREIGN KEY (parent_id)"
                "                           REFERENCES"
                "                             t_bug4518_p(id)"
                "                            ON DELETE CASCADE"
                "                            ON UPDATE NO ACTION"
                "                          )"
                " ENGINE=InnoDB");

  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                NULL, 0, (SQLCHAR *)"t_bug4518_c", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug4518_p", 11);
  IS_STR(my_fetch_str(Stmt, buff, 4), "id", 2);
  IS_STR(my_fetch_str(Stmt, buff, 7), "t_bug4518_c", 11);
  IS_STR(my_fetch_str(Stmt, buff, 8), "parent_id", 9);
  if (1)
  {
    is_num(my_fetch_int(Stmt, 10), SQL_RESTRICT);
    is_num(my_fetch_int(Stmt, 11), SQL_SET_NULL);
  }
  else
  {
    is_num(my_fetch_int(Stmt, 10), SQL_RESTRICT);
    is_num(my_fetch_int(Stmt, 11), SQL_RESTRICT);
  }

  /* For Bug #19923: Test that schema columns are NULL. */
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_CHAR, buff, sizeof(buff), &len));
  is_num(len, SQL_NULL_DATA);
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 6, SQL_C_CHAR, buff, sizeof(buff), &len));
  is_num(len, SQL_NULL_DATA);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                NULL, 0, (SQLCHAR *)"t_bug4518ac", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug4518_p", 11);
  IS_STR(my_fetch_str(Stmt, buff, 4), "id", 2);
  IS_STR(my_fetch_str(Stmt, buff, 7), "t_bug4518ac", 11);
  IS_STR(my_fetch_str(Stmt, buff, 8), "parent_id", 9);
  if (1)
  {
    is_num(my_fetch_int(Stmt, 10), SQL_NO_ACTION);
    is_num(my_fetch_int(Stmt, 11), SQL_CASCADE);
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt,
         "DROP TABLE t_bug4518_c, t_bug4518_c2, t_bug4518ac, t_bug4518_p");

  return OK;
}


/**
 Tests the non-error code paths in catalog.c that return an empty set to
 make sure the resulting empty result sets at least indicate the right
 number of columns.
*/
ODBC_TEST(empty_set)
{
  SQLSMALLINT columns;

  /* SQLTables(): no known table types. */
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS,
                           (SQLCHAR *)"UNKNOWN", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &columns));
  is_num(columns, 5);
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* SQLTables(): no tables found. */
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
                           (SQLCHAR *)"no_such_table", SQL_NTS, NULL, SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &columns));
  is_num(columns, 5);
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* SQLTables(): empty catalog with existing table */
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_sqltables_empty");
  OK_SIMPLE_STMT(Stmt, "create table t_sqltables_empty (x int)");
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, "", SQL_NTS, NULL, 0,
			   (SQLCHAR *) "t_sqltables_empty", SQL_NTS,
			   NULL, SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt, &columns));
  is_num(columns, 5);
  FAIL_IF(SQLFetch(Stmt) == SQL_NO_DATA_FOUND, "expected data");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_sqltables_empty");

  return OK;
}


/**
 Bug #23031: SQLTables returns inaccurate catalog information on views
*/
ODBC_TEST(t_bug23031)
{
  SQLCHAR buff[255];

  OK_SIMPLE_STMT(Stmt, "DROP VIEW IF EXISTS t_bug23031_v");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug23031_t");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug23031_t (a INT) COMMENT 'Whee!'");
  OK_SIMPLE_STMT(Stmt, "CREATE VIEW t_bug23031_v AS SELECT * FROM t_bug23031_t");

  /* Get both the table and view. */
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
                           (SQLCHAR *)"t_bug23031%", SQL_NTS, NULL, SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug23031_t", 12);
  IS_STR(my_fetch_str(Stmt, buff, 4), "BASE TABLE", 10);
  IS_STR(my_fetch_str(Stmt, buff, 5), "Whee!", 5);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug23031_v", 12);
  IS_STR(my_fetch_str(Stmt, buff, 4), "VIEW", 4);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Get just the table. */
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
                           (SQLCHAR *)"t_bug23031%", SQL_NTS,
                           (SQLCHAR *)"BASE TABLE", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug23031_t", 12);
  IS_STR(my_fetch_str(Stmt, buff, 4), "BASE TABLE", 10);
  IS_STR(my_fetch_str(Stmt, buff, 5), "Whee!", 5);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Get just the view. */
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
                           (SQLCHAR *)"t_bug23031%", SQL_NTS,
                           (SQLCHAR *)"VIEW", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug23031_v", 12);
  IS_STR(my_fetch_str(Stmt, buff, 4), "VIEW", 4);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP VIEW IF EXISTS t_bug23031_v");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug23031_t");

  return OK;
}


/**
 Bug #15713: null pointer when use the table qualifier in SQLColumns()
*/
ODBC_TEST(bug15713)
{
  SQLHDBC Connection1;
  SQLHSTMT Stmt1;
  SQLCHAR   conn[512], conn_out[512];
  SQLSMALLINT conn_out_len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug15713");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug15713 (a INT)");

  /* The connection strings must not include DATABASE. */
  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s", my_dsn, my_uid, my_pwd);
 
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &Connection1));

  CHECK_DBC_RC(Connection1, SQLDriverConnect(Connection1, NULL, conn, sizeof(conn), conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  CHECK_DBC_RC(Connection1, SQLAllocStmt(Connection1, &Stmt1));

  CHECK_STMT_RC(Stmt1, SQLColumns(Stmt1, (SQLCHAR *)my_schema, SQL_NTS,
                             NULL, 0, (SQLCHAR *)"t_bug15713", SQL_NTS,
                             NULL, 0));

  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  IS_STR(my_fetch_str(Stmt1, conn, 1), my_schema, strlen(my_schema));
  IS_STR(my_fetch_str(Stmt1, conn, 3), "t_bug15713", 10);
  IS_STR(my_fetch_str(Stmt1, conn, 4), "a", 1);

  FAIL_IF(SQLFetch(Stmt1) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_DROP));
  CHECK_DBC_RC(Connection1, SQLDisconnect(Connection1));
  CHECK_DBC_RC(Connection1, SQLFreeHandle(SQL_HANDLE_DBC, Connection1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug15713");
  return OK;
}


/**
 Bug #28316: Fatal crash with Crystal Reports and MySQL Server
*/
ODBC_TEST(t_bug28316)
{
  CHECK_STMT_RC(Stmt, SQLProcedures(Stmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS));

  return OK;
}


/**
 Bug #8860: Generic SQLColumns not supported?
*/
ODBC_TEST(bug8860)
{
  SQLCHAR buff[512];

OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug8860, `t_bug8860_a'b`");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug8860 (a INT)");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE `t_bug8860_a'b` (b INT)");

  /*
   Specifying nothing gets us columns from all of the tables in the
   current database.
  */
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0));

  /* We should have at least two rows. There may be more. */
  FAIL_IF(myrowcount(Stmt) < 2, "expected min. 2 rows");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  if (!using_dm(Connection))
  {
    /* Specifying "" as the table name gets us nothing. */
    /* But iODBC, for one, will convert our "" into a NULL. */
    CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0, (SQLCHAR *)"", SQL_NTS,
                              NULL, 0));

    is_num(myrowcount(Stmt), 0);
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Get the info from just one table.  */
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                            (SQLCHAR *)"t_bug8860", SQL_NTS,
                            NULL, 0));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug8860", 9);
  IS_STR(my_fetch_str(Stmt, buff, 4), "a", 1);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Get the info from just one table with a funny name.  */
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                            (SQLCHAR *)"t_bug8860_a\\'b", SQL_NTS,
                            NULL, 0));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug8860_a'b", 13);
  IS_STR(my_fetch_str(Stmt, buff, 4), "b", 1);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug8860, `t_bug8860_a'b`");
  return OK;
}

/**
 Bug #26934: SQLTables behavior has changed
*/
ODBC_TEST(t_bug26934)
{
  SQLHENV Env1;
  SQLHDBC Connection1;
  SQLHSTMT Stmt1;

  ODBC_Connect(&Env1, &Connection1, &Stmt1);

  OK_SIMPLE_STMT(Stmt1, "SET @@wait_timeout = 1");
  Sleep(5000);
  FAIL_IF(SQLTables(Stmt1, (SQLCHAR *)"%", 1, NULL, SQL_NTS,
                                NULL, SQL_NTS, NULL, SQL_NTS) != SQL_ERROR, "error expected");
  if (check_sqlstate(Stmt1, "08S01") != OK)
    return FAIL;

  ODBC_Disconnect(Env1, Connection1, Stmt1);

  return OK;
}


/**
 Bug #29888: Crystal wizard throws error on including tables
*/
ODBC_TEST(t_bug29888)
{
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug29888");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug29888 (a INT, b INT)");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, my_schema, SQL_NTS, NULL, SQL_NTS,
                            (SQLCHAR *)"t_bug29888", SQL_NTS,
                            (SQLCHAR *)"%", SQL_NTS));

  is_num(myrowcount(Stmt), 2);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug29888");

  return OK;
}


/**
  Bug #14407: SQLColumns gives wrong information of not nulls
*/
ODBC_TEST(t_bug14407)
{
  SQLCHAR col[10];
  SQLSMALLINT nullable;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug14407");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_bug14407(a INT NOT NULL AUTO_INCREMENT PRIMARY KEY)");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                            (SQLCHAR *)"t_bug14407", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, col, 4), "a", 1);
  is_num(my_fetch_int(Stmt, 11), SQL_NULLABLE);
  IS_STR(my_fetch_str(Stmt, col, 18), "YES", 3);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /**
    Bug #26108  MyODBC ADO field attributes reporting adFldMayBeNull for
    not null columns
  */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_bug14407");

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 1, col, sizeof(col), NULL, NULL, NULL,
                                NULL, &nullable));
  is_num(nullable, SQL_NULLABLE);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug14407");
  return OK;
}


/**
 Bug #19923: MyODBC Foreign key retrieval broken in multiple ways
 Two issues to test: schema columns should be NULL, not just an empty string,
 and more than one constraint should be returned.
*/
ODBC_TEST(t_bug19923)
{
  SQLCHAR buff[255];
  SQLLEN len;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug19923c, t_bug19923b, t_bug19923a");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug19923a (a INT PRIMARY KEY) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug19923b (b INT PRIMARY KEY) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug19923c (a INT, b INT, UNIQUE(a), UNIQUE(b),"
                "CONSTRAINT `first_constraint` FOREIGN KEY (`b`) REFERENCES `t_bug19923b` (`b`),"
                "CONSTRAINT `second_constraint` FOREIGN KEY (`a`) REFERENCES `t_bug19923a` (`a`)"
                ") ENGINE=InnoDB");

  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                NULL, 0, (SQLCHAR *)"t_bug19923c", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug19923a", 11);
  IS_STR(my_fetch_str(Stmt, buff, 4), "a", 1);
  IS_STR(my_fetch_str(Stmt, buff, 7), "t_bug19923c", 11);
  IS_STR(my_fetch_str(Stmt, buff, 8), "a", 1);
  IS_STR(my_fetch_str(Stmt, buff, 12), "second_constraint", 17);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 3), "t_bug19923b", 11);
  IS_STR(my_fetch_str(Stmt, buff, 4), "b", 1);
  IS_STR(my_fetch_str(Stmt, buff, 7), "t_bug19923c", 11);
  IS_STR(my_fetch_str(Stmt, buff, 8), "b", 1);
  IS_STR(my_fetch_str(Stmt, buff, 12), "first_constraint", 16);
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_CHAR, buff, sizeof(buff), &len));
  is_num(len, SQL_NULL_DATA);
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 6, SQL_C_CHAR, buff, sizeof(buff), &len));
  is_num(len, SQL_NULL_DATA);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug19923c, t_bug19923b, t_bug19923a");
  return OK;
}


/*
  Bug #32864 MyODBC /Crystal Reports truncated table names,
             lost fields when names > 21chars
  Fails if built with c/c and probably >=5.5 (with everything
  where SYSTEM_CHARSET_MBMAXLEN != mbmaxlen of UTF8_CHARSET_NUMBER charset)
*/
ODBC_TEST(t_bug32864)
{
  SQLLEN dispsize= 0;
  SQLULEN colsize= 0;
  SQLCHAR dummy[20];

  CHECK_STMT_RC(Stmt, SQLTables(Stmt, (SQLCHAR *)"%", SQL_NTS, NULL, 0, NULL, 0,
                           NULL, 0));
  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 3, SQL_COLUMN_DISPLAY_SIZE, NULL, 0,
                                 NULL, &dispsize));

  is_num(dispsize, 64);

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 3, dummy, sizeof(dummy), NULL, NULL,
                                &colsize, NULL, NULL));

  is_num(colsize, 64);

  return OK;
}


/*
  Bug #32989 - Crystal Reports fails if a field has a single quote
*/
ODBC_TEST(t_bug32989)
{
  SQLCHAR name[20];
  SQLLEN len;

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug32989");
  OK_SIMPLE_STMT(Stmt, "create table t_bug32989 (`doesn't work` int)");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, (SQLCHAR *)my_schema, SQL_NTS, NULL, 0,
                            (SQLCHAR *)"t_bug32989", SQL_NTS, NULL, 0));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 4, SQL_C_CHAR, name, 20, &len));
  is_num(len, 12);
  IS_STR(name, "doesn't work", 13);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug32989");
  return OK;
}


/**
 Bug #33298: ADO returns wrong parameter count in the query (always 0)
*/
ODBC_TEST(t_bug33298)
{
  SQLRETURN rc= 0;
  FAIL_IF(SQLProcedureColumns(Stmt, NULL, 0, NULL, 0,
                                         NULL, 0, NULL, 0)
                                         != SQL_SUCCESS, "Error");
  rc= SQLFetch(Stmt);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  if (rc == SQL_ERROR)
    return FAIL;

  return OK;
}

/**
 Bug #12805: ADO failed to retrieve the length of LONGBLOB columns
*/
ODBC_TEST(t_bug12805)
{
  SQLHENV     Env1;
  SQLHDBC     Connection1;
  SQLHSTMT    Stmt1;
  SQLCHAR     dummy[10];
  SQLULEN     length;  
  SQLUINTEGER len2;
  int         saved_options= my_options;

  diag("Option not supported yet");
  return SKIP;

  my_options = 1 << 27;

  ODBC_Connect(&Env1, &Connection1, &Stmt1);

  OK_SIMPLE_STMT(Stmt1, "DROP TABLE IF EXISTS bug12805");
  OK_SIMPLE_STMT(Stmt1, "CREATE TABLE bug12805("\
                 "id INT PRIMARY KEY auto_increment,"\
                 "longdata LONGBLOB NULL)");

  CHECK_STMT_RC(Stmt1, SQLColumns(Stmt1, NULL, 0, NULL, 0,
                             (SQLCHAR *)"bug12805", SQL_NTS,
                             (SQLCHAR *)"longdata", SQL_NTS));

  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));
  CHECK_STMT_RC(Stmt1, SQLGetData(Stmt1, 7, SQL_C_ULONG, &len2,
                             0, NULL));
  is_num(len2, 2147483647);
  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));

  length= 0;
  OK_SIMPLE_STMT(Stmt1, "SELECT * FROM bug12805");
  CHECK_STMT_RC(Stmt1, SQLDescribeCol(Stmt1, 2, dummy, sizeof(dummy) - 1, NULL,
                                 NULL, &length, NULL, NULL));
  is_num(length, 2147483647);

  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_CLOSE));
  ODBC_Disconnect(Env1, Connection1, Stmt1);

  /* Check without the 32-bit signed flag */
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                            (SQLCHAR *)"bug12805", SQL_NTS,
                            (SQLCHAR *)"longdata", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 7, SQL_C_ULONG, &len2,
                            0, NULL));
  is_num(len2, 4294967295UL);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  length= 0;
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM bug12805");
  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 2, dummy, sizeof(dummy), NULL, NULL,
                                 &length, NULL, NULL));
  is_num(length, 4294967295UL);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE bug12805");

  my_options= saved_options;
  return OK;
}


/**
 Bug #30770: Access Violation in myodbc3.dll
*/
ODBC_TEST(t_bug30770)
{
  SQLCHAR    buff[512];
  SQLHENV    Env1;
  SQLHDBC    Connection1;
  SQLHSTMT   Stmt1;
  SQLCHAR    conn[MAX_NAME_LEN*2];


  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug30770");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug30770 (a INT)");

  /* Connect with no default daabase */
  sprintf((char *)conn, "DRIVER=%s;SERVER=%s;" \
                        "UID=%s;PASSWORD=%s;DATABASE=%s;PORT=%u", my_drivername, "localhost",
                        my_uid, my_pwd, my_schema, my_port);
  
  is_num(mydrvconnect(&Env1, &Connection1, &Stmt1, conn), OK);

  /* Get the info from just one table.  */
  CHECK_STMT_RC(Stmt1, SQLColumns(Stmt1, NULL, SQL_NTS, NULL, SQL_NTS,
                             (SQLCHAR *)"bug30770", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));

  IS_STR(my_fetch_str(Stmt1, buff, 3), "bug30770", 9);
  IS_STR(my_fetch_str(Stmt1, buff, 4), "a", 1);

  FAIL_IF(SQLFetch(Stmt1) != SQL_NO_DATA_FOUND, "expected no data");
  ODBC_Disconnect(Env1, Connection1, Stmt1);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE bug30770");
  return OK;
}


/**
 Bug #36275: SQLTables buffer overrun
*/
ODBC_TEST(t_bug36275)
{
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, 0, NULL, 0, NULL, 0,
                           (SQLCHAR *)
/* Just a really long literal to blow out the buffer. */
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789"
"0123456789012345678901234567890123456789012345678901234567890123456789",
                           SQL_NTS));

  return OK;
}


/*
  Bug #39957 - NULL catalog did not return correct catalog in result
*/
ODBC_TEST(t_bug39957)
{
  SQLCHAR buf[50];
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug39957");
  OK_SIMPLE_STMT(Stmt, "create table t_bug39957 (x int)");
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, 0, NULL, 0,
			   (SQLCHAR *)"t_bug39957", SQL_NTS, NULL, 0));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buf, 3), "t_bug39957", 11);
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug39957");

  return OK;
}


/*
  Bug #37621 - SQLDescribeCol returns incorrect values of SQLTables data
*/
ODBC_TEST(t_bug37621)
{
  SQLCHAR szColName[128];
  SQLSMALLINT iName, iType, iScale, iNullable;
  SQLULEN uiDef;

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug37621");
  OK_SIMPLE_STMT(Stmt, "create table t_bug37621 (x int)");
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, 0, NULL, 0,
			   (SQLCHAR *)"t_bug37621", SQL_NTS, NULL, 0));
/*
  Check column properties for the REMARKS column
*/
  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 5, szColName, sizeof(szColName), 
          &iName, &iType, &uiDef, &iScale, &iNullable));

  IS_STR(szColName, "REMARKS", 8);
  is_num(iName, 7);
  if (iType != SQL_VARCHAR && iType != SQL_WVARCHAR)
    return FAIL;
  /* This can fail for the same reason as t_bug32864 */
  is_num(uiDef, 2048);
  is_num(iScale, 0);
  is_num(iNullable, 0);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug37621");

  return OK;
}


/*
Bug #34272 - SQLColumns returned wrong values for (some) TYPE_NAME
and (some) IS_NULLABLE
*/
ODBC_TEST(t_bug34272)
{
  SQLCHAR dummy[20];
  SQLULEN col6, col18, length;

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug34272");
  OK_SIMPLE_STMT(Stmt, "create table t_bug34272 (x int unsigned)");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
    (SQLCHAR *)"t_bug34272", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 6, dummy, sizeof(dummy), NULL, NULL,
    &col6, NULL, NULL));
  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 18, dummy, sizeof(dummy), NULL, NULL,
    &col18, NULL, NULL));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 6, SQL_C_CHAR, dummy, col6+1, &length));
  is_num(length,12);
  IS_STR(dummy, "INT UNSIGNED", length+1);

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 18, SQL_C_CHAR, dummy, col18+1, &length));
  is_num(length,3);
  IS_STR(dummy, "YES", length+1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug34272");

  return OK;
}


/*
  Bug #49660 - constraints with same name for tables with same name but in different db led
  to doubling of results of SQLForeignKeys
*/
ODBC_TEST(t_bug49660)
{
  SQLLEN rowsCount;
  
  OK_SIMPLE_STMT(Stmt, "drop database if exists bug49660");
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug49660");
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug49660_r");

  OK_SIMPLE_STMT(Stmt, "create database bug49660");
  OK_SIMPLE_STMT(Stmt, "create table bug49660.t_bug49660_r (id int unsigned not null primary key, name varchar(10) not null) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "create table bug49660.t_bug49660 (id int unsigned not null primary key, refid int unsigned not null,"
                "foreign key t_bug49660fk (id) references bug49660.t_bug49660_r (id)) ENGINE=InnoDB");

  OK_SIMPLE_STMT(Stmt, "create table t_bug49660_r (id int unsigned not null primary key, name varchar(10) not null) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "create table t_bug49660 (id int unsigned not null primary key, refid int unsigned not null,"
                "foreign key t_bug49660fk (id) references t_bug49660_r (id)) ENGINE=InnoDB");

  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                NULL, 0, (SQLCHAR *)"t_bug49660", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowsCount));
  is_num(rowsCount, 1); 
  /* Going another way around - sort of more reliable */
  FAIL_IF(SQLFetch(Stmt) == SQL_NO_DATA_FOUND, "expected data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop database if exists bug49660");
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug49660");
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug49660_r");

  return OK;
}


/*
  Bug #51422 - SQLForeignKeys returned keys pointing to unique fields
*/
ODBC_TEST(t_bug51422)
{
  SQLLEN rowsCount;

  diag("bug51422 not fixed yet");
  return SKIP;
  
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug51422");
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug51422_r");

  OK_SIMPLE_STMT(Stmt, "create table t_bug51422_r (id int unsigned not null primary key, ukey int unsigned not null,"
                "name varchar(10) not null, UNIQUE KEY uk(ukey)) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "create table t_bug51422 (id int unsigned not null primary key, refid int unsigned not null,"
                "foreign key t_bug51422fk (id) references t_bug51422_r (ukey))  ENGINE=InnoDB");

  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                NULL, 0, (SQLCHAR *)"t_bug51422", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowsCount));
  is_num(rowsCount, 0);

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug51422");
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug51422_r");

  return OK;
}


/*
  Bug #36441 - SQLPrimaryKeys returns mangled strings 
*/
ODBC_TEST(t_bug36441)
{
#define BUF_LEN 24

  const SQLCHAR key_column_name[][14]= {"pk_for_table1", "c1_for_table1"};

  SQLCHAR     catalog[BUF_LEN], schema[BUF_LEN], table[BUF_LEN], column[BUF_LEN];
  SQLLEN      catalog_len, schema_len, table_len, column_len;
  SQLCHAR     keyname[BUF_LEN];
  SQLSMALLINT key_seq, i;
  SQLLEN      keyname_len, key_seq_len, rowCount;

OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug36441_0123456789");
  OK_SIMPLE_STMT(Stmt, "create table t_bug36441_0123456789("
	              "pk_for_table1 integer not null auto_increment,"
	              "c1_for_table1 varchar(128) not null unique,"
	              "c2_for_table1 binary(32) null,"
                "unique_key int unsigned not null unique,"
	              "primary key(pk_for_table1, c1_for_table1))");

  CHECK_STMT_RC(Stmt, SQLPrimaryKeys(Stmt, NULL, SQL_NTS, NULL, SQL_NTS, "t_bug36441_0123456789", SQL_NTS));

  /* Test of SQLRowCount with SQLPrimaryKeys */
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowCount));
  is_num(rowCount, 2);

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR , catalog, sizeof(catalog), &catalog_len));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR , schema , sizeof(schema) , &schema_len));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_CHAR , table  , sizeof(table)  , &table_len));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 4, SQL_C_CHAR , column , sizeof(column) , &column_len));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 5, SQL_C_SHORT,&key_seq, sizeof(key_seq), &key_seq_len));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 6, SQL_C_CHAR , keyname, sizeof(keyname), &keyname_len));

  for(i= 0; i < 2; ++i)
  {
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

    is_num(catalog_len, strlen(my_schema));
    is_num(schema_len, SQL_NULL_DATA);
    IS_STR(table, "t_bug36441_0123456789", 3);
    IS_STR(column, key_column_name[i], 4);
    is_num(key_seq, i+1);
    IS_STR(keyname, "PRIMARY", 6);
  }

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug36441_0123456789");

  return OK;

#undef BUF_LEN
}


/*
  Bug #53235 - SQLColumns returns wrong transfer octet length
*/
ODBC_TEST(t_bug53235)
{
  int col_size, buf_len;

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug53235");
  OK_SIMPLE_STMT(Stmt, "create table t_bug53235 (x decimal(10,3))");
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
			   (SQLCHAR *)"t_bug53235", SQL_NTS, NULL, 0));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  col_size= my_fetch_int(Stmt, 7);
  buf_len= my_fetch_int(Stmt, 8);

  is_num(col_size, 10);
  is_num(buf_len, 12);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug53235");

  return OK;
}


/*
  Bug #50195 - SQLTablePrivileges requires select priveleges
*/
ODBC_TEST(t_bug50195)
{
  SQLHDBC     hdbc1;
  SQLHSTMT    hstmt1;
  const char  expected_privs[][12]= {"ALTER", "CREATE", "CREATE VIEW", "DELETE", "DROP", "INDEX",
                                    "INSERT", "REFERENCES", "SHOW VIEW", "TRIGGER", "UPDATE"};
  int         i;
  SQLCHAR     priv[12];
  SQLLEN      len;

  (void)SQLExecDirect(Stmt, (SQLCHAR *)"DROP USER bug50195@127.0.0.1", SQL_NTS);
  (void)SQLExecDirect(Stmt, (SQLCHAR *)"DROP USER bug50195@localhost", SQL_NTS);

  OK_SIMPLE_STMT(Stmt, "grant all on *.* to bug50195@127.0.0.1 IDENTIFIED BY 'a'");
  OK_SIMPLE_STMT(Stmt, "grant all on *.* to bug50195@localhost IDENTIFIED BY 'a'");

  OK_SIMPLE_STMT(Stmt, "revoke select on *.* from bug50195@127.0.0.1");
  OK_SIMPLE_STMT(Stmt, "revoke select on *.* from bug50195@localhost");

  /* revoking "global" select is enough, but revoking smth from mysql.tables_priv
     to have not empty result of SQLTablePrivileges */
  OK_SIMPLE_STMT(Stmt, "grant all on mysql.tables_priv to bug50195@127.0.0.1");
  OK_SIMPLE_STMT(Stmt, "grant all on mysql.tables_priv to bug50195@localhost");
  OK_SIMPLE_STMT(Stmt, "revoke select on mysql.tables_priv from bug50195@127.0.0.1");
  OK_SIMPLE_STMT(Stmt, "revoke select on mysql.tables_priv from bug50195@localhost");

  OK_SIMPLE_STMT(Stmt, "FLUSH PRIVILEGES");

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLConnect(hdbc1, my_dsn, SQL_NTS, "bug50195", SQL_NTS, "a", SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLTablePrivileges(hstmt1, "mysql", SQL_NTS, 0, 0, "tables_priv", SQL_NTS));

  /* Testing SQLTablePrivileges a bit, as we don't have separate test of it */

  for(i= 0; i < 11; ++i)
  {
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
    CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 6, SQL_C_CHAR, priv, sizeof(priv), &len));
    IS_STR(priv, expected_privs[i], len);
  }
  
  FAIL_IF(SQLFetch(hstmt1) != 100, "No data expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  OK_SIMPLE_STMT(Stmt, "DROP USER bug50195@127.0.0.1");
  OK_SIMPLE_STMT(Stmt, "DROP USER bug50195@localhost");
  
  return OK;
}


ODBC_TEST(t_sqlprocedurecolumns)
{
  SQLRETURN rc= 0;
  SQLCHAR szName[255]= {0};

  typedef struct 
  {
    char *c01_procedure_cat;
    char *c02_procedure_schem;
    char *c03_procedure_name;
    char *c04_column_name;
    short c05_column_type;
    short c06_data_type;
    char *c07_type_name;
    unsigned long c08_column_size;
    unsigned long c09_buffer_length;
    short c10_decimal_digits;
    short c11_num_prec_radix;
    short c12_nullable;
    char *c13_remarks;
    char *c14_column_def;
    short c15_sql_data_type;
    short c16_sql_datetime_sub;
    unsigned long c17_char_octet_length;
    int c18_ordinal_position;
    char *c19_is_nullable;
  }sqlproccol;

  int total_params= 0, iter= 0;

  sqlproccol data_to_check[] = {
    /*cat    schem  proc_name                  col_name     col_type         data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param1", SQL_PARAM_INPUT, SQL_TINYINT, "tinyint",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
     3,    1,       0,  10,    SQL_NULLABLE, "", 0,  SQL_TINYINT,  0,  0,    1,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param2", SQL_PARAM_OUTPUT, SQL_SMALLINT, "smallint",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
     5,    2,       0,  10,    SQL_NULLABLE, "", 0,  SQL_SMALLINT,  0,  0,    2,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param3", SQL_PARAM_INPUT,  SQL_INTEGER, "mediumint",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
     7,    3,       0,  10,    SQL_NULLABLE, "", 0,  SQL_INTEGER,  0,  0,    3,  "YES"},

    /*cat    schem  proc_name                  col_name      col_type                data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param 4", SQL_PARAM_INPUT_OUTPUT, SQL_INTEGER, "int",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
     10,     4,       0,  10,  SQL_NULLABLE, "", 0,  SQL_INTEGER,  0,  0,    4,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param5", SQL_PARAM_OUTPUT, SQL_BIGINT, "bigint",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
     19,     20,      0,  10,  SQL_NULLABLE, "", 0,  SQL_BIGINT,   0,  0,    5,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type  type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param6", SQL_PARAM_INPUT, SQL_REAL,  "float",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      7,   4,       2,  0,     SQL_NULLABLE, "", 0,  SQL_REAL,     0,  0,    6,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param7", SQL_PARAM_OUTPUT, SQL_DOUBLE,  "double",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      15,  8,       0,  0,     SQL_NULLABLE, "", 0,  SQL_DOUBLE,   0,  0,    7,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param8", SQL_PARAM_INPUT, SQL_DECIMAL,  "decimal",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      10,  11,       3,  10,    SQL_NULLABLE, "", 0,  SQL_DECIMAL,   0,  0,    8,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param9", SQL_PARAM_INPUT, SQL_CHAR,  "char",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      32,  32,      0,  0,     SQL_NULLABLE, "", 0,  SQL_CHAR,     0,  32,    9,  "YES"},

    /*cat    schem  proc_name                  col_name      col_type           data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param10", SQL_PARAM_OUTPUT, SQL_VARCHAR, "varchar",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      64,  64,      0,  0,     SQL_NULLABLE, "", 0,  SQL_VARCHAR,  0,  64,   10, "YES"},

    /*cat    schem  proc_name                  col_name      col_type         data_type          type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param11", SQL_PARAM_INPUT, SQL_LONGVARBINARY, "long varbinary",
    /*size      buf_len   dec radix  nullable      rem def sql_data_type       sub octet     pos nullable*/
      16777215, 16777215, 0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARBINARY,  0,  16777215, 12, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type          type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramA", SQL_PARAM_INPUT, SQL_LONGVARBINARY, "blob",
    /*size      buf_len   dec radix  nullable      rem def sql_data_type       sub octet     pos nullable*/
      65535,    65535,    0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARBINARY,  0,  65535,    1, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type          type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramB", SQL_PARAM_INPUT, SQL_LONGVARBINARY, "longblob",
    /*size        buf_len      dec radix  nullable      rem def sql_data_type       sub octet        pos nullable*/
     4294967295L, 2147483647L, 0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARBINARY,  0,  2147483647L, 2, "YES"},

    /*cat    schem  proc_name                  col_name     col_type               data_type          type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramC", SQL_PARAM_INPUT_OUTPUT, SQL_LONGVARBINARY, "tinyblob",
    /*size   buf_len dec radix  nullable      rem def sql_data_type       sub octet pos nullable*/
     255,    255,    0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARBINARY,  0,  255,  3, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type          type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramD", SQL_PARAM_INPUT, SQL_LONGVARBINARY, "mediumblob",
    /*size     buf_len   dec radix  nullable      rem def sql_data_type       sub octet     pos nullable*/
     16777215, 16777215, 0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARBINARY,  0,  16777215, 4, "YES"},

     /*cat    schem  proc_name                  col_name    col_type        data_type          type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramE", SQL_PARAM_INPUT, SQL_VARBINARY, "varbinary",
    /*size   buf_len dec radix  nullable      rem def sql_data_type   sub octet pos nullable*/
     128,    128,    0,  0,     SQL_NULLABLE, "", 0,  SQL_VARBINARY,  0,  128,  5, "YES"},

     /*cat    schem  proc_name                  col_name    col_type          data_type   type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramF", SQL_PARAM_OUTPUT, SQL_BINARY, "binary",
    /*size   buf_len dec radix  nullable      rem def sql_data_type   sub octet pos nullable*/
     1,      1,      0,  0,     SQL_NULLABLE, "", 0,  SQL_BINARY,     0,  1,    6, "YES"},

     /*cat    schem  proc_name                 col_name     col_type          data_type   type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramG", SQL_PARAM_INPUT,  SQL_BINARY, "binary",
    /*size   buf_len dec radix  nullable      rem def sql_data_type   sub octet pos nullable*/
     8,      8,      0,  0,     SQL_NULLABLE, "", 0,  SQL_BINARY,     0,  8,    7, "YES"},

    /*cat    schem  proc_name                  col_name      col_type          data_type          type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_param H", SQL_PARAM_INPUT, SQL_LONGVARCHAR, "long varchar",
    /*size     buf_len   dec radix  nullable      rem def sql_data_type       sub octet     pos nullable*/
     16777215, 16777215, 0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARCHAR,  0,  16777215, 8, "YES"},

    /*cat    schem  proc_name                  col_name      col_type         data_type       type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramI", SQL_PARAM_INPUT, SQL_LONGVARCHAR, "text",
    /*size      buf_len   dec radix  nullable      rem def sql_data_type    sub octet  pos nullable*/
      65535,    65535,    0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARCHAR, 0,  65535,  9, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type       type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramJ", SQL_PARAM_INPUT, SQL_LONGVARCHAR, "mediumtext",
    /*size     buf_len   dec radix  nullable      rem def sql_data_type    sub octet     pos nullable*/
     16777215, 16777215, 0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARCHAR, 0,  16777215, 10, "YES"},

     /*cat    schem  proc_name                  col_name    col_type              data_type        type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramK", SQL_PARAM_INPUT_OUTPUT, SQL_LONGVARCHAR, "longtext",
    /*size        buf_len      dec radix  nullable      rem def sql_data_type    sub octet        pos nullable*/
     4294967295L, 2147483647L, 0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARCHAR, 0,  2147483647L, 11, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type        type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramL", SQL_PARAM_INPUT, SQL_LONGVARCHAR, "tinytext",
    /*size   buf_len dec radix  nullable      rem def sql_data_type    sub octet pos nullable*/
     255,    255,    0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARCHAR, 0,  255,  12, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type    type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramM", SQL_PARAM_INPUT, SQL_NUMERIC,  "numeric",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      8,   10,      2,  10,    SQL_NULLABLE, "", 0,  SQL_NUMERIC,   0,  0,   13,  "YES"},

    /*cat    schem  proc_name                  col_name       col_type         data_type     type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_00", SQL_PARAM_INPUT, SQL_TYPE_TIMESTAMP, "datetime",
    /*size buf_len  dec radix  nullable      rem def sql_data_type  sub                  octet pos nullable*/
      19,  16,      0,  10,     SQL_NULLABLE, "", 0, SQL_DATETIME,  SQL_TYPE_TIMESTAMP,  0,    1,  "YES"},

    /*cat    schem  proc_name                  col_name       col_type          data_type      type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_01", SQL_PARAM_OUTPUT, SQL_TYPE_DATE, "date",
    /*size buf_len  dec radix  nullable      rem def sql_data_type  sub octet pos nullable*/
      10,  6,       0,  0,     SQL_NULLABLE, "", 0,  SQL_DATETIME,  SQL_TYPE_DATE,  0,    2,  "YES"},

    /*cat    schem  proc_name                  col_name       col_type          data_type      type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_02", SQL_PARAM_OUTPUT, SQL_TYPE_TIME, "time",
    /*size buf_len  dec radix  nullable      rem def sql_data_type  sub octet pos nullable*/
      8,   6,       0,  0,     SQL_NULLABLE, "", 0,  SQL_DATETIME,  SQL_TYPE_TIME,  0,    3,  "YES"},

    /*cat    schem  proc_name                  col_name       col_type                data_type           type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_03", SQL_PARAM_INPUT_OUTPUT, SQL_TYPE_TIMESTAMP, "timestamp",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub                  octet pos nullable*/
      19,  16,      0,  0,     SQL_NULLABLE, "", 0,  SQL_DATETIME, SQL_TYPE_TIMESTAMP,  0,    4,  "YES"},

    /*cat    schem  proc_name                  col_name       col_type         data_type     type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_04", SQL_PARAM_INPUT, SQL_SMALLINT, "year",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      4,   1,       0,  10,    SQL_NULLABLE, "", 0,  SQL_SMALLINT, 0,  0,    5,  "YES"},

    /*cat    schem  proc_name                       col_name        col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test4_func", "RETURN_VALUE", SQL_RETURN_VALUE, SQL_VARCHAR, "varchar",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      32,  32,      0,  0,     SQL_NULLABLE, "", 0,  SQL_VARCHAR,  0,  32,   0, "YES"},

     /*cat    schem  proc_name                       col_name    col_type         data_type    type_name */
    {my_schema, 0,     "procedure_columns_test4_func", "re_paramF", SQL_PARAM_INPUT, SQL_INTEGER, "int",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
     10,     4,       0,  10,  SQL_NULLABLE, "", 0,  SQL_INTEGER,  0,  0,    1,  "YES"},

    /*cat    schem  proc_name                               col_name        col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test4_func_noparam", "RETURN_VALUE", SQL_RETURN_VALUE, SQL_VARCHAR, "varchar",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      32,  32,      0,  0,     SQL_NULLABLE, "", 0,  SQL_VARCHAR,  0,  32,   0, "YES"},

    /*cat    schem  proc_name                  col_name           col_type         data_type type_name */
    {my_schema, 0,     "procedure_columns_test5", "re_param_set_01", SQL_PARAM_INPUT, SQL_CHAR,  "char",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      14,  14,      0,  0,     SQL_NULLABLE, "", 0,  SQL_CHAR,     0,  14,    1,  "YES"},

    /*cat    schem  proc_name                  col_name            col_type          data_type type_name */
    {my_schema, 0,     "procedure_columns_test5", "re_param_enum_02", SQL_PARAM_OUTPUT, SQL_CHAR,  "char",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      7,   7,       0,  0,     SQL_NULLABLE, "", 0,  SQL_CHAR,     0,  7,    2,  "YES"},
  };

  diag("Test seems to be not correct");
  return SKIP;
  
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test1");
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test2");
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test2_noparam");
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test3");
  OK_SIMPLE_STMT(Stmt, "drop function if exists procedure_columns_test4_func");
  OK_SIMPLE_STMT(Stmt, "drop function if exists procedure_columns_test4_func_noparam");
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test5");

  OK_SIMPLE_STMT(Stmt, "create procedure procedure_columns_test1(IN re_param1 TINYINT, OUT re_param2 SMALLINT," \
                "re_param3 MEDIUMINT, INOUT `re_param 4` INT UNSIGNED, OUT re_param5 BIGINT, re_param6 FLOAT(4,2)," \
                "OUT re_param7 DOUBLE(5, 3), IN re_param8 DECIMAL(10,3) unSIGned, re_param9 CHAR(32)," \
                "Out re_param10 VARCHAR(64) charset utf8, ignore_param INT, re_param11 long VARBINARY)" \
                "begin end;"
                );
  OK_SIMPLE_STMT(Stmt, "create procedure procedure_columns_test2(IN re_paramA bloB," \
                "IN re_paramB LONGBLOB, inout re_paramC TinyBlob, re_paramD mediumblob, IN re_paramE varbinary(128)," \
                "OUT re_paramF binary, re_paramG binary(8), `re_param H` LONG VARCHAR, IN re_paramI TEXT," \
                "re_paramJ mediumtext, INOUT re_paramK longtext, re_paramL tinytext, re_paramM numeric(8,2))" \
                "begin end;"
                );
  OK_SIMPLE_STMT(Stmt, "create procedure procedure_columns_test2_noparam()"\
                "begin end;"
                );
  
  OK_SIMPLE_STMT(Stmt, "create procedure procedure_columns_test3(IN re_param_00 datetime,"\
                "OUT re_param_01 date, OUT re_param_02 time, INOUT re_param_03 timestamp,"\
                "re_param_04 year)" \
                "begin end;"
                );

  OK_SIMPLE_STMT(Stmt, "create function procedure_columns_test4_func(re_paramF int) returns varchar(32) deterministic "\
                "begin return CONCAT('abc', paramF); end;"
                );

  OK_SIMPLE_STMT(Stmt, "create function procedure_columns_test4_func_noparam() returns varchar(32) deterministic "\
                "begin return 'abc'; end;"
                );

  OK_SIMPLE_STMT(Stmt, "create procedure procedure_columns_test5(IN re_param_set_01 SET('', \"one\", 'two', 'three'),"\
                "OUT re_param_enum_02 ENUM('', \"one\", 'tw)o', 'three m'))" \
                "begin end;"
                );

  CHECK_STMT_RC(Stmt, SQLProcedureColumns(Stmt, NULL, 0, NULL, 0,
                                     "procedure_columns_test%", SQL_NTS, 
                                     "re_%", SQL_NTS));

  while(SQLFetch(Stmt) == SQL_SUCCESS)
  {
    SQLCHAR buff[255] = {0}, *param_cat, *param_name;
    SQLUINTEGER col_size, buf_len, octet_len;

    param_cat= (SQLCHAR*)my_fetch_str(Stmt, buff, 1);
    IS_STR(param_cat, data_to_check[iter].c01_procedure_cat, 
           strlen(data_to_check[iter].c01_procedure_cat) + 1);

    IS_STR(my_fetch_str(Stmt, buff, 3), 
           data_to_check[iter].c03_procedure_name, 
           strlen(data_to_check[iter].c03_procedure_name) + 1);

    param_name= (SQLCHAR*)my_fetch_str(Stmt, buff, 4);
    diag("%s.%s", param_cat, param_name);
    IS_STR(param_name, data_to_check[iter].c04_column_name, 
           strlen(data_to_check[iter].c04_column_name) + 1);

    is_num(my_fetch_int(Stmt, 5), data_to_check[iter].c05_column_type);

    is_num(my_fetch_int(Stmt, 6), data_to_check[iter].c06_data_type);

    IS_STR(my_fetch_str(Stmt, buff, 7), 
           data_to_check[iter].c07_type_name, 
           strlen(data_to_check[iter].c07_type_name) + 1);

    col_size= my_fetch_int(Stmt, 8);
    is_num(col_size, data_to_check[iter].c08_column_size);

    buf_len= my_fetch_int(Stmt, 9);
    is_num(buf_len, data_to_check[iter].c09_buffer_length);

    diag("Iter: %d", iter);
    is_num(my_fetch_int(Stmt, 10), data_to_check[iter].c10_decimal_digits);
    
    is_num(my_fetch_int(Stmt, 11), data_to_check[iter].c11_num_prec_radix);

    is_num(my_fetch_int(Stmt, 15), data_to_check[iter].c15_sql_data_type);

    is_num(my_fetch_int(Stmt, 16), data_to_check[iter].c16_sql_datetime_sub);

    octet_len= my_fetch_int(Stmt, 17);
    is_num(octet_len, data_to_check[iter].c17_char_octet_length);

    is_num(my_fetch_int(Stmt, 18), data_to_check[iter].c18_ordinal_position);

    IS_STR(my_fetch_str(Stmt, buff, 19), 
           data_to_check[iter].c19_is_nullable, 
           strlen(data_to_check[iter].c19_is_nullable + 1));

    ++iter;
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test1");
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test2");
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test2_noparam");
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test3");
  OK_SIMPLE_STMT(Stmt, "drop function if exists procedure_columns_test4_func");
  OK_SIMPLE_STMT(Stmt, "drop function if exists procedure_columns_test4_func_noparam");
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists procedure_columns_test5");
  

  return OK;
}


ODBC_TEST(t_bug57182)
{
  SQLLEN nRowCount;
  SQLCHAR buff[24];

  OK_SIMPLE_STMT(Stmt, "drop procedure if exists bug57182");
  OK_SIMPLE_STMT(Stmt, "CREATE DEFINER=`adb`@`%` PROCEDURE `bug57182`(in id int, in name varchar(20)) "
    "BEGIN"
    "  insert into simp values (id, name);"
    "END");

  CHECK_STMT_RC(Stmt, SQLProcedureColumns(Stmt, my_schema, SQL_NTS, NULL, 0,
    "bug57182", SQL_NTS, 
    NULL, 0));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nRowCount));
  is_num(2, nRowCount)
  
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  
  IS_STR(my_fetch_str(Stmt, buff, 3), "bug57182", 9);
  IS_STR(my_fetch_str(Stmt, buff, 4), "id", 3);
  IS_STR(my_fetch_str(Stmt, buff, 7), "int", 4);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, buff, 3), "bug57182", 9);
  IS_STR(my_fetch_str(Stmt, buff, 4), "name", 5);
  IS_STR(my_fetch_str(Stmt, buff, 7), "varchar", 8);
  is_num(my_fetch_int(Stmt, 8), 20);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "No data expected");
  
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Almost the same thing but with column specified */
  CHECK_STMT_RC(Stmt, SQLProcedureColumns(Stmt, my_schema, SQL_NTS, NULL, 0,
    "bug57182", SQL_NTS, 
    "id", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nRowCount));
  is_num(1, nRowCount)

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, buff, 3), "bug57182", 9);
  IS_STR(my_fetch_str(Stmt, buff, 4), "id", 3);
  IS_STR(my_fetch_str(Stmt, buff, 7), "int", 4);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "No data expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* And testing impossible column condition - expecting to get no rows */
  CHECK_STMT_RC(Stmt, SQLProcedureColumns(Stmt, my_schema, SQL_NTS, NULL, 0,
    "bug57182", SQL_NTS, 
    "non_existing_column%", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nRowCount));
  is_num(0, nRowCount);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA, "No data expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop procedure if exists bug57182");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  return OK;
}


/* SQLRowCount() doesn't work with SQLTables and other functions
   Testing of that with SQLTables, SQLColumn is incorporated in other testcases
*/
ODBC_TEST(t_bug55870)
{
  SQLLEN  rowCount;
  SQLCHAR noI_SconnStr[512], query[256];
  SQLHDBC    hdbc1;
  SQLHSTMT   hstmt1;

  OK_SIMPLE_STMT(Stmt, "drop table if exists bug55870r");
  OK_SIMPLE_STMT(Stmt, "drop table if exists bug55870_2");
  OK_SIMPLE_STMT(Stmt, "drop table if exists bug55870");
  OK_SIMPLE_STMT(Stmt, "create table bug55870(a int not null primary key, "
    "b varchar(20) not null, c varchar(100) not null, INDEX(b)) ENGINE=InnoDB");

  /* There should be no problems with I_S version of SQLTablePrivileges. Thus need connection
     not using I_S. SQlStatistics doesn't have I_S version, but it ma change at certain point.
     Thus let's test it on NO_I_S connection too */
  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));

  sprintf((char *)noI_SconnStr, "DSN=%s;UID=%s;PWD=%s; NO_I_S=1", my_dsn, my_uid, my_pwd);

  sprintf(query, "grant Insert, Select on bug55870 to %s", my_uid);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, query, SQL_NTS));
  sprintf(query, "grant Insert (c), Select (c), Update (c) on bug55870 to %s", my_uid);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, query, SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, noI_SconnStr, sizeof(noI_SconnStr), NULL,
                                0, NULL, SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLStatistics(hstmt1, NULL, 0, NULL, 0,
                                   "bug55870", SQL_NTS,
                                   SQL_INDEX_UNIQUE, SQL_QUICK));
  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rowCount));
  is_num(rowCount, 1);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLTablePrivileges(hstmt1, my_schema, SQL_NTS, 0, 0, "bug55870",
                                    SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rowCount));
  is_num(rowCount, my_print_non_format_result(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_STMT_RC(hstmt1, SQLColumnPrivileges(hstmt1, my_schema, SQL_NTS, 0, 0, "bug55870",
                                      SQL_NTS, "c", SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rowCount));
  is_num(rowCount, my_print_non_format_result(hstmt1));

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "create table bug55870_2 (id int not null primary key, value "
                "varchar(255) not null) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "create table bug55870r (id int unsigned not null primary key,"
                "refid int not null, refid2 int not null,"
                "somevalue varchar(20) not null,  foreign key b55870fk1 (refid) "
                "references bug55870 (a), foreign key b55870fk2 (refid2) "
                "references bug55870_2 (id)) ENGINE=InnoDB");

  /* actually... looks like no-i_s version of SQLForeignKeys is broken on latest
     server versions. comment in "show table status..." contains nothing */
  CHECK_STMT_RC(hstmt1, SQLForeignKeys(hstmt1, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
    NULL, 0, (SQLCHAR *)"bug55870r", SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rowCount));
  is_num(rowCount, my_print_non_format_result(hstmt1));

  /** surprise-surprise - just removing table is not enough to remove related
      records from tables_priv and columns_priv
  */
  sprintf(query, "revoke select,insert on bug55870 from %s", my_uid);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, query, SQL_NTS));

  sprintf(query, "revoke select (c),insert (c),update (c) on bug55870 from %s", my_uid);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, query, SQL_NTS));

  /*
  OK_SIMPLE_STMT(Stmt, "drop table if exists bug55870r");
    OK_SIMPLE_STMT(Stmt, "drop table if exists bug55870_2");
    OK_SIMPLE_STMT(Stmt, "drop table if exists bug55870");*/
  

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


/**
 Bug#31067 test. Testing only part of the patch, as the report itself
 is about Access and can't be tested automatically. The test checks
 if SQLColumns returns correct default value if table's catalog specified.
*/
ODBC_TEST(t_bug31067)
{
  SQLCHAR    buff[512];

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS bug31067");
  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE bug31067");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug31067.a (a varchar(10) not null default 'bug31067')");

  /* Get the info from just one table.  */
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, (SQLCHAR *)"bug31067", SQL_NTS, NULL, SQL_NTS,
                             (SQLCHAR *)"a", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, buff, 13), "'bug31067'", 11);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE bug31067");

  return OK;
}


/* Some catalog functions can return only one row of result if previous prepare
   statement pre-execution has failed */
ODBC_TEST(bug12824839)
{
  SQLLEN      row_count;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS b12824839");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS b12824839a");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE b12824839 "
                "(id int primary key, vc_col varchar(32), int_col int)");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE b12824839a "
                "(id int, vc_col varchar(32) UNIQUE, int_col int,"
                "primary key(id,int_col))");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, my_schema, SQL_NTS, NULL, 0, "b12824839",
                            SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &row_count));
  is_num(3, row_count);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLPrimaryKeys(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
                                "b12824839a", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &row_count));
  is_num(2, row_count);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* SQLColumns was not completely fixed */
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, my_schema, SQL_NTS, NULL, 0, NULL,
                            SQL_NTS, NULL, 0));
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &row_count));

  /* There should be records at least for those 2 tables we've created */
  FAIL_IF(row_count < 6, "expected >= 6 rows");

  return OK;
}


/* If no default database is selected for the connection, call of SQLColumns
   will cause error "Unknown database 'null'" */
ODBC_TEST(sqlcolumns_nodbselected)
{
  SQLHDBC hdbc1;
  SQLHSTMT hstmt1;
  SQLCHAR conn_in[512];

  /* Just to make sure we have at least one table in our test db */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS sqlcolumns_nodbselected");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE sqlcolumns_nodbselected (id int)");

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));

  /* Connecting not specifying default db */
  sprintf((char *)conn_in, "DRIVER=%s;SERVER=%s;UID=%s;PWD=%s", my_drivername,
                              "localhost", my_uid, my_pwd);

  

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn_in, sizeof(conn_in), NULL,
                                 0, NULL,
                                 SQL_DRIVER_NOPROMPT));



  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_DBC_RC(hdbc1, SQLGetInfo(hdbc1, SQL_DATABASE_NAME,
            (SQLPOINTER) conn_in, sizeof(conn_in), NULL));
  IS_STR("null", conn_in, 5);

  CHECK_STMT_RC(hstmt1, SQLColumns(hstmt1, my_schema, SQL_NTS, NULL, 0, NULL,
                            0, NULL, 0));

  CHECK_DBC_RC(hdbc1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS sqlcolumns_nodbselected");

  return OK;
}


/**
 Bug#14085211 test. LONG TABLE NAMES CRASH OBDC DRIVER
 We will try creating databases, tables and columns with the
 maximum allowed length of 64 symbols and also try to give
 the driver very long (>1024 symbols) names to make it crash.
*/
ODBC_TEST(t_bug14085211_part1)
{
  SQLCHAR  buff[8192];
  SQLCHAR  db_64_name[65]  = "database_64_symbols_long_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  SQLCHAR  tab_64_name[65] = "table____64_symbols_long_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  SQLCHAR  col_64_name[65] = "column___64_symbols_long_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

  SQLCHAR  tab_1024_name[1025] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"\
                             "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

  sprintf(buff, "DROP DATABASE IF EXISTS %s", db_64_name);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, buff, SQL_NTS));

  sprintf(buff, "CREATE DATABASE %s", db_64_name);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, buff, SQL_NTS));

  sprintf(buff, "CREATE TABLE %s.%s(%s varchar(10))", db_64_name, tab_64_name, col_64_name);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, buff, SQL_NTS));

  /* Lets check if SQLTables can get these long names  */
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, (SQLCHAR *)db_64_name, SQL_NTS, NULL, SQL_NTS,
                                  (SQLCHAR *)tab_64_name, SQL_NTS, 
                                  "BASE TABLE" /*,VIEW" */, SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  /* check the database name */
  IS_STR(my_fetch_str(Stmt, buff, 1), db_64_name, 64);

  /* check the table name */
  IS_STR(my_fetch_str(Stmt, buff, 3), tab_64_name, 64);
  
  /* only one db/table match, so nothing should be in the results */
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "expected no data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Lets check if SQLTables can ignore 1024-characters for table name */
  FAIL_IF(SQLTables(Stmt, (SQLCHAR *)tab_1024_name, SQL_NTS, NULL, SQL_NTS,
                                  (SQLCHAR *)tab_1024_name, SQL_NTS, 
                                  "TABLE,VIEW", SQL_NTS) !=  SQL_ERROR, "Error expected");
  is_num(check_sqlstate(Stmt, "HY090"), OK);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  sprintf(buff, "DROP DATABASE IF EXISTS %s", db_64_name);
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, buff, SQL_NTS));

  return OK;
}


ODBC_TEST(t_bug14085211_part2)
{
  /* 
    TODO: test all catalog functions for extreme lengths of
          database, table and column names     
  */
  return OK;
}


/* Bug#14338051 SQLCOLUMNS WORKS INCORRECTLY IF CALLED AFTER A STATEMENT
                RETURNING RESULT
   I expect that some other catalog function can be vulnerable, too */
ODBC_TEST(t_sqlcolumns_after_select)
{
  OK_SIMPLE_STMT(Stmt, "DROP TABLE if exists b14338051");

  OK_SIMPLE_STMT(Stmt,"CREATE TABLE b14338051("
               "blob_field BLOB, text_field TEXT )");

  OK_SIMPLE_STMT(Stmt, "insert into b14338051 "
                "set blob_field= 'blob', text_field= 'text'; ");

  OK_SIMPLE_STMT(Stmt, "SELECT 'blob', 'text'");


  while (SQL_SUCCEEDED(SQLFetch(Stmt)))
  {
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, 0, NULL, 0,
                          (SQLCHAR *)"b14338051",
                          strlen("b14338051"), NULL, 0));

  is_num(myresult(Stmt), 2);

  return OK;
}

/* Bug #14555713 USING ADO, ODBC DRIVER RETURNS WRONG TYPE AND VALUE FOR BIT(>1)
                 FIELD.
   Parameters datatypes returned for SP bit(n) parameters are inconsistent with
   those retruned for corresponding column types.
 */
ODBC_TEST(t_bug14555713)
{
  OK_SIMPLE_STMT(Stmt, "drop procedure if exists b14555713");

  OK_SIMPLE_STMT(Stmt, "create procedure b14555713(OUT p1 bit(1), OUT p2 bit(9)) \
                begin\
                 set p1= 1;\
                 set p2= b'100100001';\
                end");

  CHECK_STMT_RC(Stmt, SQLProcedureColumns(Stmt, NULL, 0, NULL, 0,
                                     "b14555713", SQL_NTS, 
                                     "p%", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 6), SQL_BIT);
  is_num(my_fetch_int(Stmt, 8), 1);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 8), 2);
  is_num(my_fetch_int(Stmt, 6), SQL_BINARY);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop procedure if exists b14555713");
  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {my_columns_null, "my_columns_null"},
  {my_drop_table, "my_drop_table"},
  {my_table_dbs, "my_table_dbs"},
  {my_colpriv, "my_colpriv"},
  {t_sqlprocedures, "t_sqlprocedures"},
  {t_catalog, "t_catalog"},
  {tmysql_specialcols, "tmysql_specialcols"},
  {t_columns, "t_columns"},
  {t_tables_bug,"t_tables_bug"},
  {t_current_catalog_unicode, "t_current_catalog_unicode"},
  {t_current_catalog_ansi, "t_current_catalog_ansi"},
  {tmysql_showkeys, "tmysql_showkeys"},
  {t_sqltables, "t_sqltables"},
  {my_information_schema, "my_information_schema"},
  {t_bug4518, "t_bug4518"},
  {empty_set, "empty_set"},
  {t_bug23031, "t_bug23031"},
  {bug15713, "bug15713"},
  {t_bug28316, "t_bug28316"},
  {bug8860, "bug8860"},
  {t_bug26934, "t_bug26934"},
  {t_bug29888, "t_bug29888"},
  {t_bug14407, "t_bug14407"}, 
  {t_bug19923, "t_bug19923"},
  {t_bug32864, "t_bug32864"},
  {t_bug32989, "t_bug32989"},
  {t_bug33298, "t_bug33298"},
  {t_bug12805, "t_bug12805"},
  {t_bug30770,"t_bug30770"},
  {t_bug36275, "t_bug36275"},
  {t_bug39957, "t_bug39957"}, 
  {t_bug37621, "t_bug37621"},
  {t_bug34272, "t_bug34272"},
  {t_bug49660, "t_bug49660"},
  {t_bug51422, "t_bug51422"},
  {t_bug36441, "t_bug36441"},
  {t_bug53235, "t_bug53235"},
  {t_bug50195, "t_bug50195"},
  {t_sqlprocedurecolumns, "t_sqlprocedurecolumns"},
  {t_bug57182, "t_bug57182"},
  {t_bug55870, "t_bug55870"},
  {t_bug31067, "t_bug31067"},
  {bug12824839, "bug12824839"},
  {sqlcolumns_nodbselected, "sqlcolumns_nodbselected"},
  {t_bug14085211_part1, "t_bug14085211_part1"},
  {t_sqlcolumns_after_select, "t_sqlcolumns_after_select"},
  {t_bug14555713, "t_bug14555713"},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
