/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * FileBuffer : access to the content of a file (local or remote).
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

#ifndef FILE_BUFFER_H_INCLUDED
#define FILE_BUFFER_H_INCLUDED

#include <sys/types.h>		// Import "off_t" and "ssize_t"
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 * @var FileBuffer
 *
 *	This opaque type encapsulates the content of a file (local or remote).
 *	
 *      NOTE THAT THE FUNCTION API IS NOT THREAD SAFE. Functions which 
 *	might modify the FileBuffer state (all non-const ones) in different
 *	threads should synchronise accesses through appropriate locking.
 *
 *****************************************************************************/

typedef struct _FileBuffer FileBuffer;


/******************************************************************************
 * @var FILE_BUFFER_MAX_CONTENT_LENGTH
 *
 *	BIG FAT WARNING : the libupnp does not correctly handle
 *	HTTP download/get when CONTENT-LENGTH is bigger than "int",
 *	because the API (and implementation) is using "int" or "unsigned int" 
 *	all over the place. Therefore on a typical 32-bits hardware it is 
 *	not possible to access files with size > 2 Gb.
 *
 *****************************************************************************/

#define FILE_BUFFER_MAX_CONTENT_LENGTH		((uintmax_t) INT_MAX)


/*****************************************************************************
 * @brief 	Creates a new FileBuffer object, with a given string as
 *		content.
 *
 * @param talloc_context	the talloc parent context
 * @param content		the file content (copied internally)
 *****************************************************************************/
FileBuffer*
FileBuffer_CreateFromString (void* talloc_context, const char* content);


/*****************************************************************************
 * @brief 	Creates a new FileBuffer object, with a given url as source.
 *
 * @param talloc_context	the talloc parent context
 * @param url			the source url (copied internally)
 * @param file_size		file size if known, or -1 if not known.
 *****************************************************************************/
FileBuffer*
FileBuffer_CreateFromURL (void* talloc_context, const char* url,
			  off_t file_size);


/*****************************************************************************
 * @brief 	Returns file size if known, or -1 if not known.
 *
 * @param file		the FileBuffer object
 *****************************************************************************/
off_t
FileBuffer_GetSize (const FileBuffer* file);


/*****************************************************************************
 * @brief 	Predicate : true if FileBuffer_Read always return the exact
 *		number of bytes requested (except on EOF or error)
 *
 * @param file		the FileBuffer object
 *****************************************************************************/
bool
FileBuffer_HasExactRead (const FileBuffer* file);


/*****************************************************************************
 * @brief 	Read part of the file, into an existing buffer.
 *		Note: this method might modify the FileBuffer object, 
 *		if the file content needs to be constructed.
 *
 * @param file		the FileBuffer object
 * @param buffer	the memory buffer
 * @param size		size to read
 * @param offset	starting offset
 * @return		number of bytes copied (might be 0), or < 0 if error.
 *****************************************************************************/
ssize_t
FileBuffer_Read (FileBuffer* file, char* buffer, 
		 size_t size, off_t offset);


#ifdef __cplusplus
}; // extern "C"
#endif 


#endif // FILE_BUFFER_H_INCLUDED
