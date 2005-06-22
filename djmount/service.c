/* $Id$
 *
 * UPnP Service
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

#include "service.h"

#include <string.h>
#include <stdarg.h>	/* missing from "talloc.h" */

#include "log.h"
#include "xml_util.h"
#include "upnp_util.h"

#include <talloc.h>
#include <upnp/upnp.h>
#include <upnp/upnptools.h>
#include <upnp/LinkedList.h>




struct ServiceStruct {
  
  char* serviceId;
  char* serviceType;
  char* eventURL;
  char* controlURL;
  char* sid;

  // TBD XXX to replace by hashtable XXX
  LinkedList	variables;

  UpnpClient_Handle ctrlpt_handle;

  // TBD to implement
  struct Operations {

    void (*update_variable) (Service*, const char* name, const char* value);

  } *ops;
};




int
Service_SubscribeEventURL (Service* serv)
{
  int rc;
  if (serv == 0) {
    Log_Printf (LOG_ERROR, "Service_SubscribeEventURL NULL Service");
    rc = UPNP_E_INVALID_SERVICE;
  } else {
    Log_Printf (LOG_DEBUG, "Subscribing to EventURL %s", NN(serv->eventURL));
    int timeout = SERVICE_DEFAULT_TIMEOUT;    
    Upnp_SID sid;
    rc = UpnpSubscribe (serv->ctrlpt_handle, serv->eventURL, &timeout, sid);
    talloc_free (serv->sid);
    if ( rc == UPNP_E_SUCCESS ) {
      serv->sid = talloc_strdup (serv, sid);
      Log_Printf (LOG_DEBUG, "Subscribed to %s EventURL with SID=%s", 
		  talloc_get_name (serv), serv->sid);
    } else {
      serv->sid = 0;
      Log_Printf (LOG_ERROR, "Error Subscribing to %s EventURL -- %d", 
		  talloc_get_name (serv), rc);
    }
  }
  return rc;
}

int
Service_UnsubscribeEventURL (Service* serv)
{
  int rc;
  if (serv == 0) {
    Log_Printf (LOG_ERROR, "Service_UnsubscribeEventURL NULL Service");
    rc = UPNP_E_INVALID_SERVICE;
  } else {
    /*
     * If we have a valid control SID, then unsubscribe 
     */
    if (serv->sid == 0) {
      rc = UPNP_E_SUCCESS;
    } else {
      rc = UpnpUnSubscribe (serv->ctrlpt_handle, serv->sid);
      if ( UPNP_E_SUCCESS == rc ) {
	Log_Printf (LOG_DEBUG, "Unsubscribed from %s EventURL with SID=%s",
		    talloc_get_name (serv), serv->sid);
      } else {
	Log_Printf (LOG_ERROR, "Error unsubscribing to %s EventURL -- %d",
		    talloc_get_name (serv), rc);
      }
    }
  }
  return rc;
}


/******************************************************************************
 * destroy
 *
 * Description: 
 *	Service destructor, automatically called by "talloc_free".
 *
 *****************************************************************************/
static int
destroy (void* ptr)
{
  if (ptr) {
    Service* const serv = (Service*) ptr;

    /* If we have a valid control SID, then unsubscribe */
    Service_UnsubscribeEventURL (serv);
   
    /* Delete variable list.
     * Note that items are not destroyed : they are talloc'ed and
     * automatically deallocated when parent Service is detroyed.
     */
    ListDestroy (&serv->variables, /*freeItem=>*/ 0);

    // Reset all pointers to NULL 
    memset (serv, 0, sizeof(Service));
    
    // The "talloc'ed" strings will be deleted automatically 
  }
  return 0; // ok -> deallocate memory
}


/*****************************************************************************
 * Service_Create
 *****************************************************************************/
Service* 
Service_Create (void* context, 
		UpnpClient_Handle ctrlpt_handle, 
		IXML_Element* serviceDesc, 
		const char* base_url)
{
  Service* serv = talloc (context, Service);

  if (serv == 0) {
    Log_Print (LOG_ERROR, "Service_Create Out of Memory");
    return 0; // ---------->
  }

  *serv = (struct ServiceStruct) { }; // Initialize fields to empty values

  serv->ctrlpt_handle = ctrlpt_handle;

  const IXML_Node* const node = (IXML_Node*) serviceDesc;

  serv->serviceType = talloc_strdup (serv, XMLUtil_GetFirstNodeValue
				     (node, "serviceType"));
  Log_Printf (LOG_INFO, "Service_Create: %s", NN(serv->serviceType));

  serv->serviceId = talloc_strdup (serv, XMLUtil_GetFirstNodeValue
				   (node, "serviceId"));
  Log_Printf (LOG_DEBUG, "serviceId: %s", NN(serv->serviceId));

  if (serv->serviceId) {
    const char* const p = strrchr (serv->serviceId, ':');
    talloc_set_name (serv, "%s", (p ? p+1 : serv->serviceId));
  }

  char* relcontrolURL = XMLUtil_GetFirstNodeValue (node, "controlURL");
  UpnpUtil_ResolveURL (serv, base_url, relcontrolURL, &serv->controlURL);

  char* releventURL = XMLUtil_GetFirstNodeValue (node, "eventSubURL");
  UpnpUtil_ResolveURL (serv, base_url, releventURL, &serv->eventURL);
  
  // Subscribe to events 
  Service_SubscribeEventURL (serv);

  // Initialise list of variables
  ListInit (&serv->variables, 0, 0);

  // Register destructor
  talloc_set_destructor (serv, destroy);

  return serv;
}

/*****************************************************************************
 * Service_SetSid 
 *****************************************************************************/
int
Service_SetSid (Service* serv, Upnp_SID sid)
{
  int rc = UPNP_E_SUCCESS;
  
  if (serv == 0) {
    Log_Printf (LOG_ERROR, "Service_SetSid NULL Service");
    rc = UPNP_E_INVALID_SERVICE;
  } else {
    talloc_free (serv->sid);
    serv->sid = (sid ? talloc_strdup (serv, sid) : 0);
  }
  return rc;
}


/*****************************************************************************
 * GetVariable
 *****************************************************************************/
static ListNode*
GetVariable (const Service* serv, const char* name)
{
  if (serv && name) {
    ListNode* node;
    for (node = ListHead ((LinkedList*) &serv->variables);
	 node != 0;
	 node = ListNext ((LinkedList*) &serv->variables, node)) {
      StringPair* const var = node->item;
      if (var && var->name && strcmp (var->name, name) == 0) 
	return node; // ---------->
    }
  }
  return 0;
}

/*****************************************************************************
 * Service_UpdateState
 *****************************************************************************/
int
Service_UpdateState (Service* serv, IXML_Document* changedVariables)
{
  int rc = UPNP_E_SUCCESS;

  if (serv == 0) {
    Log_Printf (LOG_ERROR, "Service_UpdateState NULL Service");
    rc = UPNP_E_INVALID_SERVICE;
  } else {

    Log_Printf (LOG_DEBUG, "State Update for service %s", 
		talloc_get_name (serv));

    /*
     * Find all of the <e:property> tags in the document 
     */
    IXML_NodeList* properties = 
      ixmlDocument_getElementsByTagName (changedVariables, "e:property");
    
    if (properties) {
      int i, length = ixmlNodeList_length (properties);
      /*
       * Loop through each property change found 
       */
      for (i = 0; i < length; i++ ) { 
	IXML_Element* property = (IXML_Element *)
	  ixmlNodeList_item (properties, i);
	
	/*
	 * Loop through each element
	 */
	IXML_NodeList* variables =
	  ixmlElement_getElementsByTagName (property, "*");

	if (variables) {
	  int j, length1 = ixmlNodeList_length( variables );
	  for (j = 0; j < length1; j++ ) {
	    /*
	     * Extract the value, and update the state table
	     */
	    IXML_Element* variable = (IXML_Element *)
	      ixmlNodeList_item (variables, j);
	    const char* name = ixmlElement_getTagName (variable);
	    
	    if (name && strcmp (name, "e:property") != 0) {
	      const char* value = XMLUtil_GetElementValue (variable);
	      Log_Printf (LOG_DEBUG, "Variable Update '%s' = '%s'",
			  NN(name), NN(value));
	      
	      ListNode* node = GetVariable (serv, name);
	      StringPair* var;
	      if (node) {
		// Update existing node
		var = node->item;
		talloc_free (var->value);
		var->value = talloc_strdup (var, value);
	      } else {
		// New node
		var = talloc (serv, StringPair);
		var->name  = talloc_strdup (var, name);
		var->value = talloc_strdup (var, value);
		ListAddTail (&serv->variables, var);
	      }
	      if (serv->ops && serv->ops->update_variable)
		serv->ops->update_variable (serv, var->name, var->value);
	    }
	  }
	}
	ixmlNodeList_free (variables);
	variables = NULL;
      }
    }
    ixmlNodeList_free (properties);
    properties = NULL;
  }
  return rc;
}


/*****************************************************************************
 * MakeAction
 *****************************************************************************/
static IXML_Document*
MakeAction (const char* actionName, const char* serviceType,
	    int nb_params, const StringPair* params)
{
  IXML_Document* res = NULL;
    
  if (nb_params > 0 && params == NULL) 
    return 0; // ---------->

  int i;
  for (i = 0; i < nb_params; i++) {
    int rc = UpnpAddToAction (&res, actionName, serviceType, 
			      params[i].name, params[i].value);
    if (rc != UPNP_E_SUCCESS) {
      Log_Printf (LOG_ERROR, 
		  "Service MakeAction: can't add action %s=%s", 
		  NN(params[i].name), NN(params[i].value) );   
      if (res) 
	ixmlDocument_free (res);
      return 0; // ----------> 
    }
  }
     
  // Test if no parameters
  if (res == NULL) {
    res = UpnpMakeAction (actionName, serviceType, 0, NULL);
  } 
  return res;
}


/*****************************************************************************
 * Service_SendActionAsync
 *****************************************************************************/
int
Service_SendActionAsync (const Service* serv,
			 Upnp_FunPtr callback,
			 const char* actionName,
			 int nb_params, const StringPair* params)
{
  int rc = UPNP_E_SUCCESS;
  Log_Printf (LOG_DEBUG, "Service_SendActionAsync '%s'", NN(actionName));
  
  if (serv == 0) {
    Log_Printf (LOG_ERROR, "Service_SendActionAsync NULL Service");
    rc = UPNP_E_INVALID_SERVICE;
  } else {

    IXML_Document* actionNode = MakeAction (actionName, serv->serviceType, 
					    nb_params, params);
    if (actionNode == 0) {
      rc = UPNP_E_INVALID_PARAM;
    } else {
      // Send action request
      rc = UpnpSendActionAsync (serv->ctrlpt_handle, serv->controlURL,
				serv->serviceType, NULL, actionNode,
				callback, /* cookie => */ serv);
      if (rc != UPNP_E_SUCCESS) 
	Log_Printf (LOG_ERROR, "Error in UpnpSendActionAsync -- %d", rc);
      
      ixmlDocument_free (actionNode);
      actionNode = 0;
    }
  }
  return rc;
}


/*****************************************************************************
 * Service_SendAction
 *****************************************************************************/
int
Service_SendAction (const Service* serv,
		    const char* actionName,
		    int nb_params, const StringPair* params,
		    IXML_Document** response)
{
  int rc = UPNP_E_SUCCESS;
  Log_Printf (LOG_DEBUG, "Service_SendAction '%s'", NN(actionName));
  
  if (serv == NULL) {
    Log_Printf (LOG_ERROR, "Service_SendAction NULL Service");
    rc = UPNP_E_INVALID_SERVICE;
  } else if (response == NULL) {
    Log_Printf (LOG_ERROR, "Service_SendAction NULL response argument");
    rc = UPNP_E_INVALID_PARAM;
  } else {

    IXML_Document* actionNode = MakeAction (actionName, serv->serviceType, 
					    nb_params, params);
    if (actionNode == NULL) {
      rc = UPNP_E_INVALID_PARAM;
    } else {
      // Send action request
      *response = NULL;
      rc = UpnpSendAction (serv->ctrlpt_handle, serv->controlURL,
			   serv->serviceType, NULL, actionNode,
			   response);
      if (rc != UPNP_E_SUCCESS) {
	Log_Printf (LOG_ERROR, "Error in UpnpSendAction -- %d (%s)", rc,
		    UpnpGetErrorMessage (rc));
	if (*response) { 
	  // rc > 0 : SOAP-protocol error
	  Log_Printf 
	    (LOG_ERROR, 
	     "Error in UpnpSendAction -- SOAP error %s (%s)",
	     XMLUtil_GetFirstNodeValue ((IXML_Node*) *response, "errorCode"),
	     XMLUtil_GetFirstNodeValue ((IXML_Node*) *response, 
					"errorDescription"));
	  ixmlDocument_free (*response);
	  *response = NULL;
	}
      }
      ixmlDocument_free (actionNode);
      actionNode = NULL;
    }
  }
  return rc;
}



/*****************************************************************************
 * Service_GetStatusString
 *****************************************************************************/
char*
Service_GetStatusString (const Service* serv, const char* spacer) 
{
  if (serv == 0)
    return 0; // ----------> 
  
  if (spacer == 0)
    spacer = "";
  
  char* p = talloc_strdup (serv, "");
  
#define P talloc_asprintf_append 
  p=P(p, "%s    +- Internal Name   = %s\n", spacer, talloc_get_name(serv));
  p=P(p, "%s    +- ServiceId       = %s\n", spacer, NN(serv->serviceId));
  p=P(p, "%s    +- ServiceType     = %s\n", spacer, NN(serv->serviceType));
  p=P(p, "%s    +- EventURL        = %s\n", spacer, NN(serv->eventURL));
  p=P(p, "%s    +- ControlURL      = %s\n", spacer, NN(serv->controlURL));
  p=P(p, "%s    +- SID             = %s\n", spacer, NN(serv->sid));
  p=P(p, "%s    +- ServiceStateTable\n", spacer);


  // Print variables
  ListNode* node;
  for (node = ListHead ((LinkedList*) &serv->variables);
       node != 0;
       node = ListNext ((LinkedList*) &serv->variables, node)) {
    StringPair* const var = node->item;
    p=P(p, "%s         +- %-10s = %.150s%s\n", spacer, 
	NN(var->name), NN(var->value), 
	(var->value && strlen(var->value) > 150) ? "..." : "");
  }

#undef P
  return p;
}


/*****************************************************************************
 * Service Accessors
 *****************************************************************************/

const char*
Service_GetSid (const Service* serv)
{
  return (serv ? serv->sid : NULL);
}

const char*
Service_GetEventURL (const Service* serv)
{
  return (serv ? serv->eventURL : NULL);
}

const char*
Service_GetControlURL (const Service* serv)
{
  return (serv ? serv->controlURL : NULL);
}
