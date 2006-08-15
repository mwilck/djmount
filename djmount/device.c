/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * UPnP Device
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

#include "device.h"
#include "service.h"
#include "upnp_util.h"
#include "xml_util.h"
#include "log.h"
#include "talloc_util.h"

#include <time.h>
#include <stdbool.h>
#include <upnp/upnp.h>
#include <upnp/LinkedList.h>



struct _Device {

	time_t		creation_time;
	
	IXML_Document*	descDoc;
	const char*	descDocURL;
	const char*	descDocText;

	const char*	udn; 		// <UDN>
	const char*	deviceType; 	// <deviceType>
	const char*	friendlyName; 	// <friendlyName>
	
	const char*	baseURL;	// <URLBase> if exists, else descDocURL
	const char*	presURL;	// <presentationURL> resolved with base
	
	LinkedList	services; // Linked list of Service*
};



/*****************************************************************************
 * @fn	ServiceFactory
 *	Creates a Service object, whose class depends on the service type.
 *****************************************************************************/

#include "service.h"
#include "content_dir.h"

static Service* 
ServiceFactory (Device* dev,
		UpnpClient_Handle ctrlpt_handle, 
		IXML_Element* serviceDesc, 
		const char* base_url)
{
	Service* serv = NULL;
	/*
	 * Simple implementation, hardcoding the 2 possible classes of Service.
	 */
	const char* const serviceType = XMLUtil_GetFirstNodeValue
		((IXML_Node*) serviceDesc, "serviceType", true);

	if ( serviceType && strcmp (serviceType, 
				    CONTENT_DIR_SERVICE_TYPE) == 0 ) {
		serv = ContentDir_ToService 
			(ContentDir_Create (dev, ctrlpt_handle, 
					    serviceDesc, base_url));
	} else {
		serv = Service_Create (dev, ctrlpt_handle,
				       serviceDesc, base_url);
	}
	return serv;
}


/*****************************************************************************
 * @brief	Get serviceList from XML UPnP Device Description Document.
 *
 *       Given a DOM node representing a UPnP Device Description Document,
 *       this routine parses the document and finds the first service list
 *       (i.e., the service list for the root device).  The service list
 *       is returned as a DOM node list. The NodeList must be freed using
 *       NodeList_free.
 *
 * @param doc	The DOM document from which to extract the service list
 *
 *****************************************************************************/
static IXML_NodeList*
getFirstServiceList (IN IXML_Document* doc)
{
  IXML_NodeList* result = NULL;
  
  IXML_NodeList* servlistnodelist =
    ixmlDocument_getElementsByTagName (doc, "serviceList");
  if ( servlistnodelist && ixmlNodeList_length (servlistnodelist) ) {
    
    /*
     * we only care about the first service list, from the root device 
     */
    IXML_Node* servlistnode = ixmlNodeList_item (servlistnodelist, 0);
    
    // Create as list of DOM nodes 
    result = ixmlElement_getElementsByTagName ((IXML_Element*) servlistnode, 
					       "service");
  }
  
  if (servlistnodelist)
    ixmlNodeList_free (servlistnodelist);
  
  return result;
}


/**************************************************************************
 * @brief	Download the XML description document.
 *
 * 	This function is similar to "UpnpDownloadXmlDoc", but returns the text
 * 	of the XML file as well (for debugging).
 * 	Also, it doesn't abort if the MIME type is incorrect (some broken UPnP 
 * 	device send e.g. application/octet-stream instead of text/xml).
 *
 * @param	url		URL of the XML document.
 * @param	xmlDoc		(output) parsed XML document
 * @param	xml_text_malloc	(output) text of XML document. Warning :
 *				malloc'ed, not talloc'ed !
 * @return      UPNP_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
static int
downloadXmlDoc (const char* const url, 
		OUT IXML_Document** const xml_doc, 
		OUT char** const xml_text_malloc)
{
	
	if (url == NULL || xml_doc == NULL || xml_text_malloc == NULL) {
		return UPNP_E_INVALID_PARAM; // ---------->
	}

	*xml_text_malloc = NULL;
	char content_type [LINE_SIZE] = "";
	int rc = UpnpDownloadUrlItem (url, xml_text_malloc, content_type);

	if (rc != UPNP_E_SUCCESS) {
		Log_Printf (LOG_ERROR, "Device downloadXmlDoc can't download "
			    "rc = %d", rc);
	} else {
		if (strncasecmp (content_type, "text/xml", 8)) {
			Log_Printf (LOG_ERROR, "Device description at url '%s'"
				    " has MIME '%s' instead of XML ! "
				    "Trying to parse anyway ...", 
				    NN(content_type), url);
			// Continue anyway ...
		}
		
		*xml_doc = NULL;
		int rc2 = ixmlParseBufferEx (*xml_text_malloc, xml_doc);
		
		if (rc2 != IXML_SUCCESS) {
			Log_Printf (LOG_ERROR, "Device downloadXmlDoc "
				    "can't parse XML document (%d) = %s", 
				    rc2, NN(*xml_text_malloc));
			free (*xml_text_malloc);
			*xml_text_malloc = NULL;
			rc = ( (rc2 == IXML_INSUFFICIENT_MEMORY) ?
			       UPNP_E_OUTOF_MEMORY : UPNP_E_INVALID_DESC );
		}
	}
	
	if (rc == UPNP_E_SUCCESS) {
		Log_Printf (LOG_DEBUG, "Device downloadXmlDoc success, doc =\n"
			    "--------------------\n%s\n--------------------",
			    *xml_text_malloc);
	}
	return rc;
}


/******************************************************************************
 * destroy
 *
 * Description: 
 *	Device destructor, automatically called by "talloc_free".
 *
 *****************************************************************************/
static int
destroy (void* ptr)
{
  if (ptr) {
    Device* const dev = (Device*) ptr;

    /* Delete list.
     * Note that items are not destroyed : Service* are automatically
     * deallocated by "talloc" when parent Device is detroyed.
     */
    ListDestroy (&dev->services, 0); 

    // Delete description document
    if (dev->descDoc) {
      ixmlDocument_free (dev->descDoc);
      dev->descDoc = NULL;
    }  

    // Reset all pointers to NULL 
    memset (dev, 0, sizeof(Device));
    
    // The "talloc'ed" strings will be deleted automatically 
  }
  return 0; // ok -> deallocate memory
}


/*****************************************************************************
 * Device_Create
 *****************************************************************************/

Device* Device_Create (void* context, 
		       UpnpClient_Handle ctrlpt_handle, 
		       const char* descDocURL)
{
	if (descDocURL == NULL)
		return NULL; // ---------->
	Log_Print (LOG_DEBUG, "Device_Create : loading description document");
	IXML_Document* descDoc = NULL;
	char* descDocTextMalloc = NULL;
	// Note: this download can take a long time in some error cases
	// (e.g. timeout if network problems)
	int rc = downloadXmlDoc (descDocURL, &descDoc, &descDocTextMalloc);
	if (rc != UPNP_E_SUCCESS) {
		Log_Printf (LOG_ERROR,
			    "Error obtaining device description from url '%s' "
			    ": %d (%s)", descDocURL, rc, 
			    UpnpGetErrorMessage (rc));
		if (rc/100 == UPNP_E_NETWORK_ERROR/100) {
			Log_Printf (LOG_ERROR,
				    "Check device network configuration "
				    "(firewall ?)");
		}
		return NULL; // ---------->
	}
	
	Device* dev = talloc (context, Device);
	if (dev == NULL) {
		Log_Print (LOG_ERROR, "Device_Create Out of Memory");
		free (descDocTextMalloc);
		return NULL; // ---------->
	}
	
	*dev = (struct _Device) { 
		.creation_time = time (NULL),
		.descDocURL    = talloc_strdup (dev, descDocURL),
		.descDoc       = descDoc,
		.descDocText   = talloc_strdup (dev, descDocTextMalloc),
		// Other fields to empty values
	};
	free (descDocTextMalloc);
	
	/*
	 * Read key elements from description document 
	 */
	
	dev->udn = talloc_strdup (dev, Device_GetDescDocItem (dev, "UDN", 
							      true));
	Log_Printf (LOG_DEBUG, "Device_Create : UDN = %s", dev->udn);

	dev->deviceType = talloc_strdup (dev, Device_GetDescDocItem 
					 (dev, "deviceType", true));
	Log_Printf (LOG_DEBUG, "Device_Create : type = %s", dev->deviceType);
	
	dev->friendlyName = talloc_strdup (dev, Device_GetDescDocItem 
					   (dev, "friendlyName", true));

	const char* const baseURL = Device_GetDescDocItem (dev, "URLBase",
							   false);
	dev->baseURL = ( (baseURL && baseURL[0]) ? baseURL : descDocURL );

	const char* const relURL  = Device_GetDescDocItem (dev, 
							   "presentationURL",
							   false);
	dev->presURL = UpnpUtil_ResolveURL (dev, dev->baseURL, relURL);
	
	/*
	 * Find and parse services
	 */
	ListInit (&dev->services, 0, 0);
	
	IXML_NodeList* serviceList = getFirstServiceList (dev->descDoc);
	const int length = ixmlNodeList_length (serviceList);

	int i;
	for (i = 0; i < length; i++ ) {
		IXML_Element* const serviceDesc = 
			(IXML_Element *) ixmlNodeList_item (serviceList, i);
		Service* const serv = ServiceFactory (dev, ctrlpt_handle, 
						      serviceDesc, 
						      dev->baseURL);
		ListAddTail (&dev->services, serv);
	}
	
	if (serviceList) {
		ixmlNodeList_free (serviceList);
		serviceList = NULL;
	}  

	// Register destructor
	talloc_set_destructor (dev, destroy);
	
	return dev;
}


/*****************************************************************************
 * Device_GetDescDocItem
 *****************************************************************************/
const char*
Device_GetDescDocItem (const Device* dev, const char* item, bool log_error)
{
	if (dev)
		return XMLUtil_GetFirstNodeValue ((IXML_Node*) dev->descDoc, 
						  item, log_error);
	else 
		return NULL;
}


/*****************************************************************************
 * Device_GetDescDocTextCopy
 *****************************************************************************/
char*
Device_GetDescDocTextCopy (const Device* dev, void* result_context)
{
	return (dev ? talloc_strdup (result_context, dev->descDocText) : NULL);
}


/*****************************************************************************
 * Device_SusbcribeAllEvents
 *****************************************************************************/

int
Device_SusbcribeAllEvents (const Device* dev)
{  
	if (dev == NULL)
		return UPNP_E_INVALID_DEVICE; // ---------->

	Log_Printf (LOG_DEBUG, "Device_SusbcribeAllEvents %s",
		    NN(dev->friendlyName));

	int rc = UPNP_E_SUCCESS;
	ListNode* node;
	for (node = ListHead ((LinkedList*) &dev->services); 
	     node != NULL;
	     node = ListNext ((LinkedList*) &dev->services, node)) {
		int rc2 = Service_SubscribeEventURL (node->item); 
		if (rc2 != UPNP_E_SUCCESS)
			rc = rc2;
	}
	return rc;
}


/*****************************************************************************
 * Device_GetService
 *****************************************************************************/

Service*
Device_GetServiceFrom (const Device* dev, 
		       const char* servname, enum GetFrom from,
		       bool log_error)
{  
	if (servname) {
		ListNode* node;
		for (node = ListHead ((LinkedList*) &dev->services); 
		     node != NULL;
		     node = ListNext ((LinkedList*) &dev->services, node)) {
			const char* s = NULL;
			switch (from) {
			case FROM_SID:		
				s = Service_GetSid (node->item); break;
			case FROM_CONTROL_URL:	
				s = Service_GetControlURL (node->item); break;
			case FROM_EVENT_URL:	
				s = Service_GetControlURL (node->item); break;
			case FROM_SERVICE_TYPE:	
				s = Service_GetServiceType (node->item); break;
			}
			if (s && strcmp (servname, s) == 0)
				return node->item; // ---------->
		}
	}
	if (log_error)
		Log_Printf (LOG_ERROR, 
			    "Device '%s' : error finding Service '%s'",
			    NN(dev->friendlyName), NN(servname));
	return NULL;
}



/*****************************************************************************
 * Device_GetStatusString
 *****************************************************************************/
char*
Device_GetStatusString (const Device* dev, void* result_context, bool debug)
{
	if (dev == NULL)
		return NULL; // ---------->

	char* p = talloc_strdup (result_context, "");
	
	// Create a working context for temporary strings
	void* const tmp_ctx = talloc_new (p);
	
	tpr (&p, "  | \n");
	time_t const now = time (NULL);
	tpr (&p, "  +- Discovered on  = %s", ctime (&dev->creation_time));
	p[strlen(p)-1] = ' '; // remove '\n' from 'ctime'
	tpr (&p, "(%ld seconds ago)\n", (long) (now - dev->creation_time));
	tpr (&p, "  +- UDN            = %s\n", dev->udn);
	tpr (&p, "  +- DeviceType     = %s\n", dev->deviceType);
	tpr (&p, "  +- DescDocURL     = %s\n", dev->descDocURL);
	tpr (&p, "  +- FriendlyName   = %s\n", dev->friendlyName);
	tpr (&p, "  +- PresURL        = %s\n", dev->presURL);
	if (debug) {
		tpr (&p, "  +- talloc memory  = %ld blocks / %ld bytes\n", 
		     (long) talloc_total_blocks (dev),
		     (long) talloc_total_size (dev));
	}
	
	ListNode* node;
	for (node = ListHead ((LinkedList*) &dev->services); 
	     node != NULL;
	     node = ListNext ((LinkedList*) &dev->services, node)) {
		const Service* const serv = node->item;
		bool last = (node == ListTail ((LinkedList*) &dev->services));
		
		tpr (&p, "  | \n");
		if (serv == NULL) {
			tpr (&p, "  +- **ERROR** NULL Service\n");
		} else {
			tpr (&p, "  +- Service\n");
			tpr (&p, "%s", Service_GetStatusString 
			     (serv, tmp_ctx, debug,
			      (last ? "      " : "  |   ")));
		}
	}
      
	// Delete all temporary strings
	talloc_free (tmp_ctx);
	
	return p;
}


