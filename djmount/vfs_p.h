/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * vfs : virtual file system implementation for djmount 
 * (private / protected part).
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

#ifndef VFS_P_INCLUDED
#define VFS_P_INCLUDED 1

#include "vfs.h"
#include "object_p.h"

#include <errno.h>
#include <dirent.h>
#include <inttypes.h>	// Import intmax_t and PRIdMAX
#include <string.h>
#include <sys/types.h>	// Import "off_t"



/******************************************************************************
 *
 *      VFS private / protected implementation ; do not include directly.
 *
 *****************************************************************************/

OBJECT_DEFINE_STRUCT(VFS,
		     
		     bool show_debug_dir;
		     
                     );


typedef struct _VFS_BrowseStatus {
    int rc;
    const char* ptr;
} VFS_BrowseStatus;

typedef VFS_BrowseStatus (*VFS_BrowseFunction) (VFS* self,
						const char* const sub_path, 
						const VFS_Query* query, 
						void* tmp_context);

OBJECT_DEFINE_METHODS(VFS,
		      
		      VFS_BrowseFunction browse_root;

		      VFS_BrowseFunction browse_debug;

                      );




/******************************************************************************
 *
 *     HELPERS
 *
 *****************************************************************************/




/*****************************************************************************
 * @fn     vfs_match_start_of_path
 * @brief  tests if the given component(s) match the beginning of the path,
 *         and return the rest of the path.
 *	   Example: match_start_of_path("aa/bb/cc/dd", "aa/bb") -> "cc/dd"
 *****************************************************************************/
const char*
vfs_match_start_of_path (const char* path, const char* name);


/*****************************************************************************
 * Browse helpers
 *****************************************************************************/

extern int
vfs_dir_begin (register const VFS_Query* const q);

extern int
vfs_file_begin (register const VFS_Query* const q, int const d_type);

static inline int
vfs_dir_add_entry (const char* const name, int const d_type,
		   register const VFS_Query* const q)
{
	int rc = 0;
	
	if (q->stbuf && d_type == DT_DIR)
		q->stbuf->st_nlink++;					
	
	if (q->filler) 
		rc = q->filler (q->h, name, d_type, 0);		
	
	return rc;
}

extern void
vfs_file_set_string (const char* const str, bool steal,
		     const char* const location,
		     register const VFS_Query* const q);

extern void
vfs_file_set_url (const char* const url, off_t size,
		  const char* const location,
		  register const VFS_Query* const q);

static inline void
vfs_symlink_set_path (const char* const p,
		      register const VFS_Query* const q)
{
	if (p && q->lnk_buf && q->lnk_bufsiz > 0) {		
		strncpy (q->lnk_buf, p, q->lnk_bufsiz);	
		q->lnk_buf[q->lnk_bufsiz-1] = '\0';
	}
	if (q->stbuf) {	
		q->stbuf->st_size = p ? strlen (p) : 0;	
	}	
}


/*****************************************************************************
 * Browse Helper Macros
 *****************************************************************************/



#define BROWSE_BEGIN(PATH,QUERY)			\
	VFS_BrowseStatus _s = { .rc = 0, .ptr = PATH };	\
	register const VFS_Query* const _q = QUERY;	\
	if (_q == NULL || _s.ptr == NULL) {		\
		_s.rc = -EFAULT;			\
	} else {

#define BROWSE_PTR		_s.ptr

#define BROWSE_SUB(SUB)							\
	if (*_s.ptr == '\0') {						\
		_s = SUB;						\
		if (_s.rc) goto cleanup;				\
	} else {							\
		_s = SUB;						\
		if (*_s.ptr == '\0' || _s.rc != 0) goto cleanup;	\
	}

#define BROWSE_ABORT(RC)			\
	do {					\
		_s.rc = RC;			\
		goto cleanup;			\
	} while(0)

#define BROWSE_END				\
	}					\
    	cleanup:

#define BROWSE_RESULT		_s


#define DIR_BEGIN(X)							\
	if (*_s.ptr == '\0') {						\
		_s.rc = vfs_dir_add_entry (X, DT_DIR, _q);		\
		if (_s.rc) goto cleanup;				\
	} else {							\
		const char* const _p = vfs_match_start_of_path (_s.ptr, X); \
		if (_p) {						\
			_s.ptr = _p;					\
			if (*_s.ptr == '\0') {				\
				_s.rc = vfs_dir_begin (_q);		\
				if (_s.rc) goto cleanup;		\
			}   			

#define DIR_END								\
			if (*_s.ptr != '\0') BROWSE_ABORT(-ENOENT);	\
		} if (*_s.ptr == '\0') goto cleanup;			\
	}
	

#define _FILE_BEGIN(X,D_TYPE)						\
	if (*_s.ptr == '\0') {						\
		_s.rc = vfs_dir_add_entry (X, D_TYPE, _q);		\
	} else {							\
		const char* const _p = vfs_match_start_of_path (_s.ptr, X); \
		if (_p) {						\
			_s.ptr = _p;					\
			if (*_s.ptr != '\0')				\
				_s.rc = -ENOTDIR;			\
			else						\
				_s.rc = vfs_file_begin (_q, D_TYPE);	\
			if (_s.rc) goto cleanup;

#define FILE_BEGIN(X)	_FILE_BEGIN(X, DT_REG)

#define FILE_SET_STRING(CONTENT,STEAL)					\
	vfs_file_set_string ((CONTENT), (STEAL), __location__, _q)

#define FILE_SET_URL(URL,SIZE)					\
	vfs_file_set_url ((URL), (SIZE), __location__, _q)

#define _FILE_END					\
		} if (*_s.ptr == '\0') goto cleanup;	\
	}

#define FILE_END	_FILE_END


#define SYMLINK_BEGIN(X)	_FILE_BEGIN(X, DT_LNK)

#define SYMLINK_SET_PATH(PATH)	vfs_symlink_set_path (PATH, _q)
						
#define SYMLINK_END		_FILE_END



#endif // VFS_P_INCLUDED


