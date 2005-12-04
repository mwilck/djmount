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

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

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
DestroyObject (void* ptr)
{
	if (ptr) {
		DIDLObject* const o = (DIDLObject*) ptr;
		
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
	DIDLObject* o = talloc (talloc_context, DIDLObject);
	if (o) {
		*o = (DIDLObject) { 
			.is_container = is_container
		};

		IXML_Node* node = NULL;
		/* Steal the node from its parent (instead of copying it, given
		 * that the parent document is going to be deallocated anyway)
		 */
		ixmlNode_removeChild 
			(ixmlNode_getParentNode ((IXML_Node*) elem), 
			 (IXML_Node*) elem, &node);
		o->element = (IXML_Element*) node;

		o->id = talloc_strdup (o, ixmlElement_getAttribute
				       (o->element, "id"));
		o->title = String_CleanFileName (o, XMLUtil_GetFirstNodeValue
						 (node, "dc:title"));
		o->cds_class = String_StripSpaces (o, XMLUtil_GetFirstNodeValue
						   (node, "upnp:class"));
		Log_Printf (LOG_DEBUG,
			    "new DIDLObject : %s : id='%s' "
			    "title='%s' class='%s'",
			    (is_container ? "container" : "item"), 
			    NN(o->id), NN(o->title), NN(o->cds_class));
		
		// Register destructor
		talloc_set_destructor (o, DestroyObject);
	}
	return o;
}


