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

#include <ma_odbc.h>

/* Minimal query length when we tried to avoid full parsing */
#define QUERY_LEN_FOR_POOR_MAN_PARSING 32768

using namespace odbc::mariadb;

 const char* SkipSpacesAndComments(const char **CurPtr, size_t *Length, bool OverWrite= false)
{
  const char *End= *CurPtr + *Length, *Prev= nullptr, *NotTrimmed= nullptr;

  /* Making sure that we don't have leading whitespaces and/or comments,
  and the string begins from something meainingful */
  while (*CurPtr < End && *CurPtr != Prev)
  {
    Prev= *CurPtr;
    *CurPtr= StripLeadingComments(*CurPtr, Length, OverWrite);
    NotTrimmed= *CurPtr;
    *CurPtr= ::ltrim(*CurPtr);
    *Length-= *CurPtr - NotTrimmed;
  }

  return *CurPtr;
}


std::size_t SkipSpacesAndComments(SQLString& str)
{
  const char* moved= str.data();
  std::size_t offset, len= str.length();
  SkipSpacesAndComments(&moved, &len);
  offset= str.length() - len;
  if (offset > 0)
  {
    str.erase(str.begin(), str.begin() + offset);
  }
  return offset;
}


const char* SkipQuotedString(const char **CurPtr, const char *End, char Quote)
{
  while (*CurPtr < End && **CurPtr != Quote)
  {
    /* Skipping backslash and next character, if needed */
    if (**CurPtr == '\\')
    {
      ++*CurPtr;
      /* Taking care of the case, when backslash is at last position */
      if (*CurPtr == End)
      {
        break;
      }
    }
    ++*CurPtr;
  }

  return *CurPtr;
}


const char* SkipQuotedString_Noescapes(const char **CurPtr, const char *End, char Quote)
{
  while (*CurPtr < End && **CurPtr != Quote)
  {
    ++*CurPtr;
  }

  return *CurPtr;
}


int MADB_ResetParser(MADB_Stmt *Stmt, char *OriginalQuery, SQLINTEGER OriginalLength)
{
  Stmt->Query.reset();

  if (OriginalQuery != nullptr)
  {
    try
    {
      Stmt->Query.RefinedText.assign(OriginalQuery, OriginalLength);
    }
    catch (std::exception&)
    {
      return 1;
    }

    Stmt->Query.BatchAllowed=      DSN_OPTION(Stmt->Connection, MADB_OPT_FLAG_MULTI_STATEMENTS) ? '\1' : '\0';
    Stmt->Query.AnsiQuotes=        MADB_SqlMode(Stmt->Connection, MADB_ANSI_QUOTES);
    Stmt->Query.NoBackslashEscape= MADB_SqlMode(Stmt->Connection, MADB_NO_BACKSLASH_ESCAPES);
  }
 
  return 0;
}


void MADB_QUERY::reset()
{
  Original.assign("");
  RefinedText.assign("");
  Tokens.clear();
  PoorManParsing= ReturnsResult= false;
}

int MADB_ParseQuery(MADB_QUERY * Query)
{
   /* make sure we don't have trailing whitespace or semicolon */
  sqlRtrim(Query->RefinedText);
  odbc::mariadb::ltrim(Query->RefinedText);
  FixIsoFormat(Query->RefinedText);

  /* Making copy of "original" string, with minimal changes required to be able to execute */
  Query->Original.assign(Query->RefinedText);
  SkipSpacesAndComments(Query->RefinedText);

  return ParseQuery(Query);
}

/*----------------- Tokens stuff ------------------*/

const char *MADB_Token(MADB_QUERY *Query, std::size_t Idx)
{
  if (Query->Tokens.size() == 0 || Idx >= Query->Tokens.size())
    return nullptr;

  return Query->RefinedText.data() + Query->Tokens[Idx];
}


my_bool MADB_CompareToken(MADB_QUERY *Query, unsigned int Idx, char *Compare, size_t Length, unsigned int *Offset)
{
  const char *TokenString;
  
  if (!(TokenString= MADB_Token(Query, Idx)))
    return FALSE;
  if (_strnicmp(TokenString, Compare, Length) == 0)
  {
    if (Offset)
      *Offset= (unsigned int)(TokenString - Query->RefinedText.data());
    return TRUE;
  }
 
  return FALSE;
}

/* Not used atm, but may be useful */
unsigned int MADB_FindToken(MADB_QUERY *Query, char *Compare)
{
  unsigned int i;
  std::size_t TokenCount = Query->Tokens.size();
  unsigned int Offset= 0;

  for (i=0; i < TokenCount; i++)
  {
    if (MADB_CompareToken(Query, i, Compare, strlen(Compare), &Offset))
      return Offset;
  }
  return 0;
}


static const char * ParseCursorName(MADB_QUERY *Query, unsigned int *Offset)
{
  unsigned int i;
  std::size_t TokenCount= Query->Tokens.size();

  if (TokenCount < 4)
  {
    return nullptr;
  }
  for (i=0; i < TokenCount - 3; i++)
  {
    if (MADB_CompareToken(Query, i, "WHERE", 5, Offset) &&
      MADB_CompareToken(Query, i+1, "CURRENT", 7, 0) &&
      MADB_CompareToken(Query, i+2, "OF", 2, 0))
    {
      return MADB_Token(Query, i + 3);
    }
  }
  return nullptr;
}


static const char * PoorManCursorName(MADB_QUERY *Query, unsigned int *Offset)
{
  MADB_QUERY EndPiece;
  const char *Res, *str;
  std::size_t initOffset;

  /* We do poor man on long queries only, thus there is no need to check length */
  str= ::ltrim(Query->RefinedText.data() + Query->RefinedText.length() - MADB_MAX_CURSOR_NAME - 32/* "WHERE CURRENT OF" + spaces */);
  initOffset = str - Query->RefinedText.data();
  EndPiece.RefinedText.assign(str);
  /* As we did poor man parsing, we don't have full information about the query. Thus, parsing only this part at the end of the query -
     we need tockens, to check if we have WHERE CURRENT OF in usual way */
  if (ParseQuery(&EndPiece))
  {
    return nullptr;
  }

  /* Now looking for cursor name in usual way */
  Res= ParseCursorName(&EndPiece, Offset);

  /* Incrementing Offset with the offset of our part of the query */
  if (Res == nullptr)
  {
    return nullptr;
  }
  *Offset= (unsigned int)(*Offset + initOffset);
  return Query->RefinedText.data() + initOffset + (Res - EndPiece.RefinedText.data());
}


const char * MADB_ParseCursorName(MADB_QUERY *Query, unsigned int *Offset)
{
  if (Query->PoorManParsing)
  {
    return PoorManCursorName(Query, Offset);
  }

  return ParseCursorName(Query, Offset);
}


enum enum_madb_query_type MADB_GetQueryType(const char *Token1, const char *Token2)
{
  /* We need for the case when MS Access adds parenthesis around query - see ODBC-57*/
  while (*Token1 && !isalpha(*Token1))
    ++Token1;
  if (_strnicmp(Token1, "SELECT", 6) == 0 || _strnicmp(Token1, "WITH", 4) == 0)
  {
    return MADB_QUERY_SELECT;
  }
  if (_strnicmp(Token1, "INSERT", 6) == 0 || _strnicmp(Token1, "REPLACE", 7) == 0)
  {
    return MADB_QUERY_INSERT;
  }
  if (_strnicmp(Token1, "UPDATE", 6) == 0)
  {
    return MADB_QUERY_UPDATE;
  }
  if (_strnicmp(Token1, "DELETE", 6) == 0)
  {
    return MADB_QUERY_DELETE;
  }
  if (_strnicmp(Token1, "CALL", 4) == 0)
  {
    return MADB_QUERY_CALL;
  }
  if (_strnicmp(Token1, "SHOW", 4) == 0)
  {
    return MADB_QUERY_SHOW;
  }
  if (_strnicmp(Token1, "ANALYZE", 7) == 0)
  {
    return MADB_QUERY_ANALYZE;
  }
  if (_strnicmp(Token1, "EXPLAIN", 7) == 0)
  {
    return MADB_QUERY_EXPLAIN;
  }
  if (_strnicmp(Token1, "CHECK", 5) == 0)
  {
    return MADB_QUERY_CHECK;
  }
  if (_strnicmp(Token1, "EXECUTE", 7) == 0)
  {
    return MADB_QUERY_EXECUTE;
  }
  if (_strnicmp(Token1, "CREATE", 6) == 0)
  {
    if (_strnicmp(Token2, "PROCEDURE", 9) == 0)
    {
      return MADB_QUERY_CREATE_PROC;
    }
    if (_strnicmp(Token2, "FUNCTION", 8) == 0)
    {
      return MADB_QUERY_CREATE_FUNC;
    }
    if (_strnicmp(Token2, "DEFINER", 7) == 0)
    {
      return MADB_QUERY_CREATE_DEFINER;
    }
  }
  if (_strnicmp(Token1, "SET", 3) == 0)
  {
    if (_strnicmp(Token2, "NAMES", 5) == 0)
    {
      return MADB_QUERY_SET_NAMES;
    }
    else
    {
      return MADB_QUERY_SET;
    }
  }
  if (_strnicmp(Token1, "DESC", 4) == 0)
  {
    return MADB_QUERY_DESCRIBE;
  }
  if (_strnicmp(Token1, "BEGIN", 5) == 0 && _strnicmp(Token2, "NOT", 3) == 0)
  {
    return MADB_NOT_ATOMIC_BLOCK;
  }
  if (_strnicmp(Token1, "OPTIMIZE", 8) == 0
    /*&& (_strnicmp(Token2, "LOCAL", 5) || _strnicmp(Token2, "NO_WRITE_TO_BINLOG ", 5) || _strnicmp(Token2, "TABLE", 5)) == 0*/)
  {
    return MADB_QUERY_OPTIMIZE;
  }

  return MADB_QUERY_NO_RESULT;
}

/* -------------------- Tokens - End ----------------- */

/* Not used - rather a placeholder in case we need it */
const char * MADB_FindParamPlaceholder(MADB_Stmt *Stmt)
{
  return nullptr;
}

/* Function assumes that query string has been trimmed */
SQLString& FixIsoFormat(SQLString& StmtString)
{
  if (StmtString.length() > 1 && StmtString.front() == '{' && StmtString.back() == '}')
  {
    StmtString.erase(StmtString.begin());
    StmtString.erase(--StmtString.end());

    trim(StmtString);
  }

  return StmtString;
}

static BOOL ShouldWeTryPoorManParsing(MADB_QUERY *Query)
{
  return (Query->RefinedText.length() > QUERY_LEN_FOR_POOR_MAN_PARSING)
    && Query->RefinedText.find_first_of(';') == SQLString::npos
    && Query->RefinedText.find_first_of('?') == SQLString::npos;
}

int ParseQuery(MADB_QUERY *Query)
{
  const char* p = Query->RefinedText.data();
  char Quote;
  bool         ReadingToken= false;
  unsigned int StmtTokensCount= 0;
  std::size_t  Length= Query->RefinedText.length(), offset= 0;
  const char  *end= p + Length, *CurQuery= nullptr, *LastSemicolon= nullptr;
  enum enum_madb_query_type StmtType;

  Query->Tokens.reserve((unsigned int)MAX(Length / 32, 20));

  Query->PoorManParsing= ShouldWeTryPoorManParsing(Query);

  while (p < end)
  {
    if (!ReadingToken)
    {
      Length= end - p;
      SkipSpacesAndComments(&p, &Length, true);
      offset = p - Query->RefinedText.data();
      Query->Tokens.push_back(offset);
      
      ++StmtTokensCount;
      ReadingToken= true;

      /* On saving 1st statement's token, we are incrementing statements counter */
      if (StmtTokensCount == 1)
      {
        CurQuery= p;
      }
      /* Having 2 first tockens we can get statement type. And we need to know it for the case of multistatement -
         some statements may "legally" contain ';' */
      else if (StmtTokensCount == 2)
      {
        /* We are currently at 2nd token of statement, and getting previous token position from Tokens array*/
        StmtType= MADB_GetQueryType(MADB_Token(Query, Query->Tokens.size() - 2), p);

        Query->ReturnsResult= Query->ReturnsResult || !QUERY_DOESNT_RETURN_RESULT(StmtType);

        /* If we on first statement, setting QueryType*/
        if (Query->Tokens.size() == 2)
        {
          Query->QueryType= StmtType;
          if (Query->PoorManParsing)
          {
            return 0;
          }
        }
      }
      switch (*p)
      {
      /* If some of them is opening a string, on the fall-through next `quote` won't be set,
         as STRING_OR_COMMENT will be `true`. Likewise, if we are already in the string. But if we get hear,
         we are not supposed to be inside a string */
      case '"':
      case '\'':
      case '`':
      {
        const char *SavePosition;
        Quote= *p++;
        SavePosition= p; /* In case we go past eos while looking for ending quote */
        if (Query->NoBackslashEscape || Quote == '`' || /* Backtick works with ANSI_QUOTES */
          (Query->AnsiQuotes && Quote == '"'))/* In indetifier quotation backslash does not escape anything - CLI has error with that */
        {
          SkipQuotedString_Noescapes(&p, end, Quote);
        }
        else
        {
          SkipQuotedString(&p, end, Quote);
        }
        if (p >= end)
        {
          /* Basically we got ending quote here - possible in poor man case, when we look for cursor name starting from the position inside string.
             Other options are bad query of parsing error */
          p= SavePosition;
          ReadingToken= FALSE;
        }
        break;
      }
      case '?': /* This can break token(w/out space char), and be beginning of a token.
                   Thus we need it in both places */
        //Query->HasParameters= 1;
        /* Parameter placeholder is a complete token. And next one may begin right after it*/
        ReadingToken= FALSE;
        break;
      case ';':
        if (QueryIsPossiblyMultistmt(Query))
        {
          StmtTokensCount= 0;
        }
        ReadingToken= FALSE;
        /* We should not move pointer here */
        break;
      }
    }
    else
    {
      switch (*p)
      {
      case '?':
      case '"':
      case '\'':
      case '`':
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      case '-':
      case '#':
      case '/':
      case ';':
        ReadingToken= FALSE;
        /* We should not move pointer here, since this can be already beginning of new token */
        continue;
      default:
        break;
      }
    }

    ++p;
  }

  return 0;
}


const char * StripLeadingComments(const char *Str, std::size_t *Length, bool OverWrite)
{
  const char *Res= Str;
  int ClosingStrLen= 1;

  /* There is nothing to strip for sure */
  if (*Length == 0)
  {
    return Str;
  }

  if (strncmp(Str, "--", 2) == 0)
  {
    Res= strchr(Str + 2, '\n');
  }
  else if (*Str == '#')
  {
    Res= strchr(Str + 1, '\n');
  }
  else if (strncmp(Str, "/*", 2) == 0)
  {
    Res= strstr(Str + 2, "*/");
    ClosingStrLen= 2;
  }

  if (Res != Str)
  {
    if (Res != nullptr)
    {
      Res+= ClosingStrLen;
      *Length-= Res - Str;
    }
    else /* We found comment opening string, but did not find the closing string */
    {
      /* Thus moving pointer to the end of the string */
      Res= Str + *Length;
      *Length= 0;
    }

    /* On request - overwriting comment with white spaces */
    if (OverWrite)
    {
      //memset(Str, ' ', Res - Str);
    }
  }

  return Res;
}
