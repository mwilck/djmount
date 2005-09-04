/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * charset : charset management for djmount.
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



#ifndef CHARSET_H_INCLUDED
#define CHARSET_H_INCLUDED 1

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>


/*
 * Different character sets are used in djmount :
 *	- UPnP encoding, as used in libupnp
 *	- internal encoding, as used in djmount C code,
 *	- display encoding, for filenames provided to FUSE and VFS
 *
 * UPnP charset
 * ------------
 *	UPnP messages are sent and received in UTF-8.
 *	Although this is not mandatory in "UPnP Device Architecture" v1.0,
 *	this is clearly required in "UPnP Device Architecture v1.0.1 Draft"
 *	(see http://www.upnp.org/resources/documents.asp ). 
 *	Furthermore, the "libupnp" implementation assume UTF-8 in its code.
 * 
 * Internal charset
 * ----------------
 *	The internal character set used in djmount C code must meet the 
 *	following constraints:
 *	  - respects '\0' and '/' characters (assumed in various functions),
 *	  - more generally, respects ASCII characters (assumed in definitions
 *	    of constant strings e.g. printf formats).
 *	These constraints rule out usage of multi-byte encoding not compatible 
 *	with ASCII characters (like UTF-16). Instead, UTF-8 or 8-bits
 *	encodings (e.g. ISO-8859-xxx, CPxxx) should be used.
 *	To avoid unnecessary conversions with the UPnP layer, the internal
 *	charset selected in djmount C code is UTF-8.
 *
 * Display charset
 * ---------------
 *	This character set is used principally for filenames received from 
 *	or sent to the FUSE / VFS layer.
 *	The charset is selected according to user preferences
 *	(e.g. locale of user starting djmount), although obviously any
 *	encoding not compatible with ASCII will not really work
 *	(because of '\0' and '/' assumptions in FUSE or VFS).
 *
 */


/**
 * Conversion direction
 */
typedef enum _Charset_Direction {
	CHARSET_TO_UTF8,
	CHARSET_FROM_UTF8,
} Charset_Direction;


/*****************************************************************************
 * @brief Initializes the charset converter, by selecting a charset.
 *	Must be called before any other charset functions. 
 *	If no charset if provided (charset == NULL), a default
 *	is selected based on the user environment e.g. current locale.
 * @return 0 if ok, non 0 if error e.g. non-supported charset
 *****************************************************************************/
int
Charset_Initialize (const char* charset);


/*****************************************************************************
 * @brief Returns true if module has been initialized with a supported charset,
 *	  other than UTF-8. 
 *	  Returns false if charset is UTF-8, or not yet initialized, or not 
 *	  supported. In this case, all conversion functions below are no-op.
 *****************************************************************************/
bool
Charset_IsConverting();


/*****************************************************************************
 * @brief Convert a string.
 *	a) if not conversion is necessary (e.g. module initialized with UTF-8),
 *	   returns the input string "str",
 *	b) else if "buffer" is provided (not NULL) and is big enough (including
 *   	   terminating '\0') it is used to store the result,
 *  	c) else (buffer NULL or too small) the result string is dynamically 
 *	   allocated with "talloc". It should be freed using "talloc_free" 
 *	   when finished.
 *	The function returns a pointer to the result (== "str" or "buffer" 
 *	or dynamically allocated string), or NULL if error (note that illegal 
 *	character sequences do not abort the conversion but are handled 
 *	internally). 
 *****************************************************************************/
char*
Charset_ConvertString (Charset_Direction, const char* str, 
		       char* buffer, size_t bufsize,
		       void* talloc_context);


/*****************************************************************************
 * @brief Converts and prints a string.
 *	  Return value is similar to "fputs" : a non-negative number for 
 *	  success, or EOF for error (note that illegal character sequences 
 *	  do not abort the conversion but are handled internally). 
 *	  This function is more efficient than calling Charset_ConvertString
 *	  and printing the result because there is never dynamic memory
 *	  allocation.
 *****************************************************************************/
int
Charset_PrintString (Charset_Direction, const char* str, FILE* stream);


/*****************************************************************************
 * Releases resources held by the charset converter.
 *****************************************************************************/
int
Charset_Finish();



#endif // CHARSET_H_INCLUDED

