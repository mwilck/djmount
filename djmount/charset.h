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


/*****************************************************************************
 * @brief Initializes the charset converter, by selecting a charset.
 *	Must be called before any other charset functions. 
 *	May be called multiple times.
 *	If no charset if provided (charset == NULL), a default
 *	is selected based on the user environment e.g. current locale.
 * @return 0 if ok, non 0 if error e.g. non-supported charset
 *****************************************************************************/
int
Charset_Initialize (const char* charset);


/*****************************************************************************
 *	Return the buffer size necessary to convert a string 
 *	from selected charset to UTF-8 (includes  terminating '\0').
 *	This is a quick estimate : the size returned might be slightly
 *	bigger than strictly necessary.
 *****************************************************************************/
size_t
Charset_ToUtf8Size (const char* src);


/*****************************************************************************
 * @brief Convert a string from selected charset to UTF-8.
 *	  Result string is put into provided buffer "dest" of 
 *	  maximum size (including terminating '\0') "dest_size".
 *****************************************************************************/
char*
Charset_ToUtf8 (const char* src, char* dest, size_t dest_size);


/*****************************************************************************
 * @brief Convert a string from selected charset to UTF-8.
 *	  Result string is dynamically allocated with "talloc" ;
 *	  It should be freed using "talloc_free" when finished.
 *****************************************************************************/
char*
Charset_ToUtf8_talloc (void* talloc_context, const char* str);


/*****************************************************************************
 *	Return the buffer size necessary to convert a string 
 *	from UTF-8 to selected charset (includes  terminating '\0').
 *	This is a quick estimate : the size returned might be slightly
 *	bigger than strictly necessary.
 *****************************************************************************/
size_t
Charset_FromUtf8Size (const char* src);


/*****************************************************************************
 * @brief Convert a string from UTF-8 to selected charset.
 *	  Result string is put into provided buffer "dest" of 
 *	  maximum size (including terminating '\0') "dest_size".
 *****************************************************************************/
char*
Charset_FromUtf8 (const char* src, char* dest, size_t dest_size);


/*****************************************************************************
 * @brief Convert a string from UTF-8 to selected charset.
 *	  Result string is dynamically allocated with "talloc" ;
 *	  It should be freed using "talloc_free" when finished.
 *****************************************************************************/
char*
Charset_FromUtf8_talloc (void* talloc_context, const char* str);


/*****************************************************************************
 * Releases resources held by the charset converter.
 *****************************************************************************/
int
Charset_Finish();



#endif // CHARSET_H_INCLUDED

