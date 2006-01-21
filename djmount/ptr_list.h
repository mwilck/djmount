/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * PtrList - Generic list of pointers.
 * This file is part of djmount.
 *
 * The actual implementation is a mix between std::list and std::vector ...
 *
 *
 * (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef PTR_LIST_H_INCLUDED
#define PTR_LIST_H_INCLUDED


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


/******************************************************************************
 * @var PtrList_Element
 *	type of element in the list : this is simply a generic void* pointer.
 *****************************************************************************/
typedef void* PtrList_Element;


/******************************************************************************
 * @var PtrList
 *	type for the list of pointers.
 *
 *      NOTE THAT THE FUNCTION API IS NOT THREAD SAFE. Callers should
 *	take care of the necessary locks if a list is shared between 
 *	multiple threads.
 *
 *****************************************************************************/
typedef struct _PtrList {
	/*
	 * PRIVATE IMPLEMENTATION, do not use directly
	 */
	PtrList_Element*  _array;    // talloc'd array of pointers 
	size_t		  _capacity; // number of allocated elements in array 
	size_t		  _size;     // number of occupied elements
	
} PtrList;


/*****************************************************************************
 * @brief Create a new list of pointers.
 *	Creates an empty list. 
 *	When finished, the list can be destroyed with "talloc_free" 
 *	(this will also de-allocate the elements in the list if they
 *	have been talloc'ed as children of this list).
 *
 * @param talloc_context        the talloc parent context
 * @return			the newly created list
 *****************************************************************************/
PtrList*	
PtrList_Create (void* talloc_context);


/*****************************************************************************
 * @brief Create a new list of pointers, pre-allocating some memory.
 *	Creates an empty list, with some pre-allocated capacity 
 *	(capacity = number of elements that the PtrList can hold before 
 *	needing to allocate more memory).
 *
 * @param talloc_context        the talloc parent context
 * @param capacity		the initial capacity
 * @return			the newly created list
 *****************************************************************************/
PtrList*	
PtrList_CreateWithCapacity (void* talloc_context, size_t capacity);


/*****************************************************************************
 * @brief Returns true if list is empty (or NULL).
 *
 * @param list 		the list
 *****************************************************************************/
static inline bool 
PtrList_IsEmpty (const PtrList* list)
{
	return (list ? list->_size < 1 : true);
}


/*****************************************************************************
 * @brief 	Returns the size of the list = number of added elements.
 *
 * @param list 		the list
 *****************************************************************************/
static inline size_t 
PtrList_GetSize (const PtrList* list)
{
	return (list ? list->_size : 0);
}


/*****************************************************************************
 * @brief 	Adds a node to the tail of the list.
 *
 * @param list 		the list
 * @param element 	the element to add
 * @return		true if success, else false e.g. memory allocation pb.
 *****************************************************************************/
bool		
PtrList_AddTail (PtrList* list, PtrList_Element element);


/*****************************************************************************
 * @brief	Returns the head of the list (or NULL if empty list)
 *
 * @param list 		the list
 *****************************************************************************/
static inline PtrList_Element
PtrList_GetHead (const PtrList* list)
{
	return ( (list && list->_size > 0) ? list->_array[0] : NULL);
}


/*****************************************************************************
 * @brief Pre-allocate more capacity to the list, if necessary.
 *	Augment the capacity of the list, in order to be able to add new
 *	elements to the list without further memory reallocation.
 *
 * @param list 		the list
 * @param extra_size 	number of extra elements to pre-allocate
 * @return		true if success, else false e.g. memory allocation pb.
 *****************************************************************************/
bool
PtrList_ReserveExtraSize (PtrList* list, size_t extra_size);



/*****************************************************************************
 * @var PtrList_Iterator
 * 	Forward iterator.
 *
 * Usage :
 * 	PtrList_Iterator iter = PtrList_IteratorStart (list);
 *	while (PtrList_IteratorLoop (&iter)) {
 *		PtrList_Element ptr = PtrList_IteratorGetElement (&iter);
 *		...
 *	}
 *****************************************************************************/

typedef struct _PtrList_Iterator {
	/*
	 * PRIVATE IMPLEMENTATION, do not use directly
	 */
	const PtrList* const 	_list;
	int_fast32_t 	  	_index;

} PtrList_Iterator;


/** initialise iterator */
static inline PtrList_Iterator
PtrList_IteratorStart (const PtrList* list) 
{
	return (PtrList_Iterator) { ._list = list, ._index = -1	};
}

/** loop on iterator, returns false if no more elements */
static inline bool
PtrList_IteratorLoop (PtrList_Iterator* iter)
{
	// Warning : no check for 'iter' validity (NULL)
	return (iter->_list && ++iter->_index < iter->_list->_size);
}

/** access current element pointed by the iterator */
static inline PtrList_Element
PtrList_IteratorGetElement (const PtrList_Iterator* iter)
{
	// Warning : no check for 'iter' validity (NULL or iter->list is NULL)
	return iter->_list->_array [iter->_index];
}


/*****************************************************************************
 * @fn 		PTR_LIST_FOR_EACH 
 * @brief 	convenience macro to iterate over a list
 *
 *	Usage :
 *		PtrList_Element ptr = NULL;
 *		PTR_LIST_FOR_EACH_PTR(list, ptr) {
 *			...
 *		} PTR_LIST_FOR_EACH_PTR_END;
 *
 * @param LIST	the list
 * @param PTR	a variable declared to hold the current element
 *****************************************************************************/
#define PTR_LIST_FOR_EACH_PTR(LIST,PTR)					\
	do {								\
		if (LIST) {						\
			PtrList_Iterator __iter##PTR =			\
				PtrList_IteratorStart(LIST);		\
			while (PtrList_IteratorLoop(&__iter##PTR)) {	\
				PTR = PtrList_IteratorGetElement(&__iter##PTR);

#define PTR_LIST_FOR_EACH_PTR_END		\
	} } } while(0)



#endif // PTR_LIST_H_INCLUDED


