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

/********
 *
 * Code for lists, MADB_DynArray, and MADB_DynString copied (from C/C
 *
 ********/

/*
  *************** Code for handling dubble-linked lists in C *********
*/

#include <ma_odbc.h>

/*
How much overhead does malloc have. The code often allocates
something like 1024-MALLOC_OVERHEAD bytes
*/
#define MALLOC_OVERHEAD 8

/* Add a element to start of list */

MADB_List *MADB_ListAdd(MADB_List *root, MADB_List *element)
{
  if (root)
  {
    if (root->prev)			/* If add in mid of list */
      root->prev->next= element;
    element->prev=root->prev;
    root->prev=element;
  }
  else
    element->prev=0;
  element->next=root;
  return(element);			/* New root */
}


MADB_List *MADB_ListDelete(MADB_List *root, MADB_List *element)
{
  if (element->prev)
    element->prev->next=element->next;
  else
    root=element->next;
  if (element->next)
    element->next->prev=element->prev;
  return root;
}


void MADB_ListFree(MADB_List *root, unsigned int free_data)
{
  MADB_List *next;
  while (root)
  {
    next=root->next;
    if (free_data)
      free(root->data);
    free(root);
    root=next;
  }
}


MADB_List *MADB_ListCons(void *data, MADB_List *list)
{
  MADB_List *new_charset=(MADB_List*) malloc(sizeof(MADB_List));
  if (!new_charset)
    return 0;
  new_charset->data=data;
  return MADB_ListAdd(list,new_charset);
}


MADB_List *MADB_ListReverse(MADB_List *root)
{
  MADB_List *last;

  last=root;
  while (root)
  {
    last=root;
    root=root->next;
    last->next=last->prev;
    last->prev=root;
  }
  return last;
}

unsigned int MADB_ListLength(MADB_List *list)
{
  unsigned int count;
  for (count=0 ; list ; list=list->next, count++) ;
  return count;
}


int MADB_ListWalk(MADB_List *list, MADB_ListWalkAction action, char * argument)
{
  int error=0;
  while (list)
  {
    if ((error = (*action)(list->data,argument)))
      return error;
    list= MADB_LIST_REST(list);
  }
  return 0;
}


/************************** MADB_DynString ***************************/
/*
  Code for handling strings with can grow dynamicly.
  Copyright Monty Program KB.
  By monty.
*/

my_bool MADB_InitDynamicString(MADB_DynString *str, const char *init_str,
			    size_t init_alloc, size_t alloc_increment)
{
  unsigned int length;

  if (!alloc_increment)
    alloc_increment=128;
  length=1;
  if (init_str && (length= (unsigned int) strlen(init_str)+1) < init_alloc)
    init_alloc=((length+alloc_increment-1)/alloc_increment)*alloc_increment;
  if (!init_alloc)
    init_alloc=alloc_increment;

  if (!(str->str=(char*) malloc(init_alloc)))
    return(TRUE);
  str->length=length-1;
  if (init_str)
    memcpy(str->str,init_str,length);
  str->max_length=init_alloc;
  str->alloc_increment=alloc_increment;
  return(FALSE);
}

my_bool MADB_DynstrSet(MADB_DynString *str, const char *init_str)
{
  unsigned int length;

  if (init_str && (length= (unsigned int) strlen(init_str)+1) > str->max_length)
  {
    str->max_length=((length+str->alloc_increment-1)/str->alloc_increment)*
      str->alloc_increment;
    if (!str->max_length)
      str->max_length=str->alloc_increment;
    if (!(str->str=(char*) realloc(str->str,str->max_length)))
      return(TRUE);
  }
  if (init_str)
  {
    str->length=length-1;
    memcpy(str->str,init_str,length);
  }
  else
    str->length=0;
  return(FALSE);
}


my_bool MADB_DynstrRealloc(MADB_DynString *str, size_t additional_size)
{
  if (!additional_size) return(FALSE);
  if (str->length + additional_size > str->max_length)
  {
    str->max_length=((str->length + additional_size+str->alloc_increment-1)/
		     str->alloc_increment)*str->alloc_increment;
    if (!(str->str=(char*) realloc(str->str,str->max_length)))
      return(TRUE);
  }
  return(FALSE);
}


my_bool MADB_DynstrAppend(MADB_DynString *str, const char *append)
{
  return MADB_DynstrAppendMem(str,append,strlen(append));
}


my_bool MADB_DynstrAppendMem(MADB_DynString *str, const char *append,
			  size_t length)
{
  char *new_ptr;
  if (str->length+length >= str->max_length)
  {
    size_t new_length=(str->length+length+str->alloc_increment)/
      str->alloc_increment;
    new_length*=str->alloc_increment;
    if (!(new_ptr=(char*) realloc(str->str,new_length)))
      return TRUE;
    str->str=new_ptr;
    str->max_length=new_length;
  }
  memcpy(str->str + str->length,append,length);
  str->length+=length;
  str->str[str->length]=0;			/* Safety for C programs */
  return FALSE;
}


void MADB_DynstrFree(MADB_DynString *str)
{
  if (str->str)
  {
    free(str->str);
    str->str=0;
  }
}

char *MADB_DynstrMake(register char *dst, register const char *src, size_t length)
{
  while (length--)
    if (! (*dst++ = *src++))
      return dst-1;
  *dst=0;
  return dst;
}


/************* MADB_DynArray - Handling of arrays that can grow dynamicly. **************/

#undef SAFEMALLOC /* Problems with threads */


/*
  Initiate array and alloc space for init_alloc elements. Array is usable
  even if space allocation failed
*/

my_bool MADB_InitDynamicArray(MADB_DynArray *array, unsigned int element_size,
                              unsigned int init_alloc, unsigned int alloc_increment)
{
  if (!alloc_increment)
  {
    alloc_increment=MAX((8192-MALLOC_OVERHEAD)/element_size,16);
    if (init_alloc > 8 && alloc_increment > init_alloc * 2)
      alloc_increment=init_alloc*2;
  }

  if (!init_alloc)
    init_alloc=alloc_increment;
  array->elements=0;
  array->max_element=init_alloc;
  array->alloc_increment=alloc_increment;
  array->size_of_element=element_size;
  if (!(array->buffer=(char*) malloc(element_size*init_alloc)))
  {
    array->max_element=0;
    return(TRUE);
  }
  return(FALSE);
}


my_bool MADB_InsertDynamic(MADB_DynArray *array, void *element)
{
  void *buffer;
  if (array->elements == array->max_element)
  {						/* Call only when nessesary */
    if (!(buffer=MADB_AllocDynamic(array)))
      return TRUE;
  }
  else
  {
    buffer=array->buffer+(array->elements * array->size_of_element);
    array->elements++;
  }
  memcpy(buffer,element,(size_t) array->size_of_element);
  return FALSE;
}


	/* Alloc room for one element */

unsigned char *MADB_AllocDynamic(MADB_DynArray *array)
{
  if (array->elements == array->max_element)
  {
    char *new_ptr;
    if (!(new_ptr=(char*) realloc(array->buffer,(array->max_element+
			          array->alloc_increment)*
				   array->size_of_element)))
      return 0;
    array->buffer=new_ptr;
    array->max_element+=array->alloc_increment;
  }
  return (unsigned char *)array->buffer+(array->elements++ * array->size_of_element);
}


	/* remove last element from array and return it */

unsigned char *MADB_PopDynamic(MADB_DynArray *array)
{
  if (array->elements)
    return (unsigned char *)array->buffer+(--array->elements * array->size_of_element);
  return 0;
}


my_bool MADB_SetDynamic(MADB_DynArray *array, void * element, unsigned int idx)
{
  if (idx >= array->elements)
  {
    if (idx >= array->max_element)
    {
      unsigned int size;
      char *new_ptr;
      size=(idx+array->alloc_increment)/array->alloc_increment;
      size*= array->alloc_increment;
      if (!(new_ptr=(char*) realloc(array->buffer,size*
			            array->size_of_element)))
	return TRUE;
      array->buffer=new_ptr;
      array->max_element=size;
    }
    memset((array->buffer+array->elements*array->size_of_element), 0,
	  (idx - array->elements)*array->size_of_element);
    array->elements=idx+1;
  }
  memcpy(array->buffer+(idx * array->size_of_element),element,
	 (size_t) array->size_of_element);
  return FALSE;
}


void MADB_GetDynamic(MADB_DynArray *array, void * element, unsigned int idx)
{
  if (idx >= array->elements)
  {
    memset(element, 0, array->size_of_element);
    return;
  }
  memcpy(element,array->buffer+idx*array->size_of_element,
	 (size_t) array->size_of_element);
}


void MADB_DeleteDynamic(MADB_DynArray *array)
{
  if (array->buffer)
  {
    free(array->buffer);
    array->buffer=0;
    array->elements=array->max_element=0;
  }
}


void MADB_DeleteDynamicElement(MADB_DynArray *array, unsigned int idx)
{
  char *ptr=array->buffer+array->size_of_element*idx;
  array->elements--;
  memmove(ptr,ptr+array->size_of_element,
	  (array->elements-idx)*array->size_of_element);
}


void MADB_FreezeSizeDynamic(MADB_DynArray *array)
{
  unsigned int elements=MAX(array->elements,1);

  if (array->buffer && array->max_element != elements)
  {
    array->buffer=(char*) realloc(array->buffer,
			          elements*array->size_of_element);
    array->max_element=elements;
  }
}
