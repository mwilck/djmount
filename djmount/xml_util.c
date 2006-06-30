/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * XML Utilities
 * This file is part of djmount.
 *
 * (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
 *
 * Part derived from libupnp example (upnp/sample/common/sample_util.c)
 * Copyright (c) 2000-2003 Intel Corporation
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

#include "xml_util.h"
#include "log.h"
#include "talloc_util.h"
#include "string_util.h"

#include <upnp/ixml.h>
#include <stdlib.h>	




/*****************************************************************************
 * XMLUtil_GetElementValue
 *****************************************************************************/

char*
XMLUtil_GetElementValue (IN IXML_Element* element)
{
	IXML_Node* child = ixmlNode_getFirstChild ((IXML_Node *) element );

	char* res = NULL;
	
	if ( child && ixmlNode_getNodeType (child) == eTEXT_NODE ) {
		// The resulting string should be copied if necessary
		res = ixmlNode_getNodeValue (child);
	}
	
	return res;
}



/*****************************************************************************
 * XMLUtil_GetFirstNodeValue
 *****************************************************************************/
char*
XMLUtil_GetFirstNodeValue (IN const IXML_Node* node, IN const char* item,
			   bool log_error)
{
	char* res = NULL;
	
	if (node == NULL || item == NULL) {
		Log_Printf (LOG_ERROR, 
			    "GetFirstNodeItem invalid NULL parameter");
	} else {
		//// TBD hack
		//// TBD ixmlNode_getElementsByTagName isn't exported !!!
		//// TBD to clean !!
		IXML_NodeList* const nodeList = 
			ixmlDocument_getElementsByTagName 
			((IXML_Document*) node, (char *) item);
		if (nodeList == NULL) {
			Log_Printf ((log_error ? LOG_ERROR : LOG_DEBUG), 
				    "Can't find '%s' item in XML Node",
				    item);
		} else {
			IXML_Node* tmpNode = ixmlNodeList_item (nodeList, 0);
			if (tmpNode == NULL) {
				Log_Printf ((log_error ? LOG_ERROR: LOG_DEBUG),
					    "Can't find '%s' item in XML Node",
					    item );
			} else {
				IXML_Node* textNode = 
					ixmlNode_getFirstChild (tmpNode);
				
				/* Get the node value. This string will be 
				 * preserved when the NodeList is freed, 
				 * but should be copied if the IXML_Element 
				 * is to be destroyed.
				 */
				res = ixmlNode_getNodeValue (textNode);
			}
			ixmlNodeList_free (nodeList);
		}
	}
	return res;
}


/******************************************************************************
 * XMLUtil_GetDocumentString
 *****************************************************************************/
char*
XMLUtil_GetDocumentString (void* context, IXML_Document* doc)
{
	// TBD XXX
	// TBD prepend <?xml version="1.0"?> if not already done ???
	// TBD XXX
	
	char* ret = NULL;
	if (doc) {
		DOMString s = ixmlPrintDocument (doc);
		if (s) {
			ret = talloc_strdup (context, s);
			ixmlFreeDOMString (s);
		} else {
			ret = talloc_strdup (context, "(error)");
		}
	} else {
		ret = talloc_strdup (context, "(null)");
	}
	return ret;
}


/******************************************************************************
 * XMLUtil_GetNodeString
 *****************************************************************************/
char*
XMLUtil_GetNodeString (void* context, IXML_Node* node)
{
	char* ret = NULL;
	if (node) {
		DOMString s = ixmlPrintNode (node);
		if (s) {
			ret = talloc_strdup (context, s);
			ixmlFreeDOMString (s);
		} else {
			ret = talloc_strdup (context, "(error)");
		}
	} else {
		ret = talloc_strdup (context, "(null)");
	}
	return ret;
}


