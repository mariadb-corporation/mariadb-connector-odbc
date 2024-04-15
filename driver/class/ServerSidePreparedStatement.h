/************************************************************************************
   Copyright (C) 2022 MariaDB Corporation AB

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


#ifndef _SERVERSIDEPREPAREDSTATEMENT_H_
#define _SERVERSIDEPREPAREDSTATEMENT_H_

#include "ServerPrepareResult.h"
#include "PreparedStatement.h"


namespace mariadb
{

  extern "C"
  {
    // C/C's callback running param column callbacks 
    my_bool* defaultParamCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr);
    // C/C's callback running callback for the row, and then indivivual param column callbacks
    my_bool* withRowCheckCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr);
  }

class ServerSidePreparedStatement : public PreparedStatement
{
  friend my_bool* withRowCheckCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr);
  friend my_bool* defaultParamCallback(void* data, MYSQL_BIND* binds, uint32_t row_nr);

  ServerPrepareResult* serverPrepareResult= nullptr;
//  ps_param_callback    capiParamCallback=   nullptr;
  bool paramCallbackSet= false;
  //This probably won't be needed either
  ps_result_callback   capiResultCallback=  nullptr;

public:
  ~ServerSidePreparedStatement();
  ServerSidePreparedStatement(Protocol* connection, const SQLString& sql, int32_t resultSetScrollType);
  ServerSidePreparedStatement(Protocol* connection, ServerPrepareResult* pr, int32_t resultSetScrollType);

  ServerSidePreparedStatement* clone(Protocol* connection);

private:
  ServerSidePreparedStatement(
    Protocol* connection,
    int32_t resultSetScrollType
    );

  void prepare(const SQLString& sql);
  void setMetaFromResult();

public:
  //void setParameter(int32_t parameterIndex,/*const*/ ParameterHolder* holder);
  //ParameterMetaData* getParameterMetaData();
  ResultSetMetaData* getMetaData();

private:
  void executeBatchInternal(uint32_t queryParameterSize);
  void executeQueryPrologue(ServerPrepareResult* serverPrepareResult);
  void getResult();

public:
  PrepareResult* getPrepareResult() { return dynamic_cast<PrepareResult*>(serverPrepareResult); }
  bool executeInternal(int32_t fetchSize);
  uint32_t fieldCount() const;

  void close();

  const char* getError();
  uint32_t    getErrno();
  const char* getSqlState();

  bool bind(MYSQL_BIND* param);
  bool sendLongData(uint32_t paramNum, const char* data, std::size_t length);
  inline bool isServerSide() const { return true; }
  enum enum_field_types getPreferredParamType(enum enum_field_types appType) const
  {
    return appType;
  }
  bool hasMoreResults();
  void moveToNextResult();

  bool setParamCallback(ParamCodec* callback, uint32_t param= uint32_t(-1));
  bool setResultCallback(result_callback callback, uint32_t column);
  bool setCallbackData(void* data);
  };
}
#endif
