/************************************************************************************
   Copyright (C) 2017,2018 MariaDB Corporation AB
   
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

/* Code allowing to deploy MariaDB bulk operation functionality.
 * i.e. adapting ODBC param arrays to MariaDB arrays */

#ifndef _ma_bulk_h
#define _ma_bulk_h

#define MADB_DOING_BULK_OPER(_stmt) ((_stmt)->Bulk.ArraySize > 1)

/* Couple defined to make "switch"s look at least shorter, if not nicer */
#define CHAR_BINARY_TYPES SQL_C_CHAR:\
case SQL_C_BINARY:\
case SQL_LONGVARBINARY:\
case SQL_VARBINARY:\
case SQL_VARCHAR:\
case SQL_LONGVARCHAR

#define WCHAR_TYPES SQL_C_WCHAR:\
case SQL_WVARCHAR:\
case SQL_WLONGVARCHAR

#define DATETIME_TYPES SQL_C_TIMESTAMP:\
case SQL_TYPE_TIMESTAMP:\
case SQL_C_TIME:\
case SQL_TYPE_TIME:\
case SQL_C_INTERVAL_HOUR_TO_MINUTE:\
case SQL_C_INTERVAL_HOUR_TO_SECOND:\
case SQL_C_DATE:\
case SQL_TYPE_DATE

char          MADB_MapIndicatorValue(SQLLEN OdbcInd);
unsigned int  MADB_UsedParamSets(MADB_Stmt *Stmt);
BOOL          MADB_AppBufferCanBeUsed(SQLSMALLINT CType, SQLSMALLINT SqlType);
void          MADB_CleanBulkOperData(MADB_Stmt *Stmt, unsigned int ParamOffset);
SQLRETURN     MADB_InitBulkOperBuffers(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void *DataPtr, SQLLEN *OctetLengthPtr,
                                      SQLLEN *IndicatorPtr, SQLSMALLINT SqlType, MYSQL_BIND *MaBind);
SQLRETURN     MADB_SetIndicatorValue(MADB_Stmt *Stmt, MYSQL_BIND *MaBind, unsigned int row, SQLLEN OdbcIndicator);

SQLRETURN     MADB_ExecuteBulk(MADB_Stmt *Stmt, unsigned int ParamOffset);

#endif
