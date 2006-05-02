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


#ifndef STRING_UTIL_H_INCLUDED
#define STRING_UTIL_H_INCLUDED

#include <string.h>
#include <inttypes.h>
#include "talloc_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUL	'\0'


/*****************************************************************************
 * StringArray
 *****************************************************************************/

typedef struct _StringArray {
  
  int    nb;
  char*  str[]; // struct hack, aka C99 "Flexible array member"

} StringArray;


#define StringArray_talloc(ctx,nb) \
  talloc_zero_size ((ctx), sizeof (StringArray) + (nb) * sizeof (char*))


/*****************************************************************************
 * StringPair
 *****************************************************************************/

typedef struct _StringPair {

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
 * @fn		STRING_TO_INT
 * @brief  	Convert string to a (signed) integer.
 *		White spaces at begin or end of string are ignored.
 *		The range of the target variable is checked as well, to
 *		prevent overflows / underflows (work for unsigned integers 
 *		as well, excluding maximum uintmax_t).
 *
 * @param S		the string
 * @param VAR		variable to hold the result
 * @param ERRVAL	value to set in case of error e.g. conversion error
 *****************************************************************************/

intmax_t
_String_ToInteger (const char* s, intmax_t error_value);

#define STRING_TO_INT(S,VAR,ERRVAL)			\
	do {						\
		intmax_t const __temp_s2i_var =		\
			_String_ToInteger (S, ERRVAL);	\
		VAR = __temp_s2i_var;			\
		if (VAR != __temp_s2i_var)		\
			VAR = ERRVAL;			\
	} while (0)					


/*****************************************************************************
 * @fn 		String_Hash
 * @brief	Hash a string
 *****************************************************************************/
typedef uint32_t String_HashType;

String_HashType
String_Hash (const char* str);



#ifdef __cplusplus
}; // extern "C"
#endif



#endif // STRING_UTIL_H_INCLUDED




