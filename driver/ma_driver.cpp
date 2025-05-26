/************************************************************************************
   Copyright (C) 2013,2025 MariaDB Corporation plc
   
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
#include "ma_odbc.h"
#define MAX_DELETED_STMTS_COUNT_TO_KEEP 50

/* Initially this was for tools to read ODBC information about the Driver by it's name
 * Now it's also place for global(driver wide) stuff - variables, data structures, locks, and tools for accessing them
 */
/************************************* Driver wide stuff ************************************************/

std::mutex globalLock;
static MADB_List *deletedStmt= NULL;
//sles12 does not have atomic_uint32 defined
static std::atomic<uint32_t> envCount(0U);

#ifndef _WIN32
__attribute__((constructor))
static void library_constructor() {
  DriverGlobalInit();
}
__attribute__((destructor))
static void library_destructor() {
  DriverGlobalClean();
}
#endif // !_WIN32

extern "C" {
  /* {{{ DriverGlobalInit */
  void DriverGlobalInit()
  {
  }
  /* }}} */

  /* {{{  DriverGlobalClean()*/
  void DriverGlobalClean(void)
  {
    // There is no need to lock here at least the while it used the way it used now -
    // only called when library is unloaded
    if (deletedStmt)
    {
      MADB_ListFree(deletedStmt, FALSE);
    }
  }
  /* }}} */
}
/* {{{ IncrementEnvCount */
// Normally there should be 1 Env, but nothing forbids app have more than 1. 
void IncrementEnvCount()
{
  ++envCount;
}
/*}}}*/

/* {{{ DecrementEnvCount */
// If the last Env has been freed - we should probably clean the list 
void DecrementEnvCount()
{
  --envCount;
  if (!envCount)
  {
    std::lock_guard<std::mutex> localScopeLock(globalLock);
    MADB_ListFree(deletedStmt, FALSE);
    deletedStmt= NULL;
  }
}
/*}}}*/

/* {{{ CheckDeletedStmt */
// If the last Env has been freed - we should probably clean the list 
MADB_List* CheckDeletedStmt(void* stmtObjAddr)
{
  MADB_List *item= deletedStmt;
  while (item != NULL)
  {
    if (item->data == stmtObjAddr)
    {
      return item;
    }
    item= item->next;
  }
  return NULL;
}
/*}}}*/

/* {{{ RemoveStmtFromDeleted */
// If the last Env has been freed - we should probably clean the list 
bool RemoveStmtFromDeleted(void* stmtObjAddr)
{
  bool result= false;
  std::lock_guard<std::mutex> localScopeLock(globalLock);
  MADB_List* found= CheckDeletedStmt(stmtObjAddr);
  if (found)
  {
    deletedStmt= MADB_ListDelete(deletedStmt, found);
    free(found);
    result= true;
  }
  return result;
}
/*}}}*/

/* {{{ RememberDeletedStmt */
void RememberDeletedStmt(void* stmtObjAddr)
{
  MADB_List *root= deletedStmt, *item= NULL;
  int count= 0;
  while (root)
  {
    ++count;
    if (count == MAX_DELETED_STMTS_COUNT_TO_KEEP)
    {
      item= root;
      //item can't be root
      MADB_ListDelete(deletedStmt, item);
      break;
    }
    root= root->next;
  }
  if (item)
  {
    // Re-using the just deleted item
    item->data= stmtObjAddr;
    deletedStmt= MADB_ListAdd(deletedStmt, item);
  }
  else
  {
    MADB_List *newRoot= MADB_ListCons(stmtObjAddr, deletedStmt);
    // If it's NULL - there was an allocation error. Doing nothing in this case - if there is
    // no memory smth will return error sooned or later. or crash
    if (newRoot)
    {
      deletedStmt= newRoot;
    }
  }
}
/*}}}*/

/************************************* Reading ODBC Driver info *****************************************/

/* {{{ MADB_DriverInit */
MADB_Drv *MADB_DriverInit(void)
{
  return (MADB_Drv* )MADB_CALLOC(sizeof(MADB_Drv));
}
/* }}} */

/* {{{ MADB_DriverFree */
void MADB_DriverFree(MADB_Drv *Drv)
{
  if (Drv)
  {
    MADB_FREE(Drv->DriverName);
    MADB_FREE(Drv->OdbcLibrary);
    MADB_FREE(Drv->SetupLibrary);
    MADB_FREE(Drv);
  }
}
/* }}} */

/* {{{ MADB_DriverGet */
MADB_Drv * MADB_DriverGet(char *DriverName)
{
  MADB_Drv *Drv= NULL;
  char Value[2048];

  if (!DriverName ||
      !SQLGetPrivateProfileString(DriverName, "Driver", "", Value, 2048, "ODBCINST.INI"))
     return NULL;
  Drv= MADB_DriverInit();
  Drv->DriverName= _strdup(DriverName);
  Drv->OdbcLibrary= _strdup(Value);
  if (SQLGetPrivateProfileString(DriverName, "Setup", "", Value, 2048, "ODBCINST.INI"))
    Drv->SetupLibrary= _strdup(Value);
  return Drv;
}
/* }}} */
