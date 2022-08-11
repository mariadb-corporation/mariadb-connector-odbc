/************************************************************************************
   Copyright (C) 2013,2016 MariaDB Corporation AB
   
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
#include <ma_odbc.h>

extern Client_Charset utf8;

/* {{{ MADB_ErrorList[] */
MADB_ERROR MADB_ErrorList[] =
{
  { "00000", "", "", SQL_SUCCESS},
  { "01000", "", "General warning", SQL_SUCCESS_WITH_INFO},
  { "01001", "01S03", "Cursor operation conflict", SQL_SUCCESS_WITH_INFO},
  { "01002", "", "Disconnect error", SQL_SUCCESS_WITH_INFO},
  { "01003", "", "NULL value eliminated in set function", SQL_SUCCESS_WITH_INFO},
  { "01004", "", "String data, right-truncated", SQL_SUCCESS_WITH_INFO},
  { "01006", "", "Privilege not revoked", SQL_SUCCESS_WITH_INFO},
  { "01007", "", "Privilege not granted", SQL_SUCCESS_WITH_INFO},
  { "01S00", "", "Invalid connection string attribute", SQL_SUCCESS_WITH_INFO},
  { "01S01", "", "Error in row", SQL_SUCCESS_WITH_INFO},
  { "01S02", "", "Option value changed", SQL_SUCCESS_WITH_INFO},
  { "01S06", "", "Attempt to fetch before the result set returned the first rowset", SQL_SUCCESS_WITH_INFO},
  { "01S07", "", "Fractional truncation", SQL_SUCCESS_WITH_INFO},
  { "01S08", "", "Error saving File DSN", SQL_SUCCESS_WITH_INFO},
  { "01S09", "", "Invalid keyword", SQL_SUCCESS_WITH_INFO},
  { "07001", "", "Wrong number of parameters", SQL_ERROR},
  { "07002", "", "COUNT field incorrect", SQL_ERROR},
  { "07005", "2400", "Prepared statement not a cursor-specification", SQL_ERROR},
  { "07006", "", "Restricted data type attribute violation", SQL_ERROR},
  { "07009", "S1002", "Invalid descriptor index", SQL_ERROR},
  { "07S01", "", "Invalid use of default parameter", SQL_ERROR},
  { "08001", "", "Client unable to establish connection", SQL_ERROR},
  { "08002", "", "Connection name in use", SQL_ERROR},
  { "08003", "", "Connection not open", SQL_ERROR},
  { "08004", "", "Server rejected the connection", SQL_ERROR},
  { "08007", "", "Connection failure during transaction", SQL_ERROR},
  { "08S01", "", "Communication link failure", SQL_ERROR},
  { "21S01", "", "Insert value list does not match column list", SQL_ERROR},
  { "21S02", "", "Degree of derived table does not match column list", SQL_ERROR},
  { "22001", "", "String data, right-truncated", SQL_ERROR},
  { "22002", "", "Indicator variable required but not supplied", SQL_ERROR},
  { "22003", "", "Numeric value out of range", SQL_ERROR},
  { "22007", "22008", "Invalid datetime format", SQL_ERROR},
  { "22008", "", "Datetime field overflow", SQL_ERROR},
  { "22012", "", "Division by zero", SQL_ERROR},
  { "22015", "", "Interval field overflow", SQL_ERROR},
  { "22018", "22005", "Invalid character value for cast specification", SQL_ERROR},
  { "22019", "", "Invalid escape character", SQL_ERROR},
  { "22025", "", "Invalid escape sequence", SQL_ERROR},
  { "22026", "", "String data, length mismatch", SQL_ERROR},
  { "23000", "", "Integrity constraint violation", SQL_ERROR},
  { "24000", "", "Invalid cursor state", SQL_ERROR},
  { "25000", "", "Invalid transaction state", SQL_ERROR},
  { "25S01", "", "Transaction state", SQL_ERROR},
  { "25S02", "", "Transaction is still active", SQL_ERROR},
  { "25S03", "", "Transaction is rolled back", SQL_ERROR},
  { "28000", "", "Invalid authorization specification", SQL_ERROR},
  { "34000", "", "Invalid cursor name", SQL_ERROR},
  { "3C000", "", "Duplicate cursor name", SQL_ERROR},
  { "3D000", "", "Invalid catalog name", SQL_ERROR},
  { "3F000", "", "Invalid schema name", SQL_ERROR},
  { "40001", "", "Serialization failure", SQL_ERROR},
  { "40002", "", "Integrity constraint violation", SQL_ERROR},
  { "40003", "", "Statement completion unknown", SQL_ERROR},
  { "42000", "37000", "Syntax error or access violation", SQL_ERROR},
  { "42S01", "S0001", "Base table or view already exists", SQL_ERROR},
  { "42S02", "S0002", "Base table or view not found", SQL_ERROR},
  { "42S11", "S0011", "Index already exists", SQL_ERROR},
  { "42S12", "S0012", "Index not found", SQL_ERROR},
  { "42S21", "S0021", "Column already exists", SQL_ERROR},
  { "42S22", "S0022", "Column not found", SQL_ERROR},
  { "44000", "", "WITH CHECK OPTION violation", SQL_ERROR},
  { "HY000", "S1000", "General error", SQL_ERROR},
  { "HY001", "S1001", "Memory allocation error", SQL_ERROR},
  { "HY003", "S1003", "Invalid application buffer type", SQL_ERROR},
  { "HY004", "S1004", "Invalid SQL data type", SQL_ERROR},
  { "HY007", "S1010", "Associated statement is not prepared", SQL_ERROR},
  { "HY008", "S1008", "Operation canceled", SQL_ERROR},
  { "HY009", "S1009", "Invalid use of null pointer", SQL_ERROR},
  { "HY010", "S1010", "Function sequence error", SQL_ERROR},
  { "HY011", "S1011", "Attribute cannot be set now", SQL_ERROR},
  { "HY012", "S1012", "Invalid transaction operation code", SQL_ERROR},
  { "HY013", "", "Memory management error", SQL_ERROR},
  { "HY014", "", "Limit on the number of handles exceeded", SQL_ERROR},
  { "HY015", "", "No cursor name available", SQL_ERROR},
  { "HY016", "", "Cannot modify an implementation row descriptor", SQL_ERROR},
  { "HY017", "", "Invalid use of an automatically allocated descriptor handle", SQL_ERROR},
  { "HY018", "70100", "Server declined cancel request", SQL_ERROR},
  { "HY019", "22003", "Non-character and non-binary data sent in pieces", SQL_ERROR},
  { "HY020", "", "Attempt to concatenate a null value", SQL_ERROR},
  { "HY021", "", "Inconsistent descriptor information", SQL_ERROR},
  { "HY024", "S1009", "Invalid attribute value", SQL_ERROR},
  { "HY090", "S1009", "Invalid string or buffer length", SQL_ERROR},
  { "HY091", "S1091", "Invalid descriptor field identifier", SQL_ERROR},
  { "HY092", "S1092", "Invalid attribute/option identifier", SQL_ERROR},
  { "HY095", "", "Function type out of range", SQL_ERROR},
  { "HY096", "S1096", "Invalid information type", SQL_ERROR},
  { "HY097", "S1097", "Column type out of range", SQL_ERROR},
  { "HY098", "S1098", "Scope type out of range", SQL_ERROR},
  { "HY099", "S1099", "Nullable type out of range", SQL_ERROR},
  { "HY100", "S1100", "Uniqueness option type out of range", SQL_ERROR},
  { "HY101", "S1101", "Accuracy option type out of range", SQL_ERROR},
  { "HY103", "S1103", "Invalid retrieval code", SQL_ERROR},
  { "HY104", "S1104", "Invalid precision or scale value", SQL_ERROR},
  { "HY105", "S1105", "Invalid parameter type", SQL_ERROR},
  { "HY106", "S1106", "Fetch type out of range", SQL_ERROR},
  { "HY107", "S1107", "Row value out of range", SQL_ERROR},
  { "HY109", "S1109", "Invalid cursor position", SQL_ERROR},
  { "HY110", "S1110", "Invalid driver completion", SQL_ERROR},
  { "HY111", "S1111", "Invalid bookmark value", SQL_ERROR},
  { "HYC00", "S1C00", "Optional feature not implemented", SQL_ERROR},
  { "HYT00", "S1T00", "Timeout expired", SQL_ERROR},
  { "HYT01", "", "Connection timeout expired", SQL_ERROR},
  { "IM001", "", "Driver does not support this function", SQL_ERROR},
  { "IM002", "", "Data source name not found and no default driver specified", SQL_ERROR},
  { "IM003", "", "Specified driver could not be loaded", SQL_ERROR},
  { "IM004", "", "Driver's SQLAllocHandle on SQL_HANDLE_ENV failed", SQL_ERROR},
  { "IM005", "", "Driver's SQLAllocHandle on SQL_HANDLE_DBC failed", SQL_ERROR},
  { "IM006", "", "Driver's SQLSetConnectAttr failed", SQL_ERROR},
  { "IM007", "", "No data source or driver specified; dialog prohibited", SQL_ERROR},
  { "IM008", "", "Dialog failed", SQL_ERROR},
  { "IM009", "", "Unable to load translation DL", SQL_ERROR},
  { "IM010", "", "Data source name too long", SQL_ERROR},
  { "IM011", "", "Driver name too long", SQL_ERROR},
  { "IM012", "", "DRIVER keyword syntax error", SQL_ERROR},
  { "IM013", "", "Trace file error", SQL_ERROR},
  { "IM014", "", "Invalid name of File DSN", SQL_ERROR},
  { "IM015", "", "Corrupt file data source", SQL_ERROR},
  { "S1000", "", "General error", SQL_ERROR},
  { "S1107", "", "Row value out of range", SQL_ERROR},
  { "S1C00", "", "Optional feature not implemented", SQL_ERROR},
  { "", "", "", -1}
};
/* }}} */


char* MADB_PutErrorPrefix(MADB_Dbc *dbc, MADB_Error *error)
{
  /* If prefix is already there - we do not write again. One shoud reset error->PrefixLen in order to force */
  if (error->PrefixLen == 0)
  {
    error->PrefixLen= strlen(MARIADB_ODBC_ERR_PREFIX);
    strcpy_s(error->SqlErrorMsg, SQL_MAX_MESSAGE_LENGTH + 1, MARIADB_ODBC_ERR_PREFIX);
    if (dbc != NULL && dbc->mariadb != NULL)
    {
      error->PrefixLen += _snprintf(error->SqlErrorMsg + error->PrefixLen,
        SQL_MAX_MESSAGE_LENGTH + 1 - error->PrefixLen, "[%s]", mysql_get_server_info(dbc->mariadb)); 
    }
  }
  return error->SqlErrorMsg + error->PrefixLen;
}


SQLRETURN MADB_SetNativeError(MADB_Error *Error, SQLSMALLINT HandleType, void *Ptr)
{
  char *Sqlstate= NULL, *Errormsg= NULL;
  int NativeError= 0;

  switch (HandleType) {
  case SQL_HANDLE_DBC:
    Sqlstate= (char *)mysql_sqlstate((MYSQL *)Ptr);
    Errormsg= (char *)mysql_error((MYSQL *)Ptr);
    NativeError= mysql_errno((MYSQL *)Ptr);
    break;
  case SQL_HANDLE_STMT:
    Sqlstate= (char *)mysql_stmt_sqlstate((MYSQL_STMT *)Ptr);
    Errormsg= (char *)mysql_stmt_error((MYSQL_STMT *)Ptr);
    NativeError= mysql_stmt_errno((MYSQL_STMT *)Ptr);
    break;
  }
  /* work-around of probalby a bug in mariadb_stmt_execute_direct, that returns 1160 in case of lost connection */
  if ((NativeError == 2013 || NativeError == 2006 || NativeError == 1160) && (strcmp(Sqlstate, "HY000") == 0 || strcmp(Sqlstate, "00000") == 0))
  {
    Sqlstate= "08S01";
  }

  Error->ReturnValue= SQL_ERROR;
  if (Errormsg)
  {
    strcpy_s(Error->SqlErrorMsg + Error->PrefixLen, SQL_MAX_MESSAGE_LENGTH + 1 - Error->PrefixLen, Errormsg);
  }
  if (Sqlstate)
    strcpy_s(Error->SqlState, SQLSTATE_LENGTH + 1, Sqlstate);
  Error->NativeError= NativeError;
  if (Error->SqlState[0] == '0')
    Error->ReturnValue= (Error->SqlState[1] == '0') ? SQL_SUCCESS :
                        (Error->SqlState[1] == '1') ? SQL_SUCCESS_WITH_INFO : SQL_ERROR;

  return Error->ReturnValue;
}

/* {{{ MADB_SetError */
SQLRETURN MADB_SetError(MADB_Error  *Error,
                        unsigned int SqlErrorCode,
                        const char  *NativeErrorMsg,
                        unsigned int NativeError)
{
  unsigned int ErrorCode= SqlErrorCode;

  Error->ErrorNum= 0;
  if ((NativeError == 2013 || NativeError == 2006 || NativeError == 1160) && SqlErrorCode == MADB_ERR_HY000)
    ErrorCode= MADB_ERR_08S01;

  Error->ErrRecord= &MADB_ErrorList[ErrorCode];

  Error->ReturnValue= MADB_ErrorList[ErrorCode].ReturnValue;

  if (NativeErrorMsg)
  {
    strcpy_s(Error->SqlErrorMsg + Error->PrefixLen, SQL_MAX_MESSAGE_LENGTH + 1 - Error->PrefixLen, NativeErrorMsg);
  }
  else
  {
    strcpy_s(Error->SqlErrorMsg + Error->PrefixLen, SQL_MAX_MESSAGE_LENGTH + 1 - Error->PrefixLen,
             MADB_ErrorList[ErrorCode].SqlErrorMsg);
  }
  strcpy_s(Error->SqlState, SQLSTATE_LENGTH + 1, MADB_ErrorList[ErrorCode].SqlState);
  Error->NativeError= NativeError;

  return Error->ReturnValue;
}
/* }}} */

/* {{{ MADB_CopyError */
void MADB_CopyError(MADB_Error *ErrorTo, MADB_Error *ErrorFrom)
{
  ErrorTo->NativeError= ErrorFrom->NativeError;
  ErrorTo->ReturnValue= ErrorFrom->ReturnValue;
  ErrorTo->PrefixLen=   ErrorFrom->PrefixLen;
  strcpy_s(ErrorTo->SqlState, SQLSTATE_LENGTH + 1, ErrorFrom->SqlState);
  strcpy_s(ErrorTo->SqlErrorMsg, SQL_MAX_MESSAGE_LENGTH + 1, ErrorFrom->SqlErrorMsg);
}
/* }}} */

/* {{{ MADB_GetDiagRec */
SQLRETURN MADB_GetDiagRec(MADB_Error *Err, SQLSMALLINT RecNumber,
                         void *SQLState, SQLINTEGER *NativeErrorPtr,
                         void *MessageText, SQLSMALLINT BufferLength,
                         SQLSMALLINT *TextLengthPtr, my_bool isWChar,
                         SQLINTEGER OdbcVersion)
{
  MADB_Error InternalError;
  char *SqlStateVersion= Err->SqlState;
  SQLSMALLINT Length= 0;

  InternalError.PrefixLen= 0;
  MADB_CLEAR_ERROR(&InternalError);
  if (RecNumber > 1)
    return SQL_NO_DATA;
  
  /* check if we have to map the SQLState to ODBC version 2 state */
  if (OdbcVersion == SQL_OV_ODBC2)
  {
    int i= 0;
    while (MADB_ErrorList[i].SqlState[0])
    {
      if (strcmp(Err->SqlState, MADB_ErrorList[i].SqlState) == 0)
      {
        if (MADB_ErrorList[i].SqlStateV2[0])
          SqlStateVersion= MADB_ErrorList[i].SqlStateV2;
        break;
      }
      ++i;
    }
  }

  if (NativeErrorPtr)
    *NativeErrorPtr= Err->NativeError;
  if (SQLState)
    MADB_SetString(isWChar ?  &utf8 : 0, (void *)SQLState, SQL_SQLSTATE_SIZE + 1,
                   SqlStateVersion, SQL_SQLSTATE_SIZE, &InternalError);
   if (MessageText)
     Length=  (SQLSMALLINT)MADB_SetString(isWChar ?  &utf8 : 0, (void*)MessageText, BufferLength,
                   Err->SqlErrorMsg, strlen(Err->SqlErrorMsg), &InternalError);
   if (TextLengthPtr)
     *TextLengthPtr= (SQLSMALLINT)strlen(Err->SqlErrorMsg);
   
   if (!MessageText || !BufferLength)
     return SQL_SUCCESS;
   return InternalError.ReturnValue;
}
/* }}} */

/* {{{ MADB_GetDiagField */
SQLRETURN MADB_GetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle,
                            SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier, SQLPOINTER
                            DiagInfoPtr, SQLSMALLINT BufferLength,
                            SQLSMALLINT *StringLengthPtr, my_bool isWChar)
{
  MADB_Error *Err=  NULL;
  MADB_Stmt  *Stmt= NULL;
  MADB_Desc  *Desc= NULL;
  MADB_Dbc   *Dbc=  NULL;
  MADB_Env   *Env=  NULL;
  MADB_Error  Error;
  SQLLEN      Length;

  if (StringLengthPtr)
    *StringLengthPtr= 0;

  Error.PrefixLen= 0;
  MADB_CLEAR_ERROR(&Error);

  if (RecNumber > 1)
    return SQL_NO_DATA;

  switch(HandleType) {
  case SQL_HANDLE_DBC:
    Dbc= (MADB_Dbc *)Handle;
    Err= &Dbc->Error;
    break;
  case SQL_HANDLE_STMT:
    Stmt= (MADB_Stmt *)Handle;
    Err= &Stmt->Error;
    break;
  case SQL_HANDLE_ENV:
    Env= (MADB_Env *)Handle;
    Err= &Env->Error;
    break;
   case SQL_HANDLE_DESC:
     Desc= (MADB_Desc *)Handle;
     Err= &Desc->Error;
     break;
   default:
     return SQL_INVALID_HANDLE;
  }

  switch(DiagIdentifier) {
  case SQL_DIAG_CURSOR_ROW_COUNT:
    if (!Stmt)
      return SQL_ERROR;
    *(SQLLEN *)DiagInfoPtr= (Stmt->result) ?(SQLLEN)mysql_stmt_num_rows(Stmt->stmt) : 0;
    break;
  case SQL_DIAG_DYNAMIC_FUNCTION:
    if (!Stmt)
      return SQL_ERROR;
    /* Todo */
    break;
  case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
    if (!Stmt)
      return SQL_ERROR;
    *(SQLINTEGER *)DiagInfoPtr= 0;
    break;
  case SQL_DIAG_NUMBER:
    *(SQLINTEGER *)DiagInfoPtr= 1;
    break;
  case SQL_DIAG_RETURNCODE:
    *(SQLRETURN *)DiagInfoPtr= Err->ReturnValue;
    break;
  case SQL_DIAG_ROW_COUNT:
    if (HandleType != SQL_HANDLE_STMT ||
        !Stmt)
      return SQL_ERROR;
    *(SQLLEN *)DiagInfoPtr= (Stmt->stmt) ? (SQLLEN)mysql_stmt_affected_rows(Stmt->stmt) : 0;
    break;
  case SQL_DIAG_CLASS_ORIGIN:
    Length= MADB_SetString(isWChar ?  &utf8 : 0, DiagInfoPtr,  isWChar ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                     strncmp(Err->SqlState, "IM", 2)== 0 ? "ODBC 3.0" : "ISO 9075", SQL_NTS, &Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLSMALLINT)Length;
    break;
  case SQL_DIAG_COLUMN_NUMBER:
    *(SQLINTEGER *)DiagInfoPtr= SQL_COLUMN_NUMBER_UNKNOWN;
    break;
  case SQL_DIAG_CONNECTION_NAME:
    /* MariaDB ODBC Driver always returns an empty string */
    if (StringLengthPtr)
      *StringLengthPtr= 0;
    DiagInfoPtr= (isWChar) ? (SQLPOINTER)L"" : (SQLPOINTER)"";
    break;
  case SQL_DIAG_MESSAGE_TEXT:
    Length= MADB_SetString(isWChar ?  &utf8 : 0, DiagInfoPtr,  isWChar ? BufferLength / sizeof(SQLWCHAR) : BufferLength,
                                     Err->SqlErrorMsg, strlen(Err->SqlErrorMsg), &Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLSMALLINT)Length;
    break;
  case SQL_DIAG_NATIVE:
    *(SQLINTEGER *)DiagInfoPtr= Err->NativeError;
    break;
  case SQL_DIAG_ROW_NUMBER:
     if (HandleType != SQL_HANDLE_STMT ||
         RecNumber < 1)
       return SQL_ERROR;
      *(SQLLEN*)DiagInfoPtr= SQL_ROW_NUMBER_UNKNOWN;
    break;
  case SQL_DIAG_SERVER_NAME:
    {
      char *ServerName= "";
      if (Stmt && Stmt->stmt)
      {
        mariadb_get_infov(Stmt->stmt->mysql, MARIADB_CONNECTION_HOST, (void*)&ServerName);
      }
      else if (Dbc && Dbc->mariadb)
      {
        mariadb_get_infov(Dbc->mariadb, MARIADB_CONNECTION_HOST, (void*)&ServerName);
      }
      Length= MADB_SetString(isWChar ?  &utf8 : 0, DiagInfoPtr, 
                                        isWChar ? BufferLength / sizeof(SQLWCHAR) : BufferLength, 
                                        ServerName ? ServerName : "", ServerName ? strlen(ServerName) : 0, &Error);
      if (StringLengthPtr)
        *StringLengthPtr= (SQLSMALLINT)Length;
    }
    break;
  case SQL_DIAG_SQLSTATE:
    Length= MADB_SetString(isWChar ?  &utf8 : 0, DiagInfoPtr, 
                           isWChar ? BufferLength / sizeof(SQLWCHAR) : BufferLength, Err->SqlState, strlen(Err->SqlState), &Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLSMALLINT)Length;
   
    break;
  case SQL_DIAG_SUBCLASS_ORIGIN:
    Length= MADB_SetString(isWChar ?  &utf8 : 0, DiagInfoPtr, 
                           isWChar ? BufferLength / sizeof(SQLWCHAR) : BufferLength, "ODBC 3.0", 8, &Error);
    if (StringLengthPtr)
      *StringLengthPtr= (SQLSMALLINT)Length;
    break;
  default:
    return SQL_ERROR;
  }
  if (isWChar && StringLengthPtr)
    *StringLengthPtr*= sizeof(SQLWCHAR);
  return Error.ReturnValue;
}
