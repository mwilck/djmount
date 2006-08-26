/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * DIDL-Lite object
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

#include "didl_object.h"
#include "log.h"
#include "string_util.h"
#include "xml_util.h"
#include "talloc_util.h"


/******************************************************************************
 * DestroyObject
 *
 * Description: 
 *	DIDL Object destructor, automatically called by "talloc_free".
 *
 *****************************************************************************/
static int
DestroyObject (DIDLObject* const o)
{
	if (o) {
		ixmlElement_free (o->element);
		
		// The "talloc'ed" strings will be deleted automatically 
	}
	return 0; // ok -> deallocate memory
}


/******************************************************************************
 * DIDLObject_Create
 *****************************************************************************/
DIDLObject*
DIDLObject_Create (void* talloc_context,
		   IN IXML_Element* elem, 
		   IN bool is_container) 
{
	if (elem == NULL) {
		Log_Printf (LOG_ERROR, 
			    "DIDLObject can't create from NULL XML Element");
		return NULL; // ---------->
	}

	DIDLObject* o = talloc (talloc_context, DIDLObject);
	if (o) {
		*o = (DIDLObject) { 
			.is_container = is_container
		};

		IXML_Node* node = NULL;
		/* Steal the node from its parent (instead of copying it, given
		 * that the parent document is going to be deallocated anyway)
		 */
		ixmlNode_removeChild (ixmlNode_getParentNode (XML_E2N (elem)), 
				      XML_E2N (elem), &node);
		o->element = (IXML_Element*) node;

		// TBD need to copy ??
		o->id = talloc_strdup (o, ixmlElement_getAttribute
				       (o->element, "id"));
		if (o->id == NULL || o->id[0] == NUL) {
			char* s = DIDLObject_GetElementString (o, NULL);
			Log_Printf (LOG_ERROR, 
				    "DIDLObject can't create with NULL "
				    "or empty id, XML = %s", s);
			talloc_free (s);
			talloc_free (o);
			return NULL; // ---------->
		}

		o->title = XMLUtil_FindFirstElementValue (node, "dc:title", 
							  false, true);
		if (o->title == NULL)
			o->title = "";

		o->basename = String_CleanFileName (o, o->title);
		if (o->basename[0] == NUL) {
			char* s = DIDLObject_GetElementString (o, NULL);
			Log_Printf (LOG_WARNING, 
				    "DIDLObject NULL or empty <dc:title>, "
				    "XML = %s", s);
			talloc_free (s);
			talloc_free (o->basename);
			o->basename = talloc_asprintf (o, "-id-%s", o->id);
		} else if (o->basename[0] == '.' || o->basename[0] == '_') {
			o->basename[0] = '-';
		}
		
		o->cds_class = String_StripSpaces 
			(o, XMLUtil_FindFirstElementValue (node, "upnp:class",
							   false, true));
		if (o->cds_class == NULL)
			o->cds_class = "";

		char* s = ixmlElement_getAttribute (o->element, "searchable");
		o->searchable = String_ToBoolean (s, false);

		Log_Printf (LOG_DEBUG,
			    "new DIDLObject : %s : id='%s' "
			    "title='%s' class='%s'",
			    (is_container ? "container" : "item"), 
			    o->id, o->title, o->cds_class);
		
		// Register destructor
		talloc_set_destructor (o, DestroyObject);
	}
	return o;
}


/******************************************************************************
 * DIDLObject_GetElementString
 *****************************************************************************/
char*
DIDLObject_GetElementString (const DIDLObject* o, void* result_context)
{
	char* s = NULL;
	if (o) {
		s = XMLUtil_GetNodeString (result_context, 
					   XML_E2N (o->element));
	}
	return s;
}


