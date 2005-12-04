/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * PtrList - Generic list of pointers.
 * This file is part of djmount.
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

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "ptr_list.h"
#include "talloc_util.h"
#include <string.h>


// the initial default capacity of the list
#define DEFAULT_INIT_CAPACITY	16





/*****************************************************************************
 * PtrList_Create
 *****************************************************************************/

PtrList*
PtrList_Create (void* context)
{
	PtrList* list = talloc (context, PtrList);
	if (list) {
		*list = (PtrList) {
			._array    = NULL,
			._capacity = 0,
			._size     = 0,
		};
	}
	return list;
}


/*****************************************************************************
 * PtrList_CreateWithCapacity
 *****************************************************************************/

PtrList*
PtrList_CreateWithCapacity (void* context, size_t capacity)
{
	PtrList* list = PtrList_Create (context);
	(void) PtrList_ReserveExtraSize (list, capacity);
	return list;
}



/*****************************************************************************
 * PtrList_ReserveExtraSize
 *****************************************************************************/

bool
PtrList_ReserveExtraSize (PtrList* list, size_t extra_size)
{
	if (list == NULL)
		return false; // ---------->
	
	size_t const size = list->_size + extra_size;
	if (size > list->_capacity) {
		size_t capacity = (list->_capacity > 0 ? (list->_capacity * 2) 
				   : DEFAULT_INIT_CAPACITY);
		if (capacity < size)
			capacity = size;
		
		list->_array = talloc_realloc (list, list->_array, 
					       PtrList_Element, capacity);
		if (list->_array == NULL) {
			// failure
			list->_capacity = list->_size = 0;
			return false; // ---------->
		}
		list->_capacity = capacity;
	}
	return true; 
}


/*****************************************************************************
 * PtrList_AddTail
 *****************************************************************************/

bool
PtrList_AddTail (PtrList* list, PtrList_Element element)
{
	if (PtrList_ReserveExtraSize (list, 1)) {
		list->_array [list->_size++] = element;
		return true; // ---------->
	}
	return false;
}

/*****************************************************************************
 * PtrList_InsertAt
 *****************************************************************************/
bool 
PtrList_InsertAt (PtrList* list, PtrList_Element element, int i)
{
	if (list == NULL || i < 0)
		return false; // ---------->

	if (i >= list->_size)
		return PtrList_AddTail (list, element); // ---------->

	if (! PtrList_ReserveExtraSize (list, 1)) 
		return false; // ---------->
	
	// shift up elements
	memmove (list->_array + i + 1, list->_array + i,
		 (list->_size - i) * sizeof (PtrList_Element));

	list->_array[i] = element;
	list->_size++;
	return true;
}


/*****************************************************************************
 * PtrList_GetElementIndex
 *****************************************************************************/
int
PtrList_GetElementIndex (const PtrList* list, PtrList_Element element)
{
	if (list) {
		int i;
		for (i = 0; i < list->_size; i++) {
			if (list->_array[i] == element)
				return i; // ---------->
		}
	}
	return -1;
}


/*****************************************************************************
 * PtrList_GetElementAt
 *****************************************************************************/
PtrList_Element 
PtrList_GetElementAt (const PtrList* list, int i)
{
	if (list && i >= 0 && i < list->_size)
		return list->_array[i]; // ---------->
	return NULL;
}


/*****************************************************************************
 * PtrList_RemoveAt
 *****************************************************************************/
bool 
PtrList_RemoveAt (PtrList* list, int i, PtrList_Element* ret)
{
	if (list == NULL || i < 0 || i >= list->_size)
		return false; // ---------->

	if (ret)
		*ret = list->_array[i];

	int const nb = list->_size - i - 1;
	if (nb > 0) {
		// shift down elements
		memmove (list->_array + i, list->_array + i + 1,
			 nb * sizeof (PtrList_Element));
	}
	list->_size--;
	return true;
}






