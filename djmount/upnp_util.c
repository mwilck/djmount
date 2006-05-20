/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * UPnP Utilities
 * This file is part of djmount.
 *
 * (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
 *
 * Part derived from libupnp example (upnp/sample/common/sample_util.c)
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

#include "upnp_util.h"
#include "log.h"
#include "xml_util.h"
#include "talloc_util.h"




/*****************************************************************************
 * UpnpUtil_GetEventTypeString
 *****************************************************************************/
const char*
UpnpUtil_GetEventTypeString (IN Upnp_EventType e)
{
  const char* s = 0;

  switch (e) {
#define CASE(X)		case X: s = #X; break

    CASE(UPNP_DISCOVERY_ADVERTISEMENT_ALIVE);
    CASE(UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE);
    CASE(UPNP_DISCOVERY_SEARCH_RESULT);
    CASE(UPNP_DISCOVERY_SEARCH_TIMEOUT);
    /*
     * SOAP Stuff 
     */
    CASE(UPNP_CONTROL_ACTION_REQUEST);
    CASE(UPNP_CONTROL_ACTION_COMPLETE);
    CASE(UPNP_CONTROL_GET_VAR_REQUEST);
    CASE(UPNP_CONTROL_GET_VAR_COMPLETE);
    /*
     * GENA Stuff 
     */
    CASE(UPNP_EVENT_SUBSCRIPTION_REQUEST);
    CASE(UPNP_EVENT_RECEIVED);
    CASE(UPNP_EVENT_RENEWAL_COMPLETE);
    CASE(UPNP_EVENT_SUBSCRIBE_COMPLETE);
    CASE(UPNP_EVENT_UNSUBSCRIBE_COMPLETE);
    CASE(UPNP_EVENT_AUTORENEWAL_FAILED);
    CASE(UPNP_EVENT_SUBSCRIPTION_EXPIRED);

#undef CASE
  }
  return s;
}



/*****************************************************************************
 * UpnpUtil_GetEventString
 *****************************************************************************/
char*
UpnpUtil_GetEventString (void* talloc_context,
			 IN Upnp_EventType eventType, 
			 IN const void* event)
{
	char* p = talloc_strdup (talloc_context, "");
	
	// Create a working context for temporary strings
	void* const tmp_ctx = talloc_new (p);

	tpr (&p, "\n\n======================================================================\n");
	
	const char* s = UpnpUtil_GetEventTypeString (eventType);
	if (s)
		tpr (&p, "%s\n", s);
	else
		tpr (&p, "** unknown Upnp_EventType %d **\n", (int) eventType);
	
	if (event == NULL) {
		tpr (&p, "(NULL EVENT BODY)\n");
	} else {
		
		switch (eventType) {
			
			/*
			 * SSDP 
			 */
		case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
		case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
		case UPNP_DISCOVERY_SEARCH_RESULT:
		{
			const struct Upnp_Discovery* const e =
				(struct Upnp_Discovery*) event;
			
			tpr (&p, "ErrCode     =  %d\n", e->ErrCode);
			tpr (&p, "Expires     =  %d\n", e->Expires);
			tpr (&p, "DeviceId    =  %s\n", NN(e->DeviceId));
			tpr (&p, "DeviceType  =  %s\n", NN(e->DeviceType));
			tpr (&p, "ServiceType =  %s\n", NN(e->ServiceType));
			tpr (&p, "ServiceVer  =  %s\n", NN(e->ServiceVer));
			tpr (&p, "Location    =  %s\n", NN(e->Location));
			tpr (&p, "OS          =  %s\n", NN(e->Os));
			tpr (&p, "Date        =  %s\n", NN(e->Date));
			tpr (&p, "Ext         =  %s\n", NN(e->Ext));
		}
		break;
		
		case UPNP_DISCOVERY_SEARCH_TIMEOUT:
			// Nothing to print out here
			break;
			
			/*
			 * SOAP 
			 */
		case UPNP_CONTROL_ACTION_REQUEST:
		{
			const struct Upnp_Action_Request* const e =
				(struct Upnp_Action_Request*) event;
	
			tpr (&p, "ErrCode     =  %d\n", e->ErrCode);
			tpr (&p, "ErrStr      =  %s\n", NN(e->ErrStr));
			tpr (&p, "ActionName  =  %s\n", NN(e->ActionName));
			tpr (&p, "DevUDN      =  %s\n", NN(e->DevUDN));
			tpr (&p, "ServiceID   =  %s\n", NN(e->ServiceID));
			tpr (&p, "ActRequest  =  %s\n", 
			     XMLUtil_GetDocumentString (tmp_ctx, 
							e->ActionRequest));
			tpr (&p, "ActResult   =  %s\n",
			     XMLUtil_GetDocumentString (tmp_ctx, 
							e->ActionResult));
		}
		break;
		
		case UPNP_CONTROL_ACTION_COMPLETE:
		{
			const struct Upnp_Action_Complete* const e =
				(struct Upnp_Action_Complete*) event;
			
			tpr (&p, "ErrCode     =  %d\n", e->ErrCode);
			tpr (&p, "CtrlUrl     =  %s\n", NN(e->CtrlUrl));
			tpr (&p, "ActRequest  =  %s\n",
			     XMLUtil_GetDocumentString (tmp_ctx, 
							e->ActionRequest));
			tpr (&p, "ActResult   =  %s\n", 
			     XMLUtil_GetDocumentString (tmp_ctx, 
							e->ActionResult));
		}
		break;
		
		case UPNP_CONTROL_GET_VAR_REQUEST:
		{
			const struct Upnp_State_Var_Request* const e =
				(struct Upnp_State_Var_Request*) event;
			
			tpr (&p, "ErrCode     =  %d\n", e->ErrCode);
			tpr (&p, "ErrStr      =  %s\n", NN(e->ErrStr));
			tpr (&p, "DevUDN      =  %s\n", NN(e->DevUDN));
			tpr (&p, "ServiceID   =  %s\n", NN(e->ServiceID));
			tpr (&p, "StateVarName=  %s\n", NN(e->StateVarName));
			tpr (&p, "CurrentVal  =  %s\n", NN(e->CurrentVal));
		}
		break;
		
		case UPNP_CONTROL_GET_VAR_COMPLETE:
		{
			const struct Upnp_State_Var_Complete* const e =
				(struct Upnp_State_Var_Complete*) event;
			
			tpr (&p, "ErrCode     =  %d\n", e->ErrCode);
			tpr (&p, "CtrlUrl     =  %s\n", NN(e->CtrlUrl));
			tpr (&p, "StateVarName=  %s\n", NN(e->StateVarName));
			tpr (&p, "CurrentVal  =  %s\n", NN(e->CurrentVal));
		}
		break;
		
		/*
		 * GENA 
		 */
		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
		{
			const struct Upnp_Subscription_Request* const e =
				(struct Upnp_Subscription_Request*) event;
			
			tpr (&p, "ServiceID   =  %s\n", NN(e->ServiceId));
			tpr (&p, "UDN         =  %s\n", NN(e->UDN));
			tpr (&p, "SID         =  %s\n", NN(e->Sid));
		}
		break;
		
		case UPNP_EVENT_RECEIVED:
		{
			const struct Upnp_Event* const e = 
				(struct Upnp_Event*) event;
			
			tpr (&p, "SID         =  %s\n", NN(e->Sid));
			tpr (&p, "EventKey    =  %d\n", e->EventKey);
			tpr (&p, "ChangedVars =  %s\n",
			     XMLUtil_GetDocumentString (tmp_ctx, 
							e->ChangedVariables));
		}
		break;
		
		case UPNP_EVENT_RENEWAL_COMPLETE:
		{
			const struct Upnp_Event_Subscribe* const e =
				(struct Upnp_Event_Subscribe*) event;
			
			tpr (&p, "ErrCode     =  %d\n", e->ErrCode);
			tpr (&p, "SID         =  %s\n", NN(e->Sid));
			tpr (&p, "TimeOut     =  %d\n", e->TimeOut);
		}
		break;
		
		case UPNP_EVENT_SUBSCRIBE_COMPLETE:
		case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
		{
			const struct Upnp_Event_Subscribe* const e =
				(struct Upnp_Event_Subscribe*) event;
			
			tpr (&p, "ErrCode     =  %d\n", e->ErrCode);
			tpr (&p, "SID         =  %s\n", NN(e->Sid));
			tpr (&p, "PublisherURL=  %s\n", NN(e->PublisherUrl));
			tpr (&p, "TimeOut     =  %d\n", e->TimeOut);
		}
		break;
		
		case UPNP_EVENT_AUTORENEWAL_FAILED:
		case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
		{
			const struct Upnp_Event_Subscribe* const e =
				(struct Upnp_Event_Subscribe*) event;
			
			tpr (&p, "ErrCode     =  %d\n", e->ErrCode);
			tpr (&p, "SID         =  %s\n", NN(e->Sid));
			tpr (&p, "PublisherURL=  %s\n", NN(e->PublisherUrl));
			tpr (&p, "TimeOut     =  %d\n", e->TimeOut);
		}
		break;
		
		}
	}
	
	tpr (&p, "======================================================================\n\n");
	
	// Delete all temporary strings
	talloc_free (tmp_ctx);
	
	return p;
}


/******************************************************************************
 * UpnpUtil_ResolveURL
 *****************************************************************************/
int
UpnpUtil_ResolveURL (void* talloc_context, 
		     IN const char* base, IN const char* rel,
		     OUT char** resolved)
{
  int rc = UPNP_E_SUCCESS;
  *resolved = (char*) talloc_size
    (talloc_context, (base ? strlen(base) : 0) + (rel ? strlen(rel) : 0) + 1);
  
  if( *resolved == 0 ) {
    rc = UPNP_E_OUTOF_MEMORY;
  } else {
    rc = UpnpResolveURL (base, rel, *resolved);
    if( rc != UPNP_E_SUCCESS ) {
      Log_Printf (LOG_ERROR, "Error generating URL from %s + %s",
		  NN(base), NN(rel));
      (*resolved)[0] = '\0';
    }
  }
  return rc;
}

