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
 #ifndef _ma_result_h_
 #define _ma_result_h_

void MADB_StmtResetResultStructures(MADB_Stmt *Stmt);
SQLRETURN MoveNext(MADB_Stmt *Stmt, unsigned long long Offset);
SQLRETURN MADB_StmtDataSeek   (MADB_Stmt *Stmt, my_ulonglong FetchOffset);
SQLRETURN MADB_StmtMoreResults(SQLHSTMT StatementHandle);
SQLULEN   MADB_RowsToFetch(MADB_Cursor *Cursor, SQLULEN ArraySize, unsigned long long RowsInResultst);

 #endif /* _ma_result_h_ */
