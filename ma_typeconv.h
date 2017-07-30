/************************************************************************************
   Copyright (C) 2017 MariaDB Corporation AB
   
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

#pragma once
#ifndef _ma_typeconv_h
#define _ma_typeconv_h

#define MADB_CHARSIZE_FOR_NUMERIC 80
BOOL      MADB_ConversionSupported(MADB_DescRecord *From, MADB_DescRecord *To);
size_t    MADB_ConvertNumericToChar(SQL_NUMERIC_STRUCT *Numeric, char *Buffer, int *ErrorCode);
SQLLEN    MADB_CalculateLength(MADB_Stmt *Stmt, SQLLEN *OctetLengthPtr, MADB_DescRecord *CRec, void* DataPtr);
SQLRETURN MADB_C2SQL(MADB_Stmt* Stmt, MADB_DescRecord *CRec, MADB_DescRecord *SqlRec, SQLULEN ParamSetIdx, MYSQL_BIND *bind);

SQLRETURN MADB_Wchar2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                         MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr);
SQLRETURN MADB_Char2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                        MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr);
SQLRETURN MADB_Numeric2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                        MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr);
SQLRETURN MADB_Timestamp2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                        MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr);
SQLRETURN MADB_Time2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                        MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr);
SQLRETURN MADB_IntervalHtoMS2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                        MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr);
SQLRETURN MADB_Date2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                        MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr);

SQLRETURN MADB_ConvertC2Sql(MADB_Stmt *Stmt, MADB_DescRecord *CRec, void* DataPtr, SQLLEN Length,
                            MADB_DescRecord *SqlRec, MYSQL_BIND *MaBind, void **Buffer, unsigned long *LengthPtr);

#endif
