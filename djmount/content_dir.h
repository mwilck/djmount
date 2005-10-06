/* $Id$
 *
 * UPnP Content Directory Service 
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

#ifndef CONTENT_DIR_H_INCLUDED
#define CONTENT_DIR_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <upnp/ixml.h>

#include "service.h"


// ContentDirectory Service types
typedef uint_fast32_t ContentDir_Count;
typedef uint_fast32_t ContentDir_Index;


// ContentDirectory ServiceType
#define CONTENT_DIR_SERVICE_TYPE \
  "urn:schemas-upnp-org:service:ContentDirectory:1"

// ContentDirectory ServiceId
#define CONTENT_DIR_SERVICE_ID "urn:upnp-org:serviceId:ContentDirectory"



/******************************************************************************
 * @var ContentDir
 *
 *	This opaque type encapsulates access to a UPnP Content Directory
 *	service. 
 *	This type is derived from the "Service" type : all Service_* methods
 *	can be used on ContentDirectory.
 *	
 *****************************************************************************/

OBJECT_DECLARE_CLASS(ContentDir, Service);



/**
 * This structure represents a DIDL-Lite object, as specified in 
 * the ContentDirectory UPnP documentation. 
 */
// TBD rename to CDS_DIDLObject ?

typedef struct _ContentDir_Object {

  bool  is_container; // else is_item

  /*
   * The following members are required properties of every DIDL-Lite object. 
   */
  char* id;
  // TBD char* parentId;
  char* title;
  char* cds_class;
  // TBD bool  restricted; // TBD Not Yet Implemented

  /*
   * full <item> or <container> element, to access optional properties
   * e.g. "res"
   */
  IXML_Element* element;

  struct _ContentDir_Object* next;

} ContentDir_Object;



// TBD to retrieve optional properties
//_GetObjectProperty


/**
 * Linked list of DIDL-object, returned by Browse functions
 */
typedef struct _ContentDir_Children {

  ContentDir_Count   nb_objects;
  ContentDir_Object* objects;
  
} ContentDir_Children;


/**
 * Result returned by Browse functions
 * (extra level of indirection regarding Children is needed internally
 *  for cache management)
 */
typedef struct _ContentDir_BrowseResult {

  ContentDir*	        cds;
  ContentDir_Children*  children;
  
} ContentDir_BrowseResult;



/*****************************************************************************
 * @brief Create a new ContentDirectory service.
 *	This routine parses the DOM service description.
 *	The returned object can be destroyed with "talloc_free".
 *
 * @param context        the talloc parent context
 * @param ctrlpt_handle  the UPnP client handle
 * @param serviceDesc    the DOM service description document
 * @param base_url       the base url of the device description document
 *****************************************************************************/
ContentDir* 
ContentDir_Create (void* context,
		   UpnpClient_Handle ctrlpt_handle, 
		   IXML_Element* serviceDesc, 
		   const char* base_url);

/*****************************************************************************
 * Content Directory Service Actions
 * The following methods define the various ContentDirectory actions :
 * see UPnP documentation : ContentDirectory:1 Service Template Version 1.01
 *****************************************************************************/

/**
 * Browse Action, BrowseFlag = BrowseDirectChildren.
 * Return NULL if error, or an object list if ok (can be empty).
 * Result should be freed using "talloc_free" when finished.
 */
const ContentDir_BrowseResult*
ContentDir_BrowseChildren (ContentDir* cds,
			   void* result_context, 
			   const char* objectId);


/**
 * Browse Action, BrowseFlag = BrowseMetadata.
 * Return NULL if error, or a DIDL-object if ok.
 * Result should be freed using "talloc_free" when finished.
 */
ContentDir_Object*
ContentDir_BrowseMetadata (ContentDir* cds,
			   void* result_context, 
			   const char* objectId);

/** Search Action */
// TBD TBD not implemented yet
int
ContentDir_Search (const ContentDir* cds, const char* objectId);



#ifdef __cplusplus
}; // extern "C"
#endif 


#endif // CONTENT_DIR_H_INCLUDED


