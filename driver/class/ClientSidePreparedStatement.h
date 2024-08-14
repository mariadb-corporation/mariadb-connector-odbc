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


#ifndef _CLIENTSIDEPREPAREDSTATEMENT_H_
#define _CLIENTSIDEPREPAREDSTATEMENT_H_

#include <map>

#include "PreparedStatement.h"
#include "ClientPrepareResult.h"


namespace mariadb
{
class ResultSetMetaData;

class ClientSidePreparedStatement : public PreparedStatement
{
  Unique::ClientPrepareResult prepareResult;
  bool noBackslashEscapes= false;
  std::map<uint32_t, std::string> longData;

  ClientSidePreparedStatement(
    Protocol* _connection,
    int32_t resultSetScrollType,
    bool _noBackslashEscapes
    );
public:
  ClientSidePreparedStatement(
    Protocol* _connection,
    const SQLString& sql,
    int32_t resultSetScrollType,
    bool _noBackslashEscapes
    );
  ~ClientSidePreparedStatement();
  ClientSidePreparedStatement* clone(Protocol* connection);

protected:
  bool executeInternal(int32_t fetchSize);
  void executeBatchInternal(uint32_t queryParameterSize);
  PrepareResult* getPrepareResult();
  uint32_t fieldCount() const;

public:
  Longs& executeBatch();
  Longs& getServerUpdateCounts();
  ResultSetMetaData* getMetaData();
  /*ParameterMetaData* getParameterMetaData();*/

private:
  void loadParametersData();
  void executeQueryPrologue(bool isBatch= false);
  void getSingleResult();
  void getResult(bool getAllResults= false);
    ;
public:
  void close();
  uint32_t getParameterCount();

  const char* getError();
  uint32_t    getErrno();
  const char* getSqlState();

  bool bind(MYSQL_BIND* param);
  bool sendLongData(uint32_t paramNum, const char* data, std::size_t length);
  inline bool isServerSide() const { return false; }
  inline enum enum_field_types getPreferredParamType(enum enum_field_types appType) const
  { 
    return MYSQL_TYPE_STRING;
  }
  bool hasMoreResults();
  bool getMoreResults();
  bool setParamCallback(ParamCodec* callback, uint32_t param) { return true; }
  bool setCallbackData(void* data) { return true;  }
  bool isOutParams() { return false; }
  //SQLString toString();
};
}
#endif
