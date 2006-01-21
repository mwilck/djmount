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

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "cache.h"
#include "talloc_util.h"
#include "string_util.h"
#include "log.h"
#include "minmax.h"
#include <time.h>


/*
 * - Define to 1 if the cache is fixed size i.e. one-level deep hash 
 *   structure : the number of entries will never exceed the cache size.
 * - Define to 0 if the cache has a minimum size, but can grow beyond 
 *   this size : entries in excess are removed only if they have expired 
 *   (age too old).
 */
#define CACHE_FIXED_SIZE 0

#if !CACHE_FIXED_SIZE
#	include "hash.h"	// import gnulib hash
#endif


static const time_t CACHE_CLEAN_PERIOD = 10; // seconds



/******************************************************************************
 * Local types
 *****************************************************************************/

typedef struct _Entry {
  
	// "key" points to a valid talloc'ed string
	// (independantly of cached data being valid or not)
	// (never NULL if using hash table implementation, can be NULL if 
	// unused entry in fixed table implementation)
#if CACHE_FIXED_SIZE
	char* 		key;
	size_t		hash;
#else
	const char*	key;
#endif

	// Cached data "data" is valid iff current time <= rip. 
	// In particular:
	//  - "data == NULL" might be a valid cached result,
	//  - "rip == 0" always means cached data is invalid.
	time_t		rip;
	void*		data;
	
} Entry;


struct _Cache {
	size_t		 size;
	time_t		 max_age;	// set to 0 to disable ageing
	time_t		 next_clean;
#if CACHE_FIXED_SIZE
	Entry*		 table;
#else
	Hash_table*	 table;
#endif

	Cache_FreeExpiredData	free_expired_data;

	// Debug statistics
	int		 nr_access;
	int		 nr_hit;
	int		 nr_expired;
#if CACHE_FIXED_SIZE
	int		 nr_collide;
	size_t		 nr_entries;
#endif
};




/******************************************************************************
 * cache_hasher
 *****************************************************************************/

#if !CACHE_FIXED_SIZE
static size_t 
cache_hasher (const void* entry, size_t table_size)
{
	const Entry* const ce = (const Entry*) entry;
	size_t const h = String_Hash (ce->key) % table_size;
	return h;
}
#endif

/******************************************************************************
 * cache_comparator
 *****************************************************************************/

#if !CACHE_FIXED_SIZE
static bool 
cache_comparator (const void* e1, const void* e2)
{
	const Entry* const ce1 = (const Entry*) e1;
	const Entry* const ce2 = (const Entry*) e2;
	return (strcmp (ce1->key, ce2->key) == 0);
}
#endif


/******************************************************************************
 * cache_get
 * 	Lookup and/or update cache 
 *****************************************************************************/

static Entry*
cache_get (Cache* cache, const char* key, bool* hit)
{
#if CACHE_FIXED_SIZE 
	size_t const h   = String_Hash (key);
	size_t const idx = h % cache->size;
	Entry* const ce  = cache->table + idx;
	bool const same_key = (ce->key && ce->hash == h && 
			       strcmp (ce->key, key) == 0);
	if (same_key) {
		*hit = true;
	} else {
		*hit = false;
		if (ce->key) {
			Log_Printf (LOG_DEBUG, 
				    "CACHE_COLLIDE (old='%s', new='%s')",
				    ce->key, key);
			cache->nr_collide++;
			talloc_free (ce->key);
		} else {
			cache->nr_entries++;
		}
		ce->key = talloc_strdup (cache->table, key);
		if (ce->key == NULL)
			return NULL; // ---------->
		ce->hash = h;
	}
#else
	Entry const searched = { .key = key };
	Entry* ce = hash_lookup (cache->table, &searched);
	
	if (ce) {
		*hit = true;
	} else {
		*hit = false;
		ce = talloc (cache, Entry);
		if (ce) {
			ce->key = talloc_strdup (ce, key);
			if (ce->key == NULL)
				return NULL; // ---------->
			ce = hash_insert (cache->table, ce);
		}
	}
#endif
	return ce;
}


/******************************************************************************
 * cache_expire_entries
 *	garbage collection
 *****************************************************************************/

static void
cache_expire_entries (Cache* cache, time_t const now)
{
	if (now > cache->next_clean) {
		int i;
#if CACHE_FIXED_SIZE
		for (i = 0; i < cache->size; i++) {
			Entry* const ce = cache->table + i;
			if (ce->key && now > ce->rip) {
				Log_Printf (LOG_DEBUG, 
					    "CACHE_CLEAN (key='%s')", ce->key);
				if (cache->free_expired_data)
					cache->free_expired_data (ce->key, 
								  ce->data);
				ce->data = NULL;
				talloc_free (ce->key);
				ce->key = NULL;
				cache->nr_entries--;
			}
	
		}
#else
		size_t const n = hash_get_n_entries (cache->table);
		// Pb with "Hash_table" API : it is not allowed to modify 
		// the hash table when walking it. Therefore we have to get 
		// the entries first, then delete the expired ones.
		void* entries [n];
		size_t const nn = hash_get_entries (cache->table, entries, n);
		for (i = 0; i < nn; i++) {
			Entry* ce = (Entry*) entries[i];
			if (now > ce->rip) {
				Log_Printf (LOG_DEBUG, 
					    "CACHE_CLEAN (key='%s')", ce->key);
				ce = hash_delete (cache->table, ce);
				if (ce) {
					if (cache->free_expired_data)
						cache->free_expired_data 
							(ce->key, ce->data);
					ce->data = NULL;
					talloc_free (ce);
				}
			}
		}
#endif
		cache->next_clean = now + CACHE_CLEAN_PERIOD;
	}
}


/******************************************************************************
 * Cache_Get
 *****************************************************************************/
void**
Cache_Get (Cache* cache, const char* key)
{
	if (cache == NULL || key == NULL) {
		Log_Printf (LOG_ERROR, "Cache_Get NULL key or cache");
		return NULL; // ---------->
	}

	/*
	 * Lookup and/or update cache 
	 */   
	cache->nr_access++;

	const time_t now = time (NULL);
	bool hit;
	Entry* const ce  = cache_get (cache, key, &hit);
	if (ce == NULL) {
		Log_Printf (LOG_ERROR, "Cache_Get error, key='%s'", NN(key));
		return NULL; // ---------->
	}

	if (hit) {
		if (cache->max_age == 0 || now <= ce->rip) {
			Log_Printf (LOG_DEBUG, "CACHE_HIT (key='%s')", key);
			cache->nr_hit++;
		} else {
			Log_Printf (LOG_DEBUG, "CACHE_EXPIRED (key='%s')",
				    key);
			cache->nr_expired++;
			if (cache->free_expired_data)
				cache->free_expired_data (ce->key, ce->data);
			ce->rip  = now + cache->max_age;
			ce->data = NULL;
		}
	} else {
		Log_Printf (LOG_DEBUG, "CACHE_NEW (key='%s')", key);
		ce->rip  = now + cache->max_age;
		ce->data = NULL;
		if (cache->max_age > 0) 
			cache_expire_entries (cache, now);
	}
	return &(ce->data); // ---------->
}


/*****************************************************************************
 * Cache_GetStatusString
 *****************************************************************************/
char*
Cache_GetStatusString (const Cache* const cache, 
		       void* result_context, const char* spacer) 
{
	if (cache == NULL)
		return NULL; // ---------->

	char* p = talloc_strdup (result_context, "");

	if (spacer == NULL)
		spacer = "";
	
#define P talloc_asprintf_append 

	p=P(p, "%s+- Cache size      = %ld\n", spacer, (long) cache->size);
	p=P(p, "%s+- Cache max age   = %ld seconds\n", spacer, 
	    (long) cache->max_age);
#if CACHE_FIXED_SIZE
	const long nb_cached = cache->nr_entries;
#else
	const long nb_cached = hash_get_n_entries (cache->table);
#endif
	p=P(p, "%s+- Cached entries  = %ld (%d%%)\n", spacer, nb_cached,
	    (int) (nb_cached * 100 / cache->size));
	p=P(p, "%s+- Cache access    = %d\n", spacer, cache->nr_access);
	if (cache->nr_access > 0) {
		p=P(p, "%s     +- hits       = %d (%.1f%%)\n", spacer, 
		    cache->nr_hit, 
		    (float) (cache->nr_hit * 100.0 / cache->nr_access));
		p=P(p, "%s     +- expired    = %d (%.1f%%)\n", spacer, 
		    cache->nr_expired, 
		    (float) (cache->nr_expired * 100.0 / cache->nr_access));
#if CACHE_FIXED_SIZE
		p=P(p, "%s     +- collide    = %d (%.1f%%)\n", spacer, 
		    cache->nr_collide, 
		    (float) (cache->nr_collide * 100.0 / cache->nr_access));
#endif
	}
#undef P
	return p;
}


/******************************************************************************
 * cache_destroy
 *
 * Description: 
 *	Cache destructor, automatically called by "talloc_free".
 *
 *****************************************************************************/
static int
cache_destroy (void* ptr)
{
	Cache* const cache = (Cache*) ptr;

	if (cache) {
#if !CACHE_FIXED_SIZE
		hash_free (cache->table);
#endif
		cache->table = NULL;
		
		// Other "talloc'ed" fields will be deleted automatically : 
		// nothing to do 
	}
	return 0; // 0 -> ok for talloc to deallocate memory
}


/*****************************************************************************
 * Cache_Create
 *****************************************************************************/
Cache* 
Cache_Create (void* talloc_context, size_t size, time_t max_age,
	      Cache_FreeExpiredData free_expired_data)
{
	if (size < 1) {
		Log_Printf (LOG_ERROR, "Cache: invalid size : %ld\n", 
			    (long) size);
		return NULL; // ---------->
	}
	Cache* cache = talloc (talloc_context, Cache);
	if (cache == NULL)
		return NULL; // ---------->
	*cache = (Cache) { 
		.size    	   = size,
		.max_age 	   = max_age,
		.next_clean	   = 0,
		.free_expired_data = free_expired_data,
		// other data initialized to 0
	};  
#if CACHE_FIXED_SIZE
	cache->table = talloc_array (cache, Entry, size);
	if (cache->table) {
		int i;
		for (i = 0; i < size; i++)
			cache->table[i] = (Entry) { 
				.key  = NULL, 
				.rip  = 0,
				.data = NULL
			};
	}
#else
	cache->table = hash_initialize (size, NULL,
					cache_hasher, cache_comparator,
					NULL);
#endif
	if (cache->table == NULL) {
		Log_Printf (LOG_ERROR, "Cache: can't create table");
		talloc_free (cache);
		return NULL; // ---------->
	}
	
	talloc_set_destructor (cache, cache_destroy);
	return cache;
}

