/************************************************************************************
   Copyright (C) 2013,2016 MariaDB Corporation AB
   
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
#include <ma_odbc.h>

const char *MADB_GetToken(const char **Stmt, const char *End)
{
  const char *Pos= *Stmt;

  while (iswspace(*Pos) && Pos < End)
    Pos++;
  /* if we reached end of string, return */
  if (Pos == End)
    return (*Stmt= End);
  *Stmt= Pos;

  while (*Stmt < End && !iswspace(**Stmt))
    ++*Stmt;

  while (*Stmt < End && iswspace(**Stmt))
    ++*Stmt;
  return *Stmt;
}

MADB_QUERY *MADB_Tokenize(const char *Stmt)
{
  const char *End= (char *)Stmt + strlen(Stmt);
  const char *Pos;
  const char *Start= Stmt;

  MADB_QUERY *Query= (MADB_QUERY *)MADB_CALLOC(sizeof(MADB_QUERY));
  init_dynamic_array(&Query->tokens, sizeof(unsigned int), 20, 20);

  while ((Pos = MADB_GetToken(&Stmt, End)) != End)
  {
    unsigned int Offset= (unsigned int)(Stmt - Start);
    insert_dynamic(&Query->tokens, (gptr)&Offset);
    Stmt= Pos;
  } 
  return Query; 
}

void MADB_FreeTokens(MADB_QUERY *Query)
{
  if (!Query)
    return;
  delete_dynamic(&Query->tokens);
  MADB_FREE(Query);
}

char *MADB_Token(MADB_Stmt *Stmt, unsigned int Idx)
{
  char *p;
  unsigned int Offset= 0;
  DYNAMIC_ARRAY *Tokens;
  
  p= Stmt->StmtString;
  if (!Stmt->Tokens || !p)
    return NULL;
  Tokens= &Stmt->Tokens->tokens;
  if (Idx >= Tokens->elements)
    return NULL;
  get_dynamic(Tokens, (gptr)&Offset, Idx);
  return Stmt->StmtString + Offset;  
}

my_bool MADB_CompareToken(MADB_Stmt *Stmt, unsigned int Idx, char *Compare, size_t Length, unsigned int *Offset)
{
  char *TokenString;
  
  if (!(TokenString= MADB_Token(Stmt, Idx)))
    return FALSE;
  if (_strnicmp(TokenString, Compare, Length) == 0)
  {
    if (Offset)
      *Offset= (unsigned int)(TokenString - Stmt->StmtString);
    return TRUE;
  }
 
  return FALSE;
}

unsigned int MADB_FindToken(MADB_Stmt *Stmt, char *Compare)
{
  unsigned int i, TokenCount= Stmt->Tokens->tokens.elements;
  unsigned int Offset= 0;

  for (i=0; i < TokenCount; i++)
  {
    if (MADB_CompareToken(Stmt, i, Compare, strlen(Compare), &Offset))
      return Offset;
  }
  return 0;
}

char *MADB_ParseCursorName(MADB_Stmt *Stmt, unsigned int *Offset)
{
  unsigned int i,
               TokenCount= Stmt->Tokens->tokens.elements;

  if (TokenCount < 4)
    return NULL;
  for (i=0; i < TokenCount - 3; i++)
  {
    if (MADB_CompareToken(Stmt, i, "WHERE", 5, Offset) &&
        MADB_CompareToken(Stmt, i+1, "CURRENT", 7, 0) &&
        MADB_CompareToken(Stmt, i+2, "OF", 2, 0))
    {
      return MADB_Token(Stmt, i + 3);
    }
  }
  return NULL;
}


const char * SimpleParamDetector(const char *query)
{
  return strchr(query, '?');
}


const char * MADB_FindParamPlaceholder(MADB_Stmt *Stmt)
{ 
  /* Atm we do not need 100% accuracy with that. And unlikely will */
  return SimpleParamDetector(Stmt->StmtString);
}


enum enum_madb_query_type MADB_GetQueryType(MADB_Stmt *Stmt)
{
  char *p= Stmt->StmtString;

  while (*p && !isalpha(*p))
    ++p;

  if (_strnicmp(p, "SELECT", 6) == 0)
  {
    return MADB_QUERY_SELECT;
  }
  if (_strnicmp(p, "UPDATE", 6) == 0)
  {
    return MADB_QUERY_UPDATE;
  }
  if (_strnicmp(p, "DELETE", 6) == 0)
  {
    return MADB_QUERY_DELETE;
  }
  if (_strnicmp(p, "CALL", 4) == 0)
  {
    return MADB_QUERY_CALL;
  }
  if (_strnicmp(p, "SHOW", 4) == 0)
  {
    return MADB_QUERY_SHOW;
  }

  return MADB_QUERY_NO_RESULT;
}


char* FixIsoFormat(char * StmtString)
{
  size_t len;

  StmtString= trim(StmtString);
  
  len= strlen(StmtString);

  if (StmtString[0] == '{' && StmtString[len -1] == '}')
  {
    StmtString[0]=       ' ';
    StmtString[len - 1]= '\0';

    return trim(StmtString);
  }

  return StmtString;
}
