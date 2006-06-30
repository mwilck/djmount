/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "service.h"

#include <string.h>

#include "log.h"
#include "xml_util.h"
#include "upnp_util.h"
#include "talloc_util.h"

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include "service_p.h"


/** Default timeout to request during subscriptions */
#define SUBSCRIBE_DEFAULT_TIMEOUT 	1801


// Some reasonable number for number of vararg parameters to SendAction
#define MAX_VA_PARAMS	64


/******************************************************************************
 * Service_SubscribeEventURL
 *****************************************************************************/
int
Service_SubscribeEventURL (Service* serv)
{
	int rc;
	if (serv == NULL) {
		Log_Printf (LOG_ERROR, 
			    "Service_SubscribeEventURL NULL Service");
		rc = UPNP_E_INVALID_SERVICE;
	} else {
		Log_Printf (LOG_DEBUG, "Subscribing to EventURL %s", 
			    NN(serv->eventURL));
		int timeout = SUBSCRIBE_DEFAULT_TIMEOUT;    
		Upnp_SID sid;
		rc = UpnpSubscribe (serv->ctrlpt_handle, serv->eventURL, 
				    &timeout, sid);
		talloc_free (serv->sid);
		if ( rc == UPNP_E_SUCCESS ) {
			serv->sid = talloc_strdup (serv, sid);
			Log_Printf (LOG_DEBUG, 
				    "Subscribed to %s EventURL with SID=%s", 
				    talloc_get_name (serv), serv->sid);
		} else {
			serv->sid = NULL;
			Log_Printf (LOG_ERROR, 
				    "Error Subscribing to %s EventURL -- %d", 
				    talloc_get_name (serv), rc);
		}
	}
	return rc;
}


/******************************************************************************
 * Service_UnsubscribeEventURL
 *****************************************************************************/
int
Service_UnsubscribeEventURL (Service* serv)
{
	int rc;
	if (serv == NULL) {
		Log_Printf (LOG_ERROR, 
			    "Service_UnsubscribeEventURL NULL Service");
		rc = UPNP_E_INVALID_SERVICE;
	} else if (serv->sid == NULL) {
		rc = UPNP_E_SUCCESS;
	} else {
		/*
		 * If we have a valid control SID, then unsubscribe 
		 */
		rc = UpnpUnSubscribe (serv->ctrlpt_handle, 
				      serv->sid);
		if ( UPNP_E_SUCCESS == rc ) {
			Log_Printf(LOG_DEBUG, 
				   "Unsubscribed from %s EventURL with SID=%s",
				   talloc_get_name (serv), serv->sid);
		} else {
			Log_Printf (LOG_ERROR, 
				    "Error unsubscribing to %s EventURL -- %d",
				    talloc_get_name (serv), rc);
		}
	}
	return rc;
}


/******************************************************************************
 * finalize
 *
 * Description: 
 *	Service destructor
 *
 *****************************************************************************/
static void
finalize (Object* obj)
{
	Service* const serv = (Service*) obj;

	if (serv) {
		// If we have a valid control SID, then unsubscribe 
		if (serv->ctrlpt_handle)
			Service_UnsubscribeEventURL (serv);
    
		/* Delete variable list.
		 * Note that items are not destroyed : they are talloc'ed and
		 * automatically deallocated when parent Service is detroyed.
		 */
		ListDestroy (&serv->variables, /*freeItem=>*/ 0);
		
		// The "talloc'ed" strings will be deleted automatically : 
		// nothing to do 
	}
}


/*****************************************************************************
 * Service_SetSid 
 *****************************************************************************/
int
Service_SetSid (Service* serv, Upnp_SID sid)
{
	int rc = UPNP_E_SUCCESS;
	
	if (serv == NULL) {
		Log_Printf (LOG_ERROR, "Service_SetSid NULL Service");
		rc = UPNP_E_INVALID_SERVICE;
	} else {
		talloc_free (serv->sid);
		serv->sid = (sid ? talloc_strdup (serv, sid) : NULL);
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
	 node != NULL;
	 node = ListNext ((LinkedList*) &serv->variables, node)) {
      StringPair* const var = node->item;
      if (var && var->name && strcmp (var->name, name) == 0) 
	return node; // ---------->
    }
  }
  return NULL;
}

/*****************************************************************************
 * Service_UpdateState
 *****************************************************************************/
int
Service_UpdateState (Service* serv, IXML_Document* changedVariables)
{
  int rc = UPNP_E_SUCCESS;

  if (serv == NULL) {
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
	      if (OBJECT_METHOD (serv,update_variable))
		OBJECT_METHOD (serv, update_variable) (serv, 
						       var->name, var->value);
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
    return NULL; // ---------->

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
      return NULL; // ----------> 
    }
  }
     
  // Test if no parameters
  if (res == NULL) {
    res = UpnpMakeAction (actionName, serviceType, 0, NULL);
  } 
  return res;
}

/*****************************************************************************
 * ActionError
 *****************************************************************************/
static void
ActionError (Service* serv, const char* actionName,
	     int rc, IXML_Document** response)
{
	talloc_free (serv->la_name);
	serv->la_name   = talloc_strdup (serv, actionName);
	serv->la_result = rc;

	talloc_free (serv->la_error_code);
	talloc_free (serv->la_error_desc);
	serv->la_error_code = serv->la_error_desc = NULL;
	
	if (rc != UPNP_E_SUCCESS) {
		Log_Printf (LOG_ERROR, 
			    "Error in UpnpSendAction '%s' -- %d (%s)", 
			    actionName, rc, UpnpGetErrorMessage (rc));
		if (response && *response) { 
			DOMString s = ixmlDocumenttoString (*response);
			Log_Printf (LOG_DEBUG, 
				    "Error in UpnpSendAction, response = %s", 
				    s);
			ixmlFreeDOMString (s);
			// rc > 0 : SOAP-protocol error
			serv->la_error_code = talloc_strdup 
				(serv, XMLUtil_GetFirstNodeValue 
				 ((IXML_Node*) *response, "errorCode", true));
			serv->la_error_desc = talloc_strdup 
				(serv, XMLUtil_GetFirstNodeValue 
				 ((IXML_Node*) *response, "errorDescription",
				  true));
			Log_Printf (LOG_ERROR, 
				    "Error SOAP in UpnpSendAction -- %s (%s)",
				    serv->la_error_code, 
				    serv->la_error_desc);
			ixmlDocument_free (*response);
			*response = NULL;
		}
	}

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
  
  if (serv == NULL) {
    Log_Printf (LOG_ERROR, "Service_SendActionAsync NULL Service");
    rc = UPNP_E_INVALID_SERVICE;
  } else {

    IXML_Document* actionNode = MakeAction (actionName, serv->serviceType, 
					    nb_params, params);
    if (actionNode == NULL) {
      rc = UPNP_E_INVALID_PARAM;
    } else {
      // Send action request
      rc = UpnpSendActionAsync (serv->ctrlpt_handle, serv->controlURL,
				serv->serviceType, NULL, actionNode,
				callback, /* cookie => */ serv);
      if (rc != UPNP_E_SUCCESS) 
	Log_Printf (LOG_ERROR, "Error in UpnpSendActionAsync -- %d", rc);
      
      ixmlDocument_free (actionNode);
      actionNode = NULL;
    }
  }
  return rc;
}

/*****************************************************************************
 * Service_SendActionAsyncVa
 *****************************************************************************/
int	
Service_SendActionAsyncVa (const Service* serv,
			   Upnp_FunPtr callback,
			   const char* actionName, ...)
{
  // Get names+values
  StringPair params [MAX_VA_PARAMS];
  va_list ap;
  va_start (ap, actionName);
  int nb = 0;
  while ( (params[nb].name = va_arg (ap, char*)) && (nb < MAX_VA_PARAMS) ) {
    params[nb].value = va_arg (ap, char*); // TBD should be "const char*"
    nb++;
  }
  va_end (ap);
  Log_Printf (LOG_DEBUG, "Service_SendActionAsyncVa : %d pairs found", nb);
  
  return Service_SendActionAsync (serv, callback, actionName, nb, params);
}


/*****************************************************************************
 * Service_SendAction
 *****************************************************************************/
int
Service_SendAction (Service* serv,
		    IXML_Document** response,
		    const char* actionName,
		    int nb_params, const StringPair* params)
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
      ActionError (serv, actionName, rc, response);
      ixmlDocument_free (actionNode);
      actionNode = NULL;
    }
  }
  return rc;
}


/*****************************************************************************
 * Service_SendActionVa
 *****************************************************************************/
int
Service_SendActionVa (Service* serv,
		      IXML_Document** response,
		      const char* actionName, ...)
{
  // Get names+values
  StringPair params [MAX_VA_PARAMS];
  va_list ap;
  va_start (ap, actionName);
  int nb = 0;
  while ( (params[nb].name = va_arg (ap, char*)) && (nb < MAX_VA_PARAMS) ) {
    params[nb].value = va_arg (ap, char*); // TBD should be "const char*"
    nb++;
  }
  va_end (ap);
  Log_Printf (LOG_DEBUG, "Service_SendActionVa : %d pairs found", nb);
  
  return Service_SendAction (serv, response, actionName, nb, params);
}


/*****************************************************************************
 * Service_GetStatusString
 *****************************************************************************/
static char*
get_status_string (const Service* serv, 
		   void* result_context, bool debug, const char* spacer) 
{
	if (spacer == NULL)
		spacer = "";
	
	char* p = talloc_strdup (result_context, "");
	
	tpr (&p, "%s| \n", spacer);
	const char* const n = OBJECT_GET_CLASS_NAME(serv);
	tpr (&p, "%s+- Class           = %s\n", spacer, 
	     (n ? n : "**ERROR** NO CLASS"));
	tpr (&p, "%s+- Object Name     = %s\n", spacer, talloc_get_name(serv));
	tpr (&p, "%s+- ServiceType     = %s\n", spacer, NN(serv->serviceType));
	tpr (&p, "%s+- ServiceId       = %s\n", spacer, NN(serv->serviceId));
	tpr (&p, "%s+- EventURL        = %s\n", spacer, NN(serv->eventURL));
	tpr (&p, "%s+- ControlURL      = %s\n", spacer, NN(serv->controlURL));
	
	// Print variables
	tpr (&p, "%s+- ServiceStateTable\n", spacer);
	ListNode* node;
	for (node = ListHead ((LinkedList*) &serv->variables);
	     node != NULL;
	     node = ListNext ((LinkedList*) &serv->variables, node)) {
		StringPair* const var = node->item;
		tpr (&p, "%s|    +- %-10s = %.150s%s\n", spacer, 
		     NN(var->name), NN(var->value), 
		     (var->value && strlen(var->value) > 150) ? "..." : "");
	}
	
	// Last Action
	tpr (&p, "%s+- Last Action     = %s\n", spacer, NN(serv->la_name));
	if (serv->la_name) 
		tpr (&p, "%s|    +- Result     = %d (%s)\n", spacer, 
		     serv->la_result, UpnpGetErrorMessage (serv->la_result));
	if (serv->la_error_code || serv->la_error_desc) 
		tpr (&p, "%s|    +- SOAP Error = %s (%s)\n", spacer, 
		     NN(serv->la_error_code), NN(serv->la_error_desc));
	
	tpr (&p, "%s+- SID             = %s\n", spacer, NN(serv->sid));
	
	return p;
}

char*
Service_GetStatusString (const Service* serv,  
			 void* result_context, bool debug, const char* spacer) 
{
	if (OBJECT_METHOD (serv, get_status_string))
		return OBJECT_METHOD (serv, get_status_string) 
			(serv, result_context, debug, spacer);
	return NULL;
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

const char*
Service_GetServiceType (const Service* serv)
{
  return (serv ? serv->serviceType : NULL);
}


/*****************************************************************************
 * OBJECT_INIT_CLASS
 *****************************************************************************/

// Initialize class methods
static void 
init_class (Service_Class* const isa) 
{ 
	CLASS_BASE_CAST(isa)->finalize = finalize;
	isa->update_variable   = NULL;
	isa->get_status_string = get_status_string;
}

OBJECT_INIT_CLASS(Service, Object, init_class);


/*****************************************************************************
 * Service_Create
 *****************************************************************************/
Service*
Service_Create (void* talloc_context, 
		UpnpClient_Handle ctrlpt_handle, 
		IXML_Element* serviceDesc, 
		const char* base_url)
{
	OBJECT_SUPER_CONSTRUCT (Service, Object_Create, talloc_context, NULL);
	if (self == NULL)
		return NULL; // ---------->
	
	self->ctrlpt_handle = ctrlpt_handle;
	
	const IXML_Node* const node = (IXML_Node*) serviceDesc;

	self->serviceType = talloc_strdup (self, XMLUtil_GetFirstNodeValue
					   (node, "serviceType", true));
	Log_Printf (LOG_INFO, "Service_Create: %s", NN(self->serviceType));
	
	self->serviceId = talloc_strdup (self, XMLUtil_GetFirstNodeValue
					 (node, "serviceId", true));
	Log_Printf (LOG_DEBUG, "serviceId: %s", NN(self->serviceId));
	
	char* relcontrolURL = XMLUtil_GetFirstNodeValue (node, "controlURL", 
							 true);
	UpnpUtil_ResolveURL (self, base_url, relcontrolURL, &self->controlURL);
	
	char* releventURL = XMLUtil_GetFirstNodeValue (node, "eventSubURL", 
						       true);
	UpnpUtil_ResolveURL (self, base_url, releventURL, &self->eventURL);
	
	self->sid = NULL;
	
	// Initialise list of variables
	ListInit (&self->variables, 0, 0);
	
	// For debugging
	self->la_name = self->la_error_code = self->la_error_desc = NULL;
	self->la_result = UPNP_E_SUCCESS;

	return self; // ---------->
}


