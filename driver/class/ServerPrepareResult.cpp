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

#include "mysql.h"

#include "ServerPrepareResult.h"
#include "ResultSetMetaData.h"
//#include "ColumnType.h"
//#include "ColumnDefinition.h"

#include "ColumnDefinition.h"

namespace odbc
{
namespace mariadb
{
  ServerPrepareResult::~ServerPrepareResult()
  {
    mysql_stmt_close(statementId);
  }
  /**
    * PrepareStatement Result object.
    *
    * @param sql query
    * @param statementId server statement Id.
    * @param columns columns information
    * @param parameters parameters information
    * @param unProxiedProtocol indicate the protocol on which the prepare has been done
    */
  /*ServerPrepareResult::ServerPrepareResult(
    const SQLString& _sql,
    MYSQL_STMT* _statementId,
    std::vector<ColumnDefinition>& _columns,
    std::vector<ColumnDefinition>& _parameters
    )
    : sql(_sql)
    , statementId(_statementId)
    , columns(_columns)
    , parameters(_parameters)
    , metadata(mysql_stmt_result_metadata(statementId), &mysql_free_result)
  {
  }*/

  /**
  * PrepareStatement Result object.
  *
  * @param sql query
  * @param statementId server statement Id.
  * @param columns columns information
  * @param parameters parameters information
  * @param unProxiedProtocol indicate the protocol on which the prepare has been done
  */
  ServerPrepareResult::ServerPrepareResult(
    const SQLString& _sql,
    MYSQL_STMT* _statementId,
    MYSQL* dbc)
    :
      sql(_sql)
    , statementId(_statementId)
    , connection (dbc)
    , paramCount(mysql_stmt_param_count(statementId))
  {
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> metadata(mysql_stmt_result_metadata(statementId), &mysql_free_result);
    if (metadata) {
      init(mysql_fetch_fields(metadata.get()), mysql_stmt_field_count(statementId));
    }
    paramBind= nullptr;
  }


  void ServerPrepareResult::reReadColumnInfo()
  {
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> metadata(mysql_stmt_result_metadata(statementId), &mysql_free_result);
    column.clear();
    field.clear();
    init(mysql_fetch_fields(metadata.get()), mysql_stmt_field_count(statementId));
  }

  //void ServerPrepareResult::resetParameterTypeHeader()
  //{
  //  paramBind.clear();

  //  if (paramCount > 0) {
  //    paramBind.resize(paramCount);
  //  }
  //}

  size_t ServerPrepareResult::getParamCount() const
  {
    return paramCount;
  }


  MYSQL_STMT* ServerPrepareResult::getStatementId()
  {
    return statementId;
  }

  const MYSQL_FIELD* ServerPrepareResult::getFields() const
  {
    return *field.data();//reinterpret_cast<const MYSQL_FIELD*>(field.data());
  }

  const std::vector<ColumnDefinition>& ServerPrepareResult::getColumns() const
  {
    return column;
  }

  const SQLString& ServerPrepareResult::getSql() const
  {
    return sql;
  }


  ResultSetMetaData* ServerPrepareResult::getEarlyMetaData()
  {
    return new ResultSetMetaData(column);
  }


  /*void initBindStruct(MYSQL_BIND& bind, const MYSQL_BIND* paramInfo)
  {
    std::memset(&bind, 0, sizeof(bind));

    bind.buffer_type= static_cast<enum_field_types>(typeInfo.getType());
    bind.is_null= &bind.is_null_value;
    if (paramInfo.isUnsigned()) {
      bind.is_unsigned = '\1';
    }
  }*/

  /*void bindParamValue(MYSQL_BIND& bind)
  {
    bind.is_null_value= '\0';
    bind.long_data_used= '\0';

    if (param->isNullData()) {
      bind.is_null_value= '\1';
      return;
    }

    if (param->isLongData()) {
      bind.long_data_used= '\1';
      return;
    }

    if (param->isUnsigned()) {
      bind.is_unsigned= '\1';
    }

    bind.buffer= param->getValuePtr();
    bind.buffer_length=param->getValueBinLen();
  }*/

  //void ServerPrepareResult::bindParameters(std::vector<Unique::ParameterHolder>& paramValue)
  //{
  //  for (std::size_t i= 0; i < paramCount; ++i)
  //  {
  //    auto& bind= paramBind[i];

  //    initBindStruct(bind, *paramValue[i]);
  //    bindParamValue(bind, paramValue[i]);
  //  }
  //  mysql_stmt_bind_param(statementId, paramBind.data());
  //}


  //uint8_t paramRowUpdateCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr)
  //{
  //  static char indicator[]{'\0', STMT_INDICATOR_NULL};
  //  std::size_t i= 0;
  //  std::vector<Unique::ParameterHolder>& paramSet= (*static_cast<std::vector<std::vector<Unique::ParameterHolder>>*>(data))[row_nr];

  //  for (auto& param : paramSet) {
  //    if (param->isNullData()) {
  //      bind[i].u.indicator= &indicator[1];
  //      ++i;
  //      continue;
  //    }
  //    bind[i].u.indicator = &indicator[0];
  //    if (param->isUnsigned()) {
  //      bind[i].is_unsigned = '\1';
  //    }

  //    bind[i].buffer = param->getValuePtr();
  //    bind[i].buffer_length = param->getValueBinLen();
  //    ++i;
  //  }
  //  return '\0';
  //}
 

  //void ServerPrepareResult::bindParameters(MYSQL_BIND* paramValue, const int16_t* type)
  //{
  //  uint32_t i= 0;
  //  for (auto& bind : paramBind)
  //  {
  //    // Initing with first row param data
  //    initBindStruct(bind, paramValue[i]);
  //    if (type != nullptr) {
  //      bind.buffer_type= static_cast<enum_field_types>(type[i]);
  //    }
  //    ++i;
  //  }
  //  
  //  mysql_stmt_attr_set(statementId, STMT_ATTR_CB_USER_DATA, &paramValue);
  //  mysql_stmt_attr_set(statementId, STMT_ATTR_CB_PARAM, (const void*)&paramRowUpdateCallback);
  //  mysql_stmt_bind_param(statementId, paramBind);
  //}
}
}
