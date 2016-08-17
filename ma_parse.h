/************************************************************************************
   Copyright (C) 2013 SkySQL AB
   
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
#ifndef _ma_parse_h_
#define _ma_parse_h_

#include <sqlext.h>

enum enum_madb_query_type { MADB_QUERY_NO_RESULT= 0, /* Default type for the query types we are not interested in */ 
                            MADB_QUERY_INSERT,
                            MADB_QUERY_UPDATE= SQL_UPDATE,
                            MADB_QUERY_DELETE= SQL_DELETE,
                            MADB_QUERY_SELECT,
                            MADB_QUERY_SHOW,
                            MADB_QUERY_CALL
                          };

#define QUERY_DOESNT_RETURN_RESULT(query_type) ((query_type) < MADB_QUERY_SELECT)

MADB_QUERY * MADB_Tokenize(const char *Stmt);
char *       MADB_ParseCursorName(MADB_Stmt *Stmt, unsigned int *Offset);
unsigned int MADB_FindToken(MADB_Stmt *Stmt, char *Compare);
void         MADB_FreeTokens(MADB_QUERY *Query);

enum enum_madb_query_type MADB_GetQueryType(MADB_Stmt *Stmt);

const char * MADB_FindParamPlaceholder(MADB_Stmt *Stmt);
char *       FixIsoFormat(char * StmtString);
#endif /* _ma_parse_h_ */
