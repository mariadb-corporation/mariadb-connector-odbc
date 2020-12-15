/* Copyright (C) 2000 MySQL AB & MySQL Finland AB & TCX DataKonsult AB
                 2016 MariaDB Corporation AB
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*/
#ifndef _MA_LEGACY_HELPERS_H
#define _MA_LEGACY_HELPERS_H

/********
 *
 * Function definitions, types, macros for MADB_List, MADB_DynArray, and MADB_DynString copied from C/C
 *
 ********/

/*
  *************** Dubble-linked lists *********
*/
typedef struct st_ma_odbc_list {
  struct st_ma_odbc_list *prev, *next;
  void *data;
} MADB_List;

typedef int(*MADB_ListWalkAction)(void *, void *);

	/* Add a element to start of list */
MADB_List *MADB_ListAdd(MADB_List *root, MADB_List *element);
MADB_List *MADB_ListDelete(MADB_List *root, MADB_List *element);
void MADB_ListFree(MADB_List *root, unsigned int free_data);
MADB_List *MADB_ListCons(void *data, MADB_List *list);
MADB_List *MADB_ListReverse(MADB_List *root);
unsigned int MADB_ListLength(MADB_List *list);
int MADB_ListWalk(MADB_List *list, MADB_ListWalkAction action, char *argument);

#define MADB_LIST_REST(a) ((a)->next)
#define MADB_LIST_PUSH(a,b) (a)=MADB_ListCons((b),(a))
#define MADB_LIST_POP(A) {MADB_List *old=(A); (A)=MADB_ListDelete(old,old) ; free((char *) old); }

/************************** MADB_DynString ************************/
typedef struct st_ma_odbc_dynstr {
  char *str;
  size_t length,max_length,alloc_increment;
} MADB_DynString;

my_bool MADB_InitDynamicString(MADB_DynString *str, const char *init_str,
			    size_t init_alloc, size_t alloc_increment);
my_bool MADB_DynstrSet(MADB_DynString *str, const char *init_str);
my_bool MADB_DynstrRealloc(MADB_DynString *str, size_t additional_size);
my_bool MADB_DynstrAppend(MADB_DynString *str, const char *append);
my_bool MADB_DynstrAppendMem(MADB_DynString *str, const char *append,
			  size_t length);
void MADB_DynstrFree(MADB_DynString *str);
char *MADB_DynstrMake(register char *dst, register const char *src, size_t length);

/************* MADB_DynArray - Arrays that can grow dynamicly. **************/
typedef struct ma_odbc_st_dynarr {
  char *buffer;
  unsigned int elements,max_element;
  unsigned int alloc_increment;
  unsigned int size_of_element;
} MADB_DynArray;

my_bool MADB_InitDynamicArray(MADB_DynArray *array, unsigned int element_size,
                              unsigned int init_alloc, unsigned int alloc_increment);
my_bool MADB_InsertDynamic(MADB_DynArray *array, void *element);
	/* Alloc room for one element */
unsigned char *MADB_AllocDynamic(MADB_DynArray *array);
	/* remove last element from array and return it */
unsigned char *MADB_PopDynamic(MADB_DynArray *array);
my_bool MADB_SetDynamic(MADB_DynArray *array, void * element, unsigned int idx);
void MADB_GetDynamic(MADB_DynArray *array, void * element, unsigned int idx);
void MADB_DeleteDynamic(MADB_DynArray *array);
void MADB_DeleteDynamicElement(MADB_DynArray *array, unsigned int idx);
void MADB_FreezeSizeDynamic(MADB_DynArray *array);

#endif /* #ifndef _MA_LEGACY_HELPERS_H */
