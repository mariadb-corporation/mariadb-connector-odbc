/************************************************************************************
   Copyright (C) 2013,2018 MariaDB Corporation AB
   
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
                            MADB_NOT_ATOMIC_BLOCK
};

typedef struct {
  char *QueryText;
  enum enum_madb_query_type QueryType;
  MADB_DynArray ParamPos;
} SINGLE_QUERY;

typedef struct {
  char        * Original;
  char        * allocated; /* Pointer to the allocated area. The refined query may go to the right */
  char        * RefinedText;
  size_t        RefinedLength;
  MADB_DynArray Tokens;
  MADB_DynArray SubQuery; /* List of queries or batches of queries, that can be executed together at once */
  /* So far only falg whether we have any parameters */
  my_bool       HasParameters;
  /* This is more for multistatements for optimization - if none of queries returns result,
     we can send them via text protocol */
  my_bool       ReturnsResult;
  enum enum_madb_query_type QueryType;
  my_bool       PoorManParsing;

  my_bool       BatchAllowed;
  my_bool       AnsiQuotes;
  my_bool       NoBackslashEscape;

} MADB_QUERY;

#define PQUERY_UPDATE_LEN(PARSED_QUERY_PTR) (PARSED_QUERY_PTR)->RefinedLength= strlen((PARSED_QUERY_PTR)->RefinedLength)
#define STMT_COUNT(PARSED_QUERY) ((PARSED_QUERY).SubQuery.elements/* + 1*/)
#define QUERY_IS_MULTISTMT(PARSED_QUERY) (STMT_COUNT(PARSED_QUERY) > 1)

int  MADB_ResetParser(MADB_Stmt *Stmt, char *OriginalQuery, SQLINTEGER OriginalLength);
void MADB_DeleteSubqueries(MADB_QUERY *Query);
void MADB_AddSubQuery(MADB_QUERY *Query, char *SubQueryText, enum enum_madb_query_type QueryType);

void MADB_DeleteQuery(MADB_QUERY *Query);
int  MADB_ParseQuery(MADB_QUERY *Query);

#define QUERY_DOESNT_RETURN_RESULT(query_type) ((query_type) < MADB_QUERY_SELECT)

char *       MADB_ParseCursorName(MADB_QUERY *Query, unsigned int *Offset);
unsigned int MADB_FindToken(MADB_QUERY *Query, char *Compare);

enum enum_madb_query_type MADB_GetQueryType(const char *Token1, const char *Token2);

const char * MADB_FindParamPlaceholder(MADB_Stmt *Stmt);
char *       FixIsoFormat(char * StmtString, size_t *Length);
int          ParseQuery(MADB_QUERY *Query);
char *       StripLeadingComments(char *s, size_t *Length, BOOL OverWrite);

#endif /* _ma_parse_h_ */
