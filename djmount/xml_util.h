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

#include <inttypes.h>
#include <upnp/ixml.h>


#ifdef __cplusplus
extern "C" {
#endif




/*****************************************************************************
 * @brief	Get XML Element value
 *
 *       Given a DOM node such as <Channel>11</Channel>, this routine
 *       extracts the value (e.g., 11) from the node and returns it as 
 *       a string. 
 *	 The C string should be copied if necessary e.g. if the IXML_Element
 *	 is to be destroyed. 
 *
 * @param element	the DOM Elemente from which to extract the value
 *****************************************************************************/
char* 
XMLUtil_GetElementValue (IN IXML_Element *element);



/*****************************************************************************
 * @brief	Get value of first matching element.
 *
 *      Given a XML node, this routine searches for the first element
 *      named by the input string item, and returns its value as a string.
 *	The C string should be copied if necessary e.g. if the IXML_Node
 *	is to be destroyed.
 *
 * @param node 	The XML node from which to extract the value
 * @param item 	The item to search for
 *
 *****************************************************************************/
char* 
XMLUtil_GetFirstNodeValue (IN const IXML_Node* node, IN const char *item);


/*****************************************************************************
 * @brief  	Get value of first matching element, converted to integer.
 *
 *      Given a XML node, this routine searches for the first element
 *      named by the input string item, and returns its value as a
 *	(signed) integer.
 *
 * @param node 		The XML node from which to extract the value
 * @param item 		The item to search for
 * @param error_value	Value to return in case of error e.g. conversion error.
 *****************************************************************************/
intmax_t
XMLUtil_GetFirstNodeValueInteger (IN const IXML_Node* node, 
				  IN const char *item,
				  IN intmax_t error_value);


/*****************************************************************************
 * @brief Returns a string printing the content of an XML Document.
 * 	  The returned string should be freed using "talloc_free".
 *****************************************************************************/
char*
XMLUtil_GetDocumentString (void* talloc_context, IN IXML_Document* doc);


/*****************************************************************************
 * @brief Returns a string printing the content of an XML Node.
 * 	  The returned string should be freed using "talloc_free".
 *****************************************************************************/
char*
XMLUtil_GetNodeString (void* talloc_context, IN IXML_Node* node);




#ifdef __cplusplus
}; // extern "C"
#endif



#endif // XML_UTIL_H_INCLUDED



