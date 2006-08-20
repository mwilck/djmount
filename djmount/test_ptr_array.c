/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * Testing PtrArray - generic array of pointers.
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

#include "ptr_array.h"
#include <stdio.h>
#include "talloc_util.h"


#undef NDEBUG
#include <assert.h>


static void
print_array (const char* name, const PtrArray* array)
{
	printf ("\n%s : \n", name);
	assert (array != NULL);
	printf (" PtrArray size = %d\n", PtrArray_GetSize (array));
	int i = 0;
	const int* ptr = NULL;
	PTR_ARRAY_FOR_EACH_PTR (array, ptr) {
		printf (" [%d] = %d\n", i++, *ptr);
	} PTR_ARRAY_FOR_EACH_PTR_END;
}

static void
add_int_to_array (PtrArray* array, int element)
{
	int* const ptr = talloc (array, int);
	*ptr = element;
	assert ( PtrArray_Append (array, ptr) );
}

int 
main(int argc, char * argv[])
{
	talloc_enable_leak_report();

	PtrArray* array1 = PtrArray_CreateWithCapacity (NULL, 1);
	assert (array1 != NULL);
	assert (PtrArray_IsEmpty (array1));

	PtrArray* array2 = PtrArray_Create (NULL);
	assert (array2 != NULL);
	assert (PtrArray_IsEmpty (array2));

	PtrArray* array3 = PtrArray_Create (NULL);
	assert (array3 != NULL);
	assert (PtrArray_IsEmpty (array3));

#define PRINT_ARRAY(ARRAY)	print_array(#ARRAY,ARRAY)
	
	int i;
	for (i = 0; i < 10; i++) {
		add_int_to_array (array1, i);
	}
	PRINT_ARRAY (array1);
	assert (PtrArray_GetSize (array1) == 10);

	for (i = 10; i < 30; i++) {
		add_int_to_array (array2, i);
	}
	PRINT_ARRAY (array2);
	assert (PtrArray_GetSize (array2) == 20);

	int* ptr = PtrArray_GetHead (array2);
	assert (ptr != NULL && *ptr == 10);

	for (i = 0; i < 20; i++) {
		ptr = PtrArray_GetElementAt (array2, i);
		assert (ptr != NULL && *ptr == 10+i);
	}

	PRINT_ARRAY (array3);
	assert (PtrArray_IsEmpty (array3));


	ptr = PtrArray_RemoveAt (array2, 7);
	assert (ptr != NULL && *ptr == 17);
	assert (PtrArray_GetSize (array2) == 19);
	for (i = 0; i < 7; i++) {
		ptr = PtrArray_GetElementAt (array2, i);
		assert (ptr != NULL && *ptr == 10+i);
	}
	for (i = 7; i < 19; i++) {
		ptr = PtrArray_GetElementAt (array2, i);
		assert (ptr != NULL && *ptr == 11+i);
	}

	ptr = PtrArray_RemoveAtReorder (array2, 6);
	assert (ptr != NULL && *ptr == 16);
	assert (PtrArray_GetSize (array2) == 18);
	ptr = PtrArray_GetElementAt (array2, 6);
	assert (ptr != NULL && *ptr == 29);
	PRINT_ARRAY (array2);

#if 0
	for (i = -20; i < -10; i++) {
		ptrarray_insert(&array2, (void *)i, 0);
	}
	printf("\narray2\n");
	ptrarray_print(&array2);
#endif

	talloc_free (array1);
	talloc_free (array2);
	talloc_free (array3);
	array1 = array2 = array3 = NULL;
	
	size_t bytes = talloc_total_size (NULL);
	assert (bytes == 0);
	
	exit (0);
}



