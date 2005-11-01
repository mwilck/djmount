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

#include <stdarg.h>	/* missing from "talloc.h" */
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <talloc.h>

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
  if (strncmp (path, name, len) == 0 && (path[len]==NUL || path[len]=='/')) {
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
  DEVICE_LIST_CALL_SERVICE (current, deviceName, CONTENT_DIR_SERVICE_ID,
			    ContentDir, BrowseChildren,
			    tmp_ctx, "0");

  // Walk path, or until error
  while (*ptr && current && current->children) {
    // Find current directory
    const ContentDir_Object* o = current->children->objects;
    while (o) {
      if (o->is_container) {
	const char* const p = match_start_of_path (ptr, o->title);
	if (p) {
	  ptr = p;
	  break; // ---------->
	}
      }
      o = o->next;
    }
    if (o == NULL) {
      Log_Printf (LOG_DEBUG, "browse '%s' stops at '%s'", path, ptr);
      goto cleanup; // ---------->
    } else {
      char* id = o->id; // valid as long as tmp_ctx is not deallocated
      DEVICE_LIST_CALL_SERVICE (current, deviceName,
				CONTENT_DIR_SERVICE_ID,
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
 * @fn object_to_file
 *****************************************************************************/

enum GetMode {
	GET_EXTENSION,
	GET_CONTENT
};

typedef struct _FileFormat {
	const char* mimetype; 
	const char* extension;
} FileFormat;

const FileFormat FORMATS[] = {
	/*
	 * The match on mimetype is done on the begining of the string
	 * so the order of this list matters.
	 */
	{ "application/vnd.rn-realmedia",	"ram" },
	{ "audio/vnd.rn-realaudio",	   	"ram" },
	{ "audio/x-pn-realaudio", 	   	"ram" }, 
	// also "audio/x-pn-realaudio-plugin"
	{ "audio/x-realaudio", 	        	"ram" },
	{ "video/vnd.rn-realvideo", 	   	"ram" },
	{ "audio/",				"m3u" },
	{ "video/",			   	"m3u" },
	{ NULL,					NULL }
};

static char*
object_to_file_content (void* talloc_context, 
			const ContentDir_Object* const o, 
			const FileFormat* const format,
			const char* const uri,
			IXML_Element* const res)
{
	char* str = NULL;
	
	/*
	 * See description of various playlist formats at:
	 * http://gonze.com/playlists/playlist-format-survey.html
	 */
	if (strcmp (format->extension, "ram") == 0) {
		/*
		 * 1) "RAM" playlist - Real Audio content
		 */
		str = talloc_asprintf (talloc_context, "%s?title=%s\n", 
				       uri, o->title);
	} else if (strcmp (format->extension, "m3u") == 0) {
		/*
		 * 2) "M3U" playlist - Winamp, MP3, ... 
		 *     and default for all audio files 
		 */
		const char* const duration = 
			ixmlElement_getAttribute (res, "duration");
		int seconds = -1;
		if (duration) {
			int hh = 0;
			unsigned int mm = 0, ss = 0;
			if (sscanf (duration, "%d:%u:%u", &hh, &mm, &ss) == 3 
			    && hh >= 0)
				seconds = ss + 60*(mm + 60*hh);
		}
		str = talloc_asprintf (talloc_context,
				       "#EXTM3U\n"
				       "#EXTINF:%d,%s\n"
				       "%s\n", seconds, o->title, uri);
	}
	return str;
}


static char*
object_to_file (void* talloc_context, 
		const ContentDir_Object* const o, enum GetMode get)
{
	char* str = NULL;
	IXML_NodeList* const reslist = 
		ixmlElement_getElementsByTagName (o->element, "res");
	if (reslist == NULL)
		return NULL; // ---------->

	int i;
	// Loop until first result
	for (i = 0; i < ixmlNodeList_length (reslist) && str == NULL; i++) {
		IXML_Element* const res = 
			(IXML_Element*) ixmlNodeList_item (reslist, i);
		
		const char* const protocol = 
			ixmlElement_getAttribute (res, "protocolInfo");
		const char* const uri = XMLUtil_GetElementValue (res);
		char mimetype [64] = "";
		if (uri == NULL || protocol == NULL || 
		    sscanf (protocol, "http-get:*:%63[^:;]", mimetype) != 1) 
			continue; // ---------->
			
		const FileFormat* format = FORMATS;
		while (format->mimetype != NULL && str == NULL) {
			if (strncmp (mimetype, format->mimetype, 
				     strlen (format->mimetype)) == 0) {
				if (get == GET_EXTENSION) {
					str = (char*) format->extension;
				} else {
					str = object_to_file_content 
						(talloc_context, o, format,
						 uri, res);
				}
			}
			format++;
		}
	}
	ixmlNodeList_free (reslist);
	return str;
}


/*****************************************************************************
 * DJFS_Browse
 *****************************************************************************/
int
DJFS_Browse (const char* path, 
	     /* for STAT => */	    struct stat* stbuf, 
	     /* for GETDIR => */    void* h, fuse_dirfil_t filler, 
	     /* for READ => */	    void* talloc_context, char** file_content)
{
  int rc = 0;

  if (path == NULL || *path == NUL) 
    return -EFAULT; // ---------->

  const char* ptr = path;
  Log_Printf (LOG_DEBUG, "fuse browse : looking for '%s' ...", path);

  // Create a working context for temporary memory allocations
  void* tmp_ctx = talloc_new (NULL);

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
      if (*ptr == NUL && stbuf) {				\
	stbuf->st_mode  = S_IFDIR | 0555;			\
        stbuf->st_nlink = 2;					\
        stbuf->st_size  = 512;					\
      }								\
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
      if (stbuf) {						\
	stbuf->st_mode  = S_IFREG | 0444;			\
        stbuf->st_nlink = 1;					\
      }								\
      char* talloc_string = NULL;

#define FILE_END							\
      Log_Printf (LOG_DEBUG, "FILE_BEGIN '%s' size = %d",		\
		  path, talloc_string ? strlen(talloc_string) : -1 );	\
      if (stbuf)							\
        stbuf->st_size = talloc_string ? strlen(talloc_string) : 0;	\
      if (file_content) {						\
        if (talloc_string)						\
          talloc_set_name(talloc_string,"file[%s] at " __location__, path); \
        *file_content = talloc_string;					\
      } else talloc_free (talloc_string);				\
    } if (*ptr == NUL) goto cleanup;					\
  }
  

  DIR_BEGIN("") {

    const StringArray* const names = DeviceList_GetDevicesNames (tmp_ctx);

    FILE_BEGIN("devices") {
      if (names) {
        talloc_string = talloc_strdup (talloc_context, "");
        int i;
	for (i = 0; i < names->nb; i++) {
          talloc_string = talloc_asprintf_append (talloc_string, "%s\n",
						  names->str[i]);
	}
      }
      // else talloc_string stays NULL if no devices
    } FILE_END;

    if (names) {
      int i;
      for (i = 0; i < names->nb; i++) {
	const char* const devName = names->str[i];
	DIR_BEGIN(devName) {
	  FILE_BEGIN("status") {
	    talloc_string = DeviceList_GetDeviceStatusString (talloc_context,
							      devName, true);
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
		const ContentDir_Object* o = res->children->objects;
		while (o) {
		  if (o->is_container) {
		    DIR_BEGIN (o->title) {
		    } DIR_END;
		  } else {
		    char* ext = object_to_file (tmp_ctx, o, GET_EXTENSION);
		    if (ext) {
		      char* name = talloc_asprintf (tmp_ctx, "%s.%s", 
						    o->title, ext);
		      FILE_BEGIN (name) {
			talloc_string = object_to_file (talloc_context, o,
							GET_CONTENT);
		      } FILE_END;
		    }
		    char* name = talloc_asprintf (tmp_ctx, "%s.xml", o->title);
		    FILE_BEGIN (name) {
		      talloc_string = 
			talloc_asprintf 
			(talloc_context, 
			 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n%s",
			 XMLUtil_GetNodeString (tmp_ctx, 
						(IXML_Node*) o->element));
		    } FILE_END;
		  }
		  o = o->next;
		}
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
	    talloc_string = talloc_strdup (talloc_context, "essais");
	    if (talloc_string == NULL) {
	      rc = -ENOMEM; 
	      goto cleanup; // ----------> 
	    }
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
