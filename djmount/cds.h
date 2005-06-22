/* $Id$
 *
 * "CDS" = UPnP ContentDirectory Service 
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

#ifndef CDS_H_INCLUDED
#define CDS_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <upnp/ixml.h>


// ContentDirectory Service types
typedef uint_fast32_t CDS_Count;



/**
 * This structure represents a DIDL-Lite object, as specified in 
 * the ContentDirectory UPnP documentation. 
 */
// TBD rename to CDS_DIDLObject ?

typedef struct CDS_ObjectStruct {

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

  struct CDS_ObjectStruct* next;

} CDS_Object;



// TBD to retrieve optional properties
//_GetObjectProperty


/**
 * Linked list of DIDL-object, returned by Browse functions
 */
typedef struct CDS_BrowseResultStruct {

  CDS_Count   nb_containers;
  CDS_Count   nb_items;
  CDS_Object* children;
  
} CDS_BrowseResult;


/*****************************************************************************
 * Content Directory Service Actions
 * The following methods define the various CDS actions :
 * see UPnP documentation : ContentDirectory:1 Service Template Version 1.01
 *****************************************************************************/

/** Browse Action, BrowseFlag = BrowseDirectChildren */
CDS_BrowseResult*
CDS_BrowseChildren (void* result_context, 
		    const char* deviceName, const char* objectId);

/** Browse Action, BrowseFlag = BrowseMetadata */
CDS_Object*
CDS_BrowseMetadata (void* result_context, 
		    const char* deviceName, const char* objectId);

/** Search Action */
// TBD TBD not implemented yet
int
CDS_Search (const char* deviceName, const char* objectId);



#ifdef __cplusplus
}; // extern "C"
#endif 


#endif // CDS_H_INCLUDED


