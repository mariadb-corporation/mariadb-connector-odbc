  /*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013, 2018 MariaDB Corporation AB

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

  FAIL_IF(10 != myrowcount(Stmt), "expected 10 rows");

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

    IS(10 == myrowcount(Stmt));

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

    IS(8 == myrowcount(Stmt));

    CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
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
    0, 0, bData, STR_FIELD_LENGTH, bInd));
  is_num(iOdbcSetParamBufferSize(Stmt, 1, STR_FIELD_LENGTH), OK);
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
    0, 0, intField, 0, intInd));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
    0, 0, (SQLPOINTER)strField, STR_FIELD_LENGTH, strInd ));
  is_num(iOdbcSetParamBufferSize(Stmt, 3, STR_FIELD_LENGTH), OK);

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
    0, 0, bData, STR_FIELD_LENGTH, bInd));
  is_num(iOdbcSetParamBufferSize(Stmt, 1, STR_FIELD_LENGTH), OK);
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
    0, 0, intField, 0, intInd));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
    0, 0, (SQLPOINTER)strField, STR_FIELD_LENGTH, strInd ));
  is_num(iOdbcSetParamBufferSize(Stmt, 3, STR_FIELD_LENGTH), OK);

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
      diag("Wrong Bin data to has been inserted to the row #%d. Read: 0x%02X%02X%02X%02X%02X Expected: 0x%02X%02X%02X%02X%02X"
        , i + 1, buff[0], buff[1], buff[2], buff[3], buff[4]
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

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "SET @@session.sql_mode='NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,NO_BACKSLASH_ESCAPES'", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY,
    0, 0, (SQLPOINTER)bData, 0, &len));

  CHECK_STMT_RC(Stmt, SQLExecDirect(Stmt, "SELECT ?", SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_BINARY, (SQLPOINTER)buff, 6, &len));

  FAIL_IF(memcmp((const void*) buff, (const void*)bData, 5)!=0, "comparison failed");

  return OK;
}


/*
  Bug #56804 - 
*/
ODBC_TEST(t_bug56804)
{
#define PARAMSET_SIZE		10

  SQLINTEGER	len 	= 1;
  int i;
  BOOL SolidOperation= TRUE;

  SQLINTEGER	c1[PARAMSET_SIZE]=      {0, 1, 2, 3, 4, 5, 1, 7, 8, 9};
  SQLINTEGER	c2[PARAMSET_SIZE]=      {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  SQLLEN      d1[PARAMSET_SIZE]=      {4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
  SQLLEN      d2[PARAMSET_SIZE]=      {4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
  SQLUSMALLINT status[PARAMSET_SIZE]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  SQLUSMALLINT ExpectedStatus[PARAMSET_SIZE];

  SQLLEN	    paramset_size	= PARAMSET_SIZE;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug56804");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE bug56804 (c1 INT PRIMARY KEY NOT NULL, c2 INT)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO bug56804 VALUES( 1, 1 ), (9, 9009)");

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

  
  if (ServerNotOlderThan(Connection, 10, 2, 7))
  {
    /* Starting from 10.2.7 connector will use bulk operations, which is solid, and either success or fail all together */
    EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_ERROR);
    memset(ExpectedStatus, 0x00ff & SQL_PARAM_DIAG_UNAVAILABLE, sizeof(ExpectedStatus));
  }
  else
  {
    EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_SUCCESS_WITH_INFO);
    memset(ExpectedStatus, 0x00ff & SQL_PARAM_SUCCESS, sizeof(ExpectedStatus));
    /* all errors but last have SQL_PARAM_DIAG_UNAVAILABLE */
    ExpectedStatus[1]= ExpectedStatus[6]= SQL_PARAM_DIAG_UNAVAILABLE;
    ExpectedStatus[9]= SQL_PARAM_ERROR;
    SolidOperation= FALSE;
  }
  

  /* Following tests are here to ensure that driver works how it is currently
     expected to work, and they need to be changed if driver changes smth in the
     way how it reports errors in paramsets and diagnostics */
  for(i = 0; i < PARAMSET_SIZE; ++i )
  {
    diag("Paramset #%d (%d, %d)", i, c1[i], c2[i]);
    is_num(status[i], ExpectedStatus[i]);
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
    if (SolidOperation)
    {
      FAIL_IF(strstr(message, "Duplicate entry '1'") == NULL, "comparison failed");
    }
    else
    {
      FAIL_IF(strstr(message, "Duplicate entry '9'") == NULL, "comparison failed");
    }
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
  /* Need to get to next resultset to get OUT parameters */
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

  is_num(SQLMoreResults(Stmt), SQL_SUCCESS); /* Last result - SP execution status, affected rows */
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
  SQLCHAR     blobValue[50]= "initial value", buff[100]= {0};

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
  SQLCHAR     blobValue[50]= "initial value", buff[100]={0},
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
  SQLINTEGER param= 0, value;

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
  SQLCHAR     blobValue[50]= {0}/*"initial value"*/, buff[101]= {0},
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

/* Test of inserting of previously fetched values, including NULL values, empty values etc
   Also covers ODBC-51 and 52(duplicate) */
ODBC_TEST(insert_fetched_null)
{
  SQLLEN      len[6];
  SQLWCHAR    val[50], empty[50];
  SQLINTEGER  id, mask;
  SQLDOUBLE   double_val;
  HSTMT       Stmt1;
  const char     *str= "Text val";
  const SQLWCHAR *wstr= CW(str);

  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &Stmt1));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_insert_fetched_null");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_insert_fetched_null (id int not null primary key, double_val double not null,\
                                                            val varchar(50) CHARACTER SET utf8 not null,\
                                                            nullable_val varchar(50) CHARACTER SET utf8, mask int unsigned not null,\
                                                            empty_val varchar(50) CHARACTER SET utf8)");

  OK_SIMPLE_STMT(Stmt, "SELECT 1, 0.001, _utf8'Text val', NULL, 127, _utf8''");

  memset(len, 0, sizeof(len));

  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &id, 0, &len[0] ));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_DOUBLE, &double_val, 0, &len[1]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_WCHAR, &val, sizeof(val), &len[2]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 4, SQL_C_WCHAR, &val, sizeof(val), &len[3]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 5, SQL_C_LONG, &mask, 0, &len[4]));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 6, SQL_C_WCHAR, &empty, sizeof(empty), &len[5]));

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(id, 1);
  IS_WSTR(val, wstr, len[2] / sizeof(SQLWCHAR))
  is_num(len[3], SQL_NULL_DATA);
  is_num(mask, 127);
  is_num(len[5], 0);
  IS_WSTR(empty, sqlwchar_empty, len[5]);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  CHECK_STMT_RC(Stmt1, SQLBindParameter(Stmt1, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id, 0, &len[0]));
  CHECK_STMT_RC(Stmt1, SQLBindParameter(Stmt1, 2, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, &double_val, 0, &len[1]));
  CHECK_STMT_RC(Stmt1, SQLBindParameter(Stmt1, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR, 0, 0, &val, sizeof(val), &len[2]));
  CHECK_STMT_RC(Stmt1, SQLBindParameter(Stmt1, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR, 0, 0, &val, sizeof(val), &len[3]));
  CHECK_STMT_RC(Stmt1, SQLBindParameter(Stmt1, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &mask, 0, &len[4]));
  CHECK_STMT_RC(Stmt1, SQLBindParameter(Stmt1, 6, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR, 0, 0, &empty, sizeof(empty), &len[5]));

  CHECK_STMT_RC(Stmt1, SQLPrepare(Stmt1, "INSERT INTO t_insert_fetched_null(id, double_val, val, nullable_val, mask, empty_val)\
                                          VALUES(?, ?, ?, ?, ?, ?)", SQL_NTS));
  CHECK_STMT_RC(Stmt1, SQLExecute(Stmt1));
  
  OK_SIMPLE_STMT(Stmt1, "SELECT id, double_val, val, nullable_val, mask, empty_val FROM t_insert_fetched_null");

  val[0]= 0;
  empty[0]= 'a';
  id= mask= 0;
  memset(len, 0, sizeof(len));

  CHECK_STMT_RC(Stmt1, SQLBindCol(Stmt1, 1, SQL_C_LONG, &id, 0, &len[0]));
  CHECK_STMT_RC(Stmt1, SQLBindCol(Stmt1, 2, SQL_C_DOUBLE, &double_val, 0, &len[1]));
  /* Somehow with iOdbc 3.52.15 len in strlen ptr is 0, if sufficient buffer size is not passed. At least with 3.52.12 that is not observed.
   * Changelog does not give any clue. */
  if (iOdbc() && DmMinVersion(3, 52, 13))
  {
    CHECK_STMT_RC(Stmt1, SQLBindCol(Stmt1, 3, SQL_C_WCHAR, &val, sizeof(val), &len[2]));
  }
  else
  {
    CHECK_STMT_RC(Stmt1, SQLBindCol(Stmt1, 3, SQL_C_WCHAR, &val, 0, &len[2]));
  }
  CHECK_STMT_RC(Stmt1, SQLBindCol(Stmt1, 4, SQL_C_WCHAR, &val, sizeof(val), &len[3]));
  CHECK_STMT_RC(Stmt1, SQLBindCol(Stmt1, 5, SQL_C_LONG, &mask, 0, &len[4]));
  CHECK_STMT_RC(Stmt1, SQLBindCol(Stmt1, 6, SQL_C_WCHAR, &empty, sizeof(empty), &len[5]));

  CHECK_STMT_RC(Stmt1, SQLFetch(Stmt1));

  is_num(id, 1);
  is_num(len[2], strlen(str)*sizeof(SQLWCHAR));
  is_num(len[3], SQL_NULL_DATA);
  is_num(mask, 127);
  is_num(len[5], 0);
  CHECK_STMT_RC(Stmt1, SQLGetData(Stmt1, 3, SQL_C_WCHAR, &val, len[2]+sizeof(SQLWCHAR), &len[2]));
  IS_WSTR(val, wstr, len[2]/sizeof(SQLWCHAR));

  /* Testing also that with SQLGetData everything is fine */
  len[3]= 0;
  len[5]= 1;

  CHECK_STMT_RC(Stmt1, SQLGetData(Stmt1, 4, SQL_C_WCHAR, &val, sizeof(val), &len[3]));
  /* val len should not be changed, indicator should be NULL */
  IS_WSTR(val, wstr, len[2]/sizeof(SQLWCHAR));
  is_num(len[3], SQL_NULL_DATA);

  CHECK_STMT_RC(Stmt1, SQLGetData(Stmt1, 6, SQL_C_WCHAR, &empty, sizeof(empty), &len[5]));
  is_num(len[5], 0);
  IS_WSTR(empty, sqlwchar_empty, len[5]);

  CHECK_STMT_RC(Stmt1, SQLFreeStmt(Stmt1, SQL_DROP));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_insert_fetched_null");

  return OK;
}


ODBC_TEST(odbc45)
{
  SQLSMALLINT i;
  SQLLEN      len= 0;
  SQLCHAR     value;
  SQLCHAR     val[][4]=        {"0",            "1"};//, "4", "-1", "0.5", "z"},
  SQLWCHAR    valw[][4]=       { { '0', '\0' }, { '1', '\0' }, { '4', '\0' }, { '-', '1', '\0' }, { '0', '.', '5', '\0' }, { 'z', '\0' } };
  SQLRETURN   XpctdRc[]=       {SQL_SUCCESS,    SQL_SUCCESS, SQL_ERROR, SQL_ERROR, SQL_ERROR, SQL_ERROR};
  SQLCHAR     XpctdState[][6]= { "",            "",          "22003",   "22003",   "22001",   "22018" };
  SQLCHAR     XpctdValue[]=    { 0,             1 };// , 0, 1};

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc45");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE odbc45 (id int not null primary key auto_increment, val bit(1) not null)");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "INSERT INTO odbc45(val) VALUES(?)", SQL_NTS)); 
  for (i = 0; i<sizeof(val)/sizeof(val[0]); ++i)
  {
    diag("SQLCHAR binding #%d(%s)", i + 1, val[i]);
    CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_BIT, 0, 0, val[i], sizeof(val[0]), &len));
    EXPECT_STMT(Stmt, SQLExecute(Stmt), XpctdRc[i]);
    if (XpctdRc[i] != SQL_SUCCESS)
    {
      CHECK_SQLSTATE(Stmt, XpctdState[i]);
    }
  }

  for (i = 0; i < 0*sizeof(valw)/sizeof(valw[0]); ++i)
  {
    diag("SQLWCHAR binding #%d(%s)", i + 1, val[i]);
    CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_BIT, 0, 0, valw[i], sizeof(valw[0]), &len));
    EXPECT_STMT(Stmt, SQLExecute(Stmt), XpctdRc[i]);
    if (XpctdRc[i]!=SQL_SUCCESS)
    {
      CHECK_SQLSTATE(Stmt, XpctdState[i]);
    }
  }

  OK_SIMPLE_STMT(Stmt, "SELECT val FROM odbc45");

  for (i= 0; i<sizeof(XpctdValue); ++i)
  {
    CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
    SQLGetData(Stmt, 1, SQL_C_BIT, &value, sizeof(value), 0);
    is_num(value, XpctdValue[i]);
  }

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE odbc45");

  return OK;
}


ODBC_TEST(odbc151)
{
  SQLINTEGER  Val;
  SQLLEN      Len= 2, OctetLength= 0;
  SQLHANDLE   Apd;

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "SELECT ?", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_PARAM_DESC, &Apd, SQL_IS_POINTER, NULL));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &Val, Len, &Len));
  CHECK_DESC_RC(Apd, SQLGetDescField(Apd, 1, SQL_DESC_OCTET_LENGTH,   &OctetLength, SQL_IS_INTEGER, NULL));
  is_num(OctetLength, sizeof(SQLINTEGER));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &Val, Len, NULL));
  CHECK_DESC_RC(Apd, SQLGetDescField(Apd, 1, SQL_DESC_OCTET_LENGTH, &OctetLength, SQL_IS_INTEGER, NULL));
  is_num(OctetLength, sizeof(SQLINTEGER));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &Val, 0, &Len));
  CHECK_DESC_RC(Apd, SQLGetDescField(Apd, 1, SQL_DESC_OCTET_LENGTH, &OctetLength, SQL_IS_INTEGER, NULL));
  is_num(OctetLength, sizeof(SQLINTEGER));
 
  return OK;
}


ODBC_TEST(odbc182)
{
  char buffer[128];
  SQL_TIMESTAMP_STRUCT ts= {0/*year*/, 0, 3, 12/*hour*/, 34, 56, 777/*fractional*/};

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc182");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc182(col1 time)");

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIMESTAMP, SQL_TIME, 8, 3, &ts, 0, NULL));
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, "INSERT INTO t_odbc182 VALUES(?)", SQL_NTS), SQL_ERROR);
  CHECK_SQLSTATE(Stmt, "22008");

  ts.fraction= 0;
  ts.hour= 24;
  EXPECT_STMT(Stmt, SQLExecDirect(Stmt, "INSERT INTO t_odbc182 VALUES(?)", SQL_NTS), SQL_ERROR);
  CHECK_SQLSTATE(Stmt, "22007");

  ts.hour= 12;
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc182 VALUES(?)");

  OK_SIMPLE_STMT(Stmt, "SELECT col1 FROM t_odbc182");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buffer, 1), "12:34:56", 8);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc182");

  return OK;
}


ODBC_TEST(odbc212)
{
  SQLSMALLINT a;
  SQLHDESC Ipd;
#pragma warning(disable: 4996)
#pragma warning(push)
  CHECK_STMT_RC(Stmt, SQLBindParam(Stmt, 1, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &a, NULL));
#pragma warning(pop)
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_IMP_PARAM_DESC, &Ipd, SQL_IS_POINTER, NULL));
  CHECK_DESC_RC(Ipd, SQLGetDescField(Ipd, 1, SQL_DESC_PARAMETER_TYPE, &a, SQL_IS_SMALLINT, NULL));

  is_num(a, SQL_PARAM_INPUT);
  return OK;
}


ODBC_TEST(timestruct_param)
{
  SQL_TIMESTAMP_STRUCT ts= { 2020/*year*/, 4, 7, 1/*hour*/, 28, 56, 0/*fractional*/ };
  SQL_TIME_STRUCT tp= {15, 58, 33}, tr;

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIME, SQL_TYPE_TIME, 8, 0, &tp, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_TIME, SQL_TYPE_TIME, 8, 0, &tp, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_TIME, SQL_TYPE_TIME, 8, 0, &tp, 0, NULL));
  OK_SIMPLE_STMT(Stmt, "SELECT ?, CAST('15:58:33' AS TIME) = ?, {t '15:58:33'} = ?");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_TIME, &tr, sizeof(SQL_TIME_STRUCT), NULL));
  is_num(tr.hour, 15);
  is_num(tr.minute, 58);
  is_num(tr.second, 33);
  is_num(my_fetch_int(Stmt, 2), 1);
  is_num(my_fetch_int(Stmt, 3), 1);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIMESTAMP, SQL_TYPE_TIMESTAMP, 20, 0, &ts, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_TIME, SQL_TYPE_TIME, 8, 0, &tp, 0, NULL));

  OK_SIMPLE_STMT(Stmt, "SELECT 1 FROM DUAL WHERE '2020-04-07 01:28:56'=? AND CAST('15:58:33' AS TIME) = ?");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


/* Test of subsequent direct execution calls with decreasing number of parameters */
ODBC_TEST(consequent_direxec)
{
  SQLINTEGER p1= 1, p2= 2, p3= 3;

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &p1, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &p2, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &p3, 0, NULL));

  OK_SIMPLE_STMT(Stmt, "SELECT ?, ?, ?");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(my_fetch_int(Stmt, 1), 1);
  is_num(my_fetch_int(Stmt, 2), 2);
  is_num(my_fetch_int(Stmt, 3), 3);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_RESET_PARAMS));

  p2= 7;
  p3= 5;
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &p2, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &p3, 0, NULL));
  OK_SIMPLE_STMT(Stmt, "SELECT ?, ?");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(my_fetch_int(Stmt, 1), 7);
  is_num(my_fetch_int(Stmt, 2), 5);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


ODBC_TEST(odbc279)
{
  char buffer[128];
  SQL_TIME_STRUCT ts= { 12/*hour*/, 34, 56 };

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_odbc279");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_odbc279(col1 time)");

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_TIME, SQL_TIME, 8, 0, &ts, 0, NULL));
  
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_odbc279 VALUES(?)");

  OK_SIMPLE_STMT(Stmt, "SELECT col1 FROM t_odbc279");
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  IS_STR(my_fetch_str(Stmt, buffer, 1), "12:34:56", 8);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "SELECT col1 FROM t_odbc279");
  CHECK_STMT_RC(Stmt, SQLFetchScroll(Stmt, SQL_FETCH_NEXT, 1));
 
  ts.hour= ts.minute= ts.second= 0;
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_TIME, &ts, sizeof(SQL_TIME_STRUCT), NULL));

  is_num(ts.hour, 12);
  is_num(ts.minute, 34);
  is_num(ts.second, 56);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));
  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_odbc279");

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
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
  {insert_fetched_null, "insert_fetched_null"},
  {odbc45, "odbc-45-binding2bit"},
  {odbc151, "odbc-151-buffer_length"},
  {odbc182, "odbc-182-timestamp2time"},
  {odbc212, "odbc-212-sqlbindparam_inout_type"},
  {timestruct_param, "timestruct_param-seconds"},
  {consequent_direxec, "consequent_direxec"},
  {odbc279, "odbc-279-timestruct"},
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
