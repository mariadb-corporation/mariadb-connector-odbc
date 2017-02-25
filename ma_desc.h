/************************************************************************************
   Copyright (C) 2013,2015 MariaDB Corporation AB
   
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
#ifndef _ma_desc_h_
#define _ma_desc_h_

#define MADB_DESC_NONE 0
#define MADB_DESC_READ 1
#define MADB_DESC_WRITE 2
#define MADB_DESC_RW 3

#define MADB_DESC_INIT_REC_NUM 32
#define MADB_DESC_INIT_STMT_NUM 16

enum enum_madb_desc_type {MADB_DESC_APD= 0, MADB_DESC_ARD, MADB_DESC_IPD, MADB_DESC_IRD, MADB_DESC_UNKNOWN=254};

MADB_DescRecord *MADB_DescGetInternalRecord(MADB_Desc *Desc, SQLSMALLINT RecordNumber, SQLSMALLINT Type);

MADB_Desc *MADB_DescInit(MADB_Dbc *Dbc, enum enum_madb_desc_type DescType, my_bool isExternal);
SQLRETURN MADB_DescFree(MADB_Desc *Desc, my_bool RecordsOnly);
SQLRETURN MADB_DescGetField(SQLHDESC DescriptorHandle,
                            SQLSMALLINT RecNumber,
                            SQLSMALLINT FieldIdentifier,
                            SQLPOINTER ValuePtr,
                            SQLINTEGER BufferLength,
                            SQLINTEGER *StringLengthPtr,
                            my_bool isWChar);
SQLRETURN MADB_DescSetField(SQLHDESC DescriptorHandle,
                            SQLSMALLINT RecNumber,
                            SQLSMALLINT FieldIdentifier,
                            SQLPOINTER ValuePtr,
                            SQLINTEGER BufferLength,
                            my_bool isWChar);

my_bool MADB_DescSetIrdMetadata(MADB_Stmt *Stmt, MYSQL_FIELD *Fields, unsigned int NumFields);
SQLRETURN MADB_DescCopyDesc(MADB_Desc *SrcDesc, MADB_Desc *DestDesc);
SQLRETURN MADB_DescGetRec(MADB_Desc *Desc,
    SQLSMALLINT RecNumber,
    SQLCHAR *Name,
    SQLSMALLINT BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLSMALLINT *TypePtr,
    SQLSMALLINT *SubTypePtr,
    SQLLEN *LengthPtr,
    SQLSMALLINT *PrecisionPtr,
    SQLSMALLINT *ScalePtr,
    SQLSMALLINT *NullablePtr,
    BOOL isWChar);

my_bool MADB_FixColumnDataTypes(MADB_Stmt *Stmt, MADB_ShortTypeInfo *ColTypesArr);

#endif /* _ma_desc_h_ */
