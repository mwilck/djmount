/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * DeviceList : List of UPnP Devices
 * This file is part of djmount.
 *
 * (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
 *
 * Part derived from libupnp example (upnp/sample/tvctrlpt/upnp_tv_ctrlpt.c)
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

#include "device_list.h"
#include "device.h"
#include "upnp_util.h"
#include "log.h"
#include "service.h"
#include "talloc_util.h"

#include <stdbool.h>
#include <upnp/upnp.h>
#include <upnp/ithread.h>
#include <upnp/upnptools.h>
#include <upnp/LinkedList.h>



// How often to check advertisement and subscription timeouts for devices
static const unsigned int CHECK_SUBSCRIPTIONS_TIMEOUT = 10; // in seconds



static UpnpClient_Handle g_ctrlpt_handle = -1;


static ithread_t g_timer_thread;

static char* g_ssdp_target = NULL;


/*
 * Mutex for protecting the global device list
 * in a multi-threaded, asynchronous environment.
 * All functions should lock this mutex before reading
 * or writing the device list. 
 */
static ithread_mutex_t DeviceListMutex;


/*
 * The global device list
 */

struct _DeviceNode {
  char*    deviceId; // as reported by the discovery callback
  Device*  d;
  int      expires; 
};
typedef struct _DeviceNode DeviceNode;


//
// TBD XXX TBD
// TBD to replace with Hash Table for better performances XXX
// TBD XXX TBD
//
static LinkedList GlobalDeviceList;



/*****************************************************************************
 * NotifyUpdate
 *
 * Description: callback for update events
 *
 * Parameters:
 *
 *****************************************************************************/

// Function pointer for update event callbacks
static DeviceList_EventCallback gStateUpdateFun = 0;

static void
NotifyUpdate (DeviceList_EventType type,
	      // TBD	      const char* varName,
	      // TBD	      const char* varValue,
	      const DeviceNode* devnode)
{
  if (gStateUpdateFun && devnode && devnode->d)
    gStateUpdateFun (type, talloc_get_name (devnode->d));
  // TBD: Add mutex here?
}


/*****************************************************************************
 * GetDeviceNodeFromName
 *
 * Description: 
 *       Given a device name, returns the pointer to the device node.
 *       Note that this function is not thread safe.  It must be called 
 *       from a function that has locked the global device list.
 *
 * @param name 	the device name
 *
 *****************************************************************************/

static DeviceNode*
GetDeviceNodeFromName (const char* name, bool log_error)
{
  if (name) {
    ListNode* node;
    for (node = ListHead (&GlobalDeviceList);
	 node != 0;
	 node = ListNext (&GlobalDeviceList, node)) {
      DeviceNode* const devnode = node->item;
      if (devnode && devnode->d && 
	  strcmp (talloc_get_name (devnode->d), name) == 0)
	return devnode; // ---------->
    }
  }
  if (log_error)
    Log_Printf (LOG_ERROR, "Error finding Device named %s", NN(name));
  return 0;
}

static ListNode*
GetDeviceListNodeFromId (const char* deviceId)
{
  if (deviceId) {
    ListNode* node;
    for (node = ListHead (&GlobalDeviceList);
	 node != 0;
	 node = ListNext (&GlobalDeviceList, node)) {
      DeviceNode* const devnode = node->item;
      if (devnode && devnode->deviceId && 
	  strcmp (devnode->deviceId, deviceId) == 0) 
	return node; // ---------->
    }
  }
  return 0;
}

static Service*
GetService (const char* s, enum GetFrom from) 
{
	ListNode* node;
	for (node = ListHead (&GlobalDeviceList);
	     node != NULL;
	     node = ListNext (&GlobalDeviceList, node)) {
		DeviceNode* const devnode = node->item;
		if (devnode) {
			Service* const serv = Device_GetServiceFrom 
				(devnode->d, s, from, false);
			if (serv) 
				return serv; // ---------->
		}
	}
	Log_Printf (LOG_ERROR, "Can't find service matching %s in device list",
		    NN(s));
	return NULL;
}


/*****************************************************************************
 * make_device_name
 *
 * Generate a unique device name.
 * Must be deallocated with "talloc_free".
 *****************************************************************************/
static char*
make_device_name (void* talloc_context, const char* base)
{
  if (base == 0 || *base == '\0')
    base = "unnamed";
  char* name = String_CleanFileName (talloc_context, base);
  
  char* res = name;
  int i = 1;
  while (GetDeviceNodeFromName (res, false)) {
    if (res != name)
      talloc_free (res);
    res = talloc_asprintf (talloc_context, "%s_%d", name, ++i);
  }
  if (res != name)
    talloc_free (name);
  
  return res;
}


/*****************************************************************************
 * DeviceList_RemoveDevice
 *
 * Description: 
 *       Remove a device from the global device list.
 *
 * Parameters:
 *   deviceId -- The Unique Device Name for the device to remove
 *
 *****************************************************************************/
int
DeviceList_RemoveDevice (const char* deviceId)
{
	int rc = UPNP_E_SUCCESS;

	ithread_mutex_lock (&DeviceListMutex);
	
	ListNode* const node = GetDeviceListNodeFromId (deviceId);
	if (node) {
		DeviceNode* devnode = node->item;
		node->item = 0;
		ListDelNode (&GlobalDeviceList, node, /*freeItem=>*/ 0);
		// Do the notification while the global list is still locked
		NotifyUpdate (E_DEVICE_REMOVED, devnode);
		talloc_free (devnode);
	} else {
		Log_Printf (LOG_WARNING, "RemoveDevice can't find Id=%s", 
			    NN(deviceId));
		rc = UPNP_E_INVALID_DEVICE;
	}
	
	ithread_mutex_unlock (&DeviceListMutex);
	
	return rc;
}


/*****************************************************************************
 * DeviceList_RemoveAll
 *
 * Description: 
 *       Remove all devices from the global device list.
 *
 * Parameters:
 *   None
 *
 *****************************************************************************/
static int
DeviceList_RemoveAll (void)
{
  ithread_mutex_lock( &DeviceListMutex );

  ListNode* node;
  for (node = ListHead (&GlobalDeviceList);
       node != 0;
       node = ListNext (&GlobalDeviceList, node)) {
    DeviceNode* devnode = node->item;
    node->item = 0;
    // Do the notifications while the global list is still locked
    NotifyUpdate (E_DEVICE_REMOVED, devnode);
    talloc_free (devnode);
  }
  ListDestroy (&GlobalDeviceList, /*freeItem=>*/ 0);
  ListInit (&GlobalDeviceList, 0, 0);

  ithread_mutex_unlock( &DeviceListMutex );
  
  return UPNP_E_SUCCESS;
}



/*****************************************************************************
 * DeviceList_RefreshAll
 *****************************************************************************/
int
DeviceList_RefreshAll (bool remove_all)
{
	if (remove_all)
		(void) DeviceList_RemoveAll();
  
	/*
	 * Search for all 'target' providers,
	 * waiting for up to 5 seconds for the response 
	 */
	Log_Printf (LOG_DEBUG, "RefreshAll target=%s", NN(g_ssdp_target));
	int rc = UpnpSearchAsync (g_ctrlpt_handle, 5 /* seconds */, 
				  g_ssdp_target, NULL);
	if (UPNP_E_SUCCESS != rc) 
		Log_Printf (LOG_ERROR, "Error sending search request %d", rc);
	
	return rc;
}


/*****************************************************************************
 * HandleEvent
 *
 * Description: 
 *       Handle a UPnP event that was received.  Process the event and update
 *       the appropriate service state table.
 *
 * Parameters:
 *   sid -- The subscription id for the event
 *   eventkey -- The eventkey number for the event
 *   changes -- The DOM document representing the changes
 *
 *****************************************************************************/
static void
HandleEvent (Upnp_SID sid,
	     int eventkey,
	     IXML_Document* changes )
{
  ithread_mutex_lock( &DeviceListMutex );
  
  Log_Printf (LOG_DEBUG, "Received Event: %d for SID %s", eventkey, NN(sid));
  Service* const serv = GetService (sid, FROM_SID);
  if (serv) 
    Service_UpdateState (serv, changes);
  
  ithread_mutex_unlock( &DeviceListMutex );
}


/*****************************************************************************
 * AddDevice
 *
 * Description: 
 *       If the device is not already included in the global device list,
 *       add it.  Otherwise, update its advertisement expiration timeout.
 *
 * Parameters:
 *   descLocation -- The location of the description document URL
 *   expires -- The expiration time for this advertisement
 *
 *****************************************************************************/


static void
AddDevice (const char* deviceId,
	   const char* descLocation,
	   int expires)
{
	ithread_mutex_lock (&DeviceListMutex);

	DeviceNode* devnode = NULL;
	ListNode* node = GetDeviceListNodeFromId (deviceId);
	if (node) 
		devnode = node->item;
	
	if (devnode) {
		// The device is already there, so just update 
		// the advertisement timeout field
		Log_Printf (LOG_DEBUG, 
			    "AddDevice Id=%s already exists, "
			    "only update expiration = %d seconds",
			    NN(deviceId), expires);
		devnode->expires = expires;
	} else {
		// Else create a new device
		Log_Printf (LOG_DEBUG, "AddDevice try new device Id=%s", 
			    NN(deviceId));
		
		// *unlock* before trying to download the Device Description 
		// Document, which can take a long time in some error cases 
		// (e.g. timeout if network problems)
		ithread_mutex_unlock (&DeviceListMutex);

		if (descLocation == NULL) {
			Log_Printf (LOG_ERROR, 
				    "NULL description doc. URL device Id=%s", 
				    NN(deviceId));
			return; // ---------->
		}

		char* descDocText = NULL;
		char content_type [LINE_SIZE] = "";
		int rc = UpnpDownloadUrlItem (descLocation, &descDocText, 
					      content_type);
		if (rc != UPNP_E_SUCCESS) {
			Log_Printf (LOG_ERROR,
				    "Error obtaining device description from "
				    "url '%s' : %d (%s)", descLocation, rc, 
				    UpnpGetErrorMessage (rc));
			if (rc/100 == UPNP_E_NETWORK_ERROR/100) {
				Log_Printf (LOG_ERROR, "Check device network "
					    "configuration (firewall ?)");
			}
			return; // ---------->
		} 
		if (strncasecmp (content_type, "text/xml", 8)) {
			// "text/xml" is specified in UPnP Device Architecture
			// v1.0 -- however don't abort if incorrect because
			// some broken UPnP device send other MIME types 
			// (e.g. application/octet-stream).
			Log_Printf (LOG_ERROR, "Device description at url '%s'"
				    " has MIME '%s' instead of XML ! "
				    "Trying to parse anyway ...", 
				    descLocation, content_type);
		}
		
		void* context = NULL; // TBD should be parent talloc TBD XXX

		devnode = talloc (context, DeviceNode);
		// Initialize fields to empty values
		*devnode = (struct _DeviceNode) { }; 

		devnode->d = Device_Create (devnode, g_ctrlpt_handle, 
					    descLocation, deviceId,
					    descDocText);
		free (descDocText);
		descDocText = NULL;

		if (devnode->d == NULL) {
			Log_Printf (LOG_ERROR, "Can't create Device Id=%s", 
				    NN(deviceId));
			talloc_free (devnode);
			return; // ---------->
		} else {
			// If SSDP target specified, check that the device
			// matches it.
			if (strstr (g_ssdp_target, ":service:")) {
				const Service* serv = Device_GetServiceFrom 
					(devnode->d, g_ssdp_target, 
					 FROM_SERVICE_TYPE, false);
				if (serv == NULL) {
					Log_Printf (LOG_DEBUG,
						    "Discovered device Id=%s "
						    "has no '%s' service : "
						    "forgetting", NN(deviceId),
						    g_ssdp_target);
					talloc_free (devnode);
					return; // ---------->
				}
			}

			// Relock the device list (and check that the same
			// device has not already been added by another thread
			// while the list was unlocked)
			ithread_mutex_lock (&DeviceListMutex);
			node = GetDeviceListNodeFromId (deviceId);
			if (node) {
				Log_Printf (LOG_WARNING, 
					    "Device Id=%s already added",
					    NN(deviceId));
				// Delete extraneous device descriptor. Note:
				// service subscription is not yet done, so 
				// the Service destructors will not unsubscribe
				talloc_free (devnode);
			} else {
				devnode->deviceId = talloc_strdup (devnode, 
								   deviceId);
				devnode->expires = expires;
				
				// Generate a unique, friendly, name 
				// for this device
				const char* base = Device_GetDescDocItem 
					(devnode->d, "friendlyName", true);
				char* name = make_device_name (NULL, base);
				talloc_set_name (devnode->d, "%s", name);
				talloc_free (name);
				
				Log_Printf (LOG_INFO, 
					    "Add new device : Name='%s' "
					    "Id='%s' descURL='%s'", 
					    NN(talloc_get_name (devnode->d)), 
					    NN(deviceId), descLocation);

				// Insert the new device node in the list
				ListAddTail (&GlobalDeviceList, devnode);

				Device_SusbcribeAllEvents (devnode->d);
				
				// Notify New Device Added, while the global 
				// list is still locked
				NotifyUpdate (E_DEVICE_ADDED, devnode);
			}
		}
	}
	ithread_mutex_unlock (&DeviceListMutex);
}
  


/*****************************************************************************
 * EventHandlerCallback
 *
 * Description: 
 *       The callback handler registered with the SDK while registering
 *       the control point.  Detects the type of callback, and passes the 
 *       request on to the appropriate function.
 *
 * Parameters:
 *   event_type -- The type of callback event
 *   event -- Data structure containing event data
 *   cookie -- Optional data specified during callback registration
 *
 *****************************************************************************/
static int
EventHandlerCallback (Upnp_EventType event_type,
		      void* event, void* cookie)
{
	// Create a working context for temporary strings
	void* const tmp_ctx = talloc_new (NULL);

	Log_Print (LOG_DEBUG, UpnpUtil_GetEventString (tmp_ctx, event_type, 
						       event));
	
	switch ( event_type ) {
		/*
		 * SSDP Stuff 
		 */
	case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
	case UPNP_DISCOVERY_SEARCH_RESULT:
	{
		const struct Upnp_Discovery* const e =
			(struct Upnp_Discovery*) event;
		
		if (e->ErrCode != UPNP_E_SUCCESS) {
			Log_Printf (LOG_ERROR, 
				    "Error in Discovery Callback -- %d", 
				    e->ErrCode);	
		}
		// TBD else ??
      
		if (e->DeviceId && e->DeviceId[0]) { 
			Log_Printf (LOG_DEBUG, 
				    "Discovery : device type '%s' "
				    "OS '%s' at URL '%s'", NN(e->DeviceType), 
				    NN(e->Os), NN(e->Location));
			AddDevice (e->DeviceId, e->Location, e->Expires);
			Log_Printf (LOG_DEBUG, "Discovery: "
				    "DeviceList after AddDevice = \n%s",
				    DeviceList_GetStatusString (tmp_ctx));
		}
		
		break;
	}
    
	case UPNP_DISCOVERY_SEARCH_TIMEOUT:
		/*
		 * Nothing to do here... 
		 */
		break;
		
	case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    	{
		struct Upnp_Discovery* e = (struct Upnp_Discovery*) event;
      
		if (e->ErrCode != UPNP_E_SUCCESS ) {
			Log_Printf (LOG_ERROR,
				    "Error in Discovery ByeBye Callback -- %d",
				    e->ErrCode );
		}
		
		Log_Printf (LOG_DEBUG, "Received ByeBye for Device: %s",
			    e->DeviceId );
		DeviceList_RemoveDevice (e->DeviceId);
		
		Log_Printf (LOG_DEBUG, "DeviceList after byebye: \n%s",
			    DeviceList_GetStatusString (tmp_ctx));
		break;
	}
    
	/*
	 * SOAP Stuff 
	 */
	case UPNP_CONTROL_ACTION_COMPLETE:
    	{
		struct Upnp_Action_Complete* e = 
			(struct Upnp_Action_Complete*) event;
      
		if (e->ErrCode != UPNP_E_SUCCESS ) {
			Log_Printf (LOG_ERROR,
				    "Error in  Action Complete Callback -- %d",
				    e->ErrCode );
		}
		
		/*
		 * No need for any processing here, just print out results.  
		 * Service state table updates are handled by events. 
		 */
		
		break;
	}
	
	case UPNP_CONTROL_GET_VAR_COMPLETE:
		/*
		 * Not used : deprecated
		 */
		Log_Printf (LOG_WARNING, 
			    "Deprecated Get Var Complete Callback");
		break;
    
		/*
		 * GENA Stuff 
		 */
	case UPNP_EVENT_RECEIVED:
	{
		struct Upnp_Event* e = (struct Upnp_Event*) event;
		
		HandleEvent (e->Sid, e->EventKey, e->ChangedVariables);
		break;
	}
	
	case UPNP_EVENT_SUBSCRIBE_COMPLETE:
	case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
	case UPNP_EVENT_RENEWAL_COMPLETE:
	{
		struct Upnp_Event_Subscribe* e =
			(struct Upnp_Event_Subscribe*) event;
      
		if (e->ErrCode != UPNP_E_SUCCESS ) {
			Log_Printf (LOG_ERROR,
				    "Error in Event Subscribe Callback -- %d",
				    e->ErrCode );
		} else {
			Log_Printf (LOG_DEBUG, 
				    "Received Event Renewal for eventURL %s", 
				    NN(e->PublisherUrl));

			ithread_mutex_lock (&DeviceListMutex);

			Service* const serv = GetService (e->PublisherUrl,
							  FROM_EVENT_URL);
			if (serv) {
				if (event_type == 
				    UPNP_EVENT_UNSUBSCRIBE_COMPLETE)
					Service_SetSid (serv, NULL);
				else			      
					Service_SetSid (serv, e->Sid);
			}
			ithread_mutex_unlock (&DeviceListMutex);
		}
		break;
	}
    
	case UPNP_EVENT_AUTORENEWAL_FAILED:
	case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
    	{
		struct Upnp_Event_Subscribe* e = 
			(struct Upnp_Event_Subscribe*) event;

		Log_Printf (LOG_DEBUG, "Renewing subscription for eventURL %s",
			    NN(e->PublisherUrl));
     
		ithread_mutex_lock (&DeviceListMutex);
      
		Service* const serv = GetService (e->PublisherUrl, 
						  FROM_EVENT_URL);
		if (serv) 
			Service_SubscribeEventURL (serv);
		
		ithread_mutex_unlock (&DeviceListMutex);
		
		break;
	}
    
	/*
	 * ignore these cases, since this is not a device 
	 */
	case UPNP_EVENT_SUBSCRIPTION_REQUEST:
	case UPNP_CONTROL_GET_VAR_REQUEST:
	case UPNP_CONTROL_ACTION_REQUEST:
		break;
	}
  	
  	// Delete all temporary strings
	talloc_free (tmp_ctx);

	return 0;
}


/*****************************************************************************
 * _DeviceList_LockDevice
 *****************************************************************************/
Device*
_DeviceList_LockDevice (const char* deviceName)
{
	Device* dev = NULL;

	Log_Printf (LOG_DEBUG, "LockDevice : device '%s'", NN(deviceName));

	// XXX coarse implementation : lock the whole device list, 
	// XXX not only the device.
	ithread_mutex_lock (&DeviceListMutex);
	
	const DeviceNode* devnode = GetDeviceNodeFromName (deviceName, true);
	if (devnode) 
		dev = devnode->d;
	if (dev == NULL)
		ithread_mutex_unlock (&DeviceListMutex);
	return dev;
}


/*****************************************************************************
 * _DeviceList_UnlockDevice
 *****************************************************************************/
inline void
_DeviceList_UnlockDevice (Device* dev)
{
	if (dev)
		ithread_mutex_unlock (&DeviceListMutex);
}


/*****************************************************************************
 * _DeviceList_LockService
 *****************************************************************************/
Service*
_DeviceList_LockService (const char* deviceName, const char* serviceType)
{
	Service* serv = NULL;

	Log_Printf (LOG_DEBUG, "LockService : device '%s' service '%s'",
		    NN(deviceName), NN(serviceType));

	// XXX coarse implementation : lock the whole device list, 
	// XXX not only the service.
	ithread_mutex_lock (&DeviceListMutex);
	
	const DeviceNode* devnode = GetDeviceNodeFromName (deviceName, true);
	if (devnode) 
		serv = Device_GetServiceFrom (devnode->d, serviceType, 
					      FROM_SERVICE_TYPE, true);
	if (serv == NULL)
		ithread_mutex_unlock (&DeviceListMutex);
	return serv;
}


/*****************************************************************************
 * _DeviceList_UnlockService
 *****************************************************************************/
inline void
_DeviceList_UnlockService (Service* serv)
{
	if (serv)
		ithread_mutex_unlock (&DeviceListMutex);
}


/*****************************************************************************
 * DeviceList_SendActionAsync
 *****************************************************************************/
int
DeviceList_SendActionAsync (const char* deviceName, const char* serviceType,
			    const char* actionName, 
			    int nb_params, const StringPair* params)
{
  int rc = UPNP_E_INTERNAL_ERROR;
  DEVICE_LIST_CALL_SERVICE (rc, deviceName, serviceType,
			    Service, SendActionAsync,
			    EventHandlerCallback, actionName, 
			    nb_params, params);
  return rc;
}


/*****************************************************************************
 * DeviceList_SendAction
 *****************************************************************************/
IXML_Document*
DeviceList_SendAction (const char* deviceName, const char* serviceType,
		       const char* actionName, 
		       int nb_params, const StringPair* params)
{
  IXML_Document* res = NULL;
  int rc = UPNP_E_INTERNAL_ERROR;
  DEVICE_LIST_CALL_SERVICE (rc, deviceName, serviceType, Service, SendAction,
			    &res, actionName, nb_params, params);
  return (rc == UPNP_E_SUCCESS ? res : NULL);
}


/*****************************************************************************
 * GetDevicesNames
 *****************************************************************************/
PtrArray*
DeviceList_GetDevicesNames (void* context)
{
	ithread_mutex_lock (&DeviceListMutex);

	Log_Printf (LOG_DEBUG, "GetDevicesNames");
	PtrArray* const a = PtrArray_CreateWithCapacity 
		(context, ListSize (&GlobalDeviceList));
	if (a) {
		ListNode* node;
		for (node = ListHead (&GlobalDeviceList);
		     node != 0;
		     node = ListNext (&GlobalDeviceList, node)) {
			const DeviceNode* const devnode = node->item;
			if (devnode) {
				const char* const name = 
					(devnode->d ? 
					 talloc_get_name (devnode->d) : NULL);
				// add pointer directly 
				// TBD no need to copy ??? XXX
				PtrArray_Append (a, (char*) NN(name)); 
			}
		}
	}
  
	ithread_mutex_unlock (&DeviceListMutex);
	
	return a;
}



/*****************************************************************************
 * DeviceList_GetStatusString
 *****************************************************************************/
char*
DeviceList_GetStatusString (void* context)
{
  char* ret = talloc_strdup (context, "");
  if (ret) {
    ithread_mutex_lock (&DeviceListMutex);
    
    // Print the universal device names for each device 
    // in the global device list
    ListNode* node;
    for (node = ListHead (&GlobalDeviceList);
	 node != 0;
	 node = ListNext (&GlobalDeviceList, node)) {
      const DeviceNode* const devnode = node->item;
      if (devnode) {
	const char* const name = 
	  (devnode->d ? talloc_get_name(devnode->d) : 0);
	ret = talloc_asprintf_append (ret, " %-20s -- %s\n", 
				      NN(name), NN(devnode->deviceId));
      }
    }
    ithread_mutex_unlock (&DeviceListMutex);
  }
  return ret;
}



/*****************************************************************************
 * DeviceList_GetDeviceStatusString
 *****************************************************************************/
char*
DeviceList_GetDeviceStatusString (void* context, const char* deviceName,
				  bool debug) 
{
	char* p = NULL;
  
	ithread_mutex_lock (&DeviceListMutex);
	
	DeviceNode* const devnode = GetDeviceNodeFromName (deviceName, true);
	if (devnode) { 
		p = talloc_asprintf (context, "Device \"%s\" (", deviceName);
		if (devnode->expires >= 0) 
			tpr (&p, "expires in %d seconds", devnode->expires);
		else
			tpr (&p, "expired %d seconds ago", -devnode->expires);
		char* tmp = Device_GetStatusString (devnode->d, p, debug);
		tpr (&p, ")\n%s", tmp);
		talloc_free (tmp);
	} 
	
	ithread_mutex_unlock (&DeviceListMutex);
	
	return p;
}


/*****************************************************************************
 * @fn	VerifyTimeouts
 *
 * @brief Checks the advertisement for each device in the global device list.  
 *	If an advertisement expires, the device should be removed from 
 *	the list (because normally a device should refresh its advertisement 
 *	prior to expiration, according to the UPnP device architecture).
 *	However, in case the new advertisement has been missed 
 *	(e.g. network problem or misbehaving device), we first send a new
 *	search request for that device.
 *
 * @param incr	the increment to subtract from the timeouts each time the
 *            	function is called.
 *
 *****************************************************************************/
static void
VerifyTimeouts (int incr)
{
	ithread_mutex_lock (&DeviceListMutex);
  
	// During this traversal we pre-compute the next node in case 
	// the current node is deleted
	ListNode* node, *nextnode = NULL;
	for (node = ListHead (&GlobalDeviceList); 
	     node != NULL; 
	     node = nextnode) {
		nextnode = ListNext (&GlobalDeviceList, node);
		
		DeviceNode* const devnode = node->item;
		devnode->expires -= incr;
		
		if (devnode->expires <= -incr) {
			// Too late : really remove the device from the list 
			Log_Printf (LOG_DEBUG, "Remove expired device Id=%s", 
				    devnode->deviceId);
			node->item = NULL;
			ListDelNode (&GlobalDeviceList, node, /*freeItem=>*/0);
			// Do the notification while the global list is locked
			NotifyUpdate (E_DEVICE_REMOVED, devnode);
			talloc_free (devnode);

		} else if (devnode->expires <= 0) {
			// This advertisement has expired, so we should 
			// normally remove the device from the list.
			// First, send out a search request for this device 
			// UDN to try to renew.
			Log_Printf (LOG_DEBUG, 
				    "Trying to renew expired device Id=%s", 
				    devnode->deviceId);
			int rc = UpnpSearchAsync (g_ctrlpt_handle, incr,
						  devnode->deviceId, NULL);
			if (rc != UPNP_E_SUCCESS)
				Log_Printf (LOG_ERROR, 
					    "Error sending search request "
					    "for Device UDN=%s : err = %d",
					    devnode->deviceId, rc);
		}
	}
	ithread_mutex_unlock (&DeviceListMutex);
}


/*****************************************************************************
 * CheckSubscriptionsLoop
 *
 * Description: 
 *       Function that runs in its own thread and monitors advertisement
 *       and subscription timeouts for devices in the global device list.
 *
 * Parameters:
 *    None
 *
 *****************************************************************************/
static void*
CheckSubscriptionsLoop (void* arg)
{
	while (true) {
		isleep (CHECK_SUBSCRIPTIONS_TIMEOUT);
		VerifyTimeouts (CHECK_SUBSCRIPTIONS_TIMEOUT);
	}
}


/*****************************************************************************
 * DeviceList_Start
 *****************************************************************************/
int
DeviceList_Start (const char* ssdp_target, 
		  DeviceList_EventCallback eventCallback)
{
	// Cf. AddDevice : only some SSDP target are implemented 
	if (ssdp_target == NULL || 
	    (strcmp (ssdp_target, "ssdp:all") != 0 &&
	     strstr (ssdp_target, ":service:") == NULL)) {
		Log_Printf (LOG_ERROR, "DeviceList : invalid or not "
			    "implemented SSDP target '%s", NN(ssdp_target));
		return UPNP_E_INVALID_PARAM; // ---------->
	}

	int rc;
	unsigned short port = 0;
	char* ip_address = NULL;
	
	gStateUpdateFun = eventCallback;
	
	ithread_mutex_init (&DeviceListMutex, NULL);
	
	ListInit (&GlobalDeviceList, 0, 0);
	
	// Makes the XML parser more tolerant to malformed text
	ixmlRelaxParser ('?');
	
	Log_Printf (LOG_DEBUG, "Intializing UPnP with ipaddress=%s port=%d",
		    NN(ip_address), port);
	rc = UpnpInit (ip_address, port);
	if( UPNP_E_SUCCESS != rc ) {
		Log_Printf (LOG_ERROR, "UpnpInit() Error: %d", rc);
		UpnpFinish();
		if (rc == UPNP_E_SOCKET_ERROR) {
			Log_Printf (LOG_ERROR, "Check network configuration, "
				    "in particular that a multicast route "
				    "is set for the default network "
				    "interface");
		}
		return rc; // ---------->
	}
	
	if ( NULL == ip_address )
		ip_address = UpnpGetServerIpAddress();
	if ( 0 == port )
		port = UpnpGetServerPort();
	
	Log_Printf (LOG_INFO, "UPnP Initialized (%s:%d)", 
		    NN(ip_address), port);
	
	Log_Printf (LOG_DEBUG, "Registering Control Point" );
	rc = UpnpRegisterClient (EventHandlerCallback,
				 &g_ctrlpt_handle, &g_ctrlpt_handle);
	if( UPNP_E_SUCCESS != rc ) {
		Log_Printf (LOG_ERROR, "Error registering CP: %d", rc);
		UpnpFinish();
		return rc; // ---------->
	}
	
	Log_Printf (LOG_DEBUG, "Control Point Registered" );
	
	g_ssdp_target = talloc_strdup (NULL, ssdp_target);
	DeviceList_RefreshAll (true);
	
	// start a timer thread
	ithread_create (&g_timer_thread, NULL, CheckSubscriptionsLoop, NULL);
	
	return rc;
}


/*****************************************************************************
 * DeviceList_Stop
 *****************************************************************************/
int
DeviceList_Stop (void)
{
	int rc;
	
	/*
	 * Reverse all "Start" operations
	 */
	
	ithread_cancel (g_timer_thread);
	
	DeviceList_RemoveAll();
	talloc_free (g_ssdp_target);
	g_ssdp_target = NULL;

	UpnpUnRegisterClient (g_ctrlpt_handle);
	rc = UpnpFinish();
	
	ListDestroy (&GlobalDeviceList, /*freeItem=>*/ 0);
	
	ithread_mutex_destroy (&DeviceListMutex);
	
	gStateUpdateFun = 0;
	
	return rc;
}

