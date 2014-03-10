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

#include <Windows.h>
#include <stdlib.h>
#include <CommCtrl.h>
#include <tchar.h>
#include <windowsx.h>
#include <winuser.h>
#include <odbcinst.h>

typedef BOOL (*DSNDialog)(HWND hwndParent,
                                   WORD fRequest,
                                   LPCSTR lpszDriver,
                                   LPCSTR lpszAttributes);

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE h0, LPTSTR lpCmdLine, int nCmdShow)
{
  HMODULE hmod= NULL;
  FARPROC fproc;
  BOOL ret;
  DSNDialog DsnFunc= NULL;
  if ((hmod= LoadLibrary("maodbcs.dll")))
  {
    if (DsnFunc= (DSNDialog)GetProcAddress(hmod, "ConfigDSN"))
      ret= DsnFunc(0,ODBC_CONFIG_DSN, "MariaDB ODBC Driver", "DSN=test\0OPTIONS=2\0\0");
    FreeLibrary(hmod);
  }
  return 1;
}
