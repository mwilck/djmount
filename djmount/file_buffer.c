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
#include <inttypes.h>	// Import uintmax_t and PRIuMAX

#include <upnp/upnp.h>
#include <upnp/upnptools.h>


// Same definition as in "libupnp/upnp/src/inc/httpreadwrite.h"
#ifndef HTTP_DEFAULT_TIMEOUT
#	define HTTP_DEFAULT_TIMEOUT	(30 + MINIMUM_DELAY)
#endif


struct _FileBuffer {
	const char*	url;
	const char*	content;
	size_t		content_size; 
};


/******************************************************************************
 * FileBuffer_CreateFromString
 *****************************************************************************/
FileBuffer*
FileBuffer_CreateFromString (void* talloc_context, const char* content)
{
	FileBuffer* const file = talloc (talloc_context, FileBuffer);
	if (file) {
		*file = (FileBuffer) {
			.content_size = 0,
			.content      = NULL,
			.url	      = NULL
		};
		if (content) {
			file->content = talloc_strdup (file, content);
			file->content_size = strlen (content);
		}
	}
	return file;
}


/******************************************************************************
 * FileBuffer_CreateFromURL
 *****************************************************************************/
FileBuffer*
FileBuffer_CreateFromURL (void* talloc_context, const char* url)
{
	FileBuffer* const file = talloc (talloc_context, FileBuffer);
	if (file) {
		*file = (FileBuffer) {
			.content_size = 0,
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
 * FileBuffer_IsURL
 *****************************************************************************/
bool
FileBuffer_IsURL (const FileBuffer* file)
{
	return (file && file->url);
}


/******************************************************************************
 * FileBuffer_Read
 *****************************************************************************/

ssize_t
FileBuffer_Read (FileBuffer* file, char* buffer, 
		 size_t size, off_t offset)
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
		if (file->content_size > 0 && offset < file->content_size) {
			n = MIN (size, file->content_size - offset);
			if (n > 0)
				memcpy (buffer, file->content + offset, n);
		}
	} else if (file->url) {
		/*
		 * Read from URL
		 */
		Log_Printf (LOG_DEBUG, 
			    "UPNP Get Http url '%s' "
			    "size %" PRIuMAX " offset %" PRIuMAX,
			    file->url, (uintmax_t) size, (uintmax_t) offset);
		// TBD
		// TBD this is not optimised !! open / close on each read
		// TBD

		void* handle      = NULL;
		int contentLength = 0;
		int httpStatus    = 0;
		char* contentType = NULL;
		// TBD "UpnpOpenHttpGetEx" has strange prototypes for
		// length : "int" is not sufficient for large files !!???
		int rc = UpnpOpenHttpGetEx (file->url, &handle,
					    &contentType, &contentLength,
					    &httpStatus,
					    offset, offset + size,
					    HTTP_DEFAULT_TIMEOUT
					    );
		if (rc != UPNP_E_SUCCESS) 
			goto HTTP_FAIL; // ---------->
		// TBD TBD free contentType ??? I don't know ...

		// TBD "UpnpReadHttpGet" has strange prototypes for
		// length : "u_int" is not sufficient for large files !!???
		unsigned int read_size = size;
		rc = UpnpReadHttpGet (handle, buffer, &read_size,
				      HTTP_DEFAULT_TIMEOUT);
		int rc2 = UpnpCloseHttpGet (handle);
		if (rc != UPNP_E_SUCCESS) 
			goto HTTP_FAIL; // ---------->
		if (rc2 != UPNP_E_SUCCESS) {
			rc = rc2;
			goto HTTP_FAIL; // ---------->
		}
		n = read_size;

	HTTP_FAIL:
		if (rc != UPNP_E_SUCCESS) {
			Log_Printf (LOG_ERROR, 
				    "UPNP Get Http url '%s' error %d (%s)", 
				    file->url, rc, UpnpGetErrorMessage (rc));
			switch (rc) {
			case UPNP_E_OUTOF_MEMORY : 	n = -ENOMEM;
			default:			n = -EIO;
			}
		}
	}
	return n;
}

