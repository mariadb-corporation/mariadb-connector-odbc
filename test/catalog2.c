/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2017, 2024 MariaDB Corporation AB

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

/* Columns order num for SQLColumns */
#define COLUMN_NAME        4
#define DATA_TYPE          5
#define COLUMN_SIZE        7
#define BUFFER_LENGTH      8
#define DECIMAL_DIGITS     9
#define NUM_PREC_RADIX    10
#define NULLABLE          11
#define SQL_DATA_TYPE     14
#define SQL_DATETIME_SUB  15
#define CHAR_OCTET_LENGTH 16
#define ORDINAL_POSITION  17
/*
  Bug #37621 - SQLDescribeCol returns incorrect values of SQLTables data
*/
ODBC_TEST(t_bug37621)
{
  SQLCHAR szColName[128];
  SQLSMALLINT iName, iType, iScale, iNullable;
  SQLULEN uiDef;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug37621");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug37621 (x INT)");
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, 0, NULL, 0,
			   (SQLCHAR *)"t_bug37621", SQL_NTS, NULL, 0));
/*
  Check column properties for the REMARKS column
*/
  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 5, szColName, sizeof(szColName), 
          &iName, &iType, &uiDef, &iScale, &iNullable));

  IS_STR(szColName, "REMARKS", 8);
  is_num(iName, 7);
  // In MySQL corresponding field is TEXT, I don't think it's a problem if REMARKS is long varchar
  if (!(iType == SQL_VARCHAR || iType == SQL_WVARCHAR || (IsMysql && (iType == SQL_LONGVARCHAR || iType == SQL_WLONGVARCHAR))))
  {
    diag("REMARKS described with wrong data type %hd", iType);
    return FAIL;
  }
  /* This can fail for the same reason as t_bug32864 */
  if (!IsMysql)
  {
    // As mentioned above - on MySQL it's blob. We not gonna check its size. Let's assume the right value is returned there ;)
    is_num(uiDef, 2048);
  }
  is_num(iScale, 0);
  // Again, I maybe need to look into it. But for TEXT field is considered NULLABLE. I don't know if mysql can have NULL there, but unlikely.
  // On other hand catalog function result unlikely should be nullable per se. So, maybe we need to tweak metadata hier, but it does not look
  // like a huge problem
  is_num(iNullable, IsMysql);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug37621");

  return OK;
}


/*
Bug #34272 - SQLColumns returned wrong values for (some) TYPE_NAME
and (some) IS_NULLABLE
*/
ODBC_TEST(t_bug34272)
{
  SQLCHAR dummy[20];
  SQLULEN col6= 0, col18= 0;
  SQLLEN length= 0, buf_len= 0;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug34272");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug34272 (x INT UNSIGNED)");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
    (SQLCHAR *)"t_bug34272", SQL_NTS, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 6, dummy, sizeof(dummy), NULL, NULL,
    &col6, NULL, NULL));
  CHECK_STMT_RC(Stmt, SQLDescribeCol(Stmt, 18, dummy, sizeof(dummy), NULL, NULL,
    &col18, NULL, NULL));
  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 6, SQL_DESC_DISPLAY_SIZE, NULL, 0, NULL, &length));
  FAIL_IF(length < 0, "Returned negative display length value for 32b SQLLEN");
  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 6, SQL_DESC_LENGTH, NULL, 0, NULL, &length));
  FAIL_IF(length < 0, "Returned negative column length value for 32b SQLLEN");
  /* On ODBC2 tests run iODBC this field identifier is invalid */
  if (!iOdbc() || OdbcVer != SQL_OV_ODBC2)
  {
    CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, 6, SQL_DESC_OCTET_LENGTH, NULL, 0, NULL, &length));
    FAIL_IF(length < 0, "Returned negative octet length value for 32b SQLLEN");
  }

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  buf_len= col6 + 1;
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 6, SQL_C_CHAR, dummy, buf_len < 0 ? 0x7FFFFFFF : buf_len, &length));
  is_num(length,12);
  IS_STR(dummy, "INT UNSIGNED", length+1);

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 18, SQL_C_CHAR, dummy, col18 + 1, &length));
  is_num(length,3);
  IS_STR(dummy, "YES", length+1);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug34272");

  return OK;
}


/*
  Bug #49660 - constraints with same name for tables with same name but in different db led
  to doubling of results of SQLForeignKeys
*/
ODBC_TEST(t_bug49660)
{
  SQLLEN rowsCount;

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE IF EXISTS bug49660");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug49660");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug49660_r");

  OK_SIMPLE_STMT(Stmt, "CREATE DATABASE bug49660");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug49660.t_bug49660_r (id INT UNSIGNED NOT NULL PRIMARY KEY, name VARCHAR(10) NOT NULL) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug49660.t_bug49660 (id INT UNSIGNED NOT NULL PRIMARY KEY, refid INT UNSIGNED NOT NULL,"
                "FOREIGN KEY t_bug49660fk (id) REFERENCES bug49660.t_bug49660_r (id)) ENGINE=InnoDB");

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug49660_r (id INT UNSIGNED NOT NULL PRIMARY KEY, name VARCHAR(10) NOT NULL) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug49660 (id INT UNSIGNED NOT NULL PRIMARY KEY, refid INT UNSIGNED NOT NULL,"
                "FOREIGN KEY t_bug49660fk (id) REFERENCES t_bug49660_r (id)) ENGINE=InnoDB");

  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                NULL, 0, (SQLCHAR *)"t_bug49660", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowsCount));
  is_num(rowsCount, 1); 
  /* Going another way around - sort of more reliable */
  FAIL_IF(SQLFetch(Stmt) == SQL_NO_DATA_FOUND, "expected data");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt,SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP DATABASE bug49660");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug49660");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_bug49660_r");

  return OK;
}


/*
  Bug #51422 - SQLForeignKeys returned keys pointing to unique fields
*/
ODBC_TEST(t_bug51422)
{
  SQLLEN rowsCount;
  
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

OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug36441_0123456789");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug36441_0123456789("
	              "pk_for_table1 INTEGER NOT NULL AUTO_INCREMENT,"
	              "c1_for_table1 VARCHAR(128) NOT NULL UNIQUE,"
	              "c2_for_table1 BINARY(32) NULL,"
                "unique_key INT UNSIGNED NOT NULL UNIQUE,"
	              "PRIMARY KEY(pk_for_table1, c1_for_table1))");

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
  char        expected_privs[][24]={ "ALTER", "CREATE", "CREATE VIEW", "DELETE", "DROP", "INDEX",
                                    "INSERT", "REFERENCES", "SHOW VIEW", "TRIGGER", "UPDATE" },
              expected_103[][24]={ "ALTER", "CREATE", "CREATE VIEW", "DELETE", "DELETE VERSIONING ROWS",
                         "DROP", "INDEX", "INSERT", "REFERENCES", "SHOW VIEW", "TRIGGER", "UPDATE" },
              *expected= expected_privs[0];
  int         i, privs_count= sizeof(expected_privs)/sizeof(expected_privs[0]);
  SQLCHAR     priv[24], dropUser[24 + sizeof(my_host)], createUser[52 + sizeof(my_host)], grantAll[36 + sizeof(my_host)], revokeSelect[44 + sizeof(my_host)];
  SQLLEN      len;

  SKIPIF(IsMaxScale || IsSkySqlHa, "Not stable with Maxscale - error on login with created user");

  if (ServerNotOlderThan(Connection, 10, 3, 4))
  {
    expected= expected_103[0];
    privs_count= sizeof(expected_103)/sizeof(expected_103[0]);

    if (ServerNotOlderThan(Connection, 10, 3, 15))
    {
      strcpy(expected_103[4], "DELETE HISTORY");
    }
  }
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug50195");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug50195 (i INT NOT NULL)");

  /* Basically this can be used for Travis as well, and the if's above and below can be removed*/
  _snprintf(dropUser, sizeof(dropUser), "DROP USER bug50195@'%s'", my_host);
  _snprintf(createUser, sizeof(createUser), "CREATE USER bug50195@'%s' IDENTIFIED BY 's3CureP@wd'", my_host);
  _snprintf(grantAll, sizeof(grantAll), "GRANT ALL ON bug50195 TO bug50195@'%s'", my_host);
  _snprintf(revokeSelect, sizeof(revokeSelect), "REVOKE SELECT ON bug50195 FROM bug50195@'%s'", my_host);
  SQLExecDirect(Stmt, dropUser, SQL_NTS);

  OK_SIMPLE_STMT(Stmt, createUser);

  if (!SQL_SUCCEEDED(SQLExecDirect(Stmt, grantAll, SQL_NTS)))
  {
    odbc_print_error(SQL_HANDLE_STMT, Stmt);
    if (get_native_errcode(Stmt) == 1142)
    {
      skip("Test user doesn't have enough privileges to run this test");
    }
    return FAIL;
  }

  OK_SIMPLE_STMT(Stmt, revokeSelect);

  OK_SIMPLE_STMT(Stmt, "FLUSH PRIVILEGES");

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));

  hstmt1= DoConnect(hdbc1, FALSE, my_dsn, "bug50195", "s3CureP@wd",  0, NULL, NULL, NULL, NULL);

  if (hstmt1 == NULL)
  {
    diag("Couldn't connect with new user or allocate the stmt");

    OK_SIMPLE_STMT(Stmt, dropUser);
    OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug50195");

    return FAIL;
  }

  CHECK_STMT_RC(hstmt1, SQLTablePrivileges(hstmt1, NULL, 0, 0, 0, "bug50195", SQL_NTS));

  /* Testing SQLTablePrivileges a bit, as we don't have separate test of it */

  for(i= 0; i < privs_count; ++i)
  {
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
    CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 6, SQL_C_CHAR, priv, sizeof(priv), &len));
    IS_STR(priv, expected, len);
    expected+= sizeof(expected_privs[0]);
  }
  
  FAIL_IF(SQLFetch(hstmt1) != 100, "No data expected");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  OK_SIMPLE_STMT(Stmt, dropUser);
  
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug50195");

  return OK;
}


ODBC_TEST(t_sqlprocedurecolumns)
{
  SQLHDBC   Hdbc1;
  SQLHSTMT  Hstmt1;
  SQLRETURN rc= 0;
  SQLCHAR   szName[255]= {0};

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
  } sqlproccol;

  int total_params= 0, iter= 0;

  sqlproccol re_names[] = {
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
    {my_schema, 0,     "procedure_columns_test1", "re_param6", SQL_PARAM_INPUT, SQL_DECIMAL,  "float",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      4,   6,       2,  10,     SQL_NULLABLE, "", 0,  SQL_DECIMAL,     0,  0,    6,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param7", SQL_PARAM_OUTPUT, SQL_DECIMAL,  "double",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
       5,  7,       2,  10,     SQL_NULLABLE, "", 0,  SQL_DECIMAL,   0,  0,    7,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param8", SQL_PARAM_INPUT, SQL_DECIMAL,  "decimal",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      10,  11,       3,  10,    SQL_NULLABLE, "", 0,  SQL_DECIMAL,   0,  0,    8,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param9", SQL_PARAM_INPUT, SQL_WCHAR,  "char",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      32,  32,      0,  0,     SQL_NULLABLE, "", 0,  SQL_WCHAR,     0,  32,    9,  "YES"},

    /*cat    schem  proc_name                  col_name      col_type           data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param10", SQL_PARAM_OUTPUT, SQL_WVARCHAR, "varchar",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      64,  64,      0,  0,     SQL_NULLABLE, "", 0,  SQL_WVARCHAR,  0,  64,   10, "YES"},

    /*cat    schem  proc_name                  col_name      col_type         data_type          type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param11", SQL_PARAM_INPUT, SQL_LONGVARBINARY, "mediumblob",
    /*size      buf_len   dec radix  nullable      rem def sql_data_type       sub octet     pos nullable*/
      16777215, 16777215, 0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARBINARY,  0,  16777215, 12, "YES"},

    /*cat    schem  proc_name                  col_name     col_type          data_type    type_name */
    {my_schema, 0,     "procedure_columns_test1", "re_param12", SQL_PARAM_INPUT, SQL_DOUBLE,  "double",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      15,  8,       0,  10,     SQL_NULLABLE, "", 0,  SQL_DOUBLE,   0,  0,    13,  "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type  type_name */
    { my_schema, 0,     "procedure_columns_test1", "re_param13", SQL_PARAM_INPUT, SQL_REAL,  "float",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      7,   4,       0,  10,     SQL_NULLABLE, "", 0,  SQL_REAL,     0,  0,    14,  "YES" },
    /*----------------------------------------------------------------------------------------------------*/
    /*cat    schem  proc_name                  col_name     col_type         data_type          type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramA", SQL_PARAM_INPUT, SQL_LONGVARBINARY, "blob",
    /*size      buf_len   dec radix  nullable      rem def sql_data_type       sub octet     pos nullable*/
      65535,    65535,    0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARBINARY,  0,  65535,    1, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type          type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramB", SQL_PARAM_INPUT, SQL_LONGVARBINARY, "longblob",
    /*size        buf_len      dec radix  nullable      rem def sql_data_type       sub octet        pos nullable*/
    IsMysql ? 4294967295L : 2147483647L, IsMysql ? 4294967295L : 2147483647L, 0,  0,     SQL_NULLABLE, "", 0,  SQL_LONGVARBINARY,  0,
    IsMysql ? 4294967295L : 2147483647L, 2, "YES"},

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
    {my_schema, 0,     "procedure_columns_test2", "re_param H", SQL_PARAM_INPUT, SQL_WLONGVARCHAR, "mediumtext",
    /*size     buf_len   dec radix  nullable      rem def sql_data_type       sub octet     pos nullable*/
     16777215, 16777215, 0,  0,     SQL_NULLABLE, "", 0,  SQL_WLONGVARCHAR,  0,  16777215, 8, "YES"},

    /*cat    schem  proc_name                  col_name      col_type         data_type       type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramI", SQL_PARAM_INPUT, SQL_WLONGVARCHAR, "text",
    /*size      buf_len   dec radix  nullable      rem def sql_data_type    sub octet  pos nullable*/
      65535,    65535,    0,  0,     SQL_NULLABLE, "", 0,  SQL_WLONGVARCHAR, 0,  65535,  9, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type       type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramJ", SQL_PARAM_INPUT, SQL_WLONGVARCHAR, "mediumtext",
    /*size     buf_len   dec radix  nullable      rem def sql_data_type    sub octet     pos nullable*/
     16777215, 16777215, 0,  0,     SQL_NULLABLE, "", 0,  SQL_WLONGVARCHAR, 0,  16777215, 10, "YES"},

     /*cat    schem  proc_name                  col_name    col_type              data_type        type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramK", SQL_PARAM_INPUT_OUTPUT, SQL_WLONGVARCHAR, "longtext",
    /*size        buf_len      dec radix  nullable      rem def sql_data_type    sub octet        pos nullable*/
     IsMysql ? 4294967295L : 2147483647L, IsMysql ? 4294967295L : 2147483647L, 0,  0,     SQL_NULLABLE, "", 0,  SQL_WLONGVARCHAR, 0,
     IsMysql ? 4294967295L : 2147483647L, 11, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type        type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramL", SQL_PARAM_INPUT, SQL_WLONGVARCHAR, "tinytext",
    /*size   buf_len dec radix  nullable      rem def sql_data_type    sub octet pos nullable*/
     255,    255,    0,  0,     SQL_NULLABLE, "", 0,  SQL_WLONGVARCHAR, 0,  255,  12, "YES"},

    /*cat    schem  proc_name                  col_name     col_type         data_type    type_name */
    {my_schema, 0,     "procedure_columns_test2", "re_paramM", SQL_PARAM_INPUT, SQL_DECIMAL,  "decimal",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      8,   10,      2,  10,    SQL_NULLABLE, "", 0,  SQL_DECIMAL,   0,  0,   13,  "YES"},
    /*----------------------------------------------------------------------------------------------------*/
    /*cat    schem  proc_name                  col_name       col_type         data_type     type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_00", SQL_PARAM_INPUT, SQL_TYPE_TIMESTAMP, "datetime",
    /*size buf_len  dec radix  nullable      rem def sql_data_type  sub                  octet pos nullable*/
      19,  16,      0,   0,     SQL_NULLABLE, "", 0, SQL_DATETIME,  SQL_CODE_TIMESTAMP,  0,    1,  "YES"},

    /*cat    schem  proc_name                  col_name       col_type          data_type      type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_01", SQL_PARAM_OUTPUT, SQL_TYPE_DATE, "date",
    /*size buf_len  dec radix  nullable      rem def sql_data_type  sub             octet pos nullable*/
      10,  6,       0,  0,     SQL_NULLABLE, "", 0,  SQL_DATETIME,  SQL_CODE_DATE,  0,    2,  "YES"},

    /*cat    schem  proc_name                  col_name       col_type          data_type      type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_02", SQL_PARAM_OUTPUT, SQL_TYPE_TIME, "time",
    /*size buf_len  dec radix  nullable      rem def sql_data_type  sub octet pos nullable*/
      8,   6,       0,  0,     SQL_NULLABLE, "", 0,  SQL_DATETIME,  SQL_CODE_TIME,  0,    3,  "YES"},

    /*cat    schem  proc_name                  col_name       col_type                data_type           type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_03", SQL_PARAM_INPUT_OUTPUT, SQL_TYPE_TIMESTAMP, "timestamp",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub                  octet pos nullable*/
      19,  16,      0,  0,     SQL_NULLABLE, "", 0,  SQL_DATETIME, SQL_CODE_TIMESTAMP,  0,    4,  "YES"},

    /*cat    schem  proc_name                  col_name       col_type         data_type     type_name */
    {my_schema, 0,     "procedure_columns_test3", "re_param_04", SQL_PARAM_INPUT, SQL_SMALLINT, "year",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      4,   2,       0,   0,    SQL_NULLABLE, "", 0,  SQL_SMALLINT, 0,  0,    5,  "YES"},
    /*----------------------------------------------------------------------------------------------------*/
     /*cat    schem  proc_name                       col_name    col_type         data_type    type_name */
    {my_schema, 0,     "procedure_columns_test4_func", "re_paramF", SQL_PARAM_INPUT, SQL_INTEGER, "int",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
     10,     4,       0,  10,  SQL_NULLABLE, "", 0,  SQL_INTEGER,  0,  0,    1,  "YES"},
    /*----------------------------------------------------------------------------------------------------*/
    /*cat    schem  proc_name                  col_name           col_type         data_type type_name */
    {my_schema, 0,     "procedure_columns_test5", "re_param_set_01", SQL_PARAM_INPUT, SQL_WCHAR,  "set",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      14,  14,      0,  0,     SQL_NULLABLE, "", 0,  SQL_WCHAR,     0,  14,    1,  "YES"},

    /*cat    schem  proc_name                  col_name            col_type          data_type type_name */
    {my_schema, 0,     "procedure_columns_test5", "re_param_enum_02", SQL_PARAM_OUTPUT, SQL_WCHAR,  "enum",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
      7,   7,       0,  0,     SQL_NULLABLE, "", 0,  SQL_WCHAR,     0,  7,    2,  "YES"},
  };
  
  sqlproccol empty_name[]= {
    /*cat    schem  proc_name                       col_name        col_type          data_type    type_name */
    { my_schema, 0,     "procedure_columns_test4_func", "", SQL_RETURN_VALUE, SQL_WVARCHAR, "varchar",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
    32,  32,      0,  0,     SQL_NULLABLE, "", 0,  SQL_WVARCHAR,  0,  32,   0, "YES"},
    /*cat    schem  proc_name                               col_name        col_type          data_type    type_name */
    { my_schema, 0,     "procedure_columns_test4_func_noparam", "", SQL_RETURN_VALUE, SQL_WVARCHAR, "varchar",
    /*size buf_len  dec radix  nullable      rem def sql_data_type sub octet pos nullable*/
    32,  32,      0,  0,     SQL_NULLABLE, "", 0,  SQL_WVARCHAR,  0,  32,   0, "YES"}},
    *data_to_check;
  int i;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc1));
  /* Since results(buffer length) depends on connection charset, and expected that is a single-byte charset, we need to ensure that */
  Hstmt1= ConnectWithCharset(&Hdbc1, "latin1", NULL);
  FAIL_IF(Hstmt1 == NULL, "Could not establish connection or allocate stmt handle");

  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test1");
  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test2");
  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test2_noparam");
  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test3");
  OK_SIMPLE_STMT(Hstmt1, "drop function if exists procedure_columns_test4_func");
  OK_SIMPLE_STMT(Hstmt1, "drop function if exists procedure_columns_test4_func_noparam");
  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test5");

  OK_SIMPLE_STMT(Hstmt1, "create procedure procedure_columns_test1(IN re_param1 TINYINT, OUT re_param2 SMALLINT," \
                "re_param3 MEDIUMINT, INOUT `re_param 4` INT UNSIGNED, OUT re_param5 BIGINT, re_param6 FLOAT(4,2)," \
                "OUT re_param7 DOUBLE(5,2), IN re_param8 DECIMAL(10,3) unSIGned, re_param9 CHAR(32)," \
                "Out re_param10 VARCHAR(64) charset utf8, ignore_param INT, re_param11 long VARBINARY, re_param12 double, re_param13 float)" \
                "begin end;"
                );
  if (IsXpand)
  {
    OK_SIMPLE_STMT(Hstmt1, "create procedure procedure_columns_test2(IN re_paramA bloB," \
      "IN re_paramB LONGBLOB, inout re_paramC TinyBlob, re_paramD mediumblob, IN re_paramE varbinary(128)," \
      "OUT re_paramF binary, re_paramG binary(8), `re_param H` MEDIUMTEXT, IN re_paramI TEXT," \
      "re_paramJ mediumtext, INOUT re_paramK longtext, re_paramL tinytext, re_paramM numeric(8,2))" \
      "begin end;"
    );
  }
  else
  {
    OK_SIMPLE_STMT(Hstmt1, "create procedure procedure_columns_test2(IN re_paramA bloB," \
      "IN re_paramB LONGBLOB, inout re_paramC TinyBlob, re_paramD mediumblob, IN re_paramE varbinary(128)," \
      "OUT re_paramF binary, re_paramG binary(8), `re_param H` LONG VARCHAR, IN re_paramI TEXT," \
      "re_paramJ mediumtext, INOUT re_paramK longtext, re_paramL tinytext, re_paramM numeric(8,2))" \
      "begin end;"
    );
  }
  OK_SIMPLE_STMT(Hstmt1, "CREATE PROCEDURE procedure_columns_test2_noparam()"\
                "BEGIN END;"
                );
  
  OK_SIMPLE_STMT(Hstmt1, "CREATE PROCEDURE procedure_columns_test3(IN re_param_00 datetime,"\
                "OUT re_param_01 date, OUT re_param_02 time, INOUT re_param_03 timestamp,"\
                "re_param_04 year)" \
                "BEGIN END;"
                );

  OK_SIMPLE_STMT(Hstmt1, "create function procedure_columns_test4_func(re_paramF int) returns varchar(32) deterministic "\
                "begin return CONCAT('abc', re_paramF); end;"
                );

  OK_SIMPLE_STMT(Hstmt1, "create function procedure_columns_test4_func_noparam() returns varchar(32) deterministic "\
                "begin return 'abc'; end;"
                );

  OK_SIMPLE_STMT(Hstmt1, "create procedure procedure_columns_test5(IN re_param_set_01 SET('', \"one\", 'two', 'three'),"\
                "OUT re_param_enum_02 ENUM('', \"one\", 'tw)o', 'three m'))" \
                "begin end;"
                );

  CHECK_STMT_RC(Hstmt1, SQLProcedureColumns(Hstmt1, NULL, 0, NULL, 0,
                                     "procedure_columns_test%", SQL_NTS, 
                                     "re_%", SQL_NTS));
  data_to_check= re_names;
  for (i= 0; i < 2; ++i)
  {
    while(SQLFetch(Hstmt1) == SQL_SUCCESS)
      {
        SQLCHAR buff[255] = {0}, *param_cat, *param_name;
        SQLUINTEGER col_size, buf_len, octet_len;

        param_cat= (SQLCHAR*)my_fetch_str(Hstmt1, buff, 1);
        IS_STR(param_cat, data_to_check[iter].c01_procedure_cat, 
               strlen(data_to_check[iter].c01_procedure_cat) + 1);

        IS_STR(my_fetch_str(Hstmt1, buff, 3), 
               data_to_check[iter].c03_procedure_name, 
               strlen(data_to_check[iter].c03_procedure_name) + 1);

        param_name= (SQLCHAR*)my_fetch_str(Hstmt1, buff, 4);
        diag("%s.%s", data_to_check[iter].c01_procedure_cat, param_name);
        IS_STR(param_name, data_to_check[iter].c04_column_name, 
               strlen(data_to_check[iter].c04_column_name) + 1);

        is_num(my_fetch_int(Hstmt1, 5), data_to_check[iter].c05_column_type);

        if (OdbcVer == SQL_OV_ODBC2)
        {
          switch (data_to_check[iter].c06_data_type)
          {
          case SQL_TYPE_TIMESTAMP: data_to_check[iter].c06_data_type= SQL_TIMESTAMP; break;
          case SQL_TYPE_TIME: data_to_check[iter].c06_data_type= SQL_TIME; break;
          case SQL_TYPE_DATE: data_to_check[iter].c06_data_type= SQL_DATE; break;
          }
        }
        is_num(my_fetch_int(Hstmt1, 6), GetDefaultCharType(data_to_check[iter].c06_data_type,TRUE));

        IS_STR(my_fetch_str(Hstmt1, buff, 7), 
               data_to_check[iter].c07_type_name, 
               strlen(data_to_check[iter].c07_type_name) + 1);

        col_size= my_fetch_int(Hstmt1, 8);
        is_num(col_size, data_to_check[iter].c08_column_size);

        buf_len= my_fetch_int(Hstmt1, 9);
        is_num(buf_len, data_to_check[iter].c09_buffer_length);

        diag("Iter: %d", iter);
        is_num(my_fetch_int(Hstmt1, 10), data_to_check[iter].c10_decimal_digits);
    
        is_num(my_fetch_int(Hstmt1, 11), data_to_check[iter].c11_num_prec_radix);

        is_num(my_fetch_int(Hstmt1, 15), GetDefaultCharType(data_to_check[iter].c15_sql_data_type, TRUE));

        is_num(my_fetch_int(Hstmt1, 16), data_to_check[iter].c16_sql_datetime_sub);

        octet_len= my_fetch_int(Hstmt1, 17);
        is_num(octet_len, data_to_check[iter].c17_char_octet_length);

        is_num(my_fetch_int(Hstmt1, 18), data_to_check[iter].c18_ordinal_position);

        IS_STR(my_fetch_str(Hstmt1, buff, 19), 
               data_to_check[iter].c19_is_nullable, 
               strlen(data_to_check[iter].c19_is_nullable + 1));

        ++iter;
      }
    CHECK_STMT_RC(Hstmt1, SQLFreeStmt(Hstmt1, SQL_CLOSE));
    iter= 0;
    if (i == 0)
    {
      CHECK_STMT_RC(Hstmt1, SQLProcedureColumns(Hstmt1, NULL, 0, NULL, 0,
        "procedure_columns_test%", SQL_NTS,
        "", SQL_NTS));
      data_to_check= empty_name;
    }
  }
  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test1");
  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test2");
  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test2_noparam");
  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test3");
  OK_SIMPLE_STMT(Hstmt1, "drop function if exists procedure_columns_test4_func");
  OK_SIMPLE_STMT(Hstmt1, "drop function if exists procedure_columns_test4_func_noparam");
  OK_SIMPLE_STMT(Hstmt1, "drop procedure if exists procedure_columns_test5");

  CHECK_STMT_RC(Hstmt1, SQLFreeStmt(Hstmt1, SQL_DROP));
  CHECK_DBC_RC(Hdbc1, SQLDisconnect(Hdbc1));
  CHECK_DBC_RC(Hdbc1, SQLFreeConnect(Hdbc1));

  return OK;
}


ODBC_TEST(t_bug57182)
{
  SQLLEN nRowCount;
  SQLCHAR buff[24];

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS bug57182");
  if (!SQL_SUCCEEDED(SQLExecDirect(Stmt, "CREATE DEFINER=`adb`@`%` PROCEDURE `bug57182`(in id int, in name varchar(20)) "
    "BEGIN"
    "  insert into simp values (id, name);"
    "END", SQL_NTS)))
  {
    odbc_print_error(SQL_HANDLE_STMT, Stmt);
    if (get_native_errcode(Stmt) == 1227)
    {
      skip("Test user doesn't have enough privileges to run this test - this test may fail in some cases");
    }
    return FAIL;
  }

  CHECK_STMT_RC(Stmt, SQLProcedureColumns(Stmt, my_schema, SQL_NTS, NULL, 0,
    "bug57182", SQL_NTS, 
    NULL, 0));

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &nRowCount));
  is_num(2, nRowCount);
  
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
  is_num(1, nRowCount);

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

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug55870r");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug55870_2");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug55870");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug55870(a INT NOT NULL PRIMARY KEY, "
    "b VARCHAR(20) NOT NULL, c VARCHAR(100) NOT NULL, INDEX(b)) ENGINE=InnoDB");

  /* There should be no problems with I_S version of SQLTablePrivileges. Thus need connection
     not using I_S. SQlStatistics doesn't have I_S version, but it ma change at certain point.
     Thus let's test it on NO_I_S connection too */
  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));

  sprintf((char *)noI_SconnStr, "DSN=%s;UID=%s;PWD=%s;PORT=%u;NO_I_S=1;NO_CACHE=0", my_dsn, my_uid, my_pwd, my_port);

  sprintf(query, "GRANT Insert, Select ON bug55870 TO %s", my_uid);
  SQLExecDirect(Stmt, query, SQL_NTS);
  sprintf(query, "GRANT Insert (c), Select (c), Update (c) ON bug55870 to %s", my_uid);
  SQLExecDirect(Stmt, query, SQL_NTS);

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, noI_SconnStr, sizeof(noI_SconnStr), NULL,
                                0, NULL, SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  CHECK_STMT_RC(hstmt1, SQLStatistics(hstmt1, NULL, 0, NULL, 0,
                                   "bug55870", SQL_NTS,
                                   SQL_INDEX_UNIQUE, SQL_QUICK));
  if (NoCache == FALSE)
  {
    CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rowCount));
    is_num(rowCount, 1);
  }
  else
  {
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
    EXPECT_STMT(hstmt1, SQLFetch(hstmt1), SQL_NO_DATA);
    skip("Test does not make sense with result streaming turned on");
  }

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

  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug55870_2 (id INT NOT NULL PRIMARY KEY, value "
                "VARCHAR(255) NOT NULL) ENGINE=InnoDB");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug55870r (id INT UNSIGNED NOT NULL PRIMARY KEY,"
                "refid INT NOT NULL, refid2 INT NOT NULL,"
                "somevalue VARCHAR(20) NOT NULL,  FOREIGN KEY b55870fk1 (refid) "
                "REFERENCES bug55870 (a), FOREIGN KEY b55870fk2 (refid2) "
                "REFERENCES bug55870_2 (id)) ENGINE=InnoDB");

  /* actually... looks like no-i_s version of SQLForeignKeys is broken on latest
     server versions. comment in "show table status..." contains nothing */
  CHECK_STMT_RC(hstmt1, SQLForeignKeys(hstmt1, NULL, 0, NULL, 0, NULL, 0, NULL, 0,
    NULL, 0, (SQLCHAR *)"bug55870r", SQL_NTS));

  CHECK_STMT_RC(hstmt1, SQLRowCount(hstmt1, &rowCount));
  is_num(rowCount, my_print_non_format_result(hstmt1));

  /** surprise-surprise - just removing table is not enough to remove related
      records from tables_priv and columns_priv
  */
  sprintf(query, "REVOKE SELECT,INSERT ON bug55870 FROM %s", my_uid);
  SQLExecDirect(Stmt, query, SQL_NTS);

  sprintf(query, "REVOKE SELECT (c),INSERT (c),UPDATE (c) ON bug55870 FROM %s", my_uid);
  SQLExecDirect(Stmt, query, SQL_NTS);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE bug55870r");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE bug55870_2");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE bug55870");

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
  sprintf((char *)conn_in, "DRIVER=%s;SERVER=%s;UID=%s;PWD=%s;PORT=%d;%s", my_drivername,
                              my_servername, my_uid, my_pwd, my_port, add_connstr);

  

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn_in, (SQLSMALLINT)strlen(conn_in), NULL,
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
  CHECK_SQLSTATE(Stmt, IS_ODBC3() ? "HY090" : "S1009");

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
                          (SQLSMALLINT)strlen("b14338051"), NULL, 0));

  is_num(myrowcount(Stmt), 2);

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

/* Bug ODBC-38. It's impossible in MS Access to insert/update record in table containing date, time or timestamp field.
   The reason is that MS Access is ODBCv2 application, and SQLColumns returned for such fields SQL type for ODBCv3
 */
ODBC_TEST(odbc38)
{
  unsigned int i;
  SQLSMALLINT  expected[][4]= {{SQL_DATE,      SQL_TIME,      SQL_TIMESTAMP     , SQL_TIMESTAMP},       /* ODBCv2 */
                               {SQL_TYPE_DATE, SQL_TYPE_TIME, SQL_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP}}; /* ODBCv3 */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc38");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc38 (d date not null, t time not null, dt datetime not null, ts timestamp not null)");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
    (SQLCHAR *)"t_odbc38", SQL_NTS, NULL, 0));

  for (i= 0; i < sizeof(expected[0])/sizeof(SQLSMALLINT); ++i)
  {
    diag("ODBCv%d, row#%u", OdbcVer, i);
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    is_num(my_fetch_int(Stmt, DATA_TYPE), expected[OdbcVer - SQL_OV_ODBC2][i]);
  }
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc38");

  return OK;
}


/*
Bug ODBC-51 - SQLTables failed if string fields were bound as WCHAR, and of them(usually REMARKS) contained empty string
In fact the bug is not SQLTables(or aby other catalog function) specific.
*/
ODBC_TEST(odbc51)
{
  SQLRETURN rc;
  SQLWCHAR  remark[100];
  SQLLEN    len;

  /* Making sure there is at least one table with empty remarks */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS aaa_odbc51");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE aaa_odbc51 (a int not null)");
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, (SQLCHAR *)my_schema, (SQLSMALLINT)strlen(my_schema), NULL, 0, NULL, 0, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 5, SQL_C_WCHAR, remark, sizeof(remark), &len));

  do {
    rc = SQLFetch(Stmt);
    if (!SQL_SUCCEEDED(rc) && rc != SQL_NO_DATA)
    {
      /* Just to get diagnostics printed */
      CHECK_STMT_RC(Stmt, rc);
    }
  }
  while (rc != SQL_NO_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE aaa_odbc51");

  return OK;
}


/* Bug ODBC-131 regression with MS Access - error on linking a table.
   The reason is that MS Access expects and reads size of data even for fixed
   size types(that is kinda strange according to ODBC specs), and freaks out
   when that size is different from what it expects
   In fact Access reads columns DATA_TYPE, COLUMN_SIZE, DECIMAL_DIGITS, NULLABLE
   Being ODBC2 application, it naturally doesn't know about many of fields we test
*/
ODBC_TEST(odbc131)
{
  unsigned int i;
  SQLULEN     StrLen;
  SQLSMALLINT ShortData,
              SmallintFields[]= {DATA_TYPE, DECIMAL_DIGITS, NUM_PREC_RADIX, NULLABLE, SQL_DATA_TYPE, SQL_DATETIME_SUB},
              IntFields[]= {COLUMN_SIZE, BUFFER_LENGTH, CHAR_OCTET_LENGTH, ORDINAL_POSITION};
  SQLINTEGER  IntData;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc131");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc131 (i int, b bit(1), f float,\
                       d date not null, t time not null, dt datetime not null,\
                       ts timestamp not null, c varchar(20))");

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
    (SQLCHAR *)"t_odbc131", SQL_NTS, NULL, 0));

  while (SQLFetch(Stmt) != SQL_NO_DATA)
  {
    for (i= 0; i < sizeof(SmallintFields)/sizeof(SQLSMALLINT); ++i)
    {
      CHECK_STMT_RC(Stmt, SQLGetData(Stmt, SmallintFields[i], SQL_C_DEFAULT, &ShortData, 2, &StrLen));
      if (StrLen != SQL_NULL_DATA)
      {
        is_num(StrLen, 2);
      }
    }
    for (i= 0; i < sizeof(IntFields)/sizeof(SQLSMALLINT); ++i)
    {
      CHECK_STMT_RC(Stmt, SQLGetData(Stmt, IntFields[i], SQL_C_DEFAULT, &IntData, 4, &StrLen));
      if (StrLen != SQL_NULL_DATA)
      {
        is_num(StrLen, 4);
      }
    }
  }
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Now once again with explicit types */
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, SQL_NTS, NULL, SQL_NTS,
    (SQLCHAR *)"t_odbc131", SQL_NTS, NULL, 0));

  while (SQLFetch(Stmt) != SQL_NO_DATA)
  {
    for (i= 0; i < sizeof(SmallintFields)/sizeof(SQLSMALLINT); ++i)
    {
      CHECK_STMT_RC(Stmt, SQLGetData(Stmt, SmallintFields[i], SQL_C_SHORT, &ShortData, 2, &StrLen));
      if (StrLen != SQL_NULL_DATA)
      {
        is_num(StrLen, 2);
      }
    }
    for (i= 0; i < sizeof(IntFields)/sizeof(SQLSMALLINT); ++i)
    {
      CHECK_STMT_RC(Stmt, SQLGetData(Stmt, IntFields[i], SQL_C_LONG, &IntData, 4, &StrLen));
      if (StrLen != SQL_NULL_DATA)
      {
        is_num(StrLen, 4);
      }
    }
  }

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc131");

  return OK;
}


ODBC_TEST(odbc119)
{
  SQLCHAR StrValue[32];
  SQLLEN  RowCount;
#ifdef _WIN32
  /* Whether lower_case_table_names is 1 or 2 - it should work with lower case name. if 1 - it is created in lowercase, if 2
  * - it is still compared in lowercase.
  */
  SQLCHAR *tname= (SQLCHAR*)"t_odbc119";
#else
  SQLCHAR *tname= (SQLCHAR*)"t_Odbc119";
#endif
  /* Made the name not all lower case to address ODBC-391/370(if lower_case_table_names=2). However 391 has got its own testcase */
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_Odbc119");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_Odbc119(id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, "
    "ui_p1 INT NOT NULL, iu_p2 INT NOT NULL, a VARCHAR(100) NOT NULL, KEY `INDEX` (`a`), UNIQUE KEY `UNIQUE` (ui_p1, iu_p2)) ENGINE=InnoDB");

  /*  Here it probably can fail in case of case-insentive FS on MacOS and lower_case_table_names=1(not sure if it's even possible) */
  CHECK_STMT_RC(Stmt, SQLStatistics(Stmt, NULL, SQL_NTS, NULL, SQL_NTS, tname, SQL_NTS, SQL_INDEX_ALL, SQL_QUICK));
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &RowCount));
  is_num(RowCount, 4);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, StrValue, 6), "PRIMARY", 6);
  IS_STR(my_fetch_str(Stmt, StrValue, 9), "id", 3);
  is_num(my_fetch_int(Stmt, 4), 0);
  is_num(my_fetch_int(Stmt, 8), 1);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, StrValue, 6), "UNIQUE", 7);
  IS_STR(my_fetch_str(Stmt, StrValue, 9), "ui_p1", 6);
  is_num(my_fetch_int(Stmt, 4), 0);
  is_num(my_fetch_int(Stmt, 8), 1);
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, StrValue, 6), "UNIQUE", 7);
  IS_STR(my_fetch_str(Stmt, StrValue, 9), "iu_p2", 6);
  is_num(my_fetch_int(Stmt, 4), 0);
  is_num(my_fetch_int(Stmt, 8), 2);
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(my_fetch_str(Stmt, StrValue, 6), "INDEX", 6);
  IS_STR(my_fetch_str(Stmt, StrValue, 9), "a", 2);
  is_num(my_fetch_int(Stmt, 4), 1);
  is_num(my_fetch_int(Stmt, 8), 1);

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_Odbc119");

  return OK;
}


ODBC_TEST(odbc185)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLCHAR buff[MAX_ROW_DATA_LEN+1];
  SQLSMALLINT expectedw[]= {SQL_WCHAR, SQL_WVARCHAR, SQL_WLONGVARCHAR, SQL_WLONGVARCHAR, SQL_WLONGVARCHAR, SQL_WLONGVARCHAR, SQL_WCHAR, SQL_WCHAR},
              expecteda[]= {SQL_CHAR, SQL_VARCHAR, SQL_LONGVARCHAR, SQL_LONGVARCHAR, SQL_LONGVARCHAR, SQL_LONGVARCHAR, SQL_CHAR, SQL_CHAR};
  unsigned int i;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &hdbc1));
  CHECK_DBC_RC(hdbc1, SQLConnectW(hdbc1, wdsn, SQL_NTS, wuid, SQL_NTS,
    wpwd, SQL_NTS));

  CHECK_DBC_RC(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS t_odbc185");
  CHECK_STMT_RC(hstmt1, SQLExecDirectW(hstmt1,
    W(L"CREATE TABLE t_odbc185 (ccol CHAR(32), vcfield VARCHAR(32) not null, tcol TEXT, ttcol TINYTEXT, mtcol MEDIUMTEXT,\
                        ltcol LONGTEXT, ecol ENUM('enum val 1', 'enum val 2'), scol SET('set m1', 'set m3'))"), SQL_NTS));

  /* It doesn't matter if we call SQLColumns or SQLColumnsW */
  CHECK_STMT_RC(hstmt1, SQLColumns(hstmt1, my_schema, SQL_NTS, NULL, 0,
    "t_odbc185", SQL_NTS, NULL, SQL_NTS));
  /* Doing the same on the default connection, which is open using ANSI part of the API */
  CHECK_STMT_RC(Stmt, SQLColumnsW(Stmt, wschema, SQL_NTS, NULL, 0,
    CW("t_odbc185"), SQL_NTS,
    NULL, SQL_NTS));

  for (i= 0; i < sizeof(expectedw)/sizeof(expectedw[0]); ++i)
  {
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
    my_fetch_str(hstmt1, buff, COLUMN_NAME);
    is_num(my_fetch_int(hstmt1, DATA_TYPE), expectedw[i]);
    is_num(my_fetch_int(hstmt1, SQL_DATA_TYPE  /* SQL_DATA_TYPE */), expectedw[i]);

    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    my_fetch_str(Stmt, buff, COLUMN_NAME);
    if (iOdbc() || UnixOdbc())
    {
      is_num(my_fetch_int(Stmt, DATA_TYPE), expectedw[i]);
      is_num(my_fetch_int(Stmt, SQL_DATA_TYPE), expectedw[i]);
    }
    else
    {
      is_num(my_fetch_int(Stmt, DATA_TYPE), expecteda[i]);
      is_num(my_fetch_int(Stmt, SQL_DATA_TYPE), expecteda[i]);
    }
  }
  
  EXPECT_STMT(hstmt1, SQLFetch(hstmt1), SQL_NO_DATA_FOUND);
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA_FOUND);

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(hstmt1, SQL_CLOSE));

  OK_SIMPLE_STMT(hstmt1, "DROP TABLE t_odbc185");

  CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


ODBC_TEST(odbc152)
{
  SQLSMALLINT SqlDataType, SqlDatetimeSub= 0xffff, DataType;
  SQLLEN LenType= 0, LenSub= 0, LenConciseType;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc152");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc152 (`val` bigint(20) NOT NULL, dt datetime)");

  /* It doesn't matter if we call SQLColumns or SQLColumnsW */
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, SQL_NTS, NULL, 0,
    "t_odbc152", SQL_NTS, NULL, SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, SQL_DATA_TYPE, SQL_C_SSHORT, &SqlDataType, 2, &LenType));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, SQL_DATETIME_SUB, SQL_C_SSHORT, &SqlDatetimeSub, 2, &LenSub));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, DATA_TYPE, SQL_C_SSHORT, &DataType, 2, &LenConciseType));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(SqlDataType, SQL_BIGINT);
  is_num(LenType, 2);
  is_num(DataType, SQL_BIGINT);
  is_num(LenConciseType, 2);
  is_num(LenSub, SQL_NULL_DATA);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(DataType, OdbcVer == SQL_OV_ODBC2 ? SQL_TIMESTAMP : SQL_TYPE_TIMESTAMP);
  is_num(LenConciseType, 2);
  is_num(SqlDataType, SQL_DATETIME);
  is_num(LenType, 2);
  is_num(SqlDatetimeSub, SQL_CODE_TIMESTAMP);
  is_num(LenSub, 2);

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA_FOUND);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc152");

  return OK;
}


ODBC_TEST(odbc231)
{
  SQLINTEGER ColumnSize= 0, BufferLength= 0;
  SQLLEN     ColumnSizeLen, BufferLengthLen, Type1, Type2, Unsigned1, Unsigned2;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc231");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc231 (`val` LONGTEXT)");

  /* It doesn't matter if we call SQLColumns or SQLColumnsW */
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, SQL_NTS, NULL, 0,
    "t_odbc231", SQL_NTS, NULL, SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, COLUMN_SIZE, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &Type1));
  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, BUFFER_LENGTH, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &Type2));
  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, COLUMN_SIZE, SQL_DESC_UNSIGNED, NULL, 0, NULL, &Unsigned1));
  CHECK_STMT_RC(Stmt, SQLColAttribute(Stmt, BUFFER_LENGTH, SQL_DESC_UNSIGNED, NULL, 0, NULL, &Unsigned2));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, COLUMN_SIZE, SQL_C_SLONG, &ColumnSize, 0, &ColumnSizeLen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, BUFFER_LENGTH, SQL_C_ULONG, &BufferLength, 0, &BufferLengthLen));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num((unsigned)ColumnSize, 0xffffffff);
  is_num(ColumnSizeLen, sizeof(SQLINTEGER));
  diag("Longtext size: %lu(%lx) type %s %s, buffer length: %lu(%lx) type %s %s", ColumnSize, ColumnSize, OdbcTypeAsString((SQLSMALLINT)Type1, NULL),
    Unsigned1 ? "Unsigned" : "Signed", BufferLength, BufferLength, OdbcTypeAsString((SQLSMALLINT)Type2, NULL), Unsigned2 ? "Unsigned" : "Signed");

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA_FOUND);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc231");

  return OK;
}


/*
  ODBC-313 - more like a side effect, or rather the reason why optimization is possible.
  SQLPrimaryKeys(and SQLStatistics) parameters cannot be treated as search patterns. Thus _ should
  mean onlye _, and not any character
*/
ODBC_TEST(odbc313)
{
  SQLCHAR Catalog[MAX_NAME_LEN + 1], Table[MAX_NAME_LEN + 1], Column[MAX_NAME_LEN + 1], AddSchema[MAX_NAME_LEN + 1], temp[256];
  SQLCHAR Create[16/*CREATE DATABASE */ + MAX_NAME_LEN + 1], Drop[24/*DROP DATABASE IF EXISTS */ + MAX_NAME_LEN + 1];
  SQLLEN  CatalogLen, RowCount, TableLen, ColumnLen;
  SQLRETURN rc;
  SQLHANDLE Hdbc, Hstmt;

  strcpy(AddSchema, my_schema);
  AddSchema[0]= '_';

  sprintf(Drop, "DROP DATABASE IF EXISTS %s", AddSchema);
  rc= SQLExecDirect(Stmt, Drop, SQL_NTS);

  if (SQL_SUCCEEDED(rc))
  {
    sprintf(Create, "CREATE DATABASE %s", AddSchema);
    rc= SQLExecDirect(Stmt, Create, SQL_NTS);
  }

  if (!SQL_SUCCEEDED(rc))
  {
    skip("Test user does not have sufficient privileges. It should be able to drop/create database");
  }

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Hdbc));

  Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, AddSchema, NULL, NULL, NULL);
  if (Hstmt == NULL && (IsMaxScale || IsSkySqlHa))
  {
    char addParams[15/*INITSTMT={USE }*/ + sizeof(AddSchema)];
    sprintf(addParams, "INITSTMT={USE %s}", AddSchema);
    Hstmt= DoConnect(Hdbc, FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, addParams);
  }
  FAIL_IF(Hstmt == NULL, "Could not connect with created schema as default, or other error occurred");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc313_1");
  OK_SIMPLE_STMT(Hstmt, "DROP TABLE IF EXISTS odbc313_1");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc313x1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE odbc313_1(pk1 INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT)");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE odbc313x1(pk2 INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT)");
  OK_SIMPLE_STMT(Hstmt, "CREATE TABLE odbc313_1(pk3 INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT)");

  CHECK_STMT_RC(Stmt, SQLPrimaryKeys(Stmt, AddSchema, SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"odbc313_1", SQL_NTS));

  /* Test of SQLRowCount with SQLPrimaryKeys */
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &RowCount));
  is_num(RowCount, 1);

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, Catalog, sizeof(Catalog), &CatalogLen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_CHAR, Table, sizeof(Table), &TableLen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 4, SQL_C_CHAR, Column, sizeof(Column), &ColumnLen));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(CatalogLen, strlen(AddSchema));
  IS_STR(Catalog, AddSchema, strlen(AddSchema));
  IS_STR(Table, "odbc313_1", sizeof("odbc313_1"));
  IS_STR(Column, "pk3", sizeof("pk3"));

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "No more rows expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));

  CHECK_STMT_RC(Stmt, SQLStatistics(Stmt, AddSchema, SQL_NTS, NULL, SQL_NTS,
    (SQLCHAR*)"odbc313_1", SQL_NTS,
    SQL_INDEX_ALL, SQL_QUICK));
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &RowCount));
  is_num(RowCount, 1);

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_CHAR, Catalog, sizeof(Catalog), &CatalogLen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_CHAR, Table, sizeof(Table), &TableLen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 9, SQL_C_CHAR, Column, sizeof(Column), &ColumnLen));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(CatalogLen, strlen(AddSchema));
  IS_STR(Catalog, AddSchema, strlen(AddSchema));
  IS_STR(Table, "odbc313_1", sizeof("odbc313_1"));
  IS_STR(Column, "pk3", sizeof("pk3"));

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "No more rows expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));

  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt, SQL_BEST_ROWID, AddSchema, SQL_NTS, NULL, 0,
    (SQLCHAR*)"odbc313_1", SQL_NTS, SQL_SCOPE_SESSION, SQL_NULLABLE));
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &RowCount));
  is_num(RowCount, 1);

  Catalog[0]= '\0';
  //CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, Column, sizeof(Column), &ColumnLen));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, temp, sizeof(temp), &ColumnLen));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR(temp, "pk3", sizeof("pk3"));

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "No more rows expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_UNBIND));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc313_1");
  OK_SIMPLE_STMT(Hstmt, "DROP TABLE IF EXISTS odbc313_1");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc313x1");
  OK_SIMPLE_STMT(Stmt, Drop);

  CHECK_STMT_RC(Hstmt, SQLFreeStmt(Hstmt, SQL_DROP));
  CHECK_DBC_RC(Hdbc, SQLDisconnect(Hdbc));
  CHECK_DBC_RC(Hdbc, SQLFreeConnect(Hdbc));

  return OK;
}


/*
  ODBC-316 - various typical (minor) issues
  - empty string parameter
*/
ODBC_TEST(odbc316)
{
  SQLCHAR grant[256];
  BOOL runColumnPrivileges= TRUE, runTablePrivileges= TRUE;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc316_2");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc316_1");
  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS odbc316");
  OK_SIMPLE_STMT(Stmt,
    "CREATE PROCEDURE odbc316(IN inParam INT UNSIGNED)"
    "BEGIN"
    " SELECT inParam AS ret;"
    "END");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE odbc316_1(pk1 INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT)");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE odbc316_2(pk INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT, fk INTEGER NOT NULL,"
                       "FOREIGN KEY (fk) REFERENCES odbc316_1(pk1))");

  _snprintf(grant, sizeof(grant), "GRANT INSERT (pk1), SELECT (pk1), UPDATE (pk1) ON odbc316_1 TO %s", my_uid);

  if (!SQL_SUCCEEDED(SQLExecDirect(Stmt, grant, SQL_NTS)))
  {
    /* We could not set col privileges, thus SQLColumnPrivileges will return empty set anyway. There is no sense to test */
    runColumnPrivileges= FALSE;
  }
  _snprintf(grant, sizeof(grant), "GRANT INSERT, SELECT, UPDATE, DROP ON odbc316_1 TO %s", my_uid);
  if (!SQL_SUCCEEDED(SQLExecDirect(Stmt, grant, SQL_NTS)))
  {
    /* We could not set table privileges, thus SQLTablePrivileges will return empty set anyway. There is no sense to test */
    runTablePrivileges = FALSE;
  }
  /* Empty string Catalog name -> empty RS */
  if (runColumnPrivileges != FALSE)
  {
    CHECK_STMT_RC(Stmt, SQLColumnPrivileges(Stmt, "", SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS, NULL, SQL_NTS));
    FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, "", SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS, NULL, SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, "", SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS, NULL, SQL_NTS,
                        NULL, SQL_NTS, NULL, SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, "", SQL_NTS,
    NULL, SQL_NTS, (SQLCHAR*)"odbc316_2", SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLPrimaryKeys(Stmt, "", SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLProcedureColumns(Stmt, "", SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"odbc316", SQL_NTS, NULL, SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLProcedures(Stmt, "", SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"odbc316", SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt, SQL_BEST_ROWID, "", SQL_NTS, NULL, 0,
    (SQLCHAR*)"odbc316_1", SQL_NTS, SQL_SCOPE_SESSION, SQL_NULLABLE));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLStatistics(Stmt, "", SQL_NTS, NULL, SQL_NTS,
    (SQLCHAR*)"odbc316_1", SQL_NTS,
    SQL_INDEX_ALL, SQL_QUICK));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  if (runTablePrivileges != FALSE)
  {
    CHECK_STMT_RC(Stmt, SQLTablePrivileges(Stmt, "", SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS));
    FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }
  /* SQLTables has different special meaning of "" arguments, but only if one of others is "%" */
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, "", SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS, NULL, 0));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Empty string Schema name -> empty RS */
  if (runColumnPrivileges != FALSE)
  {
    CHECK_STMT_RC(Stmt, SQLColumnPrivileges(Stmt, NULL, SQL_NTS, "", SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS, NULL, SQL_NTS));
    FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }
  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, NULL, SQL_NTS, "", SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS, NULL, SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, NULL, SQL_NTS, "", SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS, NULL, SQL_NTS,
                        NULL, SQL_NTS, NULL, SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLForeignKeys(Stmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS,
                        "", SQL_NTS, (SQLCHAR*)"odbc316_2", SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLPrimaryKeys(Stmt, NULL, SQL_NTS, "", SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLProcedureColumns(Stmt, NULL, SQL_NTS, "", SQL_NTS, (SQLCHAR*)"odbc316", SQL_NTS, NULL, SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLProcedures(Stmt, NULL, SQL_NTS, "", SQL_NTS, (SQLCHAR*)"odbc316", SQL_NTS));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt, SQL_BEST_ROWID, NULL, SQL_NTS, "", 0,
    (SQLCHAR*)"odbc316_1", SQL_NTS, SQL_SCOPE_SESSION, SQL_NULLABLE));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLStatistics(Stmt, NULL, SQL_NTS, "", SQL_NTS,
    (SQLCHAR*)"odbc316_1", SQL_NTS,
    SQL_INDEX_ALL, SQL_QUICK));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt, SQL_BEST_ROWID, "", SQL_NTS, NULL, 0,
    (SQLCHAR*)"odbc316_1", SQL_NTS, SQL_SCOPE_SESSION, SQL_NULLABLE));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  if (runTablePrivileges != FALSE)
  {
    CHECK_STMT_RC(Stmt, SQLTablePrivileges(Stmt, NULL, SQL_NTS, "", SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS));
    FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  }
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, NULL, SQL_NTS, "", SQL_NTS, (SQLCHAR*)"odbc316_1", SQL_NTS, NULL, 0));
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "Empty ResultSet expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE odbc316");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc316_2");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE odbc316_1");

  return OK;
}


ODBC_TEST(odbc324)
{
  SQLCHAR tableType[32];

  if (ServerNotOlderThan(Connection, 10,3,4) == FALSE)
  {
    skip("The test does not make sense with servers older than 10.3.4 version");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc324");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc324 (id int not null) WITH SYSTEM VERSIONING");

  /* It doesn't matter if we call SQLColumns or SQLColumnsW */
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, my_schema, SQL_NTS, NULL, 0,
    "t_odbc324", SQL_NTS, "TABLE,VIEW", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  IS_STR("TABLE", my_fetch_str(Stmt, tableType, 4), sizeof("TABLE"));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA_FOUND);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc324");

  return OK;
}


ODBC_TEST(odbc361)
{
  char buffer[16];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc361_1");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc361_1 (key1 INT, key2 INT NOT NULL, UNIQUE INDEX t361_1key12(key1,key2))");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc361_2");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc361_2 (key1 INT, key2 INT NOT NULL, key3 INT NOT NULL PRIMARY KEY,"
                       "UNIQUE INDEX t361_2key12(key1,key2))");

  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt, SQL_BEST_ROWID, NULL, SQL_NTS, NULL, 0,
    (SQLCHAR*)"t_odbc361_1", SQL_NTS, SQL_SCOPE_SESSION, SQL_NULLABLE));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA_FOUND);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt, SQL_BEST_ROWID, NULL, SQL_NTS, NULL, 0,
    (SQLCHAR*)"t_odbc361_2", SQL_NTS, SQL_SCOPE_SESSION, SQL_NULLABLE));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR("key3", my_fetch_str(Stmt, buffer, 2), sizeof("key3"));

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA_FOUND);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLStatistics(Stmt, NULL, 0, NULL, 0,
    "t_odbc361_1", SQL_NTS, SQL_INDEX_ALL, SQL_QUICK));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR("key1", my_fetch_str(Stmt, buffer, 9), sizeof("key1"));
  is_num(1, my_fetch_int(Stmt, 4));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR("key2", my_fetch_str(Stmt, buffer, 9), sizeof("key2"));
  is_num(1, my_fetch_int(Stmt, 4));

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA_FOUND);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLStatistics(Stmt, NULL, 0, NULL, 0,
    "t_odbc361_2", SQL_NTS, SQL_INDEX_ALL, SQL_QUICK));

  /* Unique go first */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR("key3", my_fetch_str(Stmt, buffer, 9), sizeof("key3"));
  is_num(0, my_fetch_int(Stmt, 4));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR("key1", my_fetch_str(Stmt, buffer, 9), sizeof("key1"));
  is_num(1, my_fetch_int(Stmt, 4));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR("key2", my_fetch_str(Stmt, buffer, 9), sizeof("key2"));
  is_num(1, my_fetch_int(Stmt, 4));

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA_FOUND);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc361_1");
  return OK;
}

/* With lower_case_table_names=2 server the driver may not read indexes in SQLStatistics 
 * We can't assure that lower_case_table_names=2, but it's a good testcase for other modes as well.
 * Besides other catalog functions are affected - not only SQLStatistics
 * The problem arose when application reads (db or) table name with SQLTables, and it has original letter cases,
 * but then if to use it as the parameter to (catalog or) table argument for various catalog functions, the server compares
 * it to lowercase values of (db or) table name, and it failed(since ordinary argument comparison has to be cace-sensitive.
 * The test basically makes sure, that this works
 */
ODBC_TEST(odbc391)
{
  SQLCHAR tname[12], buffer[MAX_NAME_LEN], dbname[MAX_NAME_LEN], pname[12];
  SQLLEN  dbnameLen, tnameLen, pnameLen;
  SQLINTEGER len;
  BOOL found= FALSE;
  SQLCHAR  dropUser[24 + sizeof(my_host)], createUser[52 + sizeof(my_host)], grantAll[40 + sizeof(my_host)], revokeSelect[48 + sizeof(my_host)];

  if (iOdbc() && OdbcVer == SQL_OV_ODBC2)
  {
    /* It calls W version of the function, but passes the string of only 1st character(no matter what this 1st character is).
     * No ideas how to overcome that */
    skip("iOdbc behaves strangely on SQLSetConnectAttr with OdbcVer == SQL_OV_ODBC2.");
  }
  OK_SIMPLE_STMT(Stmt, "DROP SCHEMA IF EXISTS _SchemaOdbc391");
  OK_SIMPLE_STMT(Stmt, "CREATE SCHEMA _SchemaOdbc391");
  CHECK_DBC_RC(Connection, SQLGetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, buffer, sizeof(buffer), &len));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, (SQLPOINTER)"_SchemaOdbc391", 14));
  CHECK_STMT_RC(Stmt, SQLTables(Stmt, (SQLCHAR*)SQL_ALL_CATALOGS, 1, "", 0, "", 0, NULL, 0));
  while (SQLFetch(Stmt) != SQL_NO_DATA)
  {
    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_CHAR, dbname, sizeof(dbname), &dbnameLen));
    if (dbnameLen == (sizeof("_SchemaOdbc391") - 1) && strcmp(dbname + 8, "dbc391") == 0)
    {
      found= TRUE;
      break;
    }
  }
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  FAIL_IF(!found, "Created schema wasn't found");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_Odbc391");
  /* Current_Timestamp() is intentionally in mixed case, however there was no problem with that */
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_Odbc391(id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, "
    "ts TIMESTAMP  on update Current_Timestamp(), a VARCHAR(100) NOT NULL) ENGINE=InnoDB");

  CHECK_STMT_RC(Stmt, SQLTables(Stmt, dbname, (SQLSMALLINT)dbnameLen, NULL, 0,
    NULL, 0, NULL, 0));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 3, SQL_CHAR, tname, sizeof(tname), &tnameLen));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLColumns(Stmt, dbname, (SQLSMALLINT)dbnameLen, NULL, 0,
                                tname, (SQLSMALLINT)tnameLen, NULL, 0));
  is_num(3, my_print_non_format_result(Stmt));
  /* my_print_non_format_result closes cursor */

  CHECK_STMT_RC(Stmt, SQLStatistics(Stmt, dbname, (SQLSMALLINT)dbnameLen, NULL, SQL_NTS, tname, (SQLSMALLINT)tnameLen,
                                    SQL_INDEX_ALL, SQL_QUICK));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLSpecialColumns(Stmt, SQL_ROWVER,  dbname, (SQLSMALLINT)dbnameLen, NULL, 0, tname, (SQLSMALLINT)tnameLen,
                                        SQL_SCOPE_TRANSACTION, SQL_NULLABLE));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLPrimaryKeys(Stmt, dbname, (SQLSMALLINT)dbnameLen, NULL, SQL_NTS, tname, (SQLSMALLINT)tnameLen));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE ProcOdbc391(IN Id INT, IN Name VARCHAR(20))"\
    "BEGIN END;"
  );
  CHECK_STMT_RC(Stmt, SQLProcedures(Stmt, dbname, (SQLSMALLINT)dbnameLen, NULL, 0,
    NULL, 0));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 3, SQL_CHAR, pname, sizeof(pname), &pnameLen));
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt, SQLProcedureColumns(Stmt, dbname, (SQLSMALLINT)dbnameLen, NULL, 0,
    pname, (SQLSMALLINT)pnameLen, NULL, 0));
  is_num(2, my_print_non_format_result(Stmt));
  /* my_print_non_format_result closes cursor */

  _snprintf(dropUser, sizeof(dropUser), "DROP USER Odbc391@'%s'", my_host);
  /* we can have error, if there is simply no such user, and that is supposed to be the case, actually */
  SQLExecDirect(Stmt, dropUser, SQL_NTS);
  _snprintf(createUser, sizeof(createUser), "CREATE USER Odbc391@'%s' IDENTIFIED BY 's3CureP@wd'", my_host);
  CHECK_USER_OPERATION(Stmt, createUser);
  _snprintf(grantAll, sizeof(grantAll), "GRANT SELECT ON t_Odbc391 TO Odbc391@'%s'", my_host);
  CHECK_USER_OPERATIONX(Stmt, grantAll, dropUser);
  _snprintf(grantAll, sizeof(grantAll), "GRANT SELECT(id,ts,a) ON t_Odbc391 TO Odbc391@'%s'", my_host);
  CHECK_USER_OPERATIONX(Stmt, grantAll, dropUser);
  _snprintf(revokeSelect, sizeof(revokeSelect), "REVOKE SELECT(ts) ON t_Odbc391 FROM Odbc391@'%s'", my_host);
  CHECK_USER_OPERATIONX(Stmt, revokeSelect, dropUser);

  CHECK_STMT_RC(Stmt, SQLTablePrivileges(Stmt, dbname, (SQLSMALLINT)dbnameLen, NULL, 0, tname, (SQLSMALLINT)tnameLen));
  is_num(1, my_print_non_format_result(Stmt));
  CHECK_STMT_RC(Stmt, SQLColumnPrivileges(Stmt, dbname, (SQLSMALLINT)dbnameLen, NULL, 0, tname, (SQLSMALLINT)tnameLen, NULL, 0));
  is_num(2, my_print_non_format_result(Stmt));

  OK_SIMPLE_STMT(Stmt, dropUser);
  OK_SIMPLE_STMT(Stmt, "DROP SCHEMA _SchemaOdbc391");
  /* Old UnixODBC needs this(and in many other places where it looks redundant) */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_DBC_RC(Connection, SQLSetConnectAttr(Connection, SQL_ATTR_CURRENT_CATALOG, buffer, len));
  return OK;
}


ODBC_TEST(odbc435)
{
  char buffer[12];
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc435");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc435(f1 INT, f2 INT, key_part2 INT NOT NULL, key_part1 INT NOT NULL,"
                       "PRIMARY KEY(key_part1,key_part2))");


  CHECK_STMT_RC(Stmt, SQLPrimaryKeys(Stmt, NULL, SQL_NTS, NULL, SQL_NTS, "t_odbc435", 9));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(1, my_fetch_int(Stmt, 5));
  IS_STR("key_part1", my_fetch_str(Stmt, buffer, 4), 10/*lenght of key_part1*/);
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(2, my_fetch_int(Stmt, 5));
  IS_STR("key_part2", my_fetch_str(Stmt, buffer, 4), 10/*lenght of key_part2*/);
  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc435");
  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {t_bug37621, "t_bug37621", NORMAL},
  {t_bug34272, "t_bug34272", NORMAL},
  {t_bug49660, "t_bug49660", NORMAL},
  {t_bug51422, "t_bug51422", NORMAL},
  {t_bug36441, "t_bug36441", NORMAL},
  {t_bug53235, "t_bug53235", NORMAL},
  {t_bug50195, "t_bug50195", NORMAL},
  {t_sqlprocedurecolumns, "t_sqlprocedurecolumns", NORMAL},
  {t_bug57182, "t_bug57182", NORMAL},
  {t_bug55870, "t_bug55870", NORMAL},
  {t_bug31067, "t_bug31067", NORMAL},
  {bug12824839,               "bug12824839",               NORMAL},
  {sqlcolumns_nodbselected,   "sqlcolumns_nodbselected",   NORMAL},
  {t_bug14085211_part1,       "t_bug14085211_part1",       NORMAL},
  {t_sqlcolumns_after_select, "t_sqlcolumns_after_select", NORMAL},
  {t_bug14555713,             "t_bug14555713",             NORMAL},
  {odbc38,  "odbc32_odbc2_odbc3_data_types", NORMAL},
  {odbc51,  "odbc51_wchar_emptystring",      NORMAL},
  {odbc131, "odbc131_columns_data_len",      NORMAL},
  {odbc119, "odbc119_sqlstats_orderby",      NORMAL},
  {odbc185, "odbc185_sqlcolumns_wtypes",     NORMAL},
  {odbc152, "odbc152_sqlcolumns_sql_data_type", NORMAL},
  {odbc231, "odbc231_sqlcolumns_longtext",      NORMAL},
  {odbc313, "odbc313_no_patterns_for_sqlpkeys", NORMAL},
  {odbc316, "odbc316_empty_string_parameters",  NORMAL},
  {odbc324, "odbc324_sqltables_versioned_table",NORMAL},
  {odbc361, "odbc361_unique_with_nulls",        NORMAL},
  {odbc391, "odbc391_mixed_case_names",         NORMAL},
  {odbc435, "odbc435_PK_flds_order_and_seq_num",NORMAL},
  {NULL, NULL}
};


int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  int result;

  get_options(argc, argv);
  plan(tests);

  result= run_tests(my_tests);
  /* Now one more run as ODBC2 app */
  OdbcVer= SQL_OV_ODBC2;

  return run_tests(my_tests) || result;
}
