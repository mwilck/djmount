/* $Id$
 *
 * Caching object.
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

#ifndef DJMOUNT_CACHE_H_INCLUDED
#define DJMOUNT_CACHE_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>


/******************************************************************************
 * @var Cache
 *
 *	This opaque type encapsulates access to the cache.
 *
 *      NOTE THAT THE FUNCTION API IS NOT THREAD SAFE. Callers should
 *	take care of the necessary locks if a cache is shared between 
 *	multiple threads.
 *	
 *****************************************************************************/

typedef struct _Cache Cache;


/******************************************************************************
 * @var Prototype of function called to dispose of expired data
 *	(garbage collection)
 *****************************************************************************/

typedef void (*Cache_FreeExpiredData) (const char* key, void* data);


/*****************************************************************************
 * @brief 	Create a cache
 *		The returned object can be destroyed with "talloc_free".
 *		Note: destroying the cache does NOT delete the data
 *		inside the cache.
 *
 * @param context       the talloc parent context
 * @param size          the minimum number of entries in the cache
 *			(entries in excess are removed only if expired)
 * @param max_age	the maximum age in seconds of each cache entry 
 *			(before it expires). Set to zero to disable ageing.
 * @free_expired_data	the function called to dispose of expired data
 *****************************************************************************/
Cache* 
Cache_Create (void* context, size_t size, time_t max_age,
	      Cache_FreeExpiredData free_expired_data);


/******************************************************************************
 * @brief	Returns a pointer to the data for an entry,
 *		creating the entry if not already in the cache.
 *		
 *	Returns NULL if error, else the returned pointer can be dereferenced
 *	to access the cached data ; this data can be get (initially NULL 
 *	for a new entry) or set.
 *****************************************************************************/
void**
Cache_Get (Cache* cache, const char* key);


#if 0 // Not Yet Implemented
/******************************************************************************
 * @brief	Remove an entry from the cache.
 *		
 *	If "expire" is true, the old data is deleted using 
 *	"Cache_FreeExpiredData" and the function returns NULL.
 *	If "expire" is false, the data is removed from the cache but not
 *	deleted, and a pointer to the old data is returned.
 *
 *****************************************************************************/
void*
Cache_Remove (Cache* cache, const char* key);
#endif


/*****************************************************************************
 * @brief Returns the number of cached entries (or -1 if error).
 *****************************************************************************/
long 
Cache_GetNrEntries (const Cache* const cache);


#if TEST_CACHE
/*****************************************************************************
 * @brief Check the whole cache and delete any expired data in it.
 *	  (function for testing purposes only).
 *****************************************************************************/
void
_Cache_PurgeExpiredEntries (Cache* cache);
#endif


/*****************************************************************************
 * @brief Returns a string describing the state of the cache.
 * 	  The returned string should be freed using "talloc_free".
 *****************************************************************************/
char*
Cache_GetStatusString (const Cache* const cache, 
		       void* result_context, const char* spacer);




#endif // DJMOUNT_CACHE_H_INCLUDED


