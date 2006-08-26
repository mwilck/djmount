/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * XML Utilities
 * This file is part of djmount.
 *
 * (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
 *
 * Part derived from libupnp example (upnp/sample/common/sample_util.h)
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

#ifndef XML_UTIL_H_INCLUDED
#define XML_UTIL_H_INCLUDED

#include <config.h>

#include <stdbool.h>
#include <inttypes.h>
#include <upnp/ixml.h>


#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * @brief       type-safe XML Node casting.
 *
 *      These macros are safer than direct casting (e.g.
 *      IXML_Node* n = (IXML_Node*) element) and respect constness.
 *
 *****************************************************************************/

// type-casting to Node can be checked at compile-time
#define XML_E2N(ELEM_PTR)       (&((ELEM_PTR)->n))
#define XML_D2N(DOC_PTR)        (&((DOC_PTR)->n))



/*****************************************************************************
 * @brief       discard constness on pointer
 *
 *      This macro should be used in all the places where 'const' are missing
 *	e.g. the ixml API.
 *
 *****************************************************************************/

#ifndef discard_const_p
#   ifdef __GNUC__
#	define discard_const_p(type,ptr) __builtin_choose_expr	\
	(__builtin_types_compatible_p(const type*,typeof(ptr)), \
	 ((type *)(ptr)),(ptr))
// non-gcc default implementation copied from talloc.c :
#   elif defined(__intptr_t_defined) || defined(HAVE_INTPTR_T)
#	define discard_const_p(type,ptr) ((type *)((intptr_t)(ptr)))
#   else
#	define discard_const_p(type,ptr) ((type *)(ptr))
#   endif
#endif



/*****************************************************************************
 * @brief	Get XML Element text value
 *
 *       Given a DOM node such as <Channel>11</Channel>, this routine
 *       extracts the text value (e.g., 11) from the node and returns it as 
 *       a string. 
 *	 The C string should be copied if necessary e.g. if the IXML_Element
 *	 is to be destroyed. 
 *
 * @param element	the DOM Element from which to extract the value
 *****************************************************************************/
const char* 
XMLUtil_GetElementValue (IN const IXML_Element* element);


/*****************************************************************************
 * @brief	Get first XML Element matching tagName.
 *
 *      Given a XML node, returns the first Element with a given tagName,
 *	in a preorder traversal of this node tree.
 *	If 'deep' if false, search only direct children of the node, else
 *	search the whole tree.
 *	The resulting element is a pointer inside the tree and should be
 *	copied if necessary e.g. if the IXML_Node is to be destroyed.
 *
 * @param node 		the XML node from which to search the element
 * @param tagname 	the tagName to search for
 * @param deep		search whole tree or only direct children
 * @param log_error	whether to log an error or not if item not found
 *
 *****************************************************************************/
IXML_Element*
XMLUtil_FindFirstElement (const IXML_Node* const node,
			  const char* const tagname,
			  bool const deep, bool const log_error);


/*****************************************************************************
 * @brief	Get value of first XML Element matching tagName.
 *
 *      Given a XML node, searches the first Element with a given tagName,
 *	in a preorder traversal of this node tree, and returns its value 
 *	as a string.
 *	If 'deep' if false, search only direct children of the node, else
 *	recursively search the whole tree.
 *	The C string should be copied if necessary e.g. if the IXML_Node
 *	is to be destroyed.
 *
 * @param node 		the XML node from which to search the element
 * @param tagname	the tagName to search for
 * @param deep		search whole tree or only direct children
 * @param log_error	whether to log an error or not if item not found
 *
 *****************************************************************************/
const char* 
XMLUtil_FindFirstElementValue (const IXML_Node* const node,
			       const char* const tagname,
			       bool const deep, bool const log_error);


/*****************************************************************************
 * @brief Returns a string printing the content of an XML Document.
 * 	  The returned string should be freed using "talloc_free".
 *****************************************************************************/
char*
XMLUtil_GetDocumentString (void* talloc_context, IN const IXML_Document* doc);


/*****************************************************************************
 * @brief Returns a string printing the content of an XML Node.
 * 	  The returned string should be freed using "talloc_free".
 *****************************************************************************/
char*
XMLUtil_GetNodeString (void* talloc_context, IN const IXML_Node* node);




#ifdef __cplusplus
}; // extern "C"
#endif



#endif // XML_UTIL_H_INCLUDED



