/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * Get file information for the media files representing DIDL-Lite objects.
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

#ifndef MEDIA_FILE_H_INCLUDED
#define MEDIA_FILE_H_INCLUDED

#include "didl_object.h"
#include <sys/types.h>		// Import "off_t" 


#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 * @var MediaFormat
 *
 *	This public type encapsulates information for a particular format
 *	of media file, associated to a specific DIDL-Lite object.
 *
 *****************************************************************************/

typedef struct _MediaFile {
	const DIDLObject* 	o;
	char			extension [10];
	const char*		playlist;
	const char*  		uri;
	IXML_Element*  		res;
} MediaFile;



/*****************************************************************************
 * @brief 	Returns the preferred (default) format associated to
 *		a DIDL-Lite object.
 *
 * @param o	the DIDLObject.
 * @param file	the returned MediaFile information
 * @return	true if success, false if error
 *****************************************************************************/
bool
MediaFile_GetPreferred (const DIDLObject* o, OUT MediaFile* file);


/*****************************************************************************
 * @brief 	Returns the basename of a file associated to a MediaFile.
 *
 * @param result_context	parent context to allocate result, may be NULL
 * @param o			the DIDLObject
 * @param extension		the extension e.g. "mp3"
 *****************************************************************************/
char*
MediaFile_GetName (void* result_context, 
		   const DIDLObject* o, const char* extension);


/*****************************************************************************
 * @brief 	Returns the size of the <res> URL associated to
 *		a DIDL-Lite object, or -1 if no size known (not provided
 *		by server, or stream).
 *
 * @param format	the selected format
 * @param o		the DIDL-Lite object
 *****************************************************************************/
off_t
MediaFile_GetResSize (const MediaFile* const file);


/*****************************************************************************
 * @brief       Returns the complete content of the playlist file.
 *
 * @param file                  the MediaFile object
 * @param result_context        parent context to allocate result, may be NULL
 *****************************************************************************/
char*
MediaFile_GetPlaylistContent (const MediaFile* file, void* result_context);


#ifdef __cplusplus
}; // extern "C"
#endif 


#endif // MEDIA_FILE_H_INCLUDED
