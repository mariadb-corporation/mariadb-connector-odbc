/************************************************************************************
   Copyright (C) 2016, 2023 MariaDB Corporation AB
   
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

/* Server-dependent functionality and helpers to use that functionality */

#include <ma_odbc.h>

unsigned long VersionCapabilityMap[][2]= {{100202, MADB_CAPABLE_EXEC_DIRECT},
                                          {100207, MADB_ENCLOSES_COLUMN_DEF_WITH_QUOTES},
                                          {110101, MADB_MYSQL_TRANSACTION_ISOLATION}};
unsigned long MySQLVersionCapabilityMap[][2]= {{50720, MADB_MYSQL_TRANSACTION_ISOLATION}};
/*                                               {50704, MADB_SESSION_TRACKING}*/
unsigned long CapabilitiesMap[][2]= {{CLIENT_SESSION_TRACKING, MADB_SESSION_TRACKING}};
unsigned long ExtCapabilitiesMap[][2]= {{MARIADB_CLIENT_STMT_BULK_OPERATIONS >> 32, MADB_CAPABLE_PARAM_ARRAYS}};

/* {{{  */
void MADB_SetCapabilities(MADB_Dbc *Dbc, unsigned long ServerVersion, const char* ServerName)
{
  int i;
  unsigned long ServerCapabilities, ServerExtCapabilities;

  Dbc->IsMySQL= (strcmp(ServerName, "MySQL") == 0);
  /*Dbc->IsXpand= (strcmp(ServerName, "Xpand") == 0);*/

  if (Dbc->IsMySQL == 0)
  {
    for (i = 0; i < sizeof(VersionCapabilityMap) / sizeof(VersionCapabilityMap[0]); ++i)
    {
      if (ServerVersion >= VersionCapabilityMap[i][0])
      {
        Dbc->ServerCapabilities|= VersionCapabilityMap[i][1];
      }
    }
  }
  else
  {
    for (i = 0; i < sizeof(MySQLVersionCapabilityMap) / sizeof(MySQLVersionCapabilityMap[0]); ++i)
    {
      if (ServerVersion >= MySQLVersionCapabilityMap[i][0])
      {
        Dbc->ServerCapabilities|= MySQLVersionCapabilityMap[i][1];
      }
    }
  }

  mariadb_get_infov(Dbc->mariadb, MARIADB_CONNECTION_SERVER_CAPABILITIES, (void*)&ServerCapabilities);
  for (i= 0; i < sizeof(CapabilitiesMap)/sizeof(CapabilitiesMap[0]); ++i)
  {
    if (ServerCapabilities & CapabilitiesMap[i][0])
    {
      Dbc->ServerCapabilities|= CapabilitiesMap[i][1];
    }
  }

  mariadb_get_infov(Dbc->mariadb, MARIADB_CONNECTION_EXTENDED_SERVER_CAPABILITIES, (void*)&ServerExtCapabilities);
  for (i= 0; i < sizeof(ExtCapabilitiesMap)/sizeof(ExtCapabilitiesMap[0]); ++i)
  {
    if (!(Dbc->mariadb->server_capabilities & CLIENT_MYSQL)
      && (ServerExtCapabilities & ExtCapabilitiesMap[i][0]))
    {
      Dbc->ServerCapabilities|= ExtCapabilitiesMap[i][1];
    }
  }
}
/* }}} */

/* {{{  */
BOOL MADB_ServerSupports(MADB_Dbc *Dbc, char Capability)
{
  return test(Dbc->ServerCapabilities & Capability);
}
/* }}} */

/* {{{  */
const char* MADB_GetTxIsolationQuery(MADB_Dbc* Dbc)
{
  return ((Dbc->ServerCapabilities & MADB_MYSQL_TRANSACTION_ISOLATION) != 0 ? "SELECT @@transaction_isolation" : "SELECT @@tx_isolation");
}
/* }}} */

/* {{{  */
const char* MADB_GetTxIsolationVarName(MADB_Dbc* Dbc)
{
  return ((Dbc->ServerCapabilities & MADB_MYSQL_TRANSACTION_ISOLATION) != 0 ? "transaction_isolation" : "tx_isolation");
}
/* }}} */


