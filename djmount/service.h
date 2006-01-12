/* $Id$
 *
 * UPnP Service
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

#ifndef SERVICE_INCLUDED
#define SERVICE_INCLUDED 1


#include <stdarg.h>

#include <upnp/upnp.h>
#include <upnp/ixml.h>

#include "string_util.h"	// for StringPair
#include "object.h"


#ifdef __cplusplus
extern "C" {
#endif





/******************************************************************************
 * @var Service
 *
 *	This opaque type encapsulates access to a generic UPnP service.
 *	
 *      NOTE THAT THE FUNCTION API IS NOT THREAD SAFE. Function should 
 *      be called through the global functions in "devicelist.h", 
 *	who lock the global service and device list.
 *
 *****************************************************************************/

OBJECT_DECLARE_CLASS(Service, Object);



/*****************************************************************************
 * @brief Create a new generic service.
 *	This routine parses the DOM service description.
 *	The returned object can be destroyed with "talloc_free".
 *
 * @param context        the talloc parent context
 * @param ctrlpt_handle  the UPnP client handle
 * @param serviceDesc    the DOM service description document
 * @param base_url       the base url of the device description document
 *****************************************************************************/
Service* 
Service_Create (void* context,
		UpnpClient_Handle ctrlpt_handle, 
		IXML_Element* serviceDesc, 
		const char* base_url);


/******************************************************************************
 * @brief 	Subscribe the Service to its eventURL.
 *		Should normally be done just after Service creation.
 *****************************************************************************/
int
Service_SubscribeEventURL (Service* serv);


/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (asynchronous call).
 *
 * @param serv         the service object
 * @param callback     the callback to receive the results
 * @param actionName   the name of the action
 * @param nb_params    number of pairs (names + values)
 * @param params       list of pairs : names + values 
 *****************************************************************************/
int 
Service_SendActionAsync (const Service* serv, Upnp_FunPtr callback,
			 const char* actionName,
			 int nb_params, const StringPair* params);


/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (asynchronous call).
 *
 * @param serv      	the service object
 * @param callback      the callback to receive the results
 * @param actionName    the name of the action
 * @param ...		list of Name / Value pairs.
 *			This list shall be terminated by NULL / NULL.
 *****************************************************************************/
int 
Service_SendActionAsyncVa (const Service* serv,Upnp_FunPtr callback,
			   const char* actionName, ...);


/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (synchronous call).
 *
 * @param serv         the service object
 * @param response     the DOM document for the response. Allocated
 *		       by the SDK ; the caller needs to free it.
 * @param actionName   the name of the action
 * @param nb_params    number of pairs (names + values)
 * @param params       list of pairs : names + values 
 *****************************************************************************/
int 
Service_SendAction (Service* serv, 
		    OUT IXML_Document** response,
		    const char* actionName,
		    int nb_params, const StringPair* params);


/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (synchronous call).
 *
 * @param serv          the service object
 * @param response      the DOM document for the response. Allocated
 *		        by the SDK ; the caller needs to free it.
 * @param actionName    the name of the action
 * @param ...		List of Name / Value pairs.
 *			This list shall be terminated by NULL / NULL.
 *****************************************************************************/
int
Service_SendActionVa (Service* serv,
		      OUT IXML_Document** response,
		      const char* actionName, ...);



/*****************************************************************************
 * @brief Update a service state table.  
 *	Called when an event is received.
 *
 * @param serv         	     the service object
 * @param changedVariables   DOM document representing the XML received
 *                           with the event
 *
 *****************************************************************************/
int 
Service_UpdateState (IN Service* serv, 
		     IN IXML_Document* changedVariables);
  


/*****************************************************************************
 * @brief Returns a string describing the state of the service.
 * 	  The returned string should be freed using "talloc_free".
 *	  If 'debug' is true, returns extra debugging information (which
 *	  might need to be computed).
 *****************************************************************************/
char*
Service_GetStatusString (const Service* serv, 
			 void* result_context, bool debug, const char* spacer);


/*****************************************************************************
 * Accessors 
 *****************************************************************************/
const char*	Service_GetSid (const Service* serv);
const char*	Service_GetEventURL (const Service* serv);
const char*	Service_GetControlURL (const Service* serv);
const char*	Service_GetServiceType (const Service* serv);

int		Service_SetSid (Service* serv, Upnp_SID sid);



#ifdef __cplusplus
}; /* extern "C" */
#endif


#endif /* SERVICE_INCLUDED */



