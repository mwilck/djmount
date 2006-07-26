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

#include "search_help.h"

#include <ctype.h>


/*****************************************************************************

 <mount point> -+- devices
                |
                |- <devName> -+- .status -> ../.debug/<devName>/status
                |             |
                |             +- xxx -+- xxx
                |             |       |- xxx
                |             |       `- _search/
                |             |
                |             +- .metadata/ -+- xxx
                |             |              `- xxx
                |             |
                |             `- _search/ -+- search_capabilities
                |                          |- <search_criteria> -+- ...
                |                          `- ...
                |
                |- <devName>
                |
                `- .debug/

*****************************************************************************/


typedef struct _SearchHistory {

  unsigned int 	serial;
  time_t	time;
  const char*	parent_path; 
  const char*	basename;
  const char*	criteria;
  
} SearchHistory;



/*****************************************************************************
 * BrowseSearchDir
 *****************************************************************************/

static VFS_BrowseStatus
BrowseChildren (DJFS* const self, const char* const sub_path,
		const VFS_Query* const query, void* const tmp_ctx,
		const char* const devName, const DIDLObject* const parent, 
		bool const searchable, const char* const search_criteria,
		ContentDir_Children* const children);

static VFS_BrowseStatus
BrowseSearchDir (DJFS* const self, const char* const sub_path,
		 const VFS_Query* const query, void* const tmp_ctx,
		 const char* const devName, const DIDLObject* const parent, 
		 const char* const criteria_start)
{
  // Keep a pointer to acquired lock, if any
  ithread_mutex_t* lock = NULL;

  BROWSE_BEGIN (sub_path, query) {
    
    FILE_BEGIN ("search_help.txt") {
      static const char* const str = SEARCH_HELP_TXT_STRING;
      FILE_SET_STRING (str, FILE_BUFFER_STRING_EXTERN);
    } FILE_END;

    FILE_BEGIN ("search_capabilities") {
      const char* caps = NULL;
      DEVICE_LIST_CALL_SERVICE (caps, devName, 
				CONTENT_DIR_SERVICE_TYPE,
				ContentDir, GetSearchCapabilities, NULL);
      if (caps && *caps) {
	char* const str = talloc_asprintf (tmp_ctx, "%s\n", caps);
	char* s;
	for (s = str; *s != NUL; s++) {
	  if (*s == ',')
	    *s = '\n';
	}
	FILE_SET_STRING (str, FILE_BUFFER_STRING_STEAL);
      }
    } FILE_END;

    char* const parent_path = talloc_strdup (tmp_ctx, query->path);
    char* pp = parent_path + (BROWSE_PTR - query->path);
    do {
      *pp-- = NUL;
    } while (*pp == '/');

    /*
     * List existing Search criterias
     */
    SearchHistory* h;
    ithread_mutex_lock (&self->search_hist_mutex);
    lock = &self->search_hist_mutex;
    PTR_ARRAY_FOR_EACH_PTR (self->search_hist, h) {
      if (strcmp (parent_path, h->parent_path) == 0) {
	DIR_BEGIN (h->basename) {
	  VFS_SET_TIME (h->time);
	  const char* const full_criteria = (criteria_start ? talloc_asprintf
					     (tmp_ctx, "%s%s)",
					      criteria_start, h->criteria) 
					     : h->criteria);
	  const ContentDir_BrowseResult* res;
	  DEVICE_LIST_CALL_SERVICE (res, devName, 
				    CONTENT_DIR_SERVICE_TYPE,
				    ContentDir, Search,
				    tmp_ctx, parent->id, full_criteria);
	  if (res) {
	    BROWSE_SUB (BrowseChildren (self, BROWSE_PTR, query,
					tmp_ctx, devName, parent,
					true, full_criteria,
					res->children));
	  }
	} DIR_END;
      }
    } PTR_ARRAY_FOR_EACH_PTR_END;
	  
    /*
     * Create new Search criteria
     */
    char* const new_basename = talloc_strdup (tmp_ctx, BROWSE_PTR);
    if (new_basename && *new_basename) {
      bool simplified = true;
      char* t;
      for (t = new_basename; *t != NUL && *t != '/'; t++) {
	if (isspace (*t) || *t == '*' || *t == '"')
	  simplified = false;
      }
      *t = NUL;
      
      char* const new_criteria = 
	(simplified ? talloc_asprintf (tmp_ctx, 
				       "(dc:title contains \"%s\") or "
				       "(dc:creator contains \"%s\") or "
				       "(upnp:artist contains \"%s\") or "
				       "(upnp:album contains \"%s\")",
				       new_basename, new_basename,
				       new_basename, new_basename)
	 : new_basename);
      Log_Printf (LOG_DEBUG, "new search criteria '%s' (inside '%s')",
		  new_criteria, NN(criteria_start));
      const char* const full_criteria = (criteria_start ? talloc_asprintf 
					 (tmp_ctx, "%s%s)", 
					  criteria_start, new_criteria) 
					 : new_criteria);
      const ContentDir_BrowseResult* res;
      DEVICE_LIST_CALL_SERVICE (res, devName, 
				CONTENT_DIR_SERVICE_TYPE,
				ContentDir, Search,
				tmp_ctx, parent->id, full_criteria);
      // Do not create directory on empty result -> "No such file or directory"
      if (res && res->children && 
	  PtrArray_GetSize (res->children->objects) > 0) {
	SearchHistory* h = talloc (self->search_hist, SearchHistory);
	if (h) {
	  *h = (SearchHistory) {
	    .serial      = ++(self->search_hist_serial),
	    .time        = time (NULL),
	    .parent_path = talloc_steal (h, parent_path),
	    .basename	 = talloc_steal (h, new_basename),
	  };
	  h->criteria = ( (new_criteria == new_basename) ? h->basename 
			  : talloc_steal (h, new_criteria) );

	  PtrArray_Append (self->search_hist, h);
	  if (PtrArray_GetSize (self->search_hist) > self->search_hist_size) {
	    SearchHistory* const oldest = 
	      PtrArray_RemoveAt (self->search_hist, 0);
	    talloc_free (oldest);
	  }
	  DIR_BEGIN (new_basename) {
	    VFS_SET_TIME (h->time);
	    BROWSE_SUB (BrowseChildren (self, BROWSE_PTR, query,
					tmp_ctx, devName, parent,
					true, full_criteria,
					res->children));
	  } DIR_END;
	}
      }
    }

  } BROWSE_END;  
  
  // Release any acquired lock
  if (lock)
    ithread_mutex_unlock (lock);
  
  return BROWSE_RESULT;
}


/*****************************************************************************
 * BrowseChildren
 *****************************************************************************/

static VFS_BrowseStatus
BrowseChildren (DJFS* const self, const char* const sub_path,
		const VFS_Query* const query, void* const tmp_ctx,
		const char* const devName, const DIDLObject* const parent, 
		bool const searchable, const char* const search_criteria,
		ContentDir_Children* const children)
{
  // Keep a pointer to acquired lock, if any
  ithread_mutex_t* lock = NULL;
 
  BROWSE_BEGIN(sub_path, query) {
    
    if (children) {
      DIDLObject* o = NULL;               
#if CONTENT_DIR_HAVE_CHILDREN_MUTEX
      ithread_mutex_lock (&children->mutex);
      lock = &children->mutex;
#endif
      PTR_ARRAY_FOR_EACH_PTR (children->objects, o) {
	if (o->is_container) {
	  DIR_BEGIN (o->basename) {
	    const ContentDir_BrowseResult* res;
	    DEVICE_LIST_CALL_SERVICE (res, devName,
				      CONTENT_DIR_SERVICE_TYPE,
				      ContentDir, Browse,
				      tmp_ctx, o->id,
				      CONTENT_DIR_BROWSE_DIRECT_CHILDREN);
	    if (res && res->children) {
	      // Note : if we are already inside a "_search" directory 
	      // ("search_criteria" not NULL), do not allow sub-search 
	      // (might be confusing)
	      BROWSE_SUB (BrowseChildren 
			  (self, BROWSE_PTR, query, tmp_ctx, 
			   devName, o,
			   searchable && (search_criteria == NULL),
			   NULL, res->children));
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
		(tmp_ctx, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n%s",
		 DIDLObject_GetElementString (o, tmp_ctx));
	      FILE_SET_STRING (str, FILE_BUFFER_STRING_STEAL);
	    } FILE_END;
	  } PTR_ARRAY_FOR_EACH_PTR_END;
	} DIR_END;
      }
    } // if children
    
    if (searchable && parent->searchable) {
      // Check if new search directory, or extending a previous search
      if (search_criteria == NULL) {
	DIR_BEGIN ("_search") {
	  BROWSE_SUB (BrowseSearchDir (self, BROWSE_PTR, query,
				       tmp_ctx, devName, parent, NULL));
	} DIR_END;
      } else {
	char criteria_start [strlen (search_criteria) + 9];
	DIR_BEGIN ("_and") {
	  sprintf (criteria_start, "(%s) and (", search_criteria);
	  BROWSE_SUB (BrowseSearchDir (self, BROWSE_PTR, query,
				       tmp_ctx, devName, parent, 
				       criteria_start));
	} DIR_END;
	DIR_BEGIN ("_or") {
	  sprintf (criteria_start, "(%s) or (", search_criteria);
	  BROWSE_SUB (BrowseSearchDir (self, BROWSE_PTR, query,
				       tmp_ctx, devName, parent, 
				       criteria_start));
	} DIR_END;
      }
    } // if searchable
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
    
    const char* devName;
    PTR_ARRAY_FOR_EACH_PTR (names, devName) {
      DIR_BEGIN (devName) {
	if (self->flags & DJFS_SHOW_DEBUG) {
	  SYMLINK_BEGIN (".status") {
	    const char* const str = talloc_asprintf (tmp_ctx, 
						     "../%s/%s/status",
						     VFS_DEBUG_DIR_BASENAME,
						     devName);
	    SYMLINK_SET_PATH (str);
	  } SYMLINK_END;
	}
	const ContentDir_BrowseResult* current = NULL;
	DEVICE_LIST_CALL_SERVICE (current, devName, 
				  CONTENT_DIR_SERVICE_TYPE,
				  ContentDir, Browse,
				  tmp_ctx, "0", 
				  CONTENT_DIR_BROWSE_METADATA);
	if (current && current->children) {
	  const DIDLObject* const root =
	    PtrArray_GetHead (current->children->objects);
	  if (root) {
	    DEVICE_LIST_CALL_SERVICE (current, devName, 
				      CONTENT_DIR_SERVICE_TYPE,
				      ContentDir, Browse,
				      tmp_ctx, "0", 
				      CONTENT_DIR_BROWSE_DIRECT_CHILDREN);
	    bool searchable = (self->search_hist != NULL);
	    if (searchable && root->searchable) {
	      // make sure the device is really searchable (some buggy 
	      // servers return "searchable" as "true" in metadata, even
	      // though SearchCapabilities are empty)
	      const char* caps = NULL;
	      DEVICE_LIST_CALL_SERVICE (caps, devName, 
					CONTENT_DIR_SERVICE_TYPE,
					ContentDir, GetSearchCapabilities, 
					NULL);
	      searchable = (searchable && caps && *caps);
	    }
	    if (current && current->children) {
	      BROWSE_SUB (BrowseChildren (self, BROWSE_PTR, query, tmp_ctx, 
					  devName, root, searchable,
					  NULL, current->children));
	    }
	  }
	}
      } DIR_END; // devName
    } PTR_ARRAY_FOR_EACH_PTR_END;
    
  } BROWSE_END;
  
  return BROWSE_RESULT;
}


/*****************************************************************************
 * BrowseDebug
 *****************************************************************************/

static VFS_BrowseStatus
BrowseDebug (VFS* const vfs, const char* const sub_path,
	    const VFS_Query* const query, void* const tmp_ctx)
{
  DJFS* const self = (DJFS*) vfs;
  
  // Keep a pointer to acquired lock, if any
  ithread_mutex_t* lock = NULL;

  BROWSE_BEGIN(sub_path, query) {
        
    FILE_BEGIN("versions") {
      static const char* const str = PACKAGE " " VERSION "\nlibupnp "
#ifdef UPNP_VERSION_STRING
	UPNP_VERSION_STRING
#else
	"bundled"
#endif
	"\nFUSE " STRINGIFY (FUSE_MAJOR_VERSION) "." 
	STRINGIFY (FUSE_MINOR_VERSION) "\n";
      FILE_SET_STRING (str, FILE_BUFFER_STRING_EXTERN);
    } FILE_END;

    // Status of each device
    const PtrArray* const names = DeviceList_GetDevicesNames (tmp_ctx);
    const char* devName;
    PTR_ARRAY_FOR_EACH_PTR (names, devName) {
      DIR_BEGIN (devName) {
	FILE_BEGIN ("status") {
	  const char* const str = 
	    DeviceList_GetDeviceStatusString (tmp_ctx, devName, true);
	  FILE_SET_STRING (str, FILE_BUFFER_STRING_STEAL);
	} FILE_END;
      } DIR_END;
    } PTR_ARRAY_FOR_EACH_PTR_END;

    // List historized Search requests
    if (self->search_hist) {
      DIR_BEGIN ("search_history") {
	SearchHistory* h;
	ithread_mutex_lock (&self->search_hist_mutex);
	lock = &self->search_hist_mutex;
	PTR_ARRAY_FOR_EACH_PTR (self->search_hist, h) {
	  char link_name [33];
	  sprintf (link_name, "%d", h->serial);
	  SYMLINK_BEGIN (link_name) {
	    const char* const str = talloc_asprintf 
	      (tmp_ctx, "../..%s/%s", h->parent_path, h->basename);
	    SYMLINK_SET_PATH (str);
	    VFS_SET_TIME (h->time);
	  } SYMLINK_END;
	} PTR_ARRAY_FOR_EACH_PTR_END;
      } DIR_END;
    }

    // Call superclass' method
    VFS_BrowseFunction func = CLASS_METHOD (VFS, browse_debug);
    if (func) {
      BROWSE_SUB (func (vfs, BROWSE_PTR, query, tmp_ctx));
    }
    
  } BROWSE_END;
    
  // Release any acquired lock
  if (lock)
    ithread_mutex_unlock (lock);
  
  return BROWSE_RESULT;
}


/******************************************************************************
 * finalize
 *
 * Description: 
 *	DJFS destructor
 *
 *****************************************************************************/

static void
finalize (Object* obj)
{
  DJFS* const self = (DJFS*) obj;

  if (self && self->search_hist) {
    ithread_mutex_destroy (&self->search_hist_mutex);
  }

  // Other "talloc'ed" fields will be deleted automatically : 
  // nothing to do 
}


/*****************************************************************************
 * OBJECT_INIT_CLASS
 *****************************************************************************/

// Initialize class methods
static void
init_class (DJFS_Class* const isa)
{
  CLASS_BASE_CAST(isa)->finalize      = finalize;
  CLASS_SUPER_CAST(isa)->browse_root  = BrowseRoot;
  CLASS_SUPER_CAST(isa)->browse_debug = BrowseDebug;
}

OBJECT_INIT_CLASS(DJFS, VFS, init_class);


/*****************************************************************************
 * DJFS_Create
 *****************************************************************************/
DJFS*
DJFS_Create (void* talloc_context, DJFS_Flags flags, 
	     size_t search_history_size)
{
  OBJECT_SUPER_CONSTRUCT (DJFS, VFS_Create, 
			  talloc_context, (flags & DJFS_SHOW_DEBUG));
  if (self) {
    self->flags              = flags;
    self->search_hist_size   = search_history_size;
    self->search_hist_serial = 0;

    if (search_history_size > 0) {
      self->search_hist = PtrArray_Create (self);
      if (self->search_hist == NULL) {
	talloc_free (self);
	self = NULL;
      } else {
	ithread_mutexattr_t attr;
	ithread_mutexattr_init (&attr);
	ithread_mutexattr_setkind_np (&attr, ITHREAD_MUTEX_RECURSIVE_NP);
	ithread_mutex_init (&self->search_hist_mutex, &attr);
	ithread_mutexattr_destroy (&attr);
      }
    }
  }
  return self;
}



