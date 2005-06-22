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


#ifndef STRING_UTIL_H_INCLUDED
#define STRING_UTIL_H_INCLUDED

#include <string.h>
#include <stdio.h>	// Needed to compile "talloc.h"
#include <stdarg.h>	// Needed to compile "talloc.h"
#include <stdlib.h>	// Needed to compile "talloc.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <talloc.h>


#define NUL	'\0'


/*****************************************************************************
 * StringArray
 *****************************************************************************/

typedef struct StringArrayStruct {
  
  int    nb;
  char*  str[]; // struct hack, aka C99 "Flexible array member"

} StringArray;


#define StringArray_talloc(ctx,nb) \
  talloc_zero_size ((ctx), sizeof (StringArray) + (nb) * sizeof (char*))


/*****************************************************************************
 * StringPair
 *****************************************************************************/

typedef struct StringPairStruct {

  char* name;
  char* value;

} StringPair;



/*****************************************************************************
 * Strip whitespaces from the start and end of string.  
 * Return a pointer to a newly allocated copy, which must be deallocated
 * with 'talloc_free'
 * Functions
 *****************************************************************************/
char*
String_StripSpaces (void* talloc_context, const char* s);


/*****************************************************************************
 * Make string suitable for filename i.e. strip whitespaces from the start 
 * and end of string, and replace '/' with '_' characters.
 * Return a pointer to a newly allocated copy, which must be deallocated
 * with 'talloc_free'
 * Functions
 *****************************************************************************/
char*
String_CleanFileName (void* talloc_context, const char* s);


/*****************************************************************************
 * Hash a string
 *****************************************************************************/
uint32_t
String_Hash (const char* str);



#ifdef __cplusplus
}; // extern "C"
#endif



#endif // STRING_UTIL_H_INCLUDED




