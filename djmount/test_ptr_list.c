/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * Testing PtrList - generic list of pointers.
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
#include <stdio.h>
#include "talloc_util.h"


#undef NDEBUG
#include <assert.h>


static void
print_list (const char* name, const PtrList* list)
{
	printf ("\n%s : \n", name);
	assert (list != NULL);
	printf (" PtrList size = %d\n", PtrList_GetSize (list));
	int i = 0;
	const int* ptr = NULL;
	PTR_LIST_FOR_EACH_PTR (list, ptr) {
		printf (" [%d] = %d\n", i++, *ptr);
	} PTR_LIST_FOR_EACH_PTR_END;
}

static void
add_int_to_list (PtrList* list, int element)
{
	int* const ptr = talloc (list, int);
	*ptr = element;
	assert ( PtrList_AddTail (list, ptr) );
}

int 
main(int argc, char * argv[])
{
	talloc_enable_leak_report();

	PtrList* list1 = PtrList_CreateWithCapacity (NULL, 1);
	assert (list1 != NULL);
	assert (PtrList_IsEmpty (list1));

	PtrList* list2 = PtrList_Create (NULL);
	assert (list2 != NULL);
	assert (PtrList_IsEmpty (list2));

	PtrList* list3 = PtrList_Create (NULL);
	assert (list3 != NULL);
	assert (PtrList_IsEmpty (list3));

#define PRINT_LIST(LIST)	print_list(#LIST,LIST)
	
	int i;
	for (i = 0; i < 10; i++) {
		add_int_to_list (list1, i);
	}
	PRINT_LIST (list1);
	assert (PtrList_GetSize (list1) == 10);

	for (i = 10; i < 30; i++) {
		add_int_to_list (list2, i);
	}
	PRINT_LIST (list2);
	assert (PtrList_GetSize (list2) == 20);

	int* ptr = PtrList_GetHead (list2);
	assert (ptr != NULL && *ptr == 10);

	PRINT_LIST (list3);
	assert (PtrList_IsEmpty (list3));

#if 0
    {
	void * val;
	
	if (ptrlist_remove(&list3, 7, &val)) {
	    printf("Element at index 7 has value %d\n", val);
	}
	printf("\nlist3\n");
	ptrlist_print(&list3);
	if (ptrlist_remove(&list3, 0, &val)) {
	    printf("Element at index 0 has value %d\n", val);
	}
	printf("\nlist3\n");
	ptrlist_print(&list3);
	if (ptrlist_remove(&list3, ptrlist_count(&list3) - 1, &val)) {
	    printf("Element at index %d has value %d\n", 
		   ptrlist_count(&list3),  val);
	}
	printf("\nlist3\n");
	ptrlist_print(&list3);
	
    }
#endif

#if 0
    for (i = -20; i < -10; i++) {
	ptrlist_insert(&list2, (void *)i, 0);
    }
    printf("\nlist2\n");
    ptrlist_print(&list2);
#endif

    talloc_free (list1);
    talloc_free (list2);
    talloc_free (list3);
    list1 = list2 = list3 = NULL;

    int bytes = talloc_total_size (NULL);
    assert (bytes == 0);

    exit (0);
}



