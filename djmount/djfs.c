/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* $Id$
 *
 * djfs : file system implementation for djmount.
 * This file is part of djmount.
 *
 * (C) Copyright 2005-2006 Rémi Turboult <r3mi@users.sourceforge.net>
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



/*****************************************************************************
 * BrowseChildren
 *****************************************************************************/

static VFS_BrowseStatus
BrowseChildren (DJFS* const self, const char* const sub_path,
		const VFS_Query* const query, void* const tmp_ctx,
		const char* const devName, 
		ContentDir_Children* const children)
{
  // Keep a pointer to acquired lock, if any
  ithread_mutex_t* lock = NULL;
 
  BROWSE_BEGIN(sub_path, query) {
    
    if (children) {
      DIDLObject* o = NULL;               
      ithread_mutex_lock (&children->mutex);
      lock = &children->mutex;
      PTR_ARRAY_FOR_EACH_PTR (children->objects, o) {
	if (o->is_container) {
	  DIR_BEGIN (o->basename) {
	    const ContentDir_BrowseResult* res;
	    DEVICE_LIST_CALL_SERVICE (res, devName,
				      CONTENT_DIR_SERVICE_TYPE,
				      ContentDir, BrowseChildren,
				      tmp_ctx, o->id);
	    if (res) {
	      BROWSE_SUB(BrowseChildren (self, BROWSE_PTR, query, tmp_ctx, 
					 devName, 
					 res->children));
	    }
	  } DIR_END;
	} else {
	  MediaFile file = { .o = NULL };
	  if (MediaFile_GetPreferred (o, &file)) {
	    off_t const res_size = MediaFile_GetResSize (&file);
	    if ( file.playlist &&
		 ( (self->flags & DJFS_USE_PLAYLISTS) ||
		   res_size < 0 ||
		   res_size > FILE_BUFFER_MAX_CONTENT_LENGTH) ) {
	      char* name = MediaFile_GetName (tmp_ctx, o, file.playlist);
	      FILE_BEGIN (name) {
		const char* const str = MediaFile_GetPlaylistContent 
		  (&file, tmp_ctx);
		FILE_SET_STRING (str, FILE_BUFFER_STRING_STEAL);
	      } FILE_END;
	    } else {
	      char* name = MediaFile_GetName (tmp_ctx, o, file.extension);
	      FILE_BEGIN (name) {
		FILE_SET_URL (file.uri, res_size);
	      } FILE_END;
	    }
	  }
	}
      } PTR_ARRAY_FOR_EACH_PTR_END;
      if ( (self->flags & DJFS_SHOW_METADATA) && 
	   !PtrArray_IsEmpty(children->objects) ) {
	DIR_BEGIN (".metadata") {
	  PTR_ARRAY_FOR_EACH_PTR (children->objects, o) {
	    char* const name = MediaFile_GetName (tmp_ctx, o, "xml");
	    FILE_BEGIN (name) {
	      const char* const str = talloc_asprintf
		(tmp_ctx, 
		 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n%s",
		 DIDLObject_GetElementString (o, tmp_ctx));
	      FILE_SET_STRING (str, FILE_BUFFER_STRING_STEAL);
	    } FILE_END;
	  } PTR_ARRAY_FOR_EACH_PTR_END;
	} DIR_END;
      }
    }
  } BROWSE_END;  
  
  // Release any acquired lock
  if (lock)
    ithread_mutex_unlock (lock);
  
  return BROWSE_RESULT;
}


/*****************************************************************************
 * BrowseRoot
 *****************************************************************************/

static VFS_BrowseStatus
BrowseRoot (VFS* const vfs, const char* const sub_path,
	    const VFS_Query* const query, void* const tmp_ctx)
{
  DJFS* const self = (DJFS*) vfs;
  
  BROWSE_BEGIN(sub_path, query) {
    
    const PtrArray* const names = DeviceList_GetDevicesNames (tmp_ctx);
    
    FILE_BEGIN("devices") {
      if (names) {
	char* str = talloc_strdup(tmp_ctx, "");
	const char* devName;
	PTR_ARRAY_FOR_EACH_PTR (names, devName) {
	  str = talloc_asprintf_append (str, "%s\n", devName);
	} PTR_ARRAY_FOR_EACH_PTR_END;
	FILE_SET_STRING (str, FILE_BUFFER_STRING_STEAL);
      }
      // else content defaults to NULL if no devices
    } FILE_END;
    
    if (names) {
      const char* devName;
      PTR_ARRAY_FOR_EACH_PTR (names, devName) {
	DIR_BEGIN(devName) {
	  FILE_BEGIN("status") {
	    const char* const str = DeviceList_GetDeviceStatusString 
	      (tmp_ctx, devName, true);
	    FILE_SET_STRING (str, FILE_BUFFER_STRING_STEAL);
	  } FILE_END;
	  DIR_BEGIN("browse") {
	    const ContentDir_BrowseResult* current = NULL;
	    const char* const id = "0";
	    DEVICE_LIST_CALL_SERVICE (current, devName, 
				      CONTENT_DIR_SERVICE_TYPE,
				      ContentDir, BrowseChildren,
				      tmp_ctx, id);
	    if (current) {
	      BROWSE_SUB(BrowseChildren (self, BROWSE_PTR, query, tmp_ctx, 
					 devName, current->children));
	    }
	  } DIR_END;
	} DIR_END; // devName
      } PTR_ARRAY_FOR_EACH_PTR_END;
    }
    
  } BROWSE_END;
  
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



