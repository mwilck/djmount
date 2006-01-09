/* $Id$
 *
 * DeviceList : List of UPnP Devices
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


#ifndef DEVICE_LIST_INCLUDED
#define DEVICE_LIST_INCLUDED 1


#include <upnp/ixml.h>

#include "log.h"		// import Log_Level
#include "string_util.h"	// import StringArray
#include "service.h"


#ifdef __cplusplus
extern "C" {
#endif



/*****************************************************************************
 * DeviceList_EventCallback
 *
 * Description: 
 *     Prototype for passing back state changes
 *
 * Parameters:
 *   const char * varName
 *   const char * varValue
 *   const char * deviceName
 *****************************************************************************/

typedef enum DeviceList_EventType {
  // TBD E_STATE_UPDATE     = 0,
  E_DEVICE_ADDED     = 1,
  E_DEVICE_REMOVED   = 2,
  // TBD  E_GET_VAR_COMPLETE = 3

} DeviceList_EventType;

typedef void (*DeviceList_EventCallback) (DeviceList_EventType type,
					  const char* deviceName);
  // TBD					  const char* varName, 
  // TBD					  const char* varValue, 



  // TBD
int	
DeviceList_RemoveAll (void);


/*****************************************************************************
 * @fn 	  DeviceList_RefreshAll
 * @brief Clear the current global device list and issue new search
 *	  requests to build it up again from scratch.
 *
 * @param target    the search target as defined in the UPnP Device 
 *                  Architecture v1.0 specification e.g. "ssdp:all" for all.
 *****************************************************************************/
int 
DeviceList_RefreshAll (const char* target);


/*****************************************************************************
 * @fn	  DEVICE_LIST_CALL_SERVICE
 * @brief Finds a Service in the global device list, and calls the specified
 *	  methods on it (the method shall check for NULL Service).
 *
 * Example:
 *	int rc;
 *	DEVICE_LIST_CALL_SERVICE (rc, deviceName, serviceType, 
 *				  Service, SendAction,
 *	       		          actionName, nb_params, params);
 * will call:
 *	rc = Service_SendAction (the_service, actionName, nb_params, params);
 *
 *****************************************************************************/

#define DEVICE_LIST_CALL_SERVICE(RET,DEVNAME,SERVTYPE,SERVCLASS,METHOD,...) \
  do {									\
    Service* __serv = _DeviceList_LockService(DEVNAME,SERVTYPE);	\
    RET = SERVCLASS ## _ ## METHOD					\
      (OBJECT_DYNAMIC_CAST(__serv, SERVCLASS), __VA_ARGS__);		\
    _DeviceList_UnlockService(__serv);					\
  } while (0)								


/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (asynchronous call).
 *
 * @param deviceName    the device name
 * @param serviceType	the service type
 * @param actionName    the name of the action
 * @param nb_params	Number of pairs (names + values)
 * @param params	List of pairs : names + values 
 *****************************************************************************/
int 
DeviceList_SendActionAsync (const char* deviceName, const char* serviceType,
			    const char* actionName, 
			    int nb_params, const StringPair* params);


/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (synchronous call).
 *
 * @param deviceName    the device name
 * @param serviceType	the service type
 * @param actionName    the name of the action
 * @param nb_params	Number of pairs (names + values)
 * @param params	List of pairs : names + values 
 * @return              the DOM document for the response. Allocated
 *		        by the SDK ; the caller needs to free it.
 *****************************************************************************/
IXML_Document* 
DeviceList_SendAction (const char* deviceName, const char* serviceType,
		       const char* actionName, 
		       int nb_params, const StringPair* params);
  


#ifdef __cplusplus
}; // extern "C"

template<class T>
int	DeviceList_SendAction1 (const char* deviceName, 
				const char* serviceType,
				const std::string& actionName,
				const std::string& paramName,
				T paramValue)
{
  std::stringstream o;
  o << paramValue;
  return DeviceList_SendActionAsyncVa (deviceName, serviceType, 
				       actionName.c_str(),
				       paramName.c_str(), o.str().c_str(), 
				       (char*) NULL, (char*) NULL);
}

extern "C" {
#endif // __cplusplus


/*****************************************************************************
 * @brief Get the list of all device names
 * 	  The returned array should be freed using "talloc_free".
 *
 * @param talloc_context	parent context to allocate result, may be NULL
 *****************************************************************************/
StringArray*
DeviceList_GetDevicesNames (void* talloc_context);


/*****************************************************************************
 * Return a string describing the current global status of the device list.
 *
 * @param talloc_context	parent context to allocate result, may be NULL
 *****************************************************************************/
char*
DeviceList_GetStatusString (void* talloc_context);


/**
 * Print the current global status of the device list 
 * (see DeviceList_GetStatusString).
 */
int
DeviceList_PrintStatus (Log_Level);



/*****************************************************************************
 * @brief Returns a string describing the state of a device
 * 	  (identifiers and state table).
 * 	  The returned string should be freed using "talloc_free".
 *	  If 'debug' is true, returns extra debugging information (which
 *	  might need to be computed).
 *
 * @param talloc_context	parent context to allocate result, may be NULL
 * @param deviceName		the device name
 *****************************************************************************/
char*
DeviceList_GetDeviceStatusString (void* talloc_context, 
				  const char* deviceName, bool debug);


/*****************************************************************************
 * @brief Call this function to initialize the UPnP library and start the 
 *	  Control Point.  
 *
 * 	This function creates a timer thread and provides a callback
 *	handler to process any UPnP events that are received.
 * 
 * @param target    the search target as defined in the UPnP Device 
 *                  Architecture v1.0 specification e.g. "ssdp:all" for all.
 * @return UPNP_E_SUCCESS if everything went well, else a UPNP error code
 *****************************************************************************/
int 
DeviceList_Start (const char* target,
		  DeviceList_EventCallback eventCallback);


/*****************************************************************************
 * @brief      	Destroy the device list and stops the UPnP Control Point.
 *
 * @return 	UPNP_E_SUCCESS if everything went well, else a UPNP error code
 *****************************************************************************/
int 
DeviceList_Stop (void);



/*****************************************************************************
 * Internal methods, do not use directly
 *****************************************************************************/
Service*
_DeviceList_LockService (const char* deviceName, const char* serviceType);

void
_DeviceList_UnlockService (Service* serv);



#ifdef __cplusplus
}; // extern "C" 
#endif


#endif // DEVICE_LIST_INCLUDED

