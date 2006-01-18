/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * DIDL-Lite object
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

#ifndef DIDL_OBJECT_H_INCLUDED
#define DIDL_OBJECT_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <upnp/ixml.h>



/******************************************************************************
 * @var DIDLObject
 *
 * This structure represents a DIDL-Lite object, as specified in 
 * the UPnP documentation : 
 *	ContentDirectory:1 Service Template Version 1.01
 *
 *      NOTE THAT THE FUNCTION API IS NOT THREAD SAFE. Functions which 
 *	might modify the DIDLObject state (all non-const ones) in different
 *	threads should synchronise accesses through appropriate locking.
 *	
 *****************************************************************************/

typedef struct _DIDLObject {

	bool  is_container; // else is_item

	/*
	 * The following members are required properties of every 
	 * DIDL-Lite object. The "DIDLObject_Create" method make sure
	 * that those fields are never NULL, and make sure that "id" 
	 * is never empty "".
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


	/*
	 * Additional (internal) fields, for internal use.
	 * Not part of ContentDirectory fields.
	 */

	// Similar to "title", but suitable for filename generation : 
	// never empty "", or reserved name (e.g. "." or "..")
	char* basename;

} DIDLObject;


// TBD to retrieve optional properties
//_GetObjectProperty




/*****************************************************************************
 * @brief Create a new DIDL-Lite object.
 *	This routine allocates a new DIDL-Lite object. It parses 
 *	the XML description and set the appropriate fields in the object.
 *	WARNING : the IXML_Element is *removed* from its parent XML document
 * 	and stored directly inside the new object ; it is *NOT* copied.
 *
 *	When finished, the object can be destroyed with "talloc_free".
 *
 * @param talloc_context        the talloc parent context
 * @param element	 	the XML description
 * @param is_container	 	true if container, false if item
 *****************************************************************************/
DIDLObject*
DIDLObject_Create (void* talloc_context,
		   IN IXML_Element* element,
		   IN bool is_container);
	

/*****************************************************************************
 * Return a string with the XML Element of the given DIDL-Lite Object.
 *
 * @param result_context	parent context to allocate result, may be NULL
 *****************************************************************************/
char*
DIDLObject_GetElementString (const DIDLObject* o, void* result_context);



#ifdef __cplusplus
}; // extern "C"
#endif 


#endif // DIDL_OBJECT_H_INCLUDED


