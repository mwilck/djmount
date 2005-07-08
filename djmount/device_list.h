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
 *	DEVICE_LIST_CALL_SERVICE (rc, deviceName, serviceId, 
 *				  Service, SendAction,
 *	       		          actionName, nb_params, params);
 * will call:
 *	rc = Service_SendAction (the_service, actionName, nb_params, params);
 *
 *****************************************************************************/
#define TBD_DEVICE_LIST_CALL_SERVICE(RET,DEVNAME,SERVID,METHOD,...)	\
  do {								\
    Service* __serv = _DeviceList_LockService(DEVNAME,SERVID);	\
    RET = METHOD(__serv, __VA_ARGS__);				\
    _DeviceList_UnlockService(__serv);				\
  } while (0)								


#define DEVICE_LIST_CALL_SERVICE(RET,DEVNAME,SERVID,SERVTYPE,METHOD,...) \
  do {									\
    Service* __serv = _DeviceList_LockService(DEVNAME,SERVID);		\
    RET = SERVTYPE ## _ ## METHOD					\
      (OBJECT_DYNAMIC_CAST(__serv, SERVTYPE), __VA_ARGS__);		\
    _DeviceList_UnlockService(__serv);					\
  } while (0)								


/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (asynchronous call).
 *
 * @param deviceName    the device name
 * @param serviceId	the service identifier
 * @param actionName    the name of the action
 * @param nb_params	Number of pairs (names + values)
 * @param params	List of pairs : names + values 
 *****************************************************************************/
int 
DeviceList_SendActionAsync (const char* deviceName, const char* serviceId,
			    const char* actionName, 
			    int nb_params, const StringPair* params);


/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (asynchronous call).
 *
 * @param deviceName    the device name
 * @param serviceId	the service identifier
 * @param actionName    the name of the action
 * @param ...		List of Name / Value pairs.
 *			This list shall be terminated by NULL / NULL.
 *****************************************************************************/
// TBD to be deleted
#if 0
int 
DeviceList_SendActionAsyncVa (const char* deviceName, const char* serviceId,
			      const char* actionName, ...);
#endif


/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (synchronous call).
 *
 * @param deviceName    the device name
 * @param serviceId	the service identifier
 * @param actionName    the name of the action
 * @param nb_params	Number of pairs (names + values)
 * @param params	List of pairs : names + values 
 * @return              the DOM document for the response. Allocated
 *		        by the SDK ; the caller needs to free it.
 *****************************************************************************/
IXML_Document* 
DeviceList_SendAction (const char* deviceName, const char* serviceId,
		       const char* actionName, 
		       int nb_params, const StringPair* params);
  

/*****************************************************************************
 * @brief Send an Action request to the specified service of a device
 *	  (synchronous call).
 *
 * @param deviceName    the device name
 * @param serviceId	the service identifier
 * @param actionName    the name of the action
 * @param ...		List of Name / Value pairs.
 *			This list shall be terminated by NULL / NULL.
 * @return              the DOM document for the response. Allocated
 *		        by the SDK ; the caller needs to free it.
 *****************************************************************************/
// TBD to be deleted
#if 0
IXML_Document* 
DeviceList_SendActionVa (const char* deviceName, const char* serviceId,
			 const char* actionName, ...);
#endif
  


#ifdef __cplusplus
}; // extern "C"

template<class T>
int	DeviceList_SendAction1 (const char* deviceName, const char* serviceId,
				const std::string& actionName,
				const std::string& paramName,
				T paramValue)
{
  std::stringstream o;
  o << paramValue;
  return DeviceList_SendActionAsyncVa (deviceName, serviceId, 
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
DevicelList_GetDevicesNames (void* talloc_context);


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
 * Returns a string describing the state of a device
 * (identifiers and state table).
 * The returned string should be freed using "talloc_free".
 *
 * @param talloc_context	parent context to allocate result, may be NULL
 * @param deviceName		the device name
 *****************************************************************************/
char*
DeviceList_GetDeviceStatusString (void* talloc_context, 
				  const char* deviceName);

/**
 * Print the state of a device (see DeviceList_GetDeviceStatusString).
 *
 * @param level		the Log level
 * @param deviceName	the device name
 */
int
DeviceList_PrintDeviceStatus (Log_Level level, const char* deviceName);



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
_DeviceList_LockService (const char* deviceName, const char* serviceId);

void
_DeviceList_UnlockService (Service* serv);



#ifdef __cplusplus
}; // extern "C" 
#endif


#endif // DEVICE_LIST_INCLUDED

