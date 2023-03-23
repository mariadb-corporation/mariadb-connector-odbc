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


#ifndef _COLUMNINFORMATION_H_
#define _COLUMNINFORMATION_H_

#include <memory>
#include <map>

#include "mysql.h"
#include "SQLString.h"


#define MARIADB_BINARY_TYPES MYSQL_TYPE_BLOB:\
case MYSQL_TYPE_TINY_BLOB:\
case MYSQL_TYPE_MEDIUM_BLOB:\
case MYSQL_TYPE_LONG_BLOB


#define MARIADB_INTEGER_TYPES MYSQL_TYPE_LONG:\
case MYSQL_TYPE_LONGLONG:\
case MYSQL_TYPE_SHORT:\
case MYSQL_TYPE_INT24

#define MAX_TINY_BLOB   0xFF
#define MAX_BLOB_LEN    0xFFFF
#define MAX_MEDIUM_BLOB 0xFFFFFF

namespace odbc
{
namespace mariadb
{

//extern const std::map<enum enum_field_types, SQLString> typeName;
SQLString columnTypeName(enum enum_field_types type, int64_t len, int64_t charLen, bool _signed, bool binary);

class ColumnDefinition
{
  static uint8_t maxCharlen[];
  MYSQL_FIELD* metadata;
  SQLString name;
  SQLString org_name;
  SQLString table;
  SQLString org_table;
  SQLString db;

  /** For "hand-made" RS we need to take care of freeing memory, while for "natural" MYSQL_FIELD
    we have to use pointer to C/C structures(to automatically get max_length when it's calculated -
    that happens later, than the object is created).
    It has to be shared since we have copy-copyconstructor
  */
  bool owned= false;
  //const ColumnType& type;
  uint32_t length= 0;
  //SQLString db;

  void refreshPointers();

public:
  static ColumnDefinition ColumnDefinition::create(const SQLString& name, const MYSQL_FIELD* _type);

  ~ColumnDefinition();
  ColumnDefinition(const ColumnDefinition& other);
  ColumnDefinition::ColumnDefinition(const ColumnDefinition&& other);
  ColumnDefinition(const SQLString name, const MYSQL_FIELD* metadata, bool ownshipPassed= false);
  ColumnDefinition(const MYSQL_FIELD* field, bool ownshipPassed= true);
  ColumnDefinition& operator=(const ColumnDefinition& other);

public:
  SQLString getDatabase() const;
  SQLString getTable() const;
  SQLString getOriginalTable() const;
  SQLString getName() const;
  SQLString getOriginalName() const;
  int16_t getCharsetNumber() const;
  SQLString getCollation() const;
  uint32_t getLength() const;
  uint32_t getMaxLength() const;
  int64_t getPrecision() const;
  uint32_t getDisplaySize() const;
  uint8_t getDecimals() const;
  const MYSQL_FIELD* getColumnRawData() const { return metadata; }
  enum enum_field_types getColumnType() const { return metadata->type; }
  int16_t getFlags() const;
  bool isSigned() const;
  bool isNotNull() const;
  bool isPrimaryKey() const;
  bool isUniqueKey() const;
  bool isMultipleKey() const;
  bool isBlob() const;
  bool isZeroFill() const;
  bool isBinary() const;
  bool isReadonly() const;

  SQLString getColumnTypeName() const;
};

}
}
#endif
