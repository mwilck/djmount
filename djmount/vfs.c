/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * vfs : virtual file system implementation for djmount 
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

#include <config.h>

#include "vfs_p.h"

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "didl_object.h"
#include "file_buffer.h"
#include "media_file.h"
#include "talloc_util.h"
#include "log.h"
#include "content_dir.h"
#include "device_list.h"
#include "xml_util.h"



// for unknown file sizes e.g. streams
static const off_t DEFAULT_SIZE = 0; 



/*****************************************************************************
 * vfs_match_start_of_path
 *****************************************************************************/

const char*
vfs_match_start_of_path (const char* path, const char* name) 
{
	size_t const len = strlen (name);
	if (strncmp (path, name, len) == 0 && 
	    (path[len] == NUL || path[len] == '/')) {
		path += len; 
		while (*path == '/') 
			path++;					
		Log_Printf (LOG_DEBUG, "matched '%s' left '%s'", name, path);
		return path;
	}
	return NULL;
}


/*****************************************************************************
 * vfs_begin_dir
 *****************************************************************************/

inline int
vfs_begin_dir (register const VFS_Query* const q)
{
	int rc = 0;

	if (q->stbuf) {
		q->stbuf->st_mode  = S_IFDIR | 0555;
		q->stbuf->st_nlink = 2;			
		q->stbuf->st_size  = 512;
	};		
	
	if (q->filler) {				
		rc = q->filler (q->h, ".", DT_DIR, 0);	
		if (rc == 0)				
			rc = q->filler (q->h, "..", DT_DIR, 0);	
	}
	return rc;
}


/*****************************************************************************
 * vfs_begin_file
 *****************************************************************************/

inline void
vfs_begin_file (register const VFS_Query* const q)
{
	Log_Printf (LOG_DEBUG, "FILE_BEGIN '%s'", q->path);    
	
	if (q->stbuf) {	
		q->stbuf->st_mode  = S_IFREG | 0444;     
		q->stbuf->st_nlink = 1;
		q->stbuf->st_size  = DEFAULT_SIZE; // to be computed latter
	}

	if (q->file) 
		*(q->file) = NULL;
}


/*****************************************************************************
 * BrowseDebug
 *****************************************************************************/

static VFS_BrowseStatus
BrowseDebug (VFS* const self, const char* const sub_path,
	     const VFS_Query* const query, void* const tmp_ctx)
{
	BROWSE_BEGIN(sub_path, query) {
	   
		FILE_BEGIN("talloc_total") {
			const char* const str = talloc_asprintf 
				(tmp_ctx, "%" PRIdMAX " bytes\n",
				 (intmax_t) talloc_total_size (NULL));
			// Don't dump talloc_total_blocks because
			// crash on NULL context ...
			FILE_SET_SIZE (str ? strlen (str) : 0);
			FILE_SET_STRING (str, true);
		} FILE_END;
		
		FILE_BEGIN("talloc_report") {
			StringStream* const ss = StringStream_Create (tmp_ctx);
			FILE* const file = StringStream_GetFile (ss);
			talloc_report (NULL, file);
			size_t size = 0;
			const char* const str = StringStream_GetSnapshot 
				(ss, tmp_ctx, &size);
			FILE_SET_SIZE (size);
			FILE_SET_STRING (str, true);
			// close stream as early as possible 
			// to avoid too many open files
			talloc_free (ss); 
		} FILE_END;
		
		FILE_BEGIN("talloc_report_full") {
			StringStream* const ss = StringStream_Create (tmp_ctx);
			FILE* const file = StringStream_GetFile (ss);
			talloc_report_full (NULL, file);
			size_t size = 0;
			const char* const str = StringStream_GetSnapshot 
				(ss, tmp_ctx, &size);
			FILE_SET_SIZE (size);
			FILE_SET_STRING (str, true);
			// close stream as early as possible 
			// to avoid too many open files
			talloc_free (ss); 
		} FILE_END;
		
	} BROWSE_END;
	return BROWSE_RESULT;
}


/*****************************************************************************
 * VFS_Browse
 *****************************************************************************/

int
VFS_Browse (VFS* const self, const VFS_Query* q)
{
	if (q == NULL || q->path == NULL || *(q->path) == NUL)
		return -EFAULT; // ---------->

	Log_Printf (LOG_DEBUG, "fuse browse : looking for '%s' ...", q->path);
	
	// Create a working context for temporary memory allocations
	void* tmp_ctx = talloc_new (NULL);
	
	BROWSE_BEGIN(q->path, q) {
		DIR_BEGIN("") {
			VFS_BrowseFunction b = 
				OBJECT_METHOD (self, browse_root);
			if (b) {
				BROWSE_SUB (b (self, BROWSE_PTR, q, tmp_ctx));
			}
#if DEBUG
			if (self->show_debug_dir) {
				DIR_BEGIN(".debug") {
					b = OBJECT_METHOD (self, browse_debug);
					if (b) {
						BROWSE_SUB (b (self, 
							       BROWSE_PTR, 
							       q, tmp_ctx));
					}
				} DIR_END;
			}
#endif 
		} DIR_END;
	} BROWSE_END;
	
	VFS_BrowseStatus s = BROWSE_RESULT;
   
	// Sanity check - should not be possible
	if (*s.ptr != NUL && s.rc == 0) {
		Log_Printf (LOG_WARNING, 
			    "VFW_Browse : inconsistent result, path='%s'",
			    q->path);
		s.rc = -ENOENT;
	}

	// Delete all temporary storage
	talloc_free (tmp_ctx);
	tmp_ctx = NULL;
	
	// Adjust some fields
	if (s.rc == 0 && q->stbuf) {
		q->stbuf->st_blocks = (q->stbuf->st_size + 511) / 512;
		// TBD not yet implemented : time management
		// XXX   for the time being, just invent some arbitrary time 
		// XXX   different from 0 (epoch)
		struct tm Y2K = { .tm_year = 100, .tm_mon = 0, .tm_mday = 1, 
				  .tm_hour = 12 };
		q->stbuf->st_atime = q->stbuf->st_mtime = q->stbuf->st_ctime = 
			mktime (&Y2K);
	}
	
	if (s.rc) 
		Log_Printf (LOG_DEBUG, "fuse browse => error %d (%s) : "
			    "path='%s', stops at='%s'", 
			    s.rc, strerror (-s.rc), q->path, s.ptr);
	
	return s.rc;
}


/*****************************************************************************
 * OBJECT_INIT_CLASS
 *****************************************************************************/

// Initialize class methods
static void
init_class (VFS_Class* const isa)
{
        isa->browse_root  = NULL;
        isa->browse_debug = BrowseDebug;
}

OBJECT_INIT_CLASS(VFS, Object, init_class);


/*****************************************************************************
 * VFS_Create
 *****************************************************************************/
VFS*
VFS_Create (void* talloc_context, bool show_debug_dir)
{
	OBJECT_SUPER_CONSTRUCT (VFS, Object_Create, talloc_context, NULL);
        if (self) {
		self->show_debug_dir = show_debug_dir;
	}
	return self;
}


