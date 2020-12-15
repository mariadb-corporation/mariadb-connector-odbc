/************************************************************************************
   Copyright (C) 2020 MariaDB Corporation AB
   
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

#ifndef _MA_API_INTERNAL_
#define _MA_API_INTERNAL_
/**
 * "Internal" ODBC API functions - functions, that have to be called internally if API 
 * function needs to be executed
 *
 * Calling SQLFunction itself inside the connector on non-Windows platforms will result
 * in the driver manager function instead of our own function.
 */

SQLRETURN MA_SQLAllocHandle(SQLSMALLINT HandleType,
    SQLHANDLE InputHandle,
    SQLHANDLE *OutputHandlePtr);

SQLRETURN MA_SQLBindParameter(SQLHSTMT StatementHandle,
    SQLUSMALLINT ParameterNumber,
    SQLSMALLINT InputOutputType,
    SQLSMALLINT ValueType,
    SQLSMALLINT ParameterType,
    SQLULEN ColumnSize,
    SQLSMALLINT DecimalDigits,
    SQLPOINTER ParameterValuePtr,
    SQLLEN BufferLength,
    SQLLEN *StrLen_or_IndPtr);

SQLRETURN MA_SQLCancel(SQLHSTMT StatementHandle);

SQLRETURN MA_SQLEndTran(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT CompletionType);

SQLRETURN SQL_API MA_SQLGetDiagRecW(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLWCHAR *SQLState,
    SQLINTEGER *NativeErrorPtr,
    SQLWCHAR *MessageText,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *TextLengthPtr);

SQLRETURN MA_SQLGetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr);

SQLRETURN MA_SQLGetDiagRec(SQLSMALLINT HandleType,
    SQLHANDLE Handle,
    SQLSMALLINT RecNumber,
    SQLCHAR *SQLState,
    SQLINTEGER *NativeErrorPtr,
    SQLCHAR *MessageText,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *TextLengthPtr);

SQLRETURN MA_SQLGetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER BufferLength,
    SQLINTEGER *StringLengthPtr);

SQLRETURN MA_SQLSetConnectAttr(SQLHDBC ConnectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength);

SQLRETURN MA_SQLSetStmtAttr(SQLHSTMT StatementHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength);

#endif

