/************************************************************************************
   Copyright (C) 2013,2018 MariaDB Corporation AB
   
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
MYSQL_STMT* MADB_NewStmtHandle(MADB_Stmt *Stmt);
BOOL QueryIsPossiblyMultistmt(MADB_QUERY *Query);
int  SqlRtrim(char *StmtStr, int Length);
unsigned int GetMultiStatements(MADB_Stmt *Stmt, BOOL ExecDirect);
int MADB_KeyTypeCount(MADB_Dbc *Connection, char *TableName, int KeyFlag);
MYSQL_RES *MADB_ReadDefaultValues(MADB_Dbc *Dbc, const char *Catalog, const char *TableName);
int MADB_GetDefaultType(int SQLDataType);
void MADB_CopyOdbcTsToMadbTime(SQL_TIMESTAMP_STRUCT *Src, MYSQL_TIME *Dst);
void MADB_CopyMadbTimeToOdbcTs(MYSQL_TIME *Src, SQL_TIMESTAMP_STRUCT *Dst);
SQLRETURN MADB_CopyMadbTimestamp(MADB_Stmt *Stmt, MYSQL_TIME *tm, SQLPOINTER DataPtr, SQLLEN *Length, SQLLEN *Ind,
                                 SQLSMALLINT CType, SQLSMALLINT SqlType);
int  MADB_GetWCharType(int Type);

BOOL MADB_CheckODBCType(SQLSMALLINT Type);
SQLSMALLINT MADB_GetTypeFromConciseType(SQLSMALLINT ConciseType);
size_t MADB_GetTypeLength(SQLINTEGER SqlDataType, size_t Length);
SQLLEN MADB_GetDataSize(SQLSMALLINT SqlType, SQLLEN OctetLength, BOOL Unsigned,
                        SQLSMALLINT Precision, SQLSMALLINT Scale, unsigned int CharMaxLen);
int MADB_GetMaDBTypeAndLength(SQLINTEGER SqlDataType, my_bool *Unsigned, unsigned long *Length);
//char *MADB_GetDefaultColumnValue(MADB_Stmt *Stmt, char *Schema, char *TableName, char *Column);
SQLSMALLINT MapMariadDbToOdbcType(MYSQL_FIELD *field);
size_t MADB_GetHexString(char *BinaryBuffer, size_t BinaryLength,
                          char *HexBuffer, size_t HexLength);

size_t  MADB_GetDisplaySize(MYSQL_FIELD *Field, MARIADB_CHARSET_INFO *charset);
size_t  MADB_GetOctetLength(MYSQL_FIELD *Field, unsigned short MaxCharLen);
char *  MADB_GetTypeName(MYSQL_FIELD *Field);

char *  ltrim(char *Str);
char *  trim(char *Str);

my_bool MADB_CheckPtrLength(SQLINTEGER MaxLength, char *Ptr, SQLINTEGER NameLen);
void *  GetBindOffset(MADB_Desc *Ard, MADB_DescRecord *ArdRecord, void *Ptr, SQLULEN RowNumber, size_t PtrSize);
BOOL    MADB_ColumnIgnoredInAllRows(MADB_Desc *Desc, MADB_DescRecord *Rec);

SQLRETURN     MADB_DaeStmt(MADB_Stmt *Stmt, SQLUSMALLINT Operation);
MYSQL_RES *   MADB_GetDefaultColumnValues(MADB_Stmt *Stmt, MYSQL_FIELD *fields);
char *        MADB_GetDefaultColumnValue(MYSQL_RES *res, const char *Column);

/* SQL_NUMERIC stuff */
int           MADB_CharToSQLNumeric (char *buffer, MADB_Desc *Ard, MADB_DescRecord *ArdRecord,
                                     SQL_NUMERIC_STRUCT *dst_buffer, unsigned long RowNumber);
void          MADB_NumericInit      (SQL_NUMERIC_STRUCT *number, MADB_DescRecord *Ard);

int           MADB_FindNextDaeParam     (MADB_Desc *Desc, int InitialParam, SQLSMALLINT RowNumber);

BOOL          MADB_IsNumericType(SQLSMALLINT ConciseType);
BOOL          MADB_IsIntType    (SQLSMALLINT ConciseType);

/* For multistatement picks stmt handler pointed by stored index, and sets it as "current" stmt handler */
void          MADB_InstallStmt  (MADB_Stmt *Stmt, MYSQL_STMT *stmt);

/* for dummy binding */
extern my_bool DummyError;

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
#ifndef test
#define test(a)		((a) ? 1 : 0)
#endif

#define BUFFER_CHAR_LEN(blen,wchar) (wchar) ? (blen) / sizeof(SQLWCHAR) : (blen)

#define MADB_FREE(a) \
  free((a));\
  (a)= NULL;
#define MADB_ALLOC(a) malloc((a))
#define MADB_CALLOC(a) calloc((a) > 0 ? (a) : 1, sizeof(char))
#define MADB_REALLOC(a,b) realloc((a),(b))

/* If required to free old memory pointed by current ptr, and set new value */
#define MADB_RESET(ptr, newptr) do {\
  char *local_new_ptr= (newptr);\
  if (local_new_ptr != ptr) {\
    free((char*)(ptr));\
    if (local_new_ptr != NULL)\
      (ptr)= _strdup(local_new_ptr);\
    else\
      (ptr)= NULL;\
  }\
} while(0)

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

#define MADB_IS_EMPTY(STR) ((STR)==NULL || *(STR)=='\0')

#define MADB_FRACTIONAL_PART(_decimals) ((_decimals) > 0 ? (_decimals) + 1/*Decimal point*/ : 0)

#endif
