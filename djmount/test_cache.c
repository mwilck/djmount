/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * Testing Cache - caching object.
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

#define TEST_CACHE 1
#include "cache.h"
#include <stdio.h>
#include "talloc_util.h"
#include <unistd.h>


#undef NDEBUG
#include <assert.h>


#define AGE  2


static void free_expired_data (const char* key, void* data)
{
	printf ("free %s \n", key);
	talloc_free (data);
}

static void fill_cache (Cache* cache, bool create, int a, int b)
{
	int i;
	for (i = a; i < b; i++) {
		char buffer [20];
		sprintf (buffer, "[%d]", i);
		int** iptr = (int**) Cache_Get (cache, buffer);
		assert (iptr != NULL);
		if (create) {
			assert (*iptr == NULL);
			*iptr = talloc (cache, int);
			**iptr = i;
		} else {
			assert (*iptr != NULL);
			assert (**iptr == i);
		}
	}
}


int 
main (int argc, char* argv[])
{
	talloc_enable_leak_report();

	// Create a working context for memory allocations
	void* const ctx = talloc_new (NULL);

	Cache* cache0 = Cache_Create (ctx, 100, /* max_age => */ 0, NULL);
	assert (cache0 != NULL);
	assert (Cache_GetNrEntries (cache0) == 0);

	Cache* cache1 = Cache_Create (ctx, 100, AGE, free_expired_data);
	assert (cache1 != NULL);
	assert (Cache_GetNrEntries (cache1) == 0);

#define PRINT_CACHE(CACHE)  printf (#CACHE " Status = \n%s\n", \
				    Cache_GetStatusString (CACHE, ctx, "  "));


	fill_cache (cache0, true, 0, 100);
	assert (Cache_GetNrEntries (cache0) == 100);

	fill_cache (cache0, false, 0, 100);
	assert (Cache_GetNrEntries (cache0) == 100);

	fill_cache (cache1, true, 0, 10);
	assert (Cache_GetNrEntries (cache1) == 10);
	
	sleep (AGE+1);

	_Cache_PurgeExpiredEntries (cache0);
	assert (Cache_GetNrEntries (cache0) == 100);

	_Cache_PurgeExpiredEntries (cache1);
	assert (Cache_GetNrEntries (cache1) == 0);

	fill_cache (cache1, true, 0, 10);
	assert (Cache_GetNrEntries (cache1) == 10);

	sleep (AGE/2+1);
	_Cache_PurgeExpiredEntries (cache1);
	assert (Cache_GetNrEntries (cache1) == 10);

	fill_cache (cache1, true, 10, 20);
	assert (Cache_GetNrEntries (cache1) == 20);

	sleep (AGE/2+1);

	_Cache_PurgeExpiredEntries (cache1);
	assert (Cache_GetNrEntries (cache1) == 10);

	PRINT_CACHE (cache0);
	PRINT_CACHE (cache1);
	
	// Delete all storage
	talloc_free (ctx);
	
	int bytes = talloc_total_size (NULL);
	assert (bytes == 0);
	
	exit (EXIT_SUCCESS);
}



