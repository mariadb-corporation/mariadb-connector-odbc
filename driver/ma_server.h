/************************************************************************************
   Copyright (C) 2016 MariaDB Corporation AB
   
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

#ifndef _ma_server_h
#define _ma_server_h

#define MADB_CAPABLE_EXEC_DIRECT  1
#define MADB_CAPABLE_PARAM_ARRAYS 2
#define MADB_ENCLOSES_COLUMN_DEF_WITH_QUOTES 4
#define MADB_MYSQL_TRANSACTION_ISOLATION 8
#define MADB_SESSION_TRACKING 16

void MADB_SetCapabilities(MADB_Dbc *Dbc, unsigned long ServerVersion, const char* ServerName);
BOOL MADB_ServerSupports (MADB_Dbc *Dbc, char Capability);
const char* MADB_GetTxIsolationQuery(MADB_Dbc* Dbc);
const char* MADB_GetTxIsolationVarName(MADB_Dbc* Dbc);

#endif
