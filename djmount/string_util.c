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
#include <talloc.h>
#include <ctype.h>


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



