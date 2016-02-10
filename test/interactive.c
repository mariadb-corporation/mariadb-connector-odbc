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

#ifdef _WIN32
#  define WE_HAVE_SETUPLIB
#endif

#ifdef WE_HAVE_SETUPLIB
/* Test of NO_PROMPT option. Normally it is not interactive. Dialog appearance means test failure */
ODBC_TEST(ti_bug30840)
{
  HDBC        hdbc1;
  SQLCHAR     conn[512], conn_out[1024];
  SQLSMALLINT conn_out_len;

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;NO_PROMPT=1",
          my_dsn, my_uid, my_pwd);

  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, hWnd, conn, (SQLSMALLINT)strlen(conn),
                                 conn_out, (SQLSMALLINT)sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_PROMPT));

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}
#endif


/*
   Test the output string after calling SQLDriverConnect
   Note: Windows 
   TODO fix this test create a comparable output string
*/
ODBC_TEST(ti_driverconnect_outstring)
{
  HDBC        hdbc1;
  SQLRETURN   rc;
  SQLWCHAR    *connw, connw_out[1024];
  SQLSMALLINT conn_out_len;
  /* This has to be changed to use actual DSN(and not the default one) */
  SQLCHAR     conna[512], conna_out[1024];

  /* Testing how driver's doing if no out string given. ODBC-17 */
  sprintf((char*)conna, "DSN=%s;UID=%s;PWD=%s;CHARSET=utf8", my_dsn, my_uid, my_pwd);
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, NULL, conna, SQL_NTS, NULL,
                                 0, &conn_out_len, SQL_DRIVER_NOPROMPT));
  diag("OutString Length: %d", conn_out_len);
  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLDriverConnect(hdbc1, hWnd, "DRIVER=maodbc", 0, conna_out,
                                 sizeof(conna_out), &conn_out_len, SQL_DRIVER_COMPLETE));
  diag("In %d OutString %s(%d)", strlen(conna), conna_out, conn_out_len);
  is_num(conn_out_len, strlen(conna));
  IS_STR(conna_out, conna, strlen(conna) + 1);

  CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));

  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  /* This part of test has to be changed to compare in and out strings */
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  /* This part of test has to be changed to compare in and out strings */
  CHECK_ENV_RC(Env, SQLAllocHandle(SQL_HANDLE_DBC, Env, &hdbc1));

  connw= CW(conna);
  rc= SQLDriverConnectW(hdbc1, NULL, connw, SQL_NTS, connw_out,
                                 sizeof(connw_out), &conn_out_len,
                                 SQL_DRIVER_COMPLETE);
  if (SQL_SUCCEEDED(rc))
  {
    is_num(conn_out_len, strlen(conna));
    IS_WSTR(connw_out, connw, strlen(conna) + 1);
    CHECK_DBC_RC(hdbc1, SQLDisconnect(hdbc1));
  }

  CHECK_DBC_RC(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));
  
  return OK;
}



MA_ODBC_TESTS my_tests[]=
{
#ifdef WE_HAVE_SETUPLIB
  {ti_bug30840,                "bug30840_interactive",     NORMAL},
#endif
  {ti_driverconnect_outstring, "drvconnect_outstr_interactive", TO_FIX},
  {NULL, NULL, 0}
};

int main(int argc, char **argv)
{
  int   tests=    sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  DWORD dwProcID= GetCurrentProcessId();

  hWnd = GetTopWindow(GetDesktopWindow());
  while(hWnd)
  {
    DWORD dwWndProcID = 0;
    GetWindowThreadProcessId(hWnd, &dwWndProcID);
    if(dwWndProcID == dwProcID)
      break;            
    hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
  }

  get_options(argc, argv);
  plan(tests);
  return run_tests(my_tests);
}
