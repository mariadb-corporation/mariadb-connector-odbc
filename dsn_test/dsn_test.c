/************************************************************************************
   Copyright (C) 2013 SkySQL AB
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc., 
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <odbcinst.h>

typedef BOOL (*DSNDialog)(HWND hwndParent,
                                   WORD fRequest,
                                   LPCSTR lpszDriver,
                                   LPCSTR lpszAttributes);

int  main()
{
  HMODULE hmod= NULL;
  BOOL ret;
  DSNDialog DsnFunc= NULL;
  HWND hWnd;
  DWORD dwProcID= GetCurrentProcessId();

  hWnd= GetConsoleWindow();
  if (hWnd == NULL)
  {
    hWnd= GetTopWindow(GetDesktopWindow());
    while (hWnd)
    {
      DWORD dwWndProcID = 0;
      GetWindowThreadProcessId(hWnd, &dwWndProcID);
      if (dwWndProcID == dwProcID)
        break;
      hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
  }

  if ((hmod= LoadLibrary("maodbcs.dll")))
  {
    if (DsnFunc= (DSNDialog)GetProcAddress(hmod, "ConfigDSN"))
    {
      ret= DsnFunc(NULL, ODBC_ADD_DSN, getenv("TEST_DRIVER"), "DSN=dsn_test\0OPTIONS=2\0\0");
      printf("%s 1 Null hWnd and not enough info\n", ret ? "not ok" : "ok");

      printf("# The dialog is supposed to show up now - please complete info for connection\n");
      ret= DsnFunc(hWnd, ODBC_ADD_DSN, getenv("TEST_DRIVER"), "DSN=dsn_test\0OPTIONS=2\0\0");

      if (ret != FALSE)
      {
        printf("ok 2 hWnd and not enough info\n");
        ret= DsnFunc(NULL, ODBC_ADD_DSN, getenv("TEST_DRIVER"), "DSN=dsn_test\0OPTIONS=2\0\0");
        printf("%s 3 Null hWnd trying to add existing dsn \n", ret ? "not ok" : "ok");
        ret= DsnFunc(NULL, ODBC_CONFIG_DSN, getenv("TEST_DRIVER"), "DSN=dsn_test\0UID=garbage\0PWD=DoubleGarbage\0OPTIONS=2\0\0");
        printf("%s 4 Null hWnd trying to config with insufficient data \n", ret ? "not ok" : "ok");
        printf("# The dialog asking if you want to overwrite existing DSN is supposed to show up. Please say 'No'. Otherwise please cancel config dialog\n");
        ret= DsnFunc(hWnd, ODBC_ADD_DSN, getenv("TEST_DRIVER"), "DSN=dsn_test\0UID=garbage\0PWD=DoubleGarbage\0OPTIONS=2\0\0");
        printf("%s 5 Replace Prompt \n", ret ? "not ok" : "ok");
        ret= DsnFunc(NULL, ODBC_CONFIG_DSN, getenv("TEST_DRIVER"), "DSN=inexistent_dsn\0UID=garbage\0PWD=DoubleGarbage\0OPTIONS=2\0\0");
        printf("%s 6 Null hWnd trying to config inexisting DSN\n", ret ? "not ok" : "ok");
        ret= DsnFunc(NULL, ODBC_CONFIG_DSN, getenv("TEST_DRIVER"), "DSN=inexistent_dsn\0UID=garbage\0PWD=DoubleGarbage\0OPTIONS=2\0\0");
        printf("%s 7 hWnd trying to config inexisting DSN\n", ret ? "not ok" : "ok");
        ret= DsnFunc(NULL, ODBC_CONFIG_DSN, getenv("TEST_DRIVER"), "DSN=dsn_test\0OPTIONS=0\0\0");
        printf("%s 8 Null hWnd config with sufficient data \n", ret ? "ok" : "not ok");
        printf("# Please make sure that in dialog Named pipe is selected, and all options are un-checked\n");
        ret= DsnFunc(hWnd, ODBC_CONFIG_DSN, getenv("TEST_DRIVER"), "DSN=dsn_test\0NamedPipe=1\0\0");
        printf("%s 9 hWnd config with sufficient data \n", ret ? "ok" : "not ok");
      }
      else
      {
        printf("not ok 2 hWnd and not enough info\n");
        printf("skip 3\n");
        printf("skip 4\n");
        printf("skip 5\n");
        printf("skip 6\n");
        printf("skip 7\n");
        printf("skip 8\n");
        printf("skip 9\n");
      }

      ret= DsnFunc(hWnd, ODBC_REMOVE_DSN, getenv("TEST_DRIVER"), "DSN=dsn_test\0\0");
      printf("%s 10 removing dsn\n", ret ? "ok" : "not ok");
    }
    FreeLibrary(hmod);
  }

  return 1;
}
