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


#ifndef _SERVERPREPARERESULT_H_
#define _SERVERPREPARERESULT_H_

#include <vector>
#include <memory>
#include "mysql.h"
#include "PrepareResult.h"
#include "SQLString.h"


namespace odbc
{
namespace mariadb
{

//class ParameterHolder;

class ServerPrepareResult  : public PrepareResult
{
  ServerPrepareResult(const ServerPrepareResult&)= delete;
  const SQLString sql;
  MYSQL_STMT* statementId;
  /*std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> metadata;*/
  unsigned long paramCount= 0;
  MYSQL_BIND* paramBind;
  MYSQL* connection;

public:
  ~ServerPrepareResult();

  /*ServerPrepareResult(
    const SQLString& sql,
    MYSQL_STMT* statementId,
    MYSQL_FIELD* columns,
    MYSQL* dbc);*/

  ServerPrepareResult(
    const SQLString& sql,
    MYSQL_STMT* statementId,
    MYSQL* dbc);

  void reReadColumnInfo();

  //void resetParameterTypeHeader();
  size_t getParamCount() const;
  MYSQL_STMT* getStatementId();
  const MYSQL_FIELD* getFields() const;
  const std::vector<ColumnDefinition>& getColumns() const;
  const MYSQL_BIND* getParameters() const;
  const SQLString& getSql() const;
  ResultSetMetaData* getEarlyMetaData();
  /*void bindParameters(std::vector<Unique::ParameterHolder>& parameters);
  void bindParameters(MYSQL_BIND* parameters, const int16_t *type= nullptr);*/
  };

namespace Unique
{
  typedef std::unique_ptr<odbc::mariadb::ServerPrepareResult> ServerPrepareResult;
}
}
}
#endif
