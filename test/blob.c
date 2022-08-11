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

ODBC_TEST(t_blob)
{
    SQLRETURN rc;
    SQLUINTEGER j= 0;
    SQLINTEGER l;
    SQLLEN cbValue;
    SQLCHAR *blobbuf;
    SQLULEN blobbuf_size = 1024 * 1 * 6L;
    SQLULEN blob_read;
    SQLPOINTER token;
    clock_t start, finish;
    double duration;
    SQLUINTEGER blob_size = 1 * 1024L * 5L;

    //rc = SQLSetConnectOption(Connection, SQL_AUTOCOMMIT, 0L);
    //CHECK_HANDLE_RC(Connection, SQL_HANDLE_DBC, rc);

    OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS TBLOB");
    OK_SIMPLE_STMT(Stmt, "CREATE TABLE TBLOB (I INTEGER NOT NULL PRIMARY KEY,"
           "B LONGBLOB)");

    cbValue = 0;
    CHECK_STMT_RC(Stmt,  SQLPrepare(Stmt,
                              (SQLCHAR *)"INSERT INTO TBLOB VALUES (1, ?)",
                              SQL_NTS));
    CHECK_STMT_RC(Stmt,  SQLBindParameter(Stmt, SQL_PARAM_INPUT, 1, SQL_C_BINARY,
                                    SQL_LONGVARBINARY, blob_size, 0, NULL,
                                    0, &cbValue));
    cbValue = SQL_DATA_AT_EXEC;
    blobbuf = (SQLCHAR *)malloc(blobbuf_size);
    memset(blobbuf, 'A', blobbuf_size);

    start = clock();

    FAIL_IF(SQLExecute(Stmt) !=  SQL_NEED_DATA, "SQL_NEED_DATA expected");

    FAIL_IF(SQLParamData(Stmt, &token) != SQL_NEED_DATA, "SQL_NEED_DATA expected");
    {
        for (j = 0; j < blob_size; )
        {
            SDWORD s;

            s = (SDWORD)blobbuf_size;
            if (s + j > blob_size)
            {
                s -= (s + j) - blob_size;
                FAIL_IF(s + j != blob_size, "wrong size");
            }
            rc = SQLPutData(Stmt, blobbuf, s);
            CHECK_STMT_RC(Stmt, rc);
            j += (SQLUINTEGER)s;
        }
        rc = SQLParamData(Stmt, &token);
        CHECK_STMT_RC(Stmt, rc);
    }
    finish = clock();

    duration = (finish-start)/CLOCKS_PER_SEC;
    diag("j: %d", j);
    FAIL_IF(j != blob_size, "assertion");
    diag("Wrote %ld bytes in %3.3lf seconds (%lg bytes/s)",
                 j, duration, duration == 0.0 ? 9.99e99 : j / duration);

    rc = SQLTransact(NULL, Connection, SQL_COMMIT);
    CHECK_DBC_RC(Connection, rc);

    rc = SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    CHECK_STMT_RC(Stmt, rc);

    memset(blobbuf, ~0, 100);
    CHECK_STMT_RC(Stmt, SQLPrepare(Stmt,
                              (SQLCHAR *)"SELECT I, B FROM TBLOB WHERE I = 1",
                              SQL_NTS));

    start = clock();

    rc = SQLExecute(Stmt);
    CHECK_STMT_RC(Stmt, rc);
    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt, rc);
    rc = SQLGetData(Stmt, 1, SQL_C_LONG, &l, 0L, &cbValue);
    CHECK_STMT_RC(Stmt, rc);

    blob_read = 0L;
    do
    {
        rc = SQLGetData(Stmt, 2, SQL_C_BINARY, blobbuf, blobbuf_size, &cbValue);
        FAIL_IF(cbValue <= 0, "assert");
        blob_read += ((SQLUINTEGER)cbValue < blobbuf_size ? cbValue : blobbuf_size);
    } while (rc == SQL_SUCCESS_WITH_INFO);
    FAIL_IF(rc != SQL_SUCCESS, "assert");
    FAIL_IF(blob_read != blob_size, "assert");
    finish = clock();
    duration = (finish-start)/CLOCKS_PER_SEC;
    diag("Read  %ld bytes in %3.3lf seconds (%lg bytes/s)",
                 blob_read, duration, duration == 0.0 ? 9.99e99 :
                 blob_read / duration);

    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt, rc);
    free(blobbuf);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS TBLOB");

  return OK;
}

ODBC_TEST(t_1piecewrite2)
{
    SQLRETURN rc;
    SQLLEN cbValue,cbValue2;
    SQLINTEGER l;
    SQLCHAR* blobbuf;
    size_t i;

    OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS TBLOB");
    OK_SIMPLE_STMT(Stmt, "CREATE TABLE TBLOB (I INTEGER NOT NULL PRIMARY KEY,"
          "B LONG VARCHAR NOT NULL)");

    cbValue = 3510L;

    blobbuf = (SQLCHAR *)malloc((size_t)cbValue + 1);
    for (i = 0; i < (size_t)cbValue; i++)
    {
        blobbuf[i] = (char)((i % ('z' - 'a' + 1)) + 'a');
    }
    blobbuf[i] = '\0';
    l = 1;
    rc = SQLBindParameter(Stmt,SQL_PARAM_INPUT,1, SQL_C_LONG, SQL_INTEGER, 0, 0, &l,0, NULL);
    CHECK_STMT_RC(Stmt, rc);
    rc = SQLBindParameter(Stmt,SQL_PARAM_INPUT, 2, SQL_C_CHAR, SQL_LONGVARCHAR, 0, 0, blobbuf,cbValue, NULL);
    CHECK_STMT_RC(Stmt, rc);
    OK_SIMPLE_STMT(Stmt, "INSERT INTO TBLOB VALUES (1,?)");
    CHECK_STMT_RC(Stmt, rc);
    rc = SQLTransact(NULL, Connection, SQL_COMMIT);
    CHECK_DBC_RC(Connection, rc);
    memset(blobbuf, 1, (size_t)cbValue);
    rc = SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    CHECK_STMT_RC(Stmt, rc);
    OK_SIMPLE_STMT(Stmt, "SELECT B FROM TBLOB WHERE I = 1");
    CHECK_STMT_RC(Stmt, rc);
    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt, rc);
    rc = SQLGetData(Stmt, 1, SQL_C_BINARY, blobbuf, cbValue, &cbValue2);
    CHECK_STMT_RC(Stmt, rc);
    FAIL_IF(cbValue2 != cbValue, "assert");
    for (i = 0; i < (size_t)cbValue; i++)
    {
        FAIL_IF(blobbuf[i] != (char)((i % ('z' - 'a' + 1)) + 'a'), "assert");
    }
    rc = SQLFreeStmt(Stmt, SQL_CLOSE);
    CHECK_STMT_RC(Stmt, rc);
    rc = SQLTransact(NULL, Connection, SQL_COMMIT);
    CHECK_DBC_RC(Connection, rc);
    free(blobbuf);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS TBLOB");

  return OK;
}


/* Test for a simple SQLPutData and SQLParamData handling for longtext */
ODBC_TEST(t_putdata)
{
  SQLRETURN  rc;
  SQLLEN     pcbLength;
  SQLINTEGER c1;
  SQLCHAR    data[255];
  SQLPOINTER token;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_putdata");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_putdata (c1 INT, c2 LONG VARCHAR)");

  CHECK_STMT_RC(Stmt,  SQLPrepare(Stmt,
                            (SQLCHAR *)"insert into t_putdata values(?,?)",
                            SQL_NTS));

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_LONG,
                          SQL_INTEGER,0,0,&c1,0,NULL);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT,SQL_C_CHAR,
                          SQL_LONGVARCHAR,0,0,
                          (SQLPOINTER)1,0,&pcbLength);

    pcbLength =  SQL_LEN_DATA_AT_EXEC(0);

    c1 = 10;
    rc = SQLExecute(Stmt);
    FAIL_IF(rc != SQL_NEED_DATA, "assert");

    rc = SQLParamData(Stmt, &token);
    FAIL_IF(rc != SQL_NEED_DATA, "assert");

    strcpy((char *)data,"mysql ab");
    rc = SQLPutData(Stmt,data,6);
    CHECK_STMT_RC(Stmt, rc);

    strcpy((char *)data,"- the open source database company");
    rc = SQLPutData(Stmt,data,strlen((char *)data));
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLParamData(Stmt, &token);
    CHECK_STMT_RC(Stmt, rc);

    SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "select c2 from t_putdata where c1= 10");
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt, rc);

    pcbLength= 0;
    rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &pcbLength);
    CHECK_STMT_RC(Stmt, rc);
    diag("data: %s(%ld)", data, pcbLength);
    IS_STR(data, "mysql - the open source database company", 40);
    FAIL_IF(pcbLength != 40, "assert");

    SQLFreeStmt(Stmt, SQL_UNBIND);
    SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_putdata");

  return OK;
}


/* Test for a simple SQLPutData and SQLParamData handling for longtext */
ODBC_TEST(t_putdata1)
{
  SQLRETURN  rc;
  SQLLEN     pcbLength;
  SQLINTEGER c1;
  SQLCHAR    data[255];
  SQLPOINTER token;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_putdata");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_putdata (c1 INT, c2 LONG VARCHAR)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_putdata VALUES (10,'venu')");

  CHECK_STMT_RC(Stmt, 
          SQLPrepare(Stmt,
                     (SQLCHAR *)"UPDATE t_putdata SET c2= ? WHERE c1 = ?",
                     SQL_NTS));

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,
                          SQL_LONGVARCHAR,0,0,
                          (SQLPOINTER)1,0,&pcbLength);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT,SQL_C_LONG,
                          SQL_INTEGER,0,0,&c1,0,NULL);

    pcbLength =  SQL_LEN_DATA_AT_EXEC(0);

    c1 = 10;
    rc = SQLExecute(Stmt);
    FAIL_IF(rc != SQL_NEED_DATA, "assert");

    rc = SQLParamData(Stmt, &token);
    FAIL_IF(rc != SQL_NEED_DATA, "assert");

    strcpy((char *)data,"mysql ab");
    rc = SQLPutData(Stmt,data,6);
    CHECK_STMT_RC(Stmt, rc);

    strcpy((char *)data,"- the open source database company");
    rc = SQLPutData(Stmt,data,strlen((char *)data));
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLParamData(Stmt, &token);
    CHECK_STMT_RC(Stmt, rc);

    SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "select c2 from t_putdata where c1= 10");
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt, rc);

    pcbLength= 0;
    rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &pcbLength);
    CHECK_STMT_RC(Stmt, rc);
    diag("data: %s(%ld)", data, pcbLength);
    IS_STR(data,"mysql - the open source database company", 40);
    FAIL_IF(pcbLength != 40, "assert");

    SQLFreeStmt(Stmt, SQL_UNBIND);
    SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_putdata");

  return OK;
}


/* Test for a simple SQLPutData and SQLParamData handling for longtext */
ODBC_TEST(t_putdata2)
{
  SQLRETURN  rc;
  SQLLEN     pcbLength;
  SQLINTEGER c1;
  SQLCHAR    data[255];
  SQLPOINTER token;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_putdata");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_putdata (c1 INT, c2 LONG VARCHAR,"
        "c3 LONG VARCHAR)");

  CHECK_STMT_RC(Stmt,  SQLPrepare(Stmt,
                            (SQLCHAR *)"insert into t_putdata values(?,?,?)",
                            SQL_NTS));

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_LONG,
                          SQL_INTEGER,0,0,&c1,0,NULL);

    rc = SQLBindParameter(Stmt,2,SQL_PARAM_INPUT,SQL_C_CHAR,
                          SQL_LONGVARCHAR,0,0,
                          (SQLPOINTER)1,0,&pcbLength);

    rc = SQLBindParameter(Stmt,3,SQL_PARAM_INPUT,SQL_C_CHAR,
                          SQL_LONGVARCHAR,0,0,
                          (SQLPOINTER)1,0,&pcbLength);

    pcbLength =  SQL_LEN_DATA_AT_EXEC(0);

    c1 = 10;
    rc = SQLExecute(Stmt);
    FAIL_IF(rc != SQL_NEED_DATA, "assert");

    rc = SQLParamData(Stmt, &token);
    FAIL_IF(rc != SQL_NEED_DATA, "assert");

    strcpy((char *)data,"mysql ab");
    rc = SQLPutData(Stmt,data,6);
    CHECK_STMT_RC(Stmt, rc);

    strcpy((char *)data,"- the open source database company");
    rc = SQLPutData(Stmt,data,strlen((char *)data));
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLParamData(Stmt, &token);
    FAIL_IF(rc != SQL_NEED_DATA, "assert");

    strcpy((char *)data,"MySQL AB");
    rc = SQLPutData(Stmt,data, 8);
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLParamData(Stmt, &token);
    CHECK_STMT_RC(Stmt, rc);

    SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt, SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "select c2,c3 from t_putdata where c1= 10");
    CHECK_STMT_RC(Stmt, rc);

    rc = SQLFetch(Stmt);
    CHECK_STMT_RC(Stmt, rc);

    pcbLength= 0;
    rc = SQLGetData(Stmt, 1, SQL_C_CHAR, data, sizeof(data), &pcbLength);
    CHECK_STMT_RC(Stmt, rc);
    diag("data: %s(%ld)", data, pcbLength);
    IS_STR(data, "mysql - the open source database company", 40);
    FAIL_IF(pcbLength != 40, "assert");

    pcbLength= 0;
    rc = SQLGetData(Stmt, 2, SQL_C_CHAR, data, sizeof(data), &pcbLength);
    CHECK_STMT_RC(Stmt, rc);
    diag("data: %s(%ld)", data, pcbLength);
    IS_STR(data, "MySQL AB", 8);
    FAIL_IF(pcbLength != 8, "assert");

    SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt, SQL_UNBIND);
    SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_putdata");

  return OK;
}


/* Test for a simple SQLPutData and SQLParamData handling bug #1316 */
ODBC_TEST(t_putdata3)
{
  SQLRETURN   rc;
  SQLINTEGER  id, id1, id2, id3;
  SQLLEN      resId, resUTimeSec, resUTimeMSec, resDataLen, resData;

  SQLCHAR buffer[]= "MySQL - The worlds's most popular open source database";
  const int MAX_PART_SIZE = 5;

  SQLCHAR data[50];
  int commonLen= 20;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_putdata3");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE t_putdata3 (id INT, id1 INT, id2 INT, id3 INT, b BLOB)");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)
                            "INSERT INTO t_putdata3 VALUES (?, ?, ?, ?, ?)",
                            SQL_NTS));

  id= 1, id1= 2, id2= 3, id3= 4;
  resId= 0;
  resUTimeSec= resUTimeMSec= 0;
  resDataLen= 0;
  resData= SQL_LEN_DATA_AT_EXEC(0);

  CHECK_STMT_RC(Stmt,  SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG,
                                  SQL_INTEGER, 0, 0, &id, 0, &resId));

  CHECK_STMT_RC(Stmt,  SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG,
                                  SQL_INTEGER, 0, 0, &id1, 0, &resUTimeSec));

  CHECK_STMT_RC(Stmt,  SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG,
                                  SQL_INTEGER, 0, 0, &id2, 0, &resUTimeMSec));

  CHECK_STMT_RC(Stmt,  SQLBindParameter(Stmt, 4, SQL_PARAM_INPUT, SQL_C_SLONG,
                                  SQL_INTEGER, 0, 0, &id3, 0, &resDataLen));

  CHECK_STMT_RC(Stmt,  SQLBindParameter(Stmt, 5, SQL_PARAM_INPUT, SQL_C_BINARY,
                                  SQL_LONGVARBINARY, 10, 10, (SQLPOINTER)5,
                                  0, &resData));

  rc= SQLExecute(Stmt);
  if (rc == SQL_NEED_DATA)
  {
    SQLPOINTER parameter;

    if (SQLParamData(Stmt, &parameter) == SQL_NEED_DATA &&
        parameter == (SQLPOINTER)5)
    {
      int len= 0, partsize;

      /* storing long data by parts */
      while (len < commonLen)
      {
        partsize= commonLen - len;
        if (partsize > MAX_PART_SIZE)
          partsize= MAX_PART_SIZE;

        CHECK_STMT_RC(Stmt,  SQLPutData(Stmt, buffer + len, partsize));
        len+= partsize;
      }

      if (SQLParamData(Stmt, &parameter) == SQL_ERROR)
      {
        return FAIL;
      }
    }
  } /* end if (rc == SQL_NEED_DATA) */

  CHECK_STMT_RC(Stmt,  SQLFreeStmt(Stmt, SQL_UNBIND));
  CHECK_STMT_RC(Stmt,  SQLFreeStmt(Stmt, SQL_CLOSE));

  if (1)
  {
    OK_SIMPLE_STMT(Stmt, "SELECT id, id1, id2, id3, CONVERT(b, CHAR) FROM t_putdata3");

    CHECK_STMT_RC(Stmt,  SQLFetch(Stmt));

    is_num(my_fetch_int(Stmt, 1), 1);
    is_num(my_fetch_int(Stmt, 2), 2);
    is_num(my_fetch_int(Stmt, 3), 3);
    is_num(my_fetch_int(Stmt, 4), 4);

    IS_STR(my_fetch_str(Stmt, data, 5), buffer, commonLen);
  }
  else
  {
    OK_SIMPLE_STMT(Stmt, "SELECT id, id1, id2, id3, b FROM t_putdata3");

    CHECK_STMT_RC(Stmt,  SQLFetch(Stmt));

    is_num(my_fetch_int(Stmt, 1), 1);
    is_num(my_fetch_int(Stmt, 2), 2);
    is_num(my_fetch_int(Stmt, 3), 3);
    is_num(my_fetch_int(Stmt, 4), 4);

    IS_STR(my_fetch_str(Stmt, data, 5),
           "4D7953514C202D2054686520776F726C64732773", commonLen);
  }

  CHECK_STMT_RC(Stmt,  SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_putdata3");

  return OK;
}


/* Test the bug when blob size > 8k */
ODBC_TEST(t_blob_bug)
{
  SQLRETURN  rc;
  SQLCHAR    *data;
  SQLINTEGER i, val;
  SQLLEN     length;
  const SQLINTEGER max_blob_size=1024*100;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_blob");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_blob (blb LONG VARBINARY)");

  CHECK_STMT_RC(Stmt, 
          SQLPrepare(Stmt,
                     (SQLCHAR *)"INSERT INTO t_blob  VALUES (?)",SQL_NTS));

    if (!(data = (SQLCHAR *)calloc(max_blob_size,sizeof(SQLCHAR))))
    {
      SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
      SQLFreeStmt(Stmt,SQL_CLOSE);
      return FAIL;
    }

    rc = SQLBindParameter(Stmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARBINARY,
                          0,0,data,0,&length);
    CHECK_STMT_RC(Stmt, rc);

    memset(data,'X',max_blob_size);

    for (length=1024; length <= max_blob_size; length+= 1024)
    {
      diag("Length %d", length);
      rc = SQLExecute(Stmt);
      CHECK_STMT_RC(Stmt, rc);
    }

    SQLFreeStmt(Stmt,SQL_RESET_PARAMS);
    SQLFreeStmt(Stmt,SQL_CLOSE);

    OK_SIMPLE_STMT(Stmt, "SELECT length(blb) FROM t_blob");

    rc = SQLBindCol(Stmt,1,SQL_C_LONG,&val,0,NULL);
    CHECK_STMT_RC(Stmt, rc);

    for (i= 1; i <= max_blob_size/1024; i++)
    {
      rc = SQLFetch(Stmt);
      CHECK_STMT_RC(Stmt, rc);

      diag("row %d length: %d", i, val);
      FAIL_IF(val != i * 1024, "assert");
    }
    rc = SQLFetch(Stmt);
    FAIL_IF(rc != SQL_NO_DATA, "SQL_NO_DATA expected");

    free(data);

    SQLFreeStmt(Stmt,SQL_UNBIND);
    SQLFreeStmt(Stmt,SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_blob");

  return OK;
}


#define TEST_ODBC_TEXT_LEN 500
ODBC_TEST(t_text_fetch)
{
  SQLRETURN  rc;
  SQLINTEGER i;
  SQLLEN     row_count, length, ParamLength[]= {255, TEST_ODBC_TEXT_LEN*3, TEST_ODBC_TEXT_LEN*4, TEST_ODBC_TEXT_LEN*6-1};
  SQLCHAR    data[TEST_ODBC_TEXT_LEN*6+1];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_text_fetch");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_text_fetch(t1 tinytext,"
    "t2 text, t3 mediumtext, t4 longtext)");

  CHECK_STMT_RC(Stmt,
    SQLPrepare(Stmt,
    (SQLCHAR *)"insert into t_text_fetch values(?,?,?,?)",
      SQL_NTS));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char *)data, 255, iOdbc() ? &ParamLength[0] : NULL));

  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char *)data, TEST_ODBC_TEXT_LEN*3, iOdbc() ? &ParamLength[1] : NULL));
  
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char *)data, (SQLINTEGER)(TEST_ODBC_TEXT_LEN*4), iOdbc() ? &ParamLength[2] : NULL));
  
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char *)data, TEST_ODBC_TEXT_LEN*6-1, iOdbc() ? &ParamLength[3] : NULL));

  memset(data,'A',TEST_ODBC_TEXT_LEN*6);
  data[TEST_ODBC_TEXT_LEN*6]='\0';

  for (i=0; i < 10; i++)
  {
    rc = SQLExecute(Stmt);
    CHECK_STMT_RC(Stmt, rc);
  }

  SQLFreeStmt(Stmt, SQL_RESET_PARAMS);
  SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "SELECT * FROM t_text_fetch");

  row_count= 0;
  rc = SQLFetch(Stmt);
  while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
  {
      printf("# row '%lld' (lengths:", (long long)row_count);
      rc = SQLGetData(Stmt,1,SQL_C_CHAR,(char *)data,TEST_ODBC_TEXT_LEN*6,&length);
      CHECK_STMT_RC(Stmt, rc);
      printf("%lld", (long long)length);
      FAIL_IF(length != 255, "assert");

      rc = SQLGetData(Stmt,2,SQL_C_CHAR,(char *)data,TEST_ODBC_TEXT_LEN*6,&length);
      CHECK_STMT_RC(Stmt, rc);
      printf(",%lld", (long long)length);
      FAIL_IF(length != TEST_ODBC_TEXT_LEN*3, "assert");

      rc = SQLGetData(Stmt,3,SQL_C_CHAR,(char *)data,TEST_ODBC_TEXT_LEN*6,&length);
      CHECK_STMT_RC(Stmt, rc);
      printf(",%lld", (long long)length);
      FAIL_IF(length != (SQLINTEGER)(TEST_ODBC_TEXT_LEN*4), "assert");

      rc = SQLGetData(Stmt,4,SQL_C_CHAR,(char *)data,TEST_ODBC_TEXT_LEN*6,&length);
      CHECK_STMT_RC(Stmt, rc);
      printf(",%zd)\n", length);
      FAIL_IF(length != TEST_ODBC_TEXT_LEN*6-1, "assert");
      row_count++;

      rc = SQLFetch(Stmt);
  }
  diag("total rows: %ld", row_count);
  FAIL_IF(row_count != i, "assert");

  SQLFreeStmt(Stmt, SQL_UNBIND);
  SQLFreeStmt(Stmt, SQL_CLOSE);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE t_text_fetch");

  return OK;
}


/**
  Test retrieving the length of a field with a non-null zero-length buffer.
  This is how ADO does it for long-type fields.
*/
ODBC_TEST(getdata_lenonly)
{
  SQLLEN     len;
  SQLCHAR    buf[1];

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_getdata_lenonly");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_getdata_lenonly (a CHAR(4))");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_getdata_lenonly VALUES ('venu')");

  OK_SIMPLE_STMT(Stmt, "SELECT a FROM t_getdata_lenonly");
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  FAIL_IF(SQLGetData(Stmt, 1, SQL_C_CHAR, buf, 0, &len) != SQL_SUCCESS_WITH_INFO, "SQL_SUCCESS_WITH_INFO expected");
  is_num(len, 4);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_getdata_lenonly");

  return OK;
}


/**
  Bug #9781: returned SQL_Type on WKB query
*/
ODBC_TEST(t_bug9781)
{
  SQLSMALLINT name_length, data_type, decimal_digits, nullable;
  SQLCHAR column_name[SQL_MAX_COLUMN_NAME_LEN];
  SQLULEN column_size;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug9781");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug9781 (g GEOMETRY)");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug9781 VALUES (GeomFromText('POINT(0 0)'))");

  OK_SIMPLE_STMT(Stmt, "SELECT AsBinary(g) FROM t_bug9781");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLDescribeCol(Stmt, 1, column_name, sizeof(column_name),
                                &name_length, &data_type, &column_size,
                                &decimal_digits, &nullable));

  is_num(data_type, SQL_LONGVARBINARY);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug9781");
  return OK;
}


/*
 * Bug #10562 - Large blobs fail in a cursor
 */
ODBC_TEST(t_bug10562)
{
  SQLLEN bsize = 12 * 1024;
  /* Test to just insert 12k blob */
  SQLCHAR *blob = malloc(bsize);
  SQLCHAR *blobcheck = malloc(bsize);
  int result= OK;

  if (ForwardOnly == TRUE && NoCache == TRUE)
  {
    skip("The test cannot be run if FORWARDONLY and NOCACHE options are selected");
  }

  memset(blob, 'X', bsize);

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_bug10562");
  OK_SIMPLE_STMT(Stmt, "CREATE TABLE t_bug10562 ( id INT NOT NULL PRIMARY KEY DEFAULT 0, mb LONGBLOB )");
  OK_SIMPLE_STMT(Stmt, "INSERT INTO t_bug10562 (mb) VALUES ('zzzzzzzzzz')");

  OK_SIMPLE_STMT(Stmt, "SELECT id, mb FROM t_bug10562");
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLBindCol(Stmt, 2, SQL_C_BINARY, blob, bsize, &bsize));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLSetPos(Stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* Get the data back out to verify */
  OK_SIMPLE_STMT(Stmt, "SELECT mb FROM t_bug10562");
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 1, SQL_C_BINARY, blobcheck, bsize, NULL));
  if (memcmp(blob, blobcheck, bsize))
  {
    result= FAIL;
  }

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "drop table if exists t_bug10562");
  free(blob);
  free(blobcheck);
  return result;
}


/* 
  Bug#11746572: TEXT FIELDS WITH BINARY COLLATIONS 
  Test for text field with latin1_bin and latin1_swedish_ci collation
  Output of text column should contain same input value and not hexadecimal
  value of input.
*/
ODBC_TEST(t_bug_11746572)
{
  SQLCHAR     szData[MAX_ROW_DATA_LEN+1];
  SQLSMALLINT SqlType;
  SQLCHAR     ColName[MAX_NAME_LEN];

  skip("hex conversion not supported");

  OK_SIMPLE_STMT(Stmt, "DROP TABLE if exists bug_11746572");

  /* 
    create table 'bug_11746572' with blob column and text columns 
    with collation latin1_bin and latin1_swedish_ci.  
  */
  OK_SIMPLE_STMT(Stmt,"CREATE TABLE bug_11746572( blob_field BLOB ,"
    "  text_bin TEXT CHARACTER SET latin1 COLLATE latin1_bin,"
    "  text_def TEXT CHARACTER SET latin1 COLLATE latin1_swedish_ci)");

  OK_SIMPLE_STMT(Stmt, "insert into bug_11746572 "
          " set blob_field= 'blob', text_bin= 'text', "
          " text_def= 'text' ; ");

  OK_SIMPLE_STMT(Stmt, "SELECT * from bug_11746572");

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFetch(Stmt));

  /* 
    Verify inserted data is changed to hexadecimal value for blob field 
    and remains unchanged for text field for both binary and non-binary 
    collation.
  */
  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 1, SQL_C_CHAR, szData, MAX_ROW_DATA_LEN,NULL));
  IS_STR(szData, "626C6F62", 8);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 2, SQL_C_CHAR, szData, MAX_ROW_DATA_LEN,NULL));
  IS_STR(szData, "text", 4);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLGetData(Stmt, 3, SQL_C_CHAR, szData, MAX_ROW_DATA_LEN,NULL));
  IS_STR(szData, "text", 4);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLDescribeCol(Stmt, 1, ColName, MAX_NAME_LEN, 
                        NULL, &SqlType, NULL, NULL, NULL));
  is_num(SqlType, SQL_LONGVARBINARY);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLDescribeCol(Stmt, 2, ColName, MAX_NAME_LEN, 
                        NULL, &SqlType, NULL, NULL, NULL));
  is_num(SqlType, SQL_LONGVARCHAR);


  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLDescribeCol(Stmt, 3, ColName, MAX_NAME_LEN, 
                        NULL, &SqlType, NULL, NULL, NULL));
  is_num(SqlType, SQL_LONGVARCHAR);

  CHECK_HANDLE_RC(SQL_HANDLE_STMT, Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE bug_11746572");

  return OK;
}


ODBC_TEST(t_odbc_26)
{
  SQLLEN     valueLen;
  SQLWCHAR   buffer[]= {'b', 'b', 0};
  SQLCHAR    value[3];
  SQLPOINTER parameter;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug_odbc26");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE bug_odbc26 (id INT unsigned not null primary key auto_increment, value VARCHAR(300))");

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)
                            "INSERT INTO bug_odbc26(value) VALUES (?)",
                            SQL_NTS));

  valueLen= SQL_LEN_DATA_AT_EXEC(4);

  CHECK_STMT_RC(Stmt,  SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR,
                                  SQL_VARCHAR, 0, 0, (SQLPOINTER)1, 0, &valueLen));

  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_NEED_DATA);
  EXPECT_STMT(Stmt, SQLParamData(Stmt, &parameter), SQL_NEED_DATA);
  is_num(parameter, 1);

  CHECK_STMT_RC(Stmt, SQLPutData(Stmt, buffer, 2*sizeof(SQLWCHAR)));
  CHECK_STMT_RC(Stmt, SQLParamData(Stmt, &parameter));

  /* We return "N" for SQL_NEED_LONG_DATA_LEN, and this not gonna change. Thus SQL_LEN_DATA_AT_EXEC(0) and with any other parameter should work */
  valueLen= SQL_LEN_DATA_AT_EXEC(0);

  EXPECT_STMT(Stmt, SQLExecute(Stmt), SQL_NEED_DATA);

  EXPECT_STMT(Stmt, SQLParamData(Stmt, &parameter), SQL_NEED_DATA);
  is_num(parameter, 1);

  CHECK_STMT_RC(Stmt, SQLPutData(Stmt, buffer, 2*sizeof(SQLWCHAR)));
  CHECK_STMT_RC(Stmt, SQLParamData(Stmt, &parameter));

  OK_SIMPLE_STMT(Stmt, "SELECT value FROM bug_odbc26");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_CHAR, value, sizeof(value), &valueLen));
  is_num(valueLen, 2);
  IS_STR(value, "bb", 3);

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_CHAR, value, sizeof(value), &valueLen));
  is_num(valueLen, 2);
  IS_STR(value, "bb", 3);

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS bug_odbc26");

  return OK;
}

/* In fact that is the testcase for ODBC-47 */
ODBC_TEST(t_blob_reading_in_chunks)
{
  SQLLEN     valueLen;
  SQLCHAR    value[12];
  int        i= 0;

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS blob_reading");
  OK_SIMPLE_STMT(Stmt,
         "CREATE TABLE blob_reading (id INT unsigned not null primary key auto_increment, value mediumblob)");

  OK_SIMPLE_STMT(Stmt, "INSERT INTO blob_reading(value) VALUES (0x0102030405060708090a0b0c0d0e0f101112131415161718)");

  OK_SIMPLE_STMT(Stmt, "SELECT value FROM blob_reading");

  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  EXPECT_STMT(Stmt, SQLGetData(Stmt, 1, SQL_C_BINARY, value, sizeof(value), &valueLen), SQL_SUCCESS_WITH_INFO);
  CHECK_SQLSTATE(Stmt, "01004");

  is_num(valueLen, 24);
  
  for (;i < 12; ++i)
  {
    diag("#%d", i);
    is_num(value[i], i + 1);
  }

  CHECK_STMT_RC(Stmt, SQLGetData(Stmt, 1, SQL_C_BINARY, value, sizeof(value), &valueLen));
  is_num(valueLen, 12);
  for (;i < 24; ++i)
  {
    diag("#%d", i);
    is_num(value[i - 12], i + 1);
  }

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_NO_DATA);

  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS blob_reading");

  return OK;
}


/** ODBC-359 When calling SQLBindCol with a lengthPtr but no targetBuffer to get column data length
    the buffer got dereferenced without first checking if it is set.
 */
ODBC_TEST(odbc359)
{
  SQLCHAR* create_table= (SQLCHAR *)"CREATE TABLE `odbc359` (`id` bigint(20) NOT NULL, `msg` mediumtext);";
  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS odbc359");
  OK_SIMPLE_STMT(Stmt, create_table);

  // msg 'Hello there!' is 12 characters long
  OK_SIMPLE_STMT(Stmt, "INSERT INTO `odbc359` (`id`, `msg`) VALUES(1, 'Hello there!')");
  SQLWCHAR id[255];
  SQLLEN idLen, msgLen;

  OK_SIMPLE_STMTW(Stmt, CW("SELECT id, msg from odbc359"));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_WCHAR, id, sizeof(id), &idLen));

  // Binding lengthBuffer, but no targetBuffer, type must be SQL_C_WCHAR
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_WCHAR, NULL, 0, &msgLen));

  EXPECT_STMT(Stmt, SQLFetch(Stmt), SQL_SUCCESS);

  //Length of 'Hello there!'
  is_num(12 * sizeof(SQLWCHAR), msgLen);

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {t_blob, "t_blob"},
  {t_1piecewrite2, "t_1piecewrite2"},
  {t_putdata1, "t_putdata1"},
  {t_putdata2, "t_putdata2"},
  {t_putdata3, "t_putdata3"},
  {t_blob_bug, "t_blob_bug"},
  {t_text_fetch, "t_text_fetch"},
  {getdata_lenonly, "getdata_lenonly"},
  {t_bug9781, "t_bug9781"},
  {t_bug10562, "t_bug10562"},
  {t_bug_11746572, "t_bug_11746572"},
  {t_odbc_26, "t_odbc_26"},
  {t_blob_reading_in_chunks, "t_blob_reading_in_chunks"},
  { odbc359, "odbc359"},
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
