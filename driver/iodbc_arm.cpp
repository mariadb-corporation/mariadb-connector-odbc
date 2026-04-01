/************************************************************************************
   Copyright (C) 2026 MariaDB Corporation plc
   
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

// This is alternative definition for the iOdbc that calls driver ODBC functions as variadic functions,
// and for SQLSpecialColumns that causes on ARM that Nullable does not receive the value. That is observed on Apple.
// with latest iOdbc atm(3.52.16)
// Assumning __aarch64__ is defined on Apple as well and the problem is mainly iOdbc and not Apple's ARM PCS
#ifdef __aarch64__

#include <sqltypes.h>
// This is seemingly the best way to detect iOdbc at compile time. This header is included from its sqltypes and defines this macro
#ifdef _IODBCUNIX_H

#include "ma_api_internal.h"

// We can' pull in this translation unit ODBC API function declarations
#ifndef SQL_INVALID_HANDLE
#define SQL_INVALID_HANDLE          (-2)
#endif

extern "C" {
  // The only purpose of this gunctions is only to read stack parameters correctly and pass them further
  /* {{{ SQLSpecialColumns */
  SQLRETURN SQL_API SQLSpecialColumns(SQLHSTMT StatementHandle,
      SQLUSMALLINT IdentifierType,
      SQLCHAR *CatalogName,
      SQLSMALLINT NameLength1,
      SQLCHAR *SchemaName,
      SQLSMALLINT NameLength2,
      SQLCHAR *TableName,
      SQLSMALLINT NameLength3,
      SQLUINTEGER Scope,
      SQLUINTEGER Nullable)
  {
    if (!StatementHandle)
      return SQL_INVALID_HANDLE;
    return MA_SQLSpecialColumns(StatementHandle,IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2,
                                         TableName, NameLength3, (SQLUSMALLINT)Scope, (SQLUSMALLINT)Nullable);
  }
  /* }}} */

  /* {{{ SQLSpecialColumnsW */
  SQLRETURN SQL_API SQLSpecialColumnsW(SQLHSTMT StatementHandle,
      SQLUSMALLINT IdentifierType,
      SQLWCHAR *CatalogName,
      SQLSMALLINT NameLength1,
      SQLWCHAR *SchemaName,
      SQLSMALLINT NameLength2,
      SQLWCHAR *TableName,
      SQLSMALLINT NameLength3,
      SQLUINTEGER Scope,
      SQLUINTEGER Nullable)
  {
    if (!StatementHandle)
      return SQL_INVALID_HANDLE;

    return MA_SQLSpecialColumnsW(StatementHandle,IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2,
                                         TableName, NameLength3, (SQLUSMALLINT)Scope, (SQLUSMALLINT)Nullable);
  }
  /* }}} */
}
#endif  // _IODBCUNIX_H
#endif  // __aarch64__
