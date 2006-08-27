/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * XML Utilities
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

#include "xml_util.h"
#include "log.h"
#include "talloc_util.h"
#include "string_util.h"

#include <upnp/ixml.h>
#include <stdlib.h>	




/*****************************************************************************
 * XMLUtil_GetElementValue
 *****************************************************************************/

const char*
XMLUtil_GetElementValue (IN const IXML_Element* element)
{
	char* res = NULL;
	IXML_Node* child = ixmlNode_getFirstChild 
		(discard_const_p (IXML_Node, XML_E2N (element)));
	while (child && !res) {
		if (ixmlNode_getNodeType (child) == eTEXT_NODE) {
		    // The resulting string should be copied if necessary
		    res = ixmlNode_getNodeValue (child);
		}
		child = ixmlNode_getNextSibling (child);
	}
	return res;
}


/******************************************************************************
 * XMLUtil_FindFirstElement
 *****************************************************************************/

static IXML_Element*
findFirstElementRecursive (const IXML_Node* const node, 
			   const char* const tagname,
			   bool const deep)
{
	IXML_Element* res = NULL;
	IXML_Node* n = ixmlNode_getFirstChild (discard_const_p (IXML_Node, 
								node));
	while (n && !res) {
		if (ixmlNode_getNodeType (n) == eELEMENT_NODE) {
			const char* const name = ixmlNode_getNodeName (n);
			if (name && strcmp (tagname, name) == 0) {
				res = (IXML_Element*) n;
			}
		}
		if (deep && !res) {
			res = findFirstElementRecursive (n, tagname, deep);
		}
		n = ixmlNode_getNextSibling (n);
	}
	return res;
}

IXML_Element*
XMLUtil_FindFirstElement (const IXML_Node* const node,
			  const char* const tagname,
			  bool const deep, bool const log_error)
{
	IXML_Element* res = NULL;

	if (node == NULL || tagname == NULL) {
		Log_Printf (LOG_ERROR, 
			    "GetFirstElementByTagName invalid NULL parameter");
	} else {
		IXML_Node* const n = discard_const_p (IXML_Node, node);
		if (ixmlNode_getNodeType (n) == eELEMENT_NODE) {
			const char* const name = ixmlNode_getNodeName (n);
			if (name && strcmp (tagname, name) == 0) {
				res = (IXML_Element*) n;
			}
		}
		if (res == NULL) {
			res = findFirstElementRecursive (n, tagname, deep);
		}
		if (res == NULL) {
			Log_Printf ((log_error ? LOG_ERROR : LOG_DEBUG), 
				    "Can't find '%s' element in XML Node"
				    " (deep search=%d)", tagname, (int) deep);
		}
	}
	return res;
}


/******************************************************************************
 * XMLUtil_FindFirstElementValue
 *****************************************************************************/
const char* 
XMLUtil_FindFirstElementValue (const IXML_Node* const node,
			       const char* const tagname,
			       bool const deep, bool const log_error)
{
	IXML_Element* element = XMLUtil_FindFirstElement (node, tagname,
							  deep, log_error);
	return (element ? XMLUtil_GetElementValue (element) : NULL);
}


/******************************************************************************
 * XMLUtil_GetDocumentString
 *****************************************************************************/
char*
XMLUtil_GetDocumentString (void* context, const IXML_Document* doc)
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
XMLUtil_GetNodeString (void* context, const IXML_Node* node)
{
	char* ret = NULL;
	if (node) {
		DOMString s = ixmlPrintNode (discard_const_p (IXML_Node,node));
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


