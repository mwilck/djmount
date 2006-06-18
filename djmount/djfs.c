/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
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

#include "djfs_p.h"
#include "didl_object.h"
#include "file_buffer.h"
#include "media_file.h"
#include "talloc_util.h"
#include "log.h"
#include "content_dir.h"
#include "device_list.h"
#include "xml_util.h"




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
                `- .debug/

*****************************************************************************/




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
    PtrList_Iterator it = PtrList_IteratorStart (current->children->objects);
    const DIDLObject* found = NULL;
    while (PtrList_IteratorLoop (&it)) {
      const DIDLObject* o = PtrList_IteratorGetElement (&it);
      if (o->is_container) {
	const char* const p = vfs_match_start_of_path (ptr, o->basename);
	if (p) {
	  ptr = p;
	  found = o;
	  break; // ---------->
	}
      }
    }
    if (found == NULL) {
      Log_Printf (LOG_DEBUG, "browse '%s' stops at '%s'", path, ptr);
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
 * BrowseRoot
 *****************************************************************************/

static VFS_BrowseStatus
BrowseRoot (VFS* const vfs, const char* const sub_path,
	    const VFS_Query* const query, void* const tmp_ctx)
{
  DJFS* const self = (DJFS*) vfs;

  // Keep a pointer to acquired lock, if any
  ithread_mutex_t* lock = NULL;
  
  BROWSE_BEGIN(sub_path, query) {
    
    const StringArray* const names = DeviceList_GetDevicesNames (tmp_ctx);
    
    FILE_BEGIN("devices") {
      if (names) {
	char* str = talloc_strdup(tmp_ctx, "");
	int i;
	for (i = 0; i < names->nb; i++) {
	  str = talloc_asprintf_append (str, "%s\n", names->str[i]);
	}
	FILE_SET_STRING (str, true);
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
	    FILE_SET_STRING (str, true);
	  } FILE_END;
	  DIR_BEGIN("browse") {
	    size_t nb_matched = 0;
	    const ContentDir_BrowseResult* const res = 
	      _DJFS_BrowseCDS (tmp_ctx, devName, BROWSE_PTR, &nb_matched);
	    if (res && res->children) {
	      char* dirname = talloc_strndup (tmp_ctx, BROWSE_PTR, nb_matched);
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
		    DIR_BEGIN (o->basename) {
		    } DIR_END;
		  } else {
		    MediaFile file = { .o = NULL };
		    if (MediaFile_GetPreferred (o, &file)) {
		      off_t const res_size = MediaFile_GetResSize (&file);
		      if ( file.playlist &&
			   ( (self->flags & DJFS_USE_PLAYLISTS) ||
			     res_size < 0 ||
			     res_size > FILE_BUFFER_MAX_CONTENT_LENGTH) ) {
			char* name = MediaFile_GetName (tmp_ctx, o, 
							file.playlist);
			FILE_BEGIN (name) {
			  const char* const str = MediaFile_GetPlaylistContent 
			    (&file, tmp_ctx);
			  FILE_SET_STRING (str, true);
			} FILE_END;
		      } else {
			char* name = MediaFile_GetName (tmp_ctx, o,
							file.extension);
			FILE_BEGIN (name) {
			  FILE_SET_URL (file.uri, res_size);
			} FILE_END;
		      }
		    }
		  }
		} PTR_LIST_FOR_EACH_PTR_END;
		if (self->flags & DJFS_SHOW_METADATA) {
		  DIR_BEGIN (".metadata") {
		    PTR_LIST_FOR_EACH_PTR (res->children->objects, o) {
		      char* const name = MediaFile_GetName (tmp_ctx, o, "xml");
		      FILE_BEGIN (name) {
			const char* const str = talloc_asprintf
			  (tmp_ctx, 
			   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n%s",
			   DIDLObject_GetElementString (o, tmp_ctx));
			FILE_SET_STRING (str, true);
		      } FILE_END;
		    } PTR_LIST_FOR_EACH_PTR_END;
		  } DIR_END;
		}
	      } DIR_END;
	    }
	  } DIR_END; // "browse"
	} DIR_END; // devName
      }
    }
  } BROWSE_END;
  
  // Release any acquired lock
  if (lock)
    ithread_mutex_unlock (lock);
  
  return BROWSE_RESULT;
}


/*****************************************************************************
 * OBJECT_INIT_CLASS
 *****************************************************************************/

// Initialize class methods
static void
init_class (DJFS_Class* const isa)
{
  CLASS_SUPER_CAST(isa)->browse_root = BrowseRoot;
}

OBJECT_INIT_CLASS(DJFS, VFS, init_class);


/*****************************************************************************
 * DJFS_Create
 *****************************************************************************/
DJFS*
DJFS_Create (void* talloc_context, DJFS_Flags flags)
{
  OBJECT_SUPER_CONSTRUCT (DJFS, VFS_Create, 
			  talloc_context, (flags & DJFS_SHOW_DEBUG));
  if (self) {
    self->flags = flags;
  }
  return self;
}


