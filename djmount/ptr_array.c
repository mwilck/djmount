/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * PtrArray - Generic array of pointers.
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

#include <config.h>

#include "ptr_array.h"
#include "talloc_util.h"
#include <string.h>


// the initial default capacity of the array
#define DEFAULT_INIT_CAPACITY	16





/*****************************************************************************
 * PtrArray_Create
 *****************************************************************************/

PtrArray*
PtrArray_Create (void* context)
{
	PtrArray* self = talloc (context, PtrArray);
	if (self) {
		*self = (PtrArray) {
			._array    = NULL,
			._capacity = 0,
			._size     = 0,
		};
	}
	return self;
}


/*****************************************************************************
 * PtrArray_CreateWithCapacity
 *****************************************************************************/

PtrArray*
PtrArray_CreateWithCapacity (void* context, size_t capacity)
{
	PtrArray* self = PtrArray_Create (context);
	(void) PtrArray_ReserveExtraSize (self, capacity);
	return self;
}



/*****************************************************************************
 * PtrArray_ReserveExtraSize
 *****************************************************************************/

bool
PtrArray_ReserveExtraSize (PtrArray* self, size_t extra_size)
{
	if (self == NULL)
		return false; // ---------->
	
	size_t const size = self->_size + extra_size;
	if (size > self->_capacity) {
		size_t capacity = (self->_capacity > 0 ? (self->_capacity * 2) 
				   : DEFAULT_INIT_CAPACITY);
		if (capacity < size)
			capacity = size;
		
		self->_array = talloc_realloc (self, self->_array, 
					       PtrArray_Element, capacity);
		if (self->_array == NULL) {
			// failure
			self->_capacity = self->_size = 0;
			return false; // ---------->
		}
		self->_capacity = capacity;
	}
	return true; 
}


/*****************************************************************************
 * PtrArray_Append
 *****************************************************************************/

bool
PtrArray_Append (PtrArray* self, PtrArray_Element element)
{
	if (PtrArray_ReserveExtraSize (self, 1)) {
		self->_array [self->_size++] = element;
		return true; // ---------->
	}
	return false;
}


/*****************************************************************************
 * PtrArray_InsertAt
 *****************************************************************************/
bool 
PtrArray_InsertAt (PtrArray* self, PtrArray_Element element, size_t i)
{
	if (self == NULL || i < 0)
		return false; // ---------->

	if (i >= self->_size)
		return PtrArray_Append (self, element); // ---------->

	if (! PtrArray_ReserveExtraSize (self, 1)) 
		return false; // ---------->
	
	// shift up elements
	memmove (self->_array + i + 1, self->_array + i,
		 (self->_size - i) * sizeof (PtrArray_Element));

	self->_array[i] = element;
	self->_size++;
	return true;
}


/*****************************************************************************
 * PtrArray_GetElementIndex
 *****************************************************************************/
ssize_t
PtrArray_GetElementIndex (const PtrArray* self, PtrArray_Element element)
{
	if (self) {
		ssize_t i;
		for (i = 0; i < self->_size; i++) {
			if (self->_array[i] == element)
				return i; // ---------->
		}
	}
	return -1;
}


/*****************************************************************************
 * PtrArray_RemoveAt
 *****************************************************************************/
PtrArray_Element
PtrArray_RemoveAt (PtrArray* self, size_t i)
{
	if (self == NULL || i < 0 || i >= self->_size)
		return NULL; // ---------->

	PtrArray_Element const element = self->_array[i];

	size_t const nb = self->_size - i - 1;
	if (nb > 0) {
		// shift down elements
		memmove (self->_array + i, self->_array + i + 1,
			 nb * sizeof (PtrArray_Element));
	}
	self->_size--;
	return element;
}


/*****************************************************************************
 * PtrArray_RemoveAtReorder
 *****************************************************************************/
PtrArray_Element
PtrArray_RemoveAtReorder (PtrArray* self, size_t i)
{
	if (self == NULL || i < 0 || i >= self->_size)
		return NULL; // ---------->

	PtrArray_Element const element = self->_array[i];
	
	self->_array[i] = self->_array[self->_size-1];
	self->_size--;

	return element;
}






