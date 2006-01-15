/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * djfs : file system implementation for djmount.
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

#include "djfs.h"

#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <inttypes.h>	// Import intmax_t and PRIdMAX

#include "didl_object.h"
#include "file_buffer.h"
#include "media_file.h"
#include "talloc_util.h"
#include "log.h"
#include "content_dir.h"
#include "device_list.h"
#include "xml_util.h"



static const off_t DEFAULT_SIZE = 0; // for unknown file sizes e.g. streams



/*****************************************************************************

 <mount point> -+- devices
                |
                |- <friendlyName> -+- status
                |                  |
                |                  +- xxx  -+- xxx
                |                  |        |- xxx
                |                  |        `- xxx
                |                  |
                |                  |- browse/
                |                  |
                |                  `- search/
                |
                |
                |- <xxxx>
                |
                `- debug/

*****************************************************************************/




/*****************************************************************************
 * @fn     match_start_of_path
 * @brief  tests if the given component(s) match the beginning of the path,
 *         and return the rest of the path.
 *	   Example: match_start_of_path("aa/bb/cc/dd", "aa/bb") -> "cc/dd"
 *****************************************************************************/
static const char*
match_start_of_path (const char* path, const char* name) 
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


/******************************************************************************
 * _DJFS_BrowseCDS
 *****************************************************************************/
const ContentDir_BrowseResult*
_DJFS_BrowseCDS (void* result_context,
		 const char* deviceName, const char* const path, 
		 size_t* nb_char_matched)
{
	if (path == NULL)
		return NULL; // ---------->

	// Create a working context for temporary memory allocations
	void* tmp_ctx = talloc_new (NULL);
	
	// Browse root 
	const char* ptr = path;
	while (*ptr == '/')
		ptr++;
	
	const ContentDir_BrowseResult* current = NULL;
	DEVICE_LIST_CALL_SERVICE (current, deviceName, 
				  CONTENT_DIR_SERVICE_TYPE,
				  ContentDir, BrowseChildren,
				  tmp_ctx, "0");
	
	// Walk path, or until error
	while (*ptr && current && current->children) {
		// Find current directory
		PtrList_Iterator it = PtrList_IteratorStart
			(current->children->objects);
		const DIDLObject* found = NULL;
		while (PtrList_IteratorLoop (&it)) {
			const DIDLObject* o = PtrList_IteratorGetElement (&it);
			if (o->is_container) {
				const char* const p = 
					match_start_of_path (ptr, o->title);
				if (p) {
					ptr = p;
					found = o;
					break; // ---------->
				}
			}
		}
		if (found == NULL) {
			Log_Printf (LOG_DEBUG, "browse '%s' stops at '%s'", 
				    path, ptr);
			goto cleanup; // ---------->
		} else {
			// "id" valid as long as tmp_ctx is not deallocated
			char* id = found->id; 
			DEVICE_LIST_CALL_SERVICE (current, deviceName,
						  CONTENT_DIR_SERVICE_TYPE,
						  ContentDir, BrowseChildren,
						  tmp_ctx, id);
		}
	}
	
 cleanup:
	
	if (current)
		current = talloc_steal (result_context, current);
	else
		Log_Printf (LOG_ERROR, "CDS can't browse '%s' for path='%s'",
			    deviceName, path); 
	
	// Delete all temporary storage
	talloc_free (tmp_ctx);
	tmp_ctx = NULL;
	
	if (nb_char_matched) {
		// Suppress terminating "/" in nb. of character matched
		while (ptr > path && *(ptr-1) == '/')
			ptr--;
		*nb_char_matched = (ptr - path);
	}
	
	return current;
}


/*****************************************************************************
 * DJFS_Browse
 *****************************************************************************/

static inline void
stbuf_set_dir (struct stat* const stbuf)
{
	if (stbuf) {
		stbuf->st_mode  = S_IFDIR | 0555;
		stbuf->st_nlink = 2;			
		stbuf->st_size  = 512;
	}	
}

static inline void
stbuf_set_file (struct stat* const stbuf)
{
	if (stbuf) {	
		stbuf->st_mode  = S_IFREG | 0444;     
		stbuf->st_nlink = 1;
		stbuf->st_size  = DEFAULT_SIZE; // to be computed latter
	}								
}

int
DJFS_Browse (const char* const path, bool playlists,
	     /* for STAT => */	    struct stat* const stbuf, 
	     /* for GETDIR => */    void* h, fuse_dirfil_t filler, 
	     /* for READ => */	    void* _context, FileBuffer** _file)
{
  int rc = 0;

  if (path == NULL || *path == NUL) 
    return -EFAULT; // ---------->

  const char* ptr = path;
  Log_Printf (LOG_DEBUG, "fuse browse : looking for '%s' ...", path);

  // Create a working context for temporary memory allocations
  void* tmp_ctx = talloc_new (NULL);

  // Keep a pointer to acquired lock, if any
  ithread_mutex_t* lock = NULL;


#define ABORT_BROWSE(RC)			\
  do {						\
	  rc = RC;				\
	  goto cleanup;				\
  } while(0)

#define DIR_BEGIN(X)						\
  if (*ptr == NUL) {						\
    if (stbuf) stbuf->st_nlink++;				\
    if (filler) {						\
      rc = filler (h, X, DT_DIR, 0);				\
      if (rc) goto cleanup;					\
    }								\
  } else {							\
    const char* const _p = match_start_of_path (ptr, X);	\
    if (_p) {							\
      ptr = _p;							\
      if (*ptr == NUL) stbuf_set_dir (stbuf);			\
      if (*ptr == NUL && filler) {				\
	rc = filler (h, ".", DT_DIR, 0);			\
	if (rc == 0) rc = filler (h, "..", DT_DIR, 0);		\
	if (rc) goto cleanup;					\
      }								
	
#define DIR_END					\
    } if (*ptr == NUL) goto cleanup;		\
  }

#define FILE_BEGIN(X)						\
  if (*ptr == NUL) {						\
    if (filler) {						\
      rc = filler (h, X, DT_REG, 0);				\
      if (rc) goto cleanup;					\
    }								\
  } else {							\
    const char* const _p = match_start_of_path (ptr, X);	\
    if (_p) {							\
      ptr = _p;							\
      if (*ptr != NUL || filler) {				\
        rc = -ENOTDIR ;						\
	goto cleanup;						\
      }								\
      Log_Printf (LOG_DEBUG, "FILE_BEGIN '%s'", path);		\
      stbuf_set_file (stbuf);					\
      if (_file) *_file = NULL;

#define FILE_SET_SIZE(SIZE)					\
      if (stbuf) {						\
        stbuf->st_size = (SIZE);				\
        Log_Printf (LOG_DEBUG, "FILE_SET_SIZE = %" PRIdMAX,	\
                    (intmax_t) stbuf->st_size);		\
      } 

#define FILE_SET_STRING(CONTENT)					\
      if (_file) {							\
	*_file = FileBuffer_CreateFromString (_context, (CONTENT));	\
        if (*_file)							\
          talloc_set_name (*_file, "file[%s] at " __location__, path);	\
      }

#define FILE_SET_URL(URL)						\
      if (_file) {							\
	*_file = FileBuffer_CreateFromURL (_context, (URL));		\
        if (*_file)							\
          talloc_set_name (*_file, "file[%s] at " __location__, path);	\
      }

#define FILE_END				\
    } if (*ptr == NUL) goto cleanup;		\
  }
  

  DIR_BEGIN("") {

    const StringArray* const names = DeviceList_GetDevicesNames (tmp_ctx);

    FILE_BEGIN("devices") {
      if (names) {
        char* str = talloc_strdup (tmp_ctx, "");
        int i;
	for (i = 0; i < names->nb; i++) {
          str = talloc_asprintf_append (str, "%s\n", names->str[i]);
	}
	FILE_SET_STRING (str);
	FILE_SET_SIZE (str ? strlen (str) : 0);
      }
      // else content defaults to NULL if no devices
    } FILE_END;

    if (names) {
      int i;
      for (i = 0; i < names->nb; i++) {
	const char* const devName = names->str[i];
	DIR_BEGIN(devName) {
	  FILE_BEGIN("status") {
	    const char* const str = DeviceList_GetDeviceStatusString 
	      (tmp_ctx, devName, true);
	    FILE_SET_STRING (str);
	    FILE_SET_SIZE (str ? strlen (str) : 0);
	  } FILE_END;
	  DIR_BEGIN("browse") {
	    size_t nb_matched = 0;
	    const ContentDir_BrowseResult* const res = 
	      _DJFS_BrowseCDS (tmp_ctx, devName, ptr, &nb_matched);
	    if (res && res->children) {
	      char* dirname = talloc_strndup (tmp_ctx, ptr, nb_matched);
	      Log_Printf (LOG_DEBUG, "dirname = '%s'", dirname);
	      if (*dirname == NUL)
		goto skip_dir;
	      DIR_BEGIN (dirname) {
	      skip_dir: ;
  	        DIDLObject* o = NULL;               
		ithread_mutex_lock (&res->children->mutex);
                lock = &res->children->mutex;
		PTR_LIST_FOR_EACH_PTR (res->children->objects, o) {
		  if (o->is_container) {
		    DIR_BEGIN (o->title) {
		    } DIR_END;
		  } else {
		    MediaFile file = { .o = NULL };
		    if (MediaFile_GetPreferred (o, &file)) {
			off_t const res_size = MediaFile_GetResSize (&file);
			if ( file.playlist &&
			     (playlists ||
			      res_size < 0 ||
			      res_size > FILE_BUFFER_MAX_CONTENT_LENGTH) ) {
		        char* name = MediaFile_GetName (tmp_ctx, o, 
							file.playlist);
			FILE_BEGIN (name) {
			  const char* const str = MediaFile_GetPlaylistContent 
			    (&file, tmp_ctx);
			  FILE_SET_STRING (str);
			  FILE_SET_SIZE (str ? strlen (str) : 0);
			} FILE_END;
		      } else {
		        char* name = MediaFile_GetName (tmp_ctx, o, 
							file.extension);
			FILE_BEGIN (name) {
			  FILE_SET_URL (file.uri);
			  if (res_size >= 0)
		            FILE_SET_SIZE (res_size);
			} FILE_END;
		      }
		    }
		    char* const name = MediaFile_GetName (tmp_ctx, o, "xml");
		    FILE_BEGIN (name) {
		      const char* const str = talloc_asprintf 
			(tmp_ctx, 
			 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n%s",
			 DIDLObject_GetElementString (o, tmp_ctx));
		      FILE_SET_STRING (str);
		      FILE_SET_SIZE (str ? strlen (str) : 0);
		    } FILE_END;
		  }
		} PTR_LIST_FOR_EACH_PTR_END;
	      } DIR_END;
	    }
	  } DIR_END; // "browse"
	} DIR_END; // devName
      }
    }

#if DEBUG
    DIR_BEGIN("debug") {
    } DIR_END;

    DIR_BEGIN("test") {
      DIR_BEGIN("a1") {
      } DIR_END;
      
      DIR_BEGIN("a2") {
	DIR_BEGIN("b1") {
	  FILE_BEGIN("f1") {
	    const char* const str = "essais";
	    FILE_SET_SIZE (strlen (str));
	    FILE_SET_STRING (str);
	  } FILE_END;
	} DIR_END;
	
	DIR_BEGIN("b2") {
	  DIR_BEGIN ("c1") {
	  } DIR_END;
	} DIR_END;
      } DIR_END;
      
      DIR_BEGIN("a3") {
	DIR_BEGIN("b3") {
	} DIR_END;
	
	int i;
	for (i = 4; i < 10; i++) {
	  char buffer [10];
	  sprintf (buffer, "b%d", i);
	  DIR_BEGIN(buffer) {
	    DIR_BEGIN ("toto") {
	    } DIR_END;
	    
	  } DIR_END;
	}
      }DIR_END;
    } DIR_END;
#endif // DEBUG


  } DIR_END;

 cleanup:

  // Release any acquired lock
  if (lock)
    ithread_mutex_unlock (lock);

  // Delete all temporary storage
  talloc_free (tmp_ctx);
  tmp_ctx = NULL;

  if (*ptr && rc == 0) 
    rc = -ENOENT;
  
  // Adjust some fields
  if (rc == 0 && stbuf) {
    stbuf->st_blocks = (stbuf->st_size + 511) / 512;
    // TBD not yet implemented : time management
    // XXX   for the time being, just invent some arbitrary time 
    // XXX   different from 0 (epoch)
    struct tm Y2K = { .tm_year = 100, .tm_mon = 0, .tm_mday = 1, 
		      .tm_hour = 12 };
    stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = mktime (&Y2K);
  }

  if (rc) 
    Log_Printf (LOG_DEBUG, 
		"fuse browse => error %d (%s) : path='%s', stops at='%s'", 
		rc, strerror (-rc), path, ptr);
  
  return rc;
}
