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
#ifndef _ma_error_h_
#define _ma_error_h_

extern MADB_ERROR MADB_ErrorList[];

enum enum_madb_error {
  MADB_ERR_00000=0,
  MADB_ERR_01000,
  MADB_ERR_01001,
  MADB_ERR_01002,
  MADB_ERR_01003,
  MADB_ERR_01004,
  MADB_ERR_01006,
  MADB_ERR_01007,
  MADB_ERR_01S00,
  MADB_ERR_01S01,
  MADB_ERR_01S02,
  MADB_ERR_01S06,
  MADB_ERR_01S07,
  MADB_ERR_01S08,
  MADB_ERR_01S09,
  MADB_ERR_07001,
  MADB_ERR_07002,
  MADB_ERR_07005,
  MADB_ERR_07006,
  MADB_ERR_07009,
  MADB_ERR_07S01,
  MADB_ERR_08001,
  MADB_ERR_08002,
  MADB_ERR_08003,
  MADB_ERR_08004,
  MADB_ERR_08007,
  MADB_ERR_08S01,
  MADB_ERR_21S01,
  MADB_ERR_21S02,
  MADB_ERR_22001,
  MADB_ERR_22002,
  MADB_ERR_22003,
  MADB_ERR_22007,
  MADB_ERR_22008,
  MADB_ERR_22012,
  MADB_ERR_22015,
  MADB_ERR_22018,
  MADB_ERR_22019,
  MADB_ERR_22025,
  MADB_ERR_22026,
  MADB_ERR_23000,
  MADB_ERR_24000,
  MADB_ERR_25000,
  MADB_ERR_25S01,
  MADB_ERR_25S02,
  MADB_ERR_25S03,
  MADB_ERR_28000,
  MADB_ERR_34000,
  MADB_ERR_3C000,
  MADB_ERR_3D000,
  MADB_ERR_3F000,
  MADB_ERR_40001,
  MADB_ERR_40002,
  MADB_ERR_40003,
  MADB_ERR_42000,
  MADB_ERR_42S01,
  MADB_ERR_42S02,
  MADB_ERR_42S11,
  MADB_ERR_42S12,
  MADB_ERR_42S21,
  MADB_ERR_42S22,
  MADB_ERR_44000,
  MADB_ERR_HY000,
  MADB_ERR_HY001,
  MADB_ERR_HY003,
  MADB_ERR_HY004,
  MADB_ERR_HY007,
  MADB_ERR_HY008,
  MADB_ERR_HY009,
  MADB_ERR_HY010,
  MADB_ERR_HY011,
  MADB_ERR_HY012,
  MADB_ERR_HY013,
  MADB_ERR_HY014,
  MADB_ERR_HY015,
  MADB_ERR_HY016,
  MADB_ERR_HY017,
  MADB_ERR_HY018,
  MADB_ERR_HY019,
  MADB_ERR_HY020,
  MADB_ERR_HY021,
  MADB_ERR_HY024,
  MADB_ERR_HY090,
  MADB_ERR_HY091,
  MADB_ERR_HY092,
  MADB_ERR_HY095,
  MADB_ERR_HY096,
  MADB_ERR_HY097,
  MADB_ERR_HY098,
  MADB_ERR_HY099,
  MADB_ERR_HY100,
  MADB_ERR_HY101,
  MADB_ERR_HY103,
  MADB_ERR_HY104,
  MADB_ERR_HY105,
  MADB_ERR_HY106,
  MADB_ERR_HY107,
  MADB_ERR_HY109,
  MADB_ERR_HY110,
  MADB_ERR_HY111,
  MADB_ERR_HYC00,
  MADB_ERR_HYT00,
  MADB_ERR_HYT01,
  MADB_ERR_IM001,
  MADB_ERR_IM002,
  MADB_ERR_IM003,
  MADB_ERR_IM004,
  MADB_ERR_IM005,
  MADB_ERR_IM006,
  MADB_ERR_IM007,
  MADB_ERR_IM008,
  MADB_ERR_IM009,
  MADB_ERR_IM010,
  MADB_ERR_IM011,
  MADB_ERR_IM012,
  MADB_ERR_IM013,
  MADB_ERR_IM014,
  MADB_ERR_IM015,
  MADB_ERR_S1000,
  MADB_ERR_S1107,
  MADB_ERR_S1C00,
};
char* MADB_PutErrorPrefix(MADB_Dbc *dbc, MADB_Error *error);

SQLRETURN MADB_SetError(MADB_Error *Error, unsigned int SqlErrorCode, const char *SqlErrorMsg, unsigned int NativeError);
SQLRETURN MADB_SetNativeError(MADB_Error *Error, SQLSMALLINT HandleType, void *Ptr);
void MADB_CopyError(MADB_Error *ErrorTo, MADB_Error *ErrorFrom);
SQLRETURN MADB_GetDiagRec(MADB_Error *Err, SQLSMALLINT RecNumber,
                         void *SQLState, SQLINTEGER *NativeErrorPtr,
                         void *MessageText, SQLSMALLINT BufferLength,
                         SQLSMALLINT *TextLengthPtr, my_bool isWChar,
                         SQLINTEGER OdbcVersion);
SQLRETURN MADB_GetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle,
                            SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier, SQLPOINTER
                            DiagInfoPtr, SQLSMALLINT BufferLength,
                            SQLSMALLINT *StringLengthPtr, my_bool isWChar);

#define MADB_CLEAR_ERROR(a) \
  strcpy_s((a)->SqlState, SQL_SQLSTATE_SIZE+1, MADB_ErrorList[MADB_ERR_00000].SqlState); \
  (a)->SqlErrorMsg[(a)->PrefixLen]= 0; \
  (a)->NativeError= 0;\
  (a)->ReturnValue= SQL_SUCCESS;\
  (a)->ErrorNum= 0;

#define MADB_CLEAR_HANDLE_ERROR(handle_type, handle) \
  switch (handle_type) { \
  case SQL_HANDLE_ENV: \
      MADB_CLEAR_ERROR(&((MADB_Env *)Handle)->Error); \
      break; \
  case SQL_HANDLE_DBC: \
      MADB_CLEAR_ERROR(&((MADB_Dbc *)Handle)->Error); \
      break; \
  case SQL_HANDLE_STMT:\
      MADB_CLEAR_ERROR(&((MADB_Stmt *)Handle)->Error); \
    }

#define MADB_CHECK_HANDLE_CLEAR_ERROR(handle_type, handle) \
  if (handle == 0) return SQL_INVALID_HANDLE;\
  MADB_CLEAR_HANDLE_ERROR(handle_type, handle) \
  

#define MADB_NOT_IMPLEMENTED(HANDLE)\
  MADB_SetError(&(HANDLE)->Error, MADB_ERR_IM001, NULL, 0);\
  return SQL_ERROR;

#define MADB_CHECK_ATTRIBUTE(Handle, Attr, ValidAttrs)\
{\
  SQLULEN x=1, ok=0, my_attr=(SQLULEN)(Attr);\
  while (x <= ValidAttrs[0] && !ok)\
    if ((ok= ValidAttrs[x++] == my_attr))\
  if (!ok)\
  {\
    MADB_SetError(&(Handle)->Error,MADB_ERR_HY024, NULL, 0);\
    return (Handle)->Error.ReturnValue;\
  }\
}

#endif /* _ma_error_h_ */
