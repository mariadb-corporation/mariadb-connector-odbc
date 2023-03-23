/************************************************************************************
   Copyright (C) 2013,2023 MariaDB Corporation AB
   
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

#include <vector>
#include <array>
#include <sqlext.h>
#include "SQLString.h"

enum enum_madb_query_type { MADB_QUERY_NO_RESULT= 0, /* Default type for the query types we are not interested in */ 
                            MADB_QUERY_INSERT,
                            MADB_QUERY_UPDATE= SQL_UPDATE,
                            MADB_QUERY_DELETE= SQL_DELETE,
                            MADB_QUERY_CREATE_PROC,
                            MADB_QUERY_CREATE_FUNC,
                            MADB_QUERY_CREATE_DEFINER,
                            MADB_QUERY_SET,
                            MADB_QUERY_SET_NAMES,
                            MADB_QUERY_SELECT,
                            MADB_QUERY_SHOW,
                            MADB_QUERY_CALL,
                            MADB_QUERY_ANALYZE,
                            MADB_QUERY_EXPLAIN,
                            MADB_QUERY_CHECK,
                            MADB_QUERY_EXECUTE,
                            MADB_QUERY_DESCRIBE,
                            MADB_NOT_ATOMIC_BLOCK,
                            MADB_QUERY_OPTIMIZE
};

typedef struct {
  char *QueryText;
  enum enum_madb_query_type QueryType;
  MADB_DynArray ParamPos;
} SINGLE_QUERY;

 struct MADB_QUERY
 {
  std::vector<std::size_t> Tokens;
  SQLString     Original;
  SQLString     RefinedText;
  enum enum_madb_query_type QueryType= MADB_QUERY_NO_RESULT;
  bool          MultiStatement= false;
  /* Keeping it so far */
  bool       ReturnsResult= false;
  bool       PoorManParsing= false;

  bool       BatchAllowed= false;
  bool       AnsiQuotes= false;
  bool       NoBackslashEscape= false;

  MADB_QUERY()
    : Tokens()
  {}
  void reset();
};

#define PQUERY_UPDATE_LEN(PARSED_QUERY_PTR) (PARSED_QUERY_PTR)->RefinedLength= strlen((PARSED_QUERY_PTR)->RefinedLength)
#define QUERY_IS_MULTISTMT(PARSED_QUERY) (PARSED_QUERY.MultiStatement)

int  MADB_ResetParser(MADB_Stmt *Stmt, char *OriginalQuery, SQLINTEGER OriginalLength);

int  MADB_ParseQuery(MADB_QUERY *Query);

#define QUERY_DOESNT_RETURN_RESULT(query_type) ((query_type) < MADB_QUERY_SELECT)

const char * MADB_ParseCursorName(MADB_QUERY *Query, unsigned int *Offset);
unsigned int MADB_FindToken(MADB_QUERY *Query, char *Compare);

enum enum_madb_query_type MADB_GetQueryType(const char *Token1, const char *Token2);

const char * MADB_FindParamPlaceholder(MADB_Stmt *Stmt);
SQLString &  FixIsoFormat(SQLString & StmtString);
int          ParseQuery(MADB_QUERY *Query);
const char * StripLeadingComments(const char *s, std::size_t *Length, bool OverWrite= false);

#endif /* _ma_parse_h_ */
