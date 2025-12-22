/************************************************************************************
   Copyright (C) 2013,2024 MariaDB Corporation plc
   
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
#ifndef _ma_helper_h_
#define _ma_helper_h_

void CloseMultiStatements(MADB_Stmt *Stmt);
bool QueryIsPossiblyMultistmt(MADB_QUERY *Query);
int  SqlRtrim(char *StmtStr, int Length);
MYSQL_RES* MADB_ReadDefaultValues(MADB_Dbc *Dbc, const char *Catalog, const char *TableName);
int MADB_GetDefaultType(int SQLDataType);
void MADB_CopyOdbcTsToMadbTime(SQL_TIMESTAMP_STRUCT *Src, MYSQL_TIME *Dst);
void MADB_CopyMadbTimeToOdbcTs(MYSQL_TIME *Src, SQL_TIMESTAMP_STRUCT *Dst);
SQLRETURN MADB_CopyMadbTimestamp(MADB_Stmt *Stmt, MYSQL_TIME *tm, SQLPOINTER DataPtr, SQLLEN *Length, SQLLEN *Ind,
                                 SQLSMALLINT CType, SQLSMALLINT SqlType);
int  MADB_GetWCharType(int Type);

BOOL MADB_CheckODBCType(SQLSMALLINT Type);
SQLSMALLINT MADB_GetTypeFromConciseType(SQLSMALLINT ConciseType);
size_t MADB_GetTypeLength(SQLINTEGER SqlDataType, size_t Length);

/* MADB_GetDataSize is used to calcualte value for the column size descriptor field.
   It(this field) is SQLULEN since SQLDescribeCol returns it as SQLULEN. While in
   other places(SQLColAttribute) it is SQLLEN :( */
SQLULEN MADB_GetDataSize(SQLSMALLINT SqlType, unsigned long long OctetLength, BOOL Unsigned,
                        SQLSMALLINT Precision, SQLSMALLINT Scale, unsigned int CharMaxLen);
enum enum_field_types MADB_GetMaDBTypeAndLength(SQLINTEGER SqlDataType, my_bool *Unsigned, unsigned long *Length);
//char *MADB_GetDefaultColumnValue(MADB_Stmt *Stmt, char *Schema, char *TableName, char *Column);
SQLSMALLINT MapMariadDbToOdbcType(const MADB_Stmt* Stmt, const MYSQL_FIELD *field);
size_t MADB_GetHexString(char *BinaryBuffer, size_t BinaryLength,
                          char *HexBuffer, size_t HexLength);

size_t  MADB_GetDisplaySize(const MYSQL_FIELD *Field, MARIADB_CHARSET_INFO *charset, bool noBigint);
size_t  MADB_GetOctetLength(const MYSQL_FIELD *Field, unsigned short MaxCharLen);
const char *  MADB_GetTypeName(const MYSQL_FIELD *Field);

my_bool MADB_CheckPtrLength(SQLINTEGER MaxLength, char *Ptr, SQLINTEGER NameLen);
std::size_t getArrayStep(MADB_Header& header, std::size_t pointedSize);
void *  GetBindOffset(MADB_Header& DescHeader, void *Ptr, SQLULEN RowNumber, size_t PtrSize);
BOOL    MADB_ColumnIgnoredInAllRows(MADB_Desc *Desc, MADB_DescRecord *Rec);

SQLRETURN     MADB_DaeStmt(MADB_Stmt *Stmt, SQLUSMALLINT Operation);
MYSQL_RES *   MADB_GetDefaultColumnValues(MADB_Stmt *Stmt, const MYSQL_FIELD *fields);
char *        MADB_GetDefaultColumnValue(MYSQL_RES *res, const char *Column);

/* SQL_NUMERIC stuff */
int           MADB_CharToSQLNumeric (char *buffer, MADB_Desc *Ard, MADB_DescRecord *ArdRecord,
                                     SQL_NUMERIC_STRUCT *dst_buffer, unsigned long RowNumber);
void          MADB_NumericInit      (SQL_NUMERIC_STRUCT *number, MADB_DescRecord *Ard);

int           MADB_FindNextDaeParam     (MADB_Desc *Desc, int InitialParam, SQLSMALLINT RowNumber);

bool          MADB_IsDecimalType(SQLSMALLINT ConciseType);
bool          MADB_IsNumericType(SQLSMALLINT ConciseType);
bool          MADB_IsIntType    (SQLSMALLINT ConciseType);

enum enum_field_types MADB_GetNativeFieldType(MADB_Stmt *Stmt, uint32_t i);

SQLRETURN     MADB_KillAtServer(MADB_Dbc *Dbc, MADB_Error* Error);

/* Stringify macros */
#define XSTR(s) STR(s)
#define STR(s) #s

#define MADB_INT_MAX32       0x7FFFFFFFL

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MADBTEST
#define MADBTEST(a)		((a) ? 1 : 0)
#endif

#define BUFFER_CHAR_LEN(blen,wchar) (wchar) ? (blen) / sizeof(SQLWCHAR) : (blen)

#define MADB_SET_NUM_VAL(TYPE, PTR, VALUE, LENGTHPTR)\
{\
  if((PTR))\
    *(TYPE *)(PTR)= (VALUE);\
  if((LENGTHPTR))\
    *(LENGTHPTR)= sizeof(TYPE);\
}

#define MADB_SET_INTVAL(PTR, LEN, INTTYPE, VALUE) \
{\
  if((PTR))\
    *((INTTYPE *)PTR)= (VALUE); \
  LEN=sizeof(INTTYPE);\
}

#define MADB_FRACTIONAL_PART(_decimals) ((_decimals) > 0 ? (_decimals) + 1/*Decimal point*/ : 0)

#endif
