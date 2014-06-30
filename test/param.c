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

/********************************************************
* initialize tables                                     *
*********************************************************/
ODBC_TEST(my_init_table)
{
  SQLRETURN   rc;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE if exists my_demo_param");

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  /* create the table 'my_demo_param' */
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE my_demo_param(\
                            id   int,\
                            auto int primary key auto_increment,\
                            name varchar(20),\
                            timestamp timestamp)");

  return OK;
}


ODBC_TEST(my_param_insert)
{
  SQLRETURN   rc;
  SQLINTEGER  id;
  char        name[50];

  /* prepare the insert statement with parameters */
  rc = SQLPrepare(Stmt, (SQLCHAR *)"INSERT INTO my_demo_param(id,name) VALUES(?,?)",SQL_NTS);
  CHECK_STMT_RC(Stmt,rc);

  /* now supply data to parameter 1 and 2 */
  rc = SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT,
                        SQL_C_LONG, SQL_INTEGER, 0,0,
                        &id, 0, NULL);
  CHECK_STMT_RC(Stmt,rc);

  rc = SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT,
                        SQL_C_CHAR, SQL_CHAR, 0,0,
                        name, sizeof(name), NULL);
  CHECK_STMT_RC(Stmt,rc);

  /* now insert 10 rows of data */
  for (id = 0; id < 10; id++)
  {
      sprintf(name,"MySQL%d",id);

      rc = SQLExecute(Stmt);
      CHECK_STMT_RC(Stmt,rc);
  }

  /* Free statement param resorces */
  rc = SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
  CHECK_STMT_RC(Stmt,rc);

  /* Free statement cursor resorces */
  rc = SQLFreeStmt(Stmt, SQL_CLOSE);
  CHECK_STMT_RC(Stmt,rc);

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
  CHECK_DBC_RC(Connection,rc);

  /* Now fetch and verify the data */
  OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_demo_param");

  FAIL_IF(10 != myresult(Stmt), "expected 10 rows");

  return OK;
}

ODBC_TEST(test_numeric)
{
  SQLRETURN rc;
  unsigned long num= 2;

  OK_SIMPLE_STMT(Stmt, "SET GLOBAL GENERAL_LOG=1");
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_numeric");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_numeric (`Id` int(11) NOT NULL AUTO_INCREMENT, "\
                        "a varchar(255) NOT NULL, b int(11) NOT NULL, c int(11) NOT NULL, "\
                        "d int(11) NOT NULL, PRIMARY KEY (`Id`)) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=latin1");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_numeric VALUES (NULL, 'test', 1, 2, 3)");

  SQLBindParameter(Stmt,  1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_NUMERIC, 0, 0, &num, 0, NULL);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_numeric WHERE c=?");

  rc= SQLFetch(Stmt);
  FAIL_IF(rc== SQL_NO_DATA, "unexpected eof");

  return OK;
}

ODBC_TEST(unbuffered_result)
{
  SQLRETURN rc;
  SQLHSTMT Stmt1, Stmt2;

  SQLAllocHandle(SQL_HANDLE_STMT, Connection, &Stmt1);
  SQLSetStmtAttr(Stmt1, SQL_CURSOR_TYPE, SQL_CURSOR_FORWARD_ONLY, SQL_IS_INTEGER);
  OK_SIMPLE_STMT(Stmt1, "DROP TABLE IF EXISTS t1");
  OK_SIMPLE_STMT(Stmt1, "CREATE TABLE t1 (a int)");
  OK_SIMPLE_STMT(Stmt1, "INSERT INTO t1 VALUES (1),(2),(3)");
  OK_SIMPLE_STMT(Stmt1, "SELECT * from t1");

  rc= SQLFetch(Stmt1);
  FAIL_IF(rc == SQL_NO_DATA, "unexpected eof");

  SQLAllocHandle(SQL_HANDLE_STMT, Connection, &Stmt2);
  SQLSetStmtAttr(Stmt2, SQL_CURSOR_TYPE, SQL_CURSOR_FORWARD_ONLY, SQL_IS_INTEGER);
  OK_SIMPLE_STMT(Stmt2, "SELECT * from t1");

  SQLFreeStmt(Stmt1, SQL_DROP);
  SQLFreeStmt(Stmt2, SQL_DROP);
  return OK;
}


ODBC_TEST(my_param_update)
{
    SQLRETURN  rc;
    SQLLEN nRowCount;
    SQLINTEGER id=9;
    char name[]="update";

    /* prepare the insert statement with parameters */
    rc = SQLPrepare(Stmt, (SQLCHAR *)"UPDATE my_demo_param set name = ? WHERE id = ?",SQL_NTS);
    CHECK_STMT_RC(Stmt,rc);

    /* now supply data to parameter 1 and 2 */
    rc = SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT,
                          SQL_C_CHAR, SQL_CHAR, 0,0,
                          name, sizeof(name), NULL);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT,
                          SQL_C_LONG, SQL_INTEGER, 0,0,
                          &id, 0, NULL);
    CHECK_STMT_RC(Stmt,rc);

    /* now execute the update statement */
    rc = SQLExecute(Stmt);
    CHECK_STMT_RC(Stmt,rc);

    /* check the rows affected by the update statement */
    rc = SQLRowCount(Stmt, &nRowCount);
    CHECK_STMT_RC(Stmt,rc);
    diag("\n total rows updated:%d\n",nRowCount);
    IS( nRowCount == 1);

    /* Free statement param resorces */
    rc = SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    CHECK_STMT_RC(Stmt,rc);

    /* Free statement cursor resorces */
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    /* commit the transaction */
    rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    /* Now fetch and verify the data */
    OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_demo_param");
    CHECK_STMT_RC(Stmt,rc);

    IS(10 == myresult(Stmt));

  return OK;
}


ODBC_TEST(my_param_delete)
{
    SQLRETURN  rc;
    SQLINTEGER id;
    SQLLEN nRowCount;

    /* supply data to parameter 1 */
    rc = SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT,
                          SQL_C_LONG, SQL_INTEGER, 0,0,
                          &id, 0, NULL);
    CHECK_STMT_RC(Stmt,rc);

    /* execute the DELETE STATEMENT to delete 5th row  */
    id = 5;
    OK_SIMPLE_STMT(Stmt,"DELETE FROM my_demo_param WHERE id = ?");

    /* check the rows affected by the update statement */
    rc = SQLRowCount(Stmt, &nRowCount);
    CHECK_STMT_RC(Stmt,rc);
    diag(" total rows deleted:%d\n",nRowCount);
    IS( nRowCount == 1);

    SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    /* execute the DELETE STATEMENT to delete 8th row  */
    rc = SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT,
                          SQL_C_LONG, SQL_INTEGER, 0,0,
                          &id, 0, NULL);
    CHECK_STMT_RC(Stmt,rc);

    id = 8;
    OK_SIMPLE_STMT(Stmt,"DELETE FROM my_demo_param WHERE id = ?");

    /* check the rows affected by the update statement */
    rc = SQLRowCount(Stmt, &nRowCount);
    CHECK_STMT_RC(Stmt,rc);
    diag(" total rows deleted:%d\n",nRowCount);
    IS( nRowCount == 1);

    /* Free statement param resorces */
    rc = SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    CHECK_STMT_RC(Stmt,rc);

    /* Free statement cursor resorces */
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

    /* commit the transaction */
    rc = SQLEndTran(SQL_HANDLE_DBC, Connection, SQL_COMMIT);
    CHECK_DBC_RC(Connection,rc);

    /* Now fetch and verify the data */
    OK_SIMPLE_STMT(Stmt, "SELECT * FROM my_demo_param");

    IS(8 == myresult(Stmt));

    /* drop the table */
    OK_SIMPLE_STMT(Stmt,"DROP TABLE my_demo_param");

    rc = SQLFreeStmt(Stmt,SQL_CLOSE);
    CHECK_STMT_RC(Stmt,rc);

  return OK;
}


/*I really wonder what is this test about */
ODBC_TEST(tmysql_fix)
{
  SQLRETURN rc;

  diag("SQLDescribeParam not supported yet");
  return SKIP;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_err");

  OK_SIMPLE_STMT(Stmt,"CREATE TABLE tmysql_err (\
                  td date NOT NULL default '0000-00-00',\
                  node varchar(8) NOT NULL default '',\
                  tag varchar(10) NOT NULL default '',\
                  sqlname varchar(8) default NULL,\
                  fix_err varchar(100) default NULL,\
                  sql_err varchar(255) default NULL,\
                  prog_err varchar(100) default NULL\
                ) ENGINE=MyISAM");

  OK_SIMPLE_STMT(Stmt,"INSERT INTO tmysql_err VALUES\
                  ('0000-00-00','0','0','0','0','0','0'),\
                  ('2001-08-29','FIX','SQLT2','ins1',\
                  NULL,NULL, 'Error.  SQL cmd %s is not terminated or too long.'),\
                  ('0000-00-00','0','0','0','0','0','0'),('2001-08-29','FIX','SQLT2',\
                  'ins1',NULL,NULL,'Error.  SQL cmd %s is not terminated or too long.'),\
                  ('0000-00-00','0','0','0','0','0','0'),('2001-08-29','FIX','SQLT2',\
                  'ins1',NULL,NULL,'Error.  SQL cmd %s is not terminated or too long.'),\
                  ('0000-00-00','0','0','0','0','0','0'),('2001-08-29','FIX','SQLT2','ins1',\
                  NULL,NULL,'Error.  SQL cmd %s is not terminated or too long.'),\
                  ('0000-00-00','0','0','0','0','0','0'),('2001-08-29','FIX','SQLT2',\
                  'ins1',NULL,NULL,'Error.  SQL cmd %s is not terminated or too long.'),\
                  ('0000-00-00','0','0','0','0','0','0'),('2001-08-29','FIX','SQLT2',\
                  'ins1',NULL,NULL,'Error.  SQL cmd %s is not terminated or too long.'),\
                  ('0000-00-00','0','0','0','0','0','0'),('2001-08-29','FIX','SQLT2',\
                  'ins1',NULL,NULL,'Error.  SQL cmd %s is not terminated or too long.'),\
                  ('0000!-00-00','0','0','0','0','0','0'),('2001-08-29','FIX','SQLT2',\
                  'ins1',NULL,NULL,'Error.  SQL cmd %s is not terminated or too long.'),\
                  ('0000-00-00','0','0','0','0','0','0'),('2001-08-29','FIX','SQLT2',\
                  'ins1',NULL,NULL,'Error.  SQL cmd %s is not terminated or too long.')");

  /* trace based */
  {
    SQLSMALLINT pcpar,pccol,pfSqlType,pibScale,pfNullable;
    SQLSMALLINT index;
    SQLCHAR     td[30]="20010830163225";
    SQLCHAR     node[30]="FIX";
    SQLCHAR     tag[30]="SQLT2";
    SQLCHAR     sqlname[30]="ins1";
    SQLCHAR     sqlerr[30]="error";
    SQLCHAR     fixerr[30]= "fixerr";
    SQLCHAR     progerr[30]="progerr";
    SQLULEN     pcbParamDef;

    SQLFreeStmt(Stmt,SQL_CLOSE);
    rc = SQLPrepare(Stmt,
      (SQLCHAR *)"insert into tmysql_err (TD, NODE, TAG, SQLNAME, SQL_ERR,"
                 "FIX_ERR, PROG_ERR) values (?, ?, ?, ?, ?, ?, ?)", 103);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLNumParams(Stmt,&pcpar);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLNumResultCols(Stmt,&pccol);
    CHECK_STMT_RC(Stmt,rc);

    for (index=1; index <= pcpar; index++)
    {
      rc = SQLDescribeParam(Stmt,index,&pfSqlType,&pcbParamDef,&pibScale,&pfNullable);
      CHECK_STMT_RC(Stmt,rc);

      diag("descparam[%d]:%d,%d,%d,%d\n",index,pfSqlType,pcbParamDef,pibScale,pfNullable);
    }

    /* TODO: C and SQL types as numeric consts. Splendid.*/
    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,11,12,0,0,td,100,0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT,1,12,0,0,node,100,0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLBindParameter(Stmt,3,SQL_PARAM_INPUT,1,12,0,0,tag,100,0);
    CHECK_STMT_RC(Stmt,rc);
    rc = SQLBindParameter(Stmt,4,SQL_PARAM_INPUT,1,12,0,0,sqlname,100,0);
    CHECK_STMT_RC(Stmt,rc);
    rc = SQLBindParameter(Stmt,5,SQL_PARAM_INPUT,1,12,0,0,sqlerr,0,0);
    CHECK_STMT_RC(Stmt,rc);
    rc = SQLBindParameter(Stmt,6,SQL_PARAM_INPUT,1,12,0,0,fixerr,0,0);
    CHECK_STMT_RC(Stmt,rc);
    rc = SQLBindParameter(Stmt,7,SQL_PARAM_INPUT,1,12,0,0,progerr,0,0);
    CHECK_STMT_RC(Stmt,rc);

    rc = SQLExecute(Stmt);
    CHECK_STMT_RC(Stmt,rc);
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS tmysql_err");

  return OK;
}


/*
  Test basic handling of SQL_ATTR_PARAM_BIND_OFFSET_PTR
*/
ODBC_TEST(t_param_offset)
{
  const SQLINTEGER rowcnt= 5;
  SQLINTEGER i;
  struct {
    SQLINTEGER id;
    SQLINTEGER x;
  } rows[25];
  size_t row_size= (sizeof(rows) / 25);
  SQLINTEGER out_id, out_x;
  SQLULEN bind_offset= 20 * row_size;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_param_offset");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_param_offset (id int not null, x int)");

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_BIND_OFFSET_PTR,
                                &bind_offset, 0));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &rows[0].id, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &rows[0].x, 0, NULL));

  for (i= 0; i < rowcnt; ++i)
  {
    rows[20+i].id= i * 10;
    rows[20+i].x= (i * 1000) % 97;
    OK_SIMPLE_STMT(Stmt, "insert into t_param_offset values (?,?)");
    bind_offset+= row_size;
  }

  /* verify the data */

  OK_SIMPLE_STMT(Stmt, "select id, x from t_param_offset order by 1");

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &out_id, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_LONG, &out_x, 0, NULL));

  for (i= 0; i < rowcnt; ++i)
  {
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    is_num(out_id, rows[20+i].id);
    is_num(out_id, i * 10);
    is_num(out_x, rows[20+i].x);
    is_num(out_x, (i * 1000) % 97);
  }

  return OK;
}


/*
Bug 48310 - parameters array support request.
Binding by row test
*/
ODBC_TEST(paramarray_by_row)
{
#define ROWS_TO_INSERT 3
#define STR_FIELD_LENGTH 255
  typedef struct DataBinding
  {
    SQLCHAR     bData[5];
    SQLINTEGER  intField;
    SQLCHAR     strField[STR_FIELD_LENGTH];
    SQLLEN      indBin;
    SQLLEN      indInt;
    SQLLEN      indStr;
  } DATA_BINDING;

   const SQLCHAR *str[]= {"nothing for 1st", "longest string for row 2", "shortest"  };

  SQLCHAR       buff[50];
  DATA_BINDING  dataBinding[ROWS_TO_INSERT];
  SQLUSMALLINT  paramStatusArray[ROWS_TO_INSERT];
  SQLULEN       paramsProcessed, i, nLen;
  SQLLEN        rowsCount;

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "DROP TABLE IF EXISTS t_bug48310", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "CREATE TABLE t_bug48310 (id int primary key auto_increment,"\
    "bData binary(5) NULL, intField int not null, strField varchar(255) not null)", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)sizeof(DATA_BINDING), 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ROWS_TO_INSERT, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_STATUS_PTR, paramStatusArray, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &paramsProcessed, 0));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY,
    0, 0, dataBinding[0].bData, 0, &dataBinding[0].indBin));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
    0, 0, &dataBinding[0].intField, 0, &dataBinding[0].indInt));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
    0, 0, dataBinding[0].strField, 0, &dataBinding[0].indStr ));

  memcpy(dataBinding[0].bData, "\x01\x80\x00\x80\x00", 5);
  dataBinding[0].intField= 1;
 
  memcpy(dataBinding[1].bData, "\x02\x80\x00\x80", 4);
  dataBinding[1].intField= 0;
 
  memcpy(dataBinding[2].bData, "\x03\x80\x00", 3);
  dataBinding[2].intField= 223322;
 
  for (i= 0; i < ROWS_TO_INSERT; ++i)
  {
    strcpy(dataBinding[i].strField, str[i]);
    dataBinding[i].indBin= 5 - i;
    dataBinding[i].indInt= 0;
    dataBinding[i].indStr= SQL_NTS;
  }

  /* We don't expect errors in paramsets processing, thus we should get SQL_SUCCESS only*/
  FAIL_IF(SQLExecDirect(Stmt, "INSERT INTO t_bug48310 (bData, intField, strField) " \
    "VALUES (?,?,?)", SQL_NTS) != SQL_SUCCESS, "success expected");

  is_num(paramsProcessed, ROWS_TO_INSERT);

  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowsCount));
  is_num(rowsCount, ROWS_TO_INSERT);

  for (i= 0; i < paramsProcessed; ++i)
    if ( paramStatusArray[i] != SQL_PARAM_SUCCESS
      && paramStatusArray[i] != SQL_PARAM_SUCCESS_WITH_INFO )
    {
      diag("Parameter #%u status isn't successful(0x%X)", i+1, paramStatusArray[i]);
      return FAIL;
    }

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)1, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "SELECT bData, intField, strField\
                                      FROM t_bug48310\
                                      ORDER BY id", SQL_NTS));

  /* Just to make sure RowCount isn't broken */
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowsCount));
  FAIL_IF(rowsCount != 0 && rowsCount != ROWS_TO_INSERT, "Wrong row count");

  for (i= 0; i < paramsProcessed; ++i)
  {
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_BINARY, (SQLPOINTER)buff, 50, &nLen));
    IS(memcmp((const void*) buff, (const void*)dataBinding[i].bData, 5 - i)==0);
    is_num(my_fetch_int(Stmt, 2), dataBinding[i].intField);
    IS_STR(my_fetch_str(Stmt, buff, 3), dataBinding[i].strField, strlen(str[i]));
  }

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* One more check that RowCount isn't broken. check may get broken if input data
     changes */
  OK_SIMPLE_STMT(Stmt, "update t_bug48310 set strField='changed' where intField > 1");
  CHECK_STMT_RC(Stmt, SQLRowCount(Stmt, &rowsCount));
  is_num(rowsCount, 1);

  /* Clean-up */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "DROP TABLE IF EXISTS bug48310", SQL_NTS));

  return OK;

#undef ROWS_TO_INSERT
#undef STR_FIELD_LENGTH
}


/*
Bug 48310 - parameters array support request.
Binding by column test
*/
ODBC_TEST(paramarray_by_column)
{
#define ROWS_TO_INSERT 3
#define STR_FIELD_LENGTH 5
  SQLCHAR       buff[50];

  SQLCHAR       bData[ROWS_TO_INSERT][STR_FIELD_LENGTH]={{0x01, 0x80, 0x00, 0x80, 0x03},
                                          {0x02, 0x80, 0x00, 0x02},
                                          {0x03, 0x80, 0x01}};
  SQLLEN        bInd[ROWS_TO_INSERT]= {5,4,3};

  const SQLCHAR strField[ROWS_TO_INSERT][STR_FIELD_LENGTH]= {{'\0'}, {'x','\0'}, {'x','x','x','\0'} };
  SQLLEN        strInd[ROWS_TO_INSERT]= {SQL_NTS, SQL_NTS, SQL_NTS};

  SQLINTEGER    intField[ROWS_TO_INSERT] = {123321, 1, 0};
  SQLLEN        intInd[ROWS_TO_INSERT]= {5,4,3};

  SQLUSMALLINT  paramStatusArray[ROWS_TO_INSERT];
  SQLULEN       paramsProcessed, i, nLen;

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "DROP TABLE IF EXISTS t_bug48310", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "CREATE TABLE t_bug48310 (id int primary key auto_increment,"\
    "bData binary(5) NULL, intField int not null, strField varchar(255) not null)", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ROWS_TO_INSERT, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_STATUS_PTR, paramStatusArray, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &paramsProcessed, 0));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY,
    0, 0, bData, 5, bInd));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
    0, 0, intField, 0, intInd));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
    0, 0, (SQLPOINTER)strField, 5, strInd ));

  /* We don't expect errors in paramsets processing, thus we should get SQL_SUCCESS only*/
  FAIL_IF(SQLExecDirect(Stmt, "INSERT INTO t_bug48310 (bData, intField, strField) " \
    "VALUES (?,?,?)", SQL_NTS) != SQL_SUCCESS, "success expected");

  is_num(paramsProcessed, ROWS_TO_INSERT);

  for (i= 0; i < paramsProcessed; ++i)
    if ( paramStatusArray[i] != SQL_PARAM_SUCCESS
      && paramStatusArray[i] != SQL_PARAM_SUCCESS_WITH_INFO )
    {
      diag("Parameter #%u status isn't successful(0x%X)", i+1, paramStatusArray[i]);
      return FAIL;
    }

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)1, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "SELECT bData, intField, strField\
                                       FROM t_bug48310\
                                       ORDER BY id", SQL_NTS));

  for (i= 0; i < paramsProcessed; ++i)
  {
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_BINARY, (SQLPOINTER)buff, 50, &nLen));
    if (memcmp((const void*) buff, bData[i], 5 - i)!=0)
    {
      diag("Bin data inserted wrongly. Read: 0x%02X%02X%02X%02X%02X Had to be: 0x%02X%02X%02X%02X%02X"
        , buff[0], buff[1], buff[2], buff[3], buff[4]
        , bData[i][0], bData[i][1], bData[i][2], bData[i][3], bData[i][4]);
      return FAIL;
    }
    is_num(my_fetch_int(Stmt, 2), intField[i]);
    IS_STR(my_fetch_str(Stmt, buff, 3), strField[i], strlen(strField[i]));
  }

  /* Clean-up */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "DROP TABLE IF EXISTS bug48310", SQL_NTS));

  return OK;

#undef ROWS_TO_INSERT
#undef STR_FIELD_LENGTH
}


/*
Bug 48310 - parameters array support request.
Ignore paramset test
*/
ODBC_TEST(paramarray_ignore_paramset)
{
#define ROWS_TO_INSERT 4
#define STR_FIELD_LENGTH 5
  SQLCHAR       buff[50];

  SQLCHAR       bData[ROWS_TO_INSERT][STR_FIELD_LENGTH]={{0x01, 0x80, 0x00, 0x80, 0x03},
                                                        {0x02, 0x80, 0x00, 0x02},
                                                        {0x03, 0x80, 0x01}};
  SQLLEN        bInd[ROWS_TO_INSERT]= {5,4,3};

  const SQLCHAR strField[ROWS_TO_INSERT][STR_FIELD_LENGTH]= {{'\0'}, {'x','\0'}, {'x','x','x','\0'} };
  SQLLEN        strInd[ROWS_TO_INSERT]= {SQL_NTS, SQL_NTS, SQL_NTS};

  SQLINTEGER    intField[ROWS_TO_INSERT] = {123321, 1, 0};
  SQLLEN        intInd[ROWS_TO_INSERT]= {5,4,3};

  SQLUSMALLINT  paramOperationArr[ROWS_TO_INSERT]={0,SQL_PARAM_IGNORE,0,SQL_PARAM_IGNORE};
  SQLUSMALLINT  paramStatusArr[ROWS_TO_INSERT];
  SQLULEN       paramsProcessed, i, nLen, rowsInserted= 0;

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "DROP TABLE IF EXISTS t_bug48310", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "CREATE TABLE t_bug48310 (id int primary key auto_increment,"\
    "bData binary(5) NULL, intField int not null, strField varchar(255) not null)", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ROWS_TO_INSERT, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_STATUS_PTR, paramStatusArr, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_OPERATION_PTR, paramOperationArr, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &paramsProcessed, 0));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY,
    0, 0, bData, 5, bInd));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
    0, 0, intField, 0, intInd));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
    0, 0, (SQLPOINTER)strField, 5, strInd ));

  /* We don't expect errors in paramsets processing, thus we should get SQL_SUCCESS only*/
  FAIL_IF(SQLExecDirect(Stmt, "INSERT INTO t_bug48310 (bData, intField, strField) " \
    "VALUES (?,?,?)", SQL_NTS) != SQL_SUCCESS, "success expected");

  is_num(paramsProcessed, ROWS_TO_INSERT);

  for (i= 0; i < paramsProcessed; ++i)
  {
    if (paramOperationArr[i] == SQL_PARAM_IGNORE)
    {
      is_num(paramStatusArr[i], SQL_PARAM_UNUSED);
    }
    else if ( paramStatusArr[i] != SQL_PARAM_SUCCESS
      && paramStatusArr[i] != SQL_PARAM_SUCCESS_WITH_INFO )
    {
      diag("Parameter #%u status isn't successful(0x%X)", i+1, paramStatusArr[i]);
      return FAIL;
    }
  }

  /* Resetting statements attributes */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)1, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, NULL, 0));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "SELECT bData, intField, strField\
                                      FROM t_bug48310\
                                      ORDER BY id", SQL_NTS));

  i= 0;
  while(i < paramsProcessed)
  {
    if (paramStatusArr[i] == SQL_PARAM_UNUSED)
    {
      ++i;
      continue;
    }

    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

    CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_BINARY, (SQLPOINTER)buff, 50, &nLen));

    if (memcmp((const void*) buff, bData[i], 5 - i)!=0)
    {
      diag("Bin data inserted wrongly. Read: 0x%02X%02X%02X%02X%02X Had to be: 0x%02X%02X%02X%02X%02X"
        , buff[0], buff[1], buff[2], buff[3], buff[4]
      , bData[i][0], bData[i][1], bData[i][2], bData[i][3], bData[i][4]);
      return FAIL;
    }
    is_num(my_fetch_int(Stmt, 2), intField[i]);
    IS_STR(my_fetch_str(Stmt, buff, 3), strField[i], strlen(strField[i]));

    ++rowsInserted;
    ++i;
  }

  /* Making sure that there is nothing else to fetch ... */
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  /* ... and that inserted was less than SQL_ATTR_PARAMSET_SIZE rows */
  IS( rowsInserted < ROWS_TO_INSERT);
  
  /* Clean-up */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "DROP TABLE IF EXISTS bug48310", SQL_NTS));

  return OK;

#undef ROWS_TO_INSERT
#undef STR_FIELD_LENGTH
}


/*
  Bug 48310 - parameters array support request.
  Select statement.
*/
ODBC_TEST(paramarray_select)
{
#define STMTS_TO_EXEC 3

  SQLINTEGER    intField[STMTS_TO_EXEC] = {3, 1, 2};
  SQLLEN        intInd[STMTS_TO_EXEC]= {5,4,3};

  SQLUSMALLINT  paramStatusArray[STMTS_TO_EXEC];
  SQLULEN       paramsProcessed, i;

  diag("select with paramarray not supported");
  return SKIP;

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)STMTS_TO_EXEC, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_STATUS_PTR, paramStatusArray, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &paramsProcessed, 0));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
    0, 0, intField, 0, intInd));

  /* We don't expect errors in paramsets processing, thus we should get SQL_SUCCESS only*/
  FAIL_IF(SQLExecDirect(Stmt, "SELect ?,'So what'", SQL_NTS) != SQL_SUCCESS, "success expected");
  is_num(paramsProcessed, STMTS_TO_EXEC);

  for (i= 0; i < paramsProcessed; ++i)
  {
    if ( paramStatusArray[i] != SQL_PARAM_SUCCESS
      && paramStatusArray[i] != SQL_PARAM_SUCCESS_WITH_INFO )
    {
      diag("Parameter #%u status isn't successful(0x%X)", i+1, paramStatusArray[i]);
      return FAIL;
    }

    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

    is_num(my_fetch_int(Stmt, 1), intField[i]);
  }

  /* Clean-up */
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;

#undef STMTS_TO_EXEC
}


/*
  Bug #49029 - Server with sql mode NO_BACKSLASHES_ESCAPE obviously
  can work incorrectly (at least) with binary parameters
*/
ODBC_TEST(t_bug49029)
{
  const SQLCHAR bData[6]= "\x01\x80\x00\x80\x01";
  SQLCHAR buff[6];
  SQLULEN len= 5;

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "set @@session.sql_mode='NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_BACKSLASH_ESCAPES'", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY,
    0, 0, (SQLPOINTER)bData, 0, &len));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "select ?", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_BINARY, (SQLPOINTER)buff, 6, &len));

  FAIL_IF(memcmp((const void*) buff, (const void*)bData, 5)!=0, "comparison failed");

  return OK;
}


/*
  Bug #56804 - Server with sql mode NO_BACKSLASHES_ESCAPE obviously
  can work incorrectly (at least) with binary parameters
*/
ODBC_TEST(t_bug56804)
{
#define PARAMSET_SIZE		10

  SQLINTEGER	len 	= 1;
  int i;

  SQLINTEGER	c1[PARAMSET_SIZE]=      {0, 1, 2, 3, 4, 5, 1, 7, 8, 9};
  SQLINTEGER	c2[PARAMSET_SIZE]=      {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  SQLLEN      d1[PARAMSET_SIZE]=      {4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
  SQLLEN      d2[PARAMSET_SIZE]=      {4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
  SQLUSMALLINT status[PARAMSET_SIZE]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  SQLSMALLINT	paramset_size	= PARAMSET_SIZE;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug56804");
  OK_SIMPLE_STMT(Stmt, "create table bug56804 (c1 int primary key not null, c2 int)");
  OK_SIMPLE_STMT(Stmt, "insert into bug56804 values( 1, 1 ), (9, 9009)");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"insert into bug56804 values( ?,? )", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr( Stmt, SQL_ATTR_PARAMSET_SIZE,
    (SQLPOINTER)paramset_size, SQL_IS_UINTEGER ));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr( Stmt, SQL_ATTR_PARAM_STATUS_PTR,
    status, SQL_IS_POINTER ));

  CHECK_STMT_RC(Stmt, SQLBindParameter( Stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG,
    SQL_DECIMAL, 4, 0, c1, 4, d1));

  CHECK_STMT_RC(Stmt, SQLBindParameter( Stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG,
    SQL_DECIMAL, 4, 0, c2, 4, d2));

  FAIL_IF(SQLExecute(Stmt) != SQL_SUCCESS_WITH_INFO, "swi expected");

  /* Following tests are here to ensure that driver works how it is currently
     expected to work, and they need to be changed if driver changes smth in the
     way how it reports errors in paramsets and diagnostics */
  for(i = 0; i < PARAMSET_SIZE; ++i )
  {
    diag("Paramset #%d (%d, %d)", i, c1[i], c2[i]);
    switch (i)
    {
    case 1:
    case 6:
      /* all errors but last have SQL_PARAM_DIAG_UNAVAILABLE */
      is_num(status[i], SQL_PARAM_DIAG_UNAVAILABLE);
      break;
    case 9:
      /* Last error -  we are supposed to get SQL_PARAM_ERROR for it */
      is_num(status[i], SQL_PARAM_ERROR);
      break;
    default:
      is_num(status[i], SQL_PARAM_SUCCESS);
    }
  }

  {
    SQLCHAR     sqlstate[6]= {0};
    SQLCHAR     message[255]= {0};
    SQLINTEGER  native_err= 0;
    SQLSMALLINT msglen= 0;

    i= 0;
    while(SQL_SUCCEEDED(SQLGetDiagRec(SQL_HANDLE_STMT, Stmt, ++i, sqlstate,
      &native_err, message, sizeof(message), &msglen)))
    {
      diag("%d) [%s] %s %d", i, sqlstate, message, native_err);
    }

    /* just to make sure we got 1 diagnostics record ... */
    is_num(i, 2);
    /* ... and what the record is for the last error */
    FAIL_IF(strstr(message, "Duplicate entry '9'") == NULL, "comparison failed");
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug56804");

  return OK;
#undef PARAMSET_SIZE
}


/*
  Bug 59772 - Column parameter binding makes SQLExecute not to return
  SQL_ERROR on disconnect
*/
ODBC_TEST(t_bug59772)
{
#define ROWS_TO_INSERT 3

    SQLRETURN rc;
    SQLCHAR   buf_kill[50];

    SQLINTEGER    intField[ROWS_TO_INSERT] = {123321, 1, 0};
    SQLLEN        intInd[ROWS_TO_INSERT]= {5,4,3};

    SQLUSMALLINT  paramStatusArray[ROWS_TO_INSERT];
    SQLULEN       paramsProcessed, i;

    SQLINTEGER connection_id;

    SQLHENV henv2;
    SQLHDBC  hdbc2;
    SQLHSTMT hstmt2;

    int overall_result= OK;

    /* Create a new connection that we deliberately will kill */
    ODBC_Connect(&henv2, &hdbc2, &hstmt2);

    OK_SIMPLE_STMT(hstmt2, "SELECT connection_id()");
    CHECK_STMT_RC(hstmt2, SQLFetch(hstmt2));
    connection_id= my_fetch_int(hstmt2, 1);
    CHECK_STMT_RC(hstmt2, SQLFreeStmt(hstmt2, SQL_CLOSE));

    CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "DROP TABLE IF EXISTS t_bug59772", SQL_NTS));
    CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "CREATE TABLE t_bug59772 (id int primary key auto_increment,"\
      "intField int)", SQL_NTS));

    CHECK_STMT_RC(hstmt2, SQLSetStmtAttr(hstmt2, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0));
    CHECK_STMT_RC(hstmt2, SQLSetStmtAttr(hstmt2, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ROWS_TO_INSERT, 0));
    CHECK_STMT_RC(hstmt2, SQLSetStmtAttr(hstmt2, SQL_ATTR_PARAM_STATUS_PTR, paramStatusArray, 0));
    CHECK_STMT_RC(hstmt2, SQLSetStmtAttr(hstmt2, SQL_ATTR_PARAMS_PROCESSED_PTR, &paramsProcessed, 0));

    CHECK_STMT_RC(hstmt2, SQLPrepare(hstmt2, "INSERT INTO t_bug59772 (intField) VALUES (?)", SQL_NTS));

    CHECK_STMT_RC(hstmt2, SQLBindParameter(hstmt2, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
      0, 0, intField, 0, intInd));

    /* From another connection, kill the connection created above */
    sprintf(buf_kill, "KILL %d", connection_id);
    CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, (SQLCHAR *)buf_kill, SQL_NTS));

    rc= SQLExecute(hstmt2);
    
    /* The result should be SQL_ERROR */
    if (rc != SQL_ERROR)
      overall_result= FAIL;

    for (i= 0; i < paramsProcessed; ++i)

      /* We expect error statuses for all parameters */
      if ( paramStatusArray[i] != ((i + 1 < ROWS_TO_INSERT) ? 
            SQL_PARAM_DIAG_UNAVAILABLE : SQL_PARAM_ERROR) )
      {
        diag("Parameter #%u status isn't successful(0x%X)", i+1, paramStatusArray[i]);
        overall_result= FAIL;
      }

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
    CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "DROP TABLE IF EXISTS t_bug59772", SQL_NTS));

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt2);
    SQLDisconnect(hdbc2);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc2);
    SQLFreeHandle(SQL_HANDLE_ENV, henv2);

    return overall_result;
#undef ROWS_TO_INSERT
}


ODBC_TEST(t_odbcoutparams)
{
  SQLSMALLINT ncol, i;
  SQLINTEGER  par[3]= {10, 20, 30}, val;
  SQLLEN      len;
  SQLSMALLINT type[]= {SQL_PARAM_INPUT, SQL_PARAM_OUTPUT, SQL_PARAM_INPUT_OUTPUT};
  SQLCHAR     str[20]= "initial value", buff[20];

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS t_odbcoutparams");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE t_odbcoutparams("
                "  IN p_in INT, "
                "  OUT p_out INT, "
                "  INOUT p_inout INT) "
                "BEGIN "
                "  SET p_in = p_in*10, p_out = (p_in+p_inout)*10, p_inout = p_inout*10; "
                "END");



  for (i=0; i < sizeof(par)/sizeof(SQLINTEGER); ++i)
  {
    CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, i+1, type[i], SQL_C_LONG, SQL_INTEGER, 0,
      0, &par[i], 0, NULL));
  }

  OK_SIMPLE_STMT(Stmt, "CALL t_odbcoutparams(?, ?, ?)");

  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 2);

  is_num(par[1], 1300);
  is_num(par[2], 300);
  
  /* Only 1 row always - we still can get them as a result */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1300);
  is_num(my_fetch_int(Stmt, 2), 300);
  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS t_odbcoutparams");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE t_odbcoutparams("
                "  IN p_in INT, "
                "  OUT p_out INT, "
                "  INOUT p_inout INT) "
                "BEGIN "
                "  SELECT p_in, p_out, p_inout; "
                "  SET p_in = 300, p_out = 100, p_inout = 200; "
                "END");
  OK_SIMPLE_STMT(Stmt, "CALL t_odbcoutparams(?, ?, ?)");
  /* rs-1 */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 3);

  is_num(my_fetch_int(Stmt, 1), 10);
  /* p_out does not have value at the moment */
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_INTEGER, &val, 0, &len));
  is_num(len, SQL_NULL_DATA);
  is_num(my_fetch_int(Stmt, 3), 300);

  FAIL_IF(SQLFetch(Stmt) != SQL_NO_DATA_FOUND, "eof expected");
  CHECK_STMT_RC(Stmt, SQLMoreResults(Stmt));

  is_num(par[1], 100);
  is_num(par[2], 200);

  FAIL_IF(SQLMoreResults(Stmt) != SQL_NO_DATA, "eof expected");
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS t_odbcoutparams");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE t_odbcoutparams("
                "  OUT p_out VARCHAR(19), "
                "  IN p_in INT, "
                "  INOUT p_inout INT) "
                "BEGIN "
                "  SET p_in = 300, p_out := 'This is OUT param', p_inout = 200; "
                "  SELECT p_inout, p_in, substring(p_out, 9);"
                "END");
  
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, 0,
      0, str, sizeof(str)/sizeof(SQLCHAR), NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
      0, &par[0], 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT_OUTPUT, SQL_C_LONG,
      SQL_INTEGER, 0, 0, &par[1], 0, NULL));

  OK_SIMPLE_STMT(Stmt, "CALL t_odbcoutparams(?, ?, ?)");

  /* rs-1 */
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 3);
  
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 200);
  is_num(my_fetch_int(Stmt, 2), 300);
  IS_STR(my_fetch_str(Stmt, buff, 3), "OUT param", 10);

  CHECK_STMT_RC(Stmt, SQLMoreResults(Stmt));
  IS_STR(str, "This is OUT param", 18);
  is_num(par[1], 200);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buff, 1), "This is OUT param", 18);
  is_num(my_fetch_int(Stmt, 2), 200);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE t_odbcoutparams");

  return OK;
}


ODBC_TEST(t_bug14501952)
{
  SQLSMALLINT ncol;
  SQLLEN      len= 0;
  SQLCHAR     blobValue[50]= "initial value", buff[100];

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS bug14501952");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE bug14501952 (INOUT param1 BLOB)\
                  BEGIN\
                    SET param1= 'this is blob value from SP ';\
                  END;");



  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT_OUTPUT,
    SQL_C_BINARY, SQL_LONGVARBINARY, 50, 0, &blobValue, sizeof(blobValue),
    &len));

  OK_SIMPLE_STMT(Stmt, "CALL bug14501952(?)");

  IS_STR(blobValue, "this is blob value from SP ", 27);
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 1);

  /* Only 1 row always - we still can get them as a result */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_BINARY, buff, sizeof(buff),
                            &len));
  IS_STR(buff, blobValue, 27);
  
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE bug14501952");

  return OK;
}


/* Bug#14563386 More than one BLOB(or any big data types) OUT param caused crash
 */
ODBC_TEST(t_bug14563386)
{
  SQLSMALLINT ncol;
  SQLLEN      len= 0, len1= 0;
  SQLCHAR     blobValue[50]= "initial value", buff[100],
              binValue[50]= "varbinary init value";

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS b14563386");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE b14563386 (INOUT blob_param \
                        BLOB, INOUT bin_param LONG VARBINARY)\
                  BEGIN\
                    SET blob_param = ' BLOB! ';\
                    SET bin_param = ' LONG VARBINARY ';\
                  END;");



  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT_OUTPUT,
    SQL_C_BINARY, SQL_LONGVARBINARY, 50, 0, &blobValue, sizeof(blobValue),
    &len));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT_OUTPUT,
    SQL_C_BINARY, SQL_LONGVARBINARY, 50, 0, &binValue, sizeof(binValue),
    &len1));
  OK_SIMPLE_STMT(Stmt, "CALL b14563386(?, ?)");

  IS_STR(blobValue, " BLOB! ", 7);
  IS_STR(binValue, " LONG VARBINARY ", 16);
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 2);

  /* Only 1 row always - we still can get them as a result */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_BINARY, buff, sizeof(buff),
                            &len));
  IS_STR(buff, blobValue, 7);
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_BINARY, buff, sizeof(buff),
                            &len));
  IS_STR(buff, binValue, 16);
  
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE b14563386");

  return OK;
}


/* Bug#14551229(could not repeat) Procedure with signed out parameter */
ODBC_TEST(t_bug14551229)
{
  SQLINTEGER param, value;

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS b14551229");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE b14551229 (OUT param INT)\
                  BEGIN\
                    SELECT -1 into param from dual;\
                  END;");



  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_OUTPUT,
    SQL_C_SLONG, SQL_INTEGER, 50, 0, &param, 0, 0));

  OK_SIMPLE_STMT(Stmt, "CALL b14551229(?)");

  is_num(param, -1);

  /* Only 1 row always - we still can get them as a result */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_SLONG, &value, 0, 0));
  is_num(value, -1);
  
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE b14551229");

  return OK;
}


/* Bug#14560916 ASSERT for INOUT parameter of BIT(10) type */
ODBC_TEST(t_bug14560916)
{
  char        param[2]={1,1};
  SQLINTEGER  value;
  SQLLEN      len= 0;

  diag("fix me");
  return SKIP;

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS b14560916");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug14560916");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug14560916 (a BIT(10))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO bug14560916  values(b'1001000001')");

  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE b14560916 (INOUT param bit(10))\
                  BEGIN\
                    SELECT a INTO param FROM bug14560916;\
                  END;");



  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT_OUTPUT,
    SQL_C_CHAR, SQL_CHAR, 0, 0, &param, sizeof(param), &len));

  /* Parameter is used to make sure that ssps will be used */
  OK_SIMPLE_STMT(Stmt, "select a from bug14560916 where ? OR 1");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
 // CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_SLONG, &value, 0, 0));
 // is_num(value, 577);
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_BINARY, &param, sizeof(param),
                            &len));
  is_num(len, 2);
  IS_STR(param, "\2A", 2);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  

  OK_SIMPLE_STMT(Stmt, "CALL b14560916(?)");

  is_num(len, 2);
  IS_STR(param, "\2A", 2);

  /* Only 1 row always - we still can get them as a result */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_SLONG, &value, 0, 0));
  is_num(value, 577);
  param[0]= param[1]= 1;
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_BINARY, &param, sizeof(param),
                            &len));
  is_num(len, 2);
  IS_STR(param, "\2A", 2);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE b14560916");

  return OK;
}


/* Bug#14586094 Crash while executing SP having blob and varchar OUT parameters
  (could not repeat)
 */
ODBC_TEST(t_bug14586094)
{
  SQLSMALLINT ncol;
  SQLLEN      len= SQL_NTS, len1= SQL_NTS;
  SQLCHAR     blobValue[50]= {0}/*"initial value"*/, buff[101],
              vcValue[101]= "varchar init value";

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS b14586094");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE b14586094 (INOUT blob_param \
                    BLOB(50), INOUT vc_param VARCHAR(100))\
                  BEGIN\
                    SET blob_param = ' BLOB! ';\
                    SET vc_param = 'varchar';\
                  END;");



  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT_OUTPUT,
    SQL_C_BINARY, SQL_LONGVARBINARY, 50, 0, &blobValue, sizeof(blobValue),
    &len));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT_OUTPUT,
    SQL_C_CHAR, SQL_VARCHAR, 50, 0, &vcValue, sizeof(vcValue), &len1));
  OK_SIMPLE_STMT(Stmt, "CALL b14586094(?, ?)");

  IS_STR(blobValue, " BLOB! ", 7);
  IS_STR(vcValue, "varchar", 9);
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 2);

  /* Only 1 row always - we still can get them as a result */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_BINARY, buff, sizeof(buff),
                            &len));
  IS_STR(buff, blobValue, 7);
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 2, SQL_C_CHAR, buff, sizeof(buff),
                            &len));
  IS_STR(buff, vcValue, 9);
  
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE b14586094");

  return OK;
}


ODBC_TEST(t_longtextoutparam)
{
  SQLSMALLINT ncol;
  SQLLEN      len= 0;
  SQLCHAR     blobValue[50]= "initial value", buff[100];


  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE IF EXISTS t_longtextoutparam");
  OK_SIMPLE_STMT(Stmt, "CREATE PROCEDURE t_longtextoutparam (INOUT param1 LONGTEXT)\
                  BEGIN\
                    SET param1= 'this is LONGTEXT value from SP ';\
                  END;");



  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT_OUTPUT,
    SQL_C_BINARY, SQL_LONGVARBINARY, 50, 0, &blobValue, sizeof(blobValue),
    &len));

  OK_SIMPLE_STMT(Stmt, "CALL t_longtextoutparam(?)");

  IS_STR(blobValue, "this is LONGTEXT value from SP ", 32);
  CHECK_STMT_RC(Stmt, SQLNumResultCols(Stmt,&ncol));
  is_num(ncol, 1);

  /* Only 1 row always - we still can get them as a result */
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, buff, sizeof(buff),
                            &len));
  IS_STR(buff, blobValue, 32);
  
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP PROCEDURE t_longtextoutparam");

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {unbuffered_result, "unbuffered_result"},
  {my_init_table, "my_init_table"},
  {my_param_insert, "my_param_insert"},
  {my_param_update, "my_param_update"},
  {my_param_delete, "my_param_delete"},
  {tmysql_fix, "tmysql_fix"},
  {t_param_offset, "t_param_offset"},
  {paramarray_by_row, "paramarray_by_row"},
  {paramarray_by_column, "paramarray_by_column"},
  {paramarray_ignore_paramset, "paramarray_ignore_paramset"},
  {paramarray_select, "paramarray_select"},
  {t_bug49029, "t_bug49029"},
  {t_bug56804, "t_bug56804"},
  {t_bug59772, "t_bug59772"},
  {t_odbcoutparams, "t_odbcoutparams"},
  {t_bug14501952, "t_bug14501952"},
  {t_bug14563386, "t_bug14563386"},
  {t_bug14551229, "t_bug14551229"},
  {t_bug14560916, "t_bug14560916"},
  {t_bug14586094, "t_bug14586094"},
  {t_longtextoutparam, "t_longtextoutparam"},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
