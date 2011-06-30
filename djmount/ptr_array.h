/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id: ptr_array.h 197 2006-06-20 20:31:38Z r3mi $
 *
 * PtrArray - Generic array of pointers.
 * This file is part of djmount.
 *
 * Some of the API inspired from GLib's GPtrArray.
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

#ifndef PTR_ARRAY_H_INCLUDED
#define PTR_ARRAY_H_INCLUDED


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


/******************************************************************************
 * @var PtrArray_Element
 *	type of element in the array : this is simply a generic void* pointer.
 *****************************************************************************/
typedef void* PtrArray_Element;


/******************************************************************************
 * @var PtrArray
 *	type for the array of pointers.
 *
 *      NOTE THAT THE FUNCTION API IS NOT THREAD SAFE. Callers should
 *	take care of the necessary locks if an array is shared between 
 *	multiple threads.
 *
 *****************************************************************************/
typedef struct _PtrArray {
	/*
	 * PRIVATE IMPLEMENTATION, do not use directly
	 */
	PtrArray_Element*  _array;    // talloc'd array of pointers 
	size_t		  _capacity; // number of allocated elements in array 
	size_t		  _size;     // number of occupied elements
	
} PtrArray;


/*****************************************************************************
 * @brief Create a new array of pointers.
 *	Creates an empty array. 
 *	When finished, the array can be destroyed with "talloc_free" 
 *	(this will also de-allocate the elements in the array if they
 *	have been talloc'ed as children of this array).
 *
 * @param talloc_context        the talloc parent context
 * @return			the newly created array
 *****************************************************************************/
PtrArray*	
PtrArray_Create (void* talloc_context);


/*****************************************************************************
 * @brief Create a new array of pointers, pre-allocating some memory.
 *	Creates an empty array, with some pre-allocated capacity 
 *	(capacity = number of elements that the PtrArray can hold before 
 *	needing to allocate more memory).
 *
 * @param talloc_context        the talloc parent context
 * @param capacity		the initial capacity
 * @return			the newly created array
 *****************************************************************************/
PtrArray*	
PtrArray_CreateWithCapacity (void* talloc_context, size_t capacity);


/*****************************************************************************
 * @brief Returns true if array is empty (or NULL).
 *
 * @param self 		the array
 *****************************************************************************/
static inline bool 
PtrArray_IsEmpty (const PtrArray* self)
{
	return (self ? self->_size < 1 : true);
}


/*****************************************************************************
 * @brief 	Returns the size of the array = number of added elements.
 *
 * @param self 		the array
 *****************************************************************************/
static inline size_t 
PtrArray_GetSize (const PtrArray* self)
{
	return (self ? self->_size : 0);
}


/*****************************************************************************
 * @brief 	Adds an element to the end of the array.
 * 		The array will grow in size automatically if necessary.
 *
 * @param self 		the array
 * @param element 	the element to add
 * @return		true if success, else false e.g. memory allocation pb.
 *****************************************************************************/
bool		
PtrArray_Append (PtrArray* self, PtrArray_Element element);


/*****************************************************************************
 * @brief	Returns the 1st element of the array (or NULL if empty array)
 *
 * @param self 	the array
 *****************************************************************************/
static inline PtrArray_Element
PtrArray_GetHead (const PtrArray* const self)
{
	return ( (self && self->_size > 0) ? self->_array[0] : NULL);
}


/*****************************************************************************
 * @brief	Returns the element at the given index of the array
 *		(or NULL if empty array).
 *
 * @param self	the array
 *****************************************************************************/
static inline PtrArray_Element
PtrArray_GetElementAt (const PtrArray* const self, const size_t index)
{
	return ( (self && index >= 0 && index < self->_size) 
		 ? self->_array[index] : NULL);
}


/*****************************************************************************
 * @brief Pre-allocate more capacity to the array, if necessary.
 *	Augment the capacity of the array, in order to be able to add new
 *	elements to the array without further memory reallocation.
 *
 * @param self 		the arrray
 * @param extra_size 	number of extra elements to pre-allocate
 * @return		true if success, else false e.g. memory allocation pb.
 *****************************************************************************/
bool
PtrArray_ReserveExtraSize (PtrArray* self, size_t extra_size);


/*****************************************************************************
 * @brief Removes the element at the given index from the array. 
 *	  Subsequent elements are moved down one place.
 *
 * @param self 		the arrray
 * @param index 	the index of the element to remove
 * @return		the element which was removed (or NULL if error)
 *****************************************************************************/
PtrArray_Element
PtrArray_RemoveAt (PtrArray* self, size_t index);


/*****************************************************************************
 * @brief Removes the element at the given index from the array. 
 *	  The last element in the array is used to fill in the space, 
 *	  so this function does not preserve the order of the array. 
 *	  But it is faster than PtrArray_RemoveAt().
 *
 * @param self 		the arrray
 * @param index 	the index of the element to remove
 * @return		the element which was removed (or NULL if error)
 *****************************************************************************/
PtrArray_Element
PtrArray_RemoveAtReorder (PtrArray* self, size_t index);


/*****************************************************************************
 * @var PtrArray_Iterator
 * 	Forward iterator.
 *
 * Usage :
 * 	PtrArray_Iterator iter = PtrArray_IteratorStart (array);
 *	while (PtrArray_IteratorLoop (&iter)) {
 *		PtrArray_Element ptr = PtrArray_IteratorGetElement (&iter);
 *		...
 *	}
 *****************************************************************************/

typedef struct _PtrArray_Iterator {
	/*
	 * PRIVATE IMPLEMENTATION, do not use directly
	 */
	const PtrArray* const 	_array;
	int_fast32_t 	  	_index;

} PtrArray_Iterator;


/** initialise iterator */
static inline PtrArray_Iterator
PtrArray_IteratorStart (const PtrArray* array) 
{
	return (PtrArray_Iterator) { ._array = array, ._index = -1	};
}

/** loop on iterator, returns false if no more elements */
static inline bool
PtrArray_IteratorLoop (PtrArray_Iterator* iter)
{
	// Warning : no check for 'iter' validity (NULL)
	return (iter->_array && ++iter->_index < iter->_array->_size);
}

/** access current element pointed by the iterator */
static inline PtrArray_Element
PtrArray_IteratorGetElement (const PtrArray_Iterator* iter)
{
	// Warning : no check for 'iter' validity (NULL or iter->array is NULL)
	return iter->_array->_array [iter->_index];
}


/*****************************************************************************
 * @fn 		PTR_ARRAY_FOR_EACH 
 * @brief 	convenience macro to iterate over an array
 *
 *	Usage :
 *		PtrArray_Element ptr = NULL;
 *		PTR_ARRAY_FOR_EACH_PTR(array, ptr) {
 *			...
 *		} PTR_ARRAY_FOR_EACH_PTR_END;
 *
 * @param ARRAY	the array
 * @param PTR	a variable declared to hold the current element
 *****************************************************************************/
#define PTR_ARRAY_FOR_EACH_PTR(ARRAY,PTR)				\
	do {								\
		if (ARRAY) {						\
			PtrArray_Iterator __iter##PTR =			\
				PtrArray_IteratorStart(ARRAY);		\
			while (PtrArray_IteratorLoop(&__iter##PTR)) {	\
				PTR = PtrArray_IteratorGetElement	\
					(&__iter##PTR);

#define PTR_ARRAY_FOR_EACH_PTR_END		\
	} } } while(0)



#endif // PTR_ARRAY_H_INCLUDED


