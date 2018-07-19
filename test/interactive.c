/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2016 MariaDB Corporation AB

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

HWND hWnd;

/* Test of NO_PROMPT option. Normally it is not interactive. Dialog appearance means test failure */
ODBC_TEST(ti_bug30840)
{
  HDBC        hdbc1;
  SQLCHAR     conn[512], conn_out[1024];
  SQLSMALLINT conn_out_len;

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;NO_PROMPT=1",
          my_dsn, "wronguid", "wrongpwd"/*my_uid, my_pwd*/);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  /* NO_PROMPT is supposed to supress dialog invocation, and connect should fail */
  EXPECT_DBC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, (SQLSMALLINT)strlen(conn),
                                 conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_COMPLETE_REQUIRED), SQL_ERROR);
  CHECK_SQLSTATE_EX(hdbc1, SQL_HANDLE_DBC, "28000");

  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


ODBC_TEST(ti_dialogs)
{
  HDBC        hdbc1;
  SQLWCHAR    *connw, connw_out[1024];
  SQLSMALLINT conn_out_len;
  SQLCHAR     conna[512], conna_out[1024];

  /* Testing how driver's doing if no out string given. ODBC-17 */
  sprintf((char*)conna, "DRIVER=%s;TCPIP=1;SERVER=%s%s", my_drivername, my_servername, ma_strport);
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, hWnd, conna, SQL_NTS, NULL,
                                       0, &conn_out_len, SQL_DRIVER_COMPLETE));

  FAIL_IF((size_t)conn_out_len <= strlen(conna), "OutString length is too short");

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, hWnd, conna, SQL_NTS, conna_out,
                                 sizeof(conna_out), &conn_out_len, SQL_DRIVER_PROMPT));

  diag("In %d OutString %s(%d)", strlen(conna), hide_pwd(conna_out), conn_out_len);
  /* We can't say much about the out string length, but it supposed to be bigger, than of the in string */
  FAIL_IF((size_t)conn_out_len <= strlen(conna), "OutString length is too short");

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, hWnd, conna, SQL_NTS, conna_out,
                                 sizeof(conna_out), &conn_out_len, SQL_DRIVER_COMPLETE));

  diag("In %d OutString %s(%d)", strlen(conna), hide_pwd(conna_out), conn_out_len);
  FAIL_IF((size_t)conn_out_len <= strlen(conna), "OutString length is too short");

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  /* Doing the the same - SQL_DRIVER_COMPLETE(_REQUIRED) and SQL_DRIVER_PROMPT, but with W function */
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  sprintf((char*)conna, "DSN=%s;UID=wronguser;PWD=wrongpwd;", my_dsn);
  connw= CW(conna);

  CHECK_DBC_RC(hdbc1, SQLDriverConnectW(hdbc1, hWnd, connw, SQL_NTS, connw_out,
                                        sizeof(connw_out), &conn_out_len,
                                        SQL_DRIVER_COMPLETE_REQUIRED));
  /* If DSN has all required info - we should be fine */
  FAIL_IF((size_t)conn_out_len <= strlen(conna), "OutString length is too short");

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnectW(hdbc1, hWnd, connw, SQL_NTS, connw_out,
                                        sizeof(connw_out), &conn_out_len,
                                        SQL_DRIVER_COMPLETE));
  /* If DSN has all required info - we should be fine */
  FAIL_IF((size_t)conn_out_len <= strlen(conna), "OutString length is too short");
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnectW(hdbc1, hWnd, connw, SQL_NTS, connw_out,
                                        sizeof(connw_out), &conn_out_len,
                                        SQL_DRIVER_PROMPT));
  /* If DSN has all required info - we should be fine */
  FAIL_IF((size_t)conn_out_len <= strlen(conna), "OutString length is too short");

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


ODBC_TEST(t_odbc161)
{
  HDBC        hdbc1;
  SQLCHAR     conn[512], conn_out[1024];
  SQLSMALLINT conn_out_len;

  sprintf((char *)conn, "SAVEFILE=odbc161;DRIVER=%s;UID=%s;PWD=%s;SERVER=%s;PORT=%u;DB=%s",
    my_drivername, my_uid, my_pwd, my_servername, my_port, my_schema);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, (SQLSMALLINT)strlen(conn),
                                       conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
                                       SQL_DRIVER_COMPLETE_REQUIRED));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  sprintf((char *)conn, "FILEDSN=odbc161;PWD=%s", my_pwd);

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, (SQLSMALLINT)strlen(conn),
                                       conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
                                       SQL_DRIVER_COMPLETE_REQUIRED));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}

#ifdef _WIN32
#  define WE_HAVE_SETUPLIB
#endif

MA_ODBC_TESTS my_tests[]=
{
#ifdef WE_HAVE_SETUPLIB
  {ti_bug30840, "bug30840_interactive", NORMAL},
  {ti_dialogs,  "ti_dialogs",           NORMAL},
  {t_odbc161,  "t_odbc161_file_dsn",           NORMAL},
#endif
  {NULL, NULL, 0}
};

int main(int argc, char **argv)
{
  int   tests=    sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  DWORD dwProcID= GetCurrentProcessId();

  hWnd= GetConsoleWindow();
  if (hWnd == NULL)
  {
    hWnd= GetTopWindow(GetDesktopWindow());
    while(hWnd)
    {
      DWORD dwWndProcID = 0;
      GetWindowThreadProcessId(hWnd, &dwWndProcID);
      if(dwWndProcID == dwProcID)
        break;            
      hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
  }

  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
