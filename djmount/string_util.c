/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * String utilities.
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

#include "string_util.h"
#include <ctype.h>
#include "talloc_util.h"


/*****************************************************************************
 * String_StripSpaces
 *****************************************************************************/
char*
String_StripSpaces (void* talloc_context, const char* s)
{
  char* r = 0;
  if (s) {
    while (isspace (*s)) 
      s++;

    r = talloc_strdup (talloc_context, s);
    
    if (*r != '\0') {
      char* p = r + strlen (r) -1;
      // This loop will stop because the first character at least isn't
      // a blank (already stripped).
      while (isspace (*p))
	*p-- = '\0';
    }
  }
  return r;
}


/*****************************************************************************
 * String_CleanFileName
 *****************************************************************************/
char*
String_CleanFileName (void* talloc_context, const char* s)
{
  char* r = String_StripSpaces (talloc_context, s);
  if (r) {
    char* p;
    for (p = r; *p; p++) {
      if (*p == '/')
	*p = '_';
    }
  }
  return r;
}


/*****************************************************************************
 * _String_ToInteger
 *****************************************************************************/
intmax_t
_String_ToInteger (const char* s, intmax_t error_value)
{
	intmax_t ret = error_value;
	if (s && *s) {
		char* endptr = 0;
		intmax_t val = strtoimax (s, &endptr, 10);
		if (endptr) {
			// skip tailing spaces
			while (isspace (*endptr))
				endptr++;
			if (*endptr == '\0')
				ret = val;
		}
	}
	return ret;
}


/*****************************************************************************
 * String_Hash
 * Refer to :
 *	http://www.cs.yorku.ca/~oz/hash.html
 *	http://burtleburtle.net/bob/hash/doobs.html
 *****************************************************************************/

//#define HASH_ALGO_SDBM  1
#define HASH_ALGO_DJB2_XOR  1

String_HashType
String_Hash (const char* str)
{
#if HASH_ALGO_SDBM
  // This is a SDBM hash algo. 
  register String_HashType hash = 0;
  unsigned char c;

  while ((c = *str++)) {
    hash = c + (hash << 6) + (hash << 16) - hash;
  }
#elif HASH_ALGO_DJB2
  // This is a DJB2 hash algo. 
  register String_HashType hash = 5381;
  unsigned char c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c 
  }
#elif HASH_ALGO_DJB2_XOR
  register String_HashType hash = 5381; 
  unsigned char c; 

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) ^ c; // hash * 33 xor c 
  }
#endif
  return hash;
}



/*****************************************************************************
 * StringStream
 *****************************************************************************/

struct _StringStream {
	FILE*  file;
#if HAVE_OPEN_MEMSTREAM
	char*  ptr;
	size_t size;
#else
#	warning no open_memstream - using default implementation with tmpfile,
#	warning check if a better replacement exists in your environment.
#endif
};


static int
StringStream_destructor (void* ptr) 
{
	if (ptr) {
		StringStream* const ss = (StringStream*) ptr;
		if (ss->file) {
			(void) fclose (ss->file);
			ss->file = NULL;
		}
#if HAVE_OPEN_MEMSTREAM
		if (ss->ptr) {
			free (ss->ptr);
			ss->ptr = NULL;
		}
#endif
	}
	return 0; // success
}

/*****************************************************************************
 * StringStream_Create
 *****************************************************************************/

StringStream* 
StringStream_Create (void* parent_context)
{
	StringStream* ss = talloc (parent_context, StringStream);
	if (ss) {
#if HAVE_OPEN_MEMSTREAM
		*ss = (StringStream) { .ptr = NULL, .size = 0 };
		ss->file = open_memstream (&(ss->ptr), &(ss->size));
#else 
		ss->file = tmpfile();
#endif
		if (ss->file == NULL) {
			talloc_free (ss);
			ss = NULL;
		} else {
			talloc_set_destructor (ss, StringStream_destructor);
		}
	}
	return ss;
}


/*****************************************************************************
 * StringStream_GetFile
 *****************************************************************************/

FILE*
StringStream_GetFile (const StringStream* ss)
{
	return (ss ? ss->file : NULL);
}


/*****************************************************************************
 * StringStream_GetSnapshot
 *****************************************************************************/

char*
StringStream_GetSnapshot (StringStream* ss, void* result_context,
			  size_t* slen)
{
	char* res = NULL;
	if (ss && ss->file) {
#if HAVE_OPEN_MEMSTREAM
		fflush (ss->file);
		res = talloc_strdup (result_context, ss->ptr);
		if (res && slen)
			*slen = ss->size;
#else
		off_t const size = ftello (ss->file);
		res = talloc_size (result_context, size+1);
		if (res) {
			rewind (ss->file);
			if (fread (res, size, 1, ss->file) < 1) {
				talloc_free (res);
				res = NULL;
			} else {
				res[size] = '\0';
				if (slen)
					*slen = size;
			}
		}
#endif
	}
	return res;
}




