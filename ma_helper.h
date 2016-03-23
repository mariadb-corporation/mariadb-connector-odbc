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
#ifndef _ma_helper_h_
#define _ma_helper_h_

void CloseMultiStatements(MADB_Stmt *Stmt);
BOOL QueryIsPossiblyMultistmt(char *queryStr);
unsigned int GetMultiStatements(MADB_Stmt *Stmt, char *StmtStr, size_t Length);
int MADB_KeyTypeCount(MADB_Dbc *Connection, char *TableName, int KeyFlag);
MYSQL_RES *MADB_ReadDefaultValues(MADB_Dbc *Dbc, const char *Catalog, const char *TableName);
int MADB_GetDefaultType(int SQLDataType);
void MADB_CopyMadbTimestamp(MYSQL_TIME *tm, MADB_Desc *Ard, MADB_DescRecord *ArdRecord, int Type, unsigned long RowNumber);
int  MADB_GetWCharType(int Type);
my_bool MADB_get_single_row(MADB_Dbc *Connection,
                            const char *StmtString,
                            size_t Length,
                            unsigned int NumCols,
                            char **Buffers,
                            size_t *Buffer_Lengths);
bool MADB_CheckODBCType(SQLSMALLINT Type);
SQLSMALLINT MADB_GetTypeFromConciseType(SQLSMALLINT ConciseType);
size_t MADB_GetTypeLength(SQLINTEGER SqlDataType, size_t Length);
size_t MADB_GetDataSize(MADB_DescRecord *Record, MYSQL_FIELD Field, CHARSET_INFO *charset);
int MADB_GetTypeAndLength(SQLINTEGER SqlDataType, my_bool *Unsigned, unsigned long *Length);
//char *MADB_GetDefaultColumnValue(MADB_Stmt *Stmt, char *Schema, char *TableName, char *Column);
SQLSMALLINT MADB_GetODBCType(MYSQL_FIELD *field);
size_t MADB_GetHexString(char *BinaryBuffer, size_t BinaryLength,
                          char *HexBuffer, size_t HexLength);

size_t MADB_GetDisplaySize(MYSQL_FIELD field, CHARSET_INFO *charset);
size_t MADB_GetOctetLength(MYSQL_FIELD Field, unsigned short MaxCharLen);
char *MADB_GetTypeName(MYSQL_FIELD Field);
char *trim(char *Str);
my_bool MADB_CheckPtrLength(SQLINTEGER MaxLength, char *Ptr, SQLINTEGER NameLen);
void *GetBindOffset(MADB_Desc *Ard, MADB_DescRecord *ArdRecord, void *Ptr, unsigned long RowNumber, size_t PtrSize);

SQLRETURN MADB_DaeStmt(MADB_Stmt *Stmt, SQLUSMALLINT Operation);
MYSQL_RES *MADB_GetDefaultColumnValues(MADB_Stmt *Stmt, MYSQL_FIELD *fields);
char *MADB_GetDefaultColumnValue(MYSQL_RES *res, const char *Column);

/* SQL_NUMERIC stuff */
int MADB_CharToSQLNumeric(char *buffer, MADB_Desc *Ard, MADB_DescRecord *ArdRecord, unsigned long RowNumber);
size_t MADB_SqlNumericToChar(SQL_NUMERIC_STRUCT *Numeric, char *Buffer, int *ErrorCode);
void MADB_NumericInit(SQL_NUMERIC_STRUCT *number, MADB_DescRecord *Ard);
unsigned long MADB_StmtDataTell(MADB_Stmt *Stmt);

/* for dummy binding */
extern my_bool DummyError;

/* Stringify macros */
#define XSTR(s) STR(s)
#define STR(s) #s

#define BUFFER_CHAR_LEN(blen,wchar) (wchar) ? (blen) / sizeof(SQLWCHAR) : (blen)

#define MADB_FREE(a) \
  my_free((a));\
  (a)= NULL;
#define MADB_ALLOC(a) my_malloc((a), MYF(0))
#define MADB_CALLOC(a) my_malloc((a), MYF(MY_ZEROFILL))
#define MADB_REALLOC(a,b) my_realloc((a),(b),MYF(MY_ZEROFILL))

/* If required to free old memory pointed by current ptr, and set new value */
#define MADB_RESET(ptr, newptr) {\
  char *local_new_ptr= (newptr);\
  if (local_new_ptr != ptr) {\
    my_free((gptr)(ptr));\
    if (local_new_ptr != NULL)\
      (ptr)= my_strdup(local_new_ptr, MYF(0));\
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

#endif
