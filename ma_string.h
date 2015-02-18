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
#ifndef _ma_string_h_
#define _ma_string_h_

char *MADB_ConvertFromWChar(SQLWCHAR *Ptr, SQLINTEGER PtrLength, SQLULEN *Length, Client_Charset* cc, BOOL *DefaultCharUsed);
int MADB_ConvertAnsi2Unicode(Client_Charset* cc, char *AnsiString, int AnsiLength, 
                             SQLWCHAR *UnicodeString, int UnicodeLength, 
                             SQLLEN *LengthIndicator, BOOL IsNull, MADB_Error *Error);
char *MADB_GetInsertStatement(MADB_Stmt *Stmt);
char *MADB_GetTableName(MADB_Stmt *Stmt);
char *MADB_GetCatalogName(MADB_Stmt *Stmt);
my_bool MADB_DynStrUpdateSet(MADB_Stmt *Stmt, DYNAMIC_STRING *DynString);
my_bool MADB_DynStrInsertSet(MADB_Stmt *Stmt, DYNAMIC_STRING *DynString);
my_bool MADB_DynStrGetWhere(MADB_Stmt *Stmt, DYNAMIC_STRING *DynString, char *TableName, my_bool ParameterMarkers);
my_bool MADB_DynStrAppendQuoted(DYNAMIC_STRING *DynString, char *String);
my_bool MADB_DynStrGetColumns(MADB_Stmt *Stmt, DYNAMIC_STRING *DynString);
my_bool MADB_DynStrGetValues(MADB_Stmt *Stmt, DYNAMIC_STRING *DynString);
SQLWCHAR *MADB_ConvertToWchar(char *Ptr, SQLLEN PtrLength, Client_Charset* cc);
size_t MADB_SetString(Client_Charset* cc, void *Dest, unsigned int DestLength,
                      char *Src, int SrcLength, MADB_Error *Error);
my_bool MADB_ValidateStmt(char *StmtStr);
my_bool MADB_IsStatementSupported(char *StmtStr, char *token1, char *token2);

SQLINTEGER MbstrOctetLen(char *str, SQLLEN *CharLen, CHARSET_INFO *cs);
SQLLEN MbstrCharLen(char *str, SQLINTEGER OctetLen, CHARSET_INFO *cs);

#define ADJUST_LENGTH(ptr, len)\
  if((ptr) && ((len) == SQL_NTS))\
    len= strlen((ptr));\
  else if (!(ptr))\
    len= 0;

#endif
