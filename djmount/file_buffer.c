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

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "file_buffer.h"
#include "talloc_util.h"
#include "log.h"
#include "minmax.h"

#include <string.h>
#include <errno.h>
#include <inttypes.h>	// Import intmax_t and PRIdMAX

#include <upnp/upnp.h>
#include <upnp/upnptools.h>


// Same definition as in "libupnp/upnp/src/inc/httpreadwrite.h"
#ifndef HTTP_DEFAULT_TIMEOUT
#	define HTTP_DEFAULT_TIMEOUT	30
#endif


struct _FileBuffer {
	bool		exact_read;
	off_t		file_size; 
	const char*	url;
	const char*	content;
};


/******************************************************************************
 * FileBuffer_CreateFromString
 *****************************************************************************/
FileBuffer*
FileBuffer_CreateFromString (void* talloc_context, const char* content,
			     FileBuffer_StringAlloc alloc)
{
	FileBuffer* const file = talloc (talloc_context, FileBuffer);
	if (file) {
		*file = (FileBuffer) {
			.exact_read   = true,
			.file_size    = 0,
			.content      = NULL,
			.url	      = NULL
		};
		if (content) {
			switch (alloc) {
			case FILE_BUFFER_STRING_STEAL:
				file->content = talloc_steal (file, content);
				break;
			case FILE_BUFFER_STRING_EXTERN:
				file->content = content;
				break;
			case FILE_BUFFER_STRING_COPY:
			default:
				file->content = talloc_strdup (file, content);
				break;
			}
			file->file_size = strlen (content);
		}
	}
	return file;
}


/******************************************************************************
 * FileBuffer_CreateFromURL
 *****************************************************************************/
FileBuffer*
FileBuffer_CreateFromURL (void* talloc_context, const char* url, 
			  off_t file_size)
{
	FileBuffer* const file = talloc (talloc_context, FileBuffer);
	if (file) {
		*file = (FileBuffer) {
			.exact_read   = (file_size >= 0),
			.file_size    = file_size,
			.content      = NULL,
			.url	      = NULL
		};
		if (url) {
			file->url = talloc_strdup (file, url);
		}
	}
	return file;
}


/*****************************************************************************
 * FileBuffer_GetSize
 *****************************************************************************/
off_t
FileBuffer_GetSize (const FileBuffer* file)
{
	return (file && file->file_size);
}


/*****************************************************************************
 * FileBuffer_HasExactRead
 *****************************************************************************/
bool
FileBuffer_HasExactRead (const FileBuffer* file)
{
	return (file && file->exact_read);
}


/******************************************************************************
 * FileBuffer_Read
 *****************************************************************************/

ssize_t
FileBuffer_Read (FileBuffer* file, char* buffer, 
		 size_t size, const off_t offset)
{
	ssize_t n = 0;
	if (file == NULL) {
		n = -EINVAL;
	} else if (buffer == NULL) {
		n = -EFAULT;
	} else if (file->content) {
		/*
		 * Read from memory
		 */
		if (file->file_size > 0 && offset < file->file_size) {
			n = MIN (size, file->file_size - offset);
			if (n > 0)
				memcpy (buffer, file->content + offset, n);
		}
	} else if (file->url) {
		/*
		 * Read from URL
		 */

		Log_Printf (LOG_DEBUG, 
			    "GetHttp url '%s' size %" PRIdMAX 
			    " offset %" PRIdMAX " (file_size %" PRIdMAX ")",
			    file->url, (intmax_t) size, (intmax_t) offset,
			    (intmax_t) file->file_size);
		
		/*
		 * Warning : the libupnp API (UpnpOpenHttpGetEx, 
		 * UpnpReadHttpGet ...) has strange prototypes for length 
		 * and ranges : "int" is not sufficient for large files !
		 */
		if (offset > FILE_BUFFER_MAX_CONTENT_LENGTH ||
		    offset > FILE_BUFFER_MAX_CONTENT_LENGTH - size) {
			Log_Printf (LOG_ERROR, 
				    "GetHttp url '%s' overflowed "
				    "size %" PRIdMAX " or offset %" PRIdMAX,
				    file->url, (intmax_t) size, 
				    (intmax_t) offset);
			return -EOVERFLOW; // ---------->
		}

		// Adjust request to file size, if known
		if (file->file_size >= 0) {
			if (offset > file->file_size - size) {
				size = MAX (0, file->file_size - offset);
				Log_Printf (LOG_DEBUG, 
					    "GetHttp truncate to size %" 
					    PRIdMAX, (intmax_t) size);
			}
		}

		// TBD
		// TBD this is not optimised !! open / close on each read
		// TBD

		void* handle      = NULL;
		int contentLength = 0;
		int httpStatus    = 0;
		char* contentType = NULL;
		int rc = UpnpOpenHttpGetEx (file->url, &handle,
					    &contentType, &contentLength,
					    &httpStatus,
					    offset, offset + size,
					    HTTP_DEFAULT_TIMEOUT
					    );
		if (rc != UPNP_E_SUCCESS) 
			goto HTTP_CHECK; // ---------->
		// TBD TBD free contentType ??? I don't know ...

		/*
		 * Read available bytes, or all bytes requested if exact_read :
		 * perform a loop because I am not sure that HTTP GET guaranty
		 * to return the exact number of bytes requested.
		 */
		do {
			unsigned int read_size = size - n;
			if (n > 0) {
				Log_Printf (LOG_DEBUG, 
					    "UpnpReadHttpGet loop ! url '%s' "
					    "read %" PRIdMAX " left %" PRIdMAX,
					    file->url, (intmax_t) n, 
					    (intmax_t) read_size);
			}
			
			rc = UpnpReadHttpGet (handle, buffer + n, &read_size,
					      HTTP_DEFAULT_TIMEOUT);
			if (rc != UPNP_E_SUCCESS) {
				(void) UpnpCloseHttpGet (handle);
				goto HTTP_CHECK; // ---------->
			}

			// Prevent infinite loop (shouldn't happen though)
			if (read_size == 0)
				break; // ---------->
			n += read_size;

		} while (file->exact_read && n < size);

		rc = UpnpCloseHttpGet (handle);

	HTTP_CHECK:
		if (rc != UPNP_E_SUCCESS) {
			Log_Printf (LOG_ERROR, 
				    "GetHttp url '%s' (size %" PRIdMAX 
				    ", offset %" PRIdMAX ", file_size %" 
				    PRIdMAX ") : error %d (%s)", 
				    file->url, (intmax_t) size, 
				    (intmax_t) offset, 
				    (intmax_t) file->file_size, 
				    rc, UpnpGetErrorMessage (rc));
			switch (rc) {
			case UPNP_E_OUTOF_MEMORY : 	n = -ENOMEM; break;
			default:			n = -EIO;    break;
			}
		}
	}
	return n;
}

