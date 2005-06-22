/* $Id$
 *
 * UPnP Utilities
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

#ifndef UPNP_UTIL_H
#define UPNP_UTIL_H

#include "log.h" // import Log_Level

#include <upnp/upnptools.h>


#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * UpnpUtil_PrintEvent
 *
 * Description: 
 *       Prints callback event structure details.
 *
 * Parameters:
 *   EventType -- The type of callback event
 *   Event -- The callback event structure
 *
 *****************************************************************************/
void
UpnpUtil_PrintEvent (IN Log_Level level,
		     IN Upnp_EventType EventType, 
		     IN const void *Event);


/*****************************************************************************
 * Returns a string with callback event structure details.
 * The returned string should be freed using "talloc_free".
 *
 * @param talloc_context   talloc context to allocate new string (may be 0)
 * @param eventType        the type of callback event	
 * @param event            the callback event structure
 *****************************************************************************/
char*
UpnpUtil_GetEventString (void* talloc_context,
			 IN Upnp_EventType eventType, 
			 IN const void* event);


/*****************************************************************************
 * Returns a string with the name of a Upnp_EventType.
 * @return 	a static constant string, or NULL if unknown event type.
 *****************************************************************************/
const char*
UpnpUtil_GetEventTypeString (IN Upnp_EventType e);



/******************************************************************************
 * Combines a base URL and a relative URL into a single absolute URL.
 * Similar to "UpnpResolveURL" but allocates the memory for the result.
 *
 * @param talloc_context  parent context to allocate result, may be NULL
 * @param baseURL	  the base URL to combine
 * @param relURL	  the URL relative to baseURL
 * @param absURL   	  pointer to store the absolute URL. The returned 
 *			  string should be freed using "talloc_free".
 * @return UPNP_E_SUCCESS if everything went well, else a UPNP error code
 *****************************************************************************/
int
UpnpUtil_ResolveURL (void* talloc_context, 
		     IN const char* baseURL, IN const char* relURL,
		     OUT char** absURL);



#ifdef __cplusplus
}; // extern "C"
#endif



#endif // UPNP_UTIL_H


