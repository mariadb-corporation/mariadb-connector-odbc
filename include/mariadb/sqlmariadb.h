/************************************************************************************
   Copyright (C) 2025 MariaDB Corporation plc
   
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

/* MariaDB specific ODBC data types, diagnoctice, connection und statement attributes */
#ifndef _SQLMARIADB_h_
#define _SQLMARIADB_h_

#define SQL_MARIADB_SQL_TYPE_BASE       25100
#define SQL_MARIADB_DESCRIPTOR_BASE     25100
#define SQL_MARIADB_DIAGNOSTIC_BASE     25100
#define SQL_MARIADB_INFO_TYPE_BASE      25100
#define SQL_MARIADB_CONNECT_ATTR_BASE   25100
#define SQL_MARIADB_STATEMENT_ATTR_BASE 25100

#define SQL_ATTR_EXECDIRECT_ON_SERVER (SQL_MARIADB_CONNECT_ATTR_BASE + 0)
#define SQL_ATTR_PREPARE_ON_CLIENT (SQL_MARIADB_CONNECT_ATTR_BASE + 1)

#endif /* _SQLMARIADB_h_ */
