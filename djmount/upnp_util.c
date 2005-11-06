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

#define P p=talloc_asprintf_append 

  P(p, "\n\n======================================================================\n");
  P(p, "----------------------------------------------------------------------\n");

  const char* s = UpnpUtil_GetEventTypeString (eventType);
  if (s)
    P(p, "%s\n", s);
  else
    P(p, "** unknown Upnp_EventType %d **\n", (int) eventType);
  
  if (event == 0) {
    P(p, "(NULL EVENT BODY)\n");
  } else {
  
    switch (eventType) {
      
      /*
       * SSDP 
       */
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
      {
	const struct Upnp_Discovery* const d_event =
	  (struct Upnp_Discovery *) event;
	
	P(p, "ErrCode     =  %d\n", d_event->ErrCode);
	P(p, "Expires     =  %d\n", d_event->Expires);
	P(p, "DeviceId    =  %s\n", NN(d_event->DeviceId));
	P(p, "DeviceType  =  %s\n", NN(d_event->DeviceType));
	P(p, "ServiceType =  %s\n", NN(d_event->ServiceType));
	P(p, "ServiceVer  =  %s\n", NN(d_event->ServiceVer));
	P(p, "Location    =  %s\n", NN(d_event->Location));
	P(p, "OS          =  %s\n", NN(d_event->Os));
	P(p, "Date        =  %s\n", NN(d_event->Date));
	P(p, "Ext         =  %s\n", NN(d_event->Ext));
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
	const struct Upnp_Action_Request* const a_event =
	  (struct Upnp_Action_Request *) event;
	
	P(p, "ErrCode     =  %d\n", a_event->ErrCode);
	P(p, "ErrStr      =  %s\n", NN(a_event->ErrStr));
	P(p, "ActionName  =  %s\n", NN(a_event->ActionName));
	P(p, "DevUDN      =  %s\n", NN(a_event->DevUDN));
	P(p, "ServiceID   =  %s\n", NN(a_event->ServiceID));
	P(p, "ActRequest  =  %s\n", 
	  XMLUtil_GetDocumentString (tmp_ctx, a_event->ActionRequest));
	P(p, "ActResult   =  %s\n",
	  XMLUtil_GetDocumentString (tmp_ctx, a_event->ActionResult));
      }
      break;
      
    case UPNP_CONTROL_ACTION_COMPLETE:
      {
	const struct Upnp_Action_Complete* const a_event =
	  (struct Upnp_Action_Complete *) event;
	
	P(p, "ErrCode     =  %d\n", a_event->ErrCode);
	P(p, "CtrlUrl     =  %s\n", NN(a_event->CtrlUrl));
	P(p, "ActRequest  =  %s\n",
	  XMLUtil_GetDocumentString (tmp_ctx, a_event->ActionRequest));
	P(p, "ActResult   =  %s\n", 
	  XMLUtil_GetDocumentString (tmp_ctx, a_event->ActionResult));
      }
      break;
      
    case UPNP_CONTROL_GET_VAR_REQUEST:
      {
	const struct Upnp_State_Var_Request* const sv_event =
	  (struct Upnp_State_Var_Request *) event;
	
	P(p, "ErrCode     =  %d\n", sv_event->ErrCode);
	P(p, "ErrStr      =  %s\n", NN(sv_event->ErrStr));
	P(p, "DevUDN      =  %s\n", NN(sv_event->DevUDN));
	P(p, "ServiceID   =  %s\n", NN(sv_event->ServiceID));
	P(p, "StateVarName=  %s\n", NN(sv_event->StateVarName));
	P(p, "CurrentVal  =  %s\n", NN(sv_event->CurrentVal));
      }
      break;
      
    case UPNP_CONTROL_GET_VAR_COMPLETE:
      {
	const struct Upnp_State_Var_Complete* const sv_event =
	  (struct Upnp_State_Var_Complete *) event;
	
	P(p, "ErrCode     =  %d\n", sv_event->ErrCode);
	P(p, "CtrlUrl     =  %s\n", NN(sv_event->CtrlUrl));
	P(p, "StateVarName=  %s\n", NN(sv_event->StateVarName));
	P(p, "CurrentVal  =  %s\n", NN(sv_event->CurrentVal));
      }
      break;
      
      /*
       * GENA 
       */
    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
      {
	const struct Upnp_Subscription_Request* const sr_event =
	  (struct Upnp_Subscription_Request *) event;
	
	P(p, "ServiceID   =  %s\n", NN(sr_event->ServiceId));
	P(p, "UDN         =  %s\n", NN(sr_event->UDN));
	P(p, "SID         =  %s\n", NN(sr_event->Sid));
      }
      break;
      
    case UPNP_EVENT_RECEIVED:
      {
	const struct Upnp_Event* const e_event = (struct Upnp_Event *) event;
	
	P(p, "SID         =  %s\n", NN(e_event->Sid));
	P(p, "EventKey    =  %d\n", e_event->EventKey);
	P(p, "ChangedVars =  %s\n", 
	  XMLUtil_GetDocumentString (tmp_ctx, e_event->ChangedVariables));
      }
      break;
      
    case UPNP_EVENT_RENEWAL_COMPLETE:
      {
	const struct Upnp_Event_Subscribe* const es_event =
	  (struct Upnp_Event_Subscribe *) event;
	
	P(p, "ErrCode     =  %d\n", es_event->ErrCode);
	P(p, "SID         =  %s\n", NN(es_event->Sid));
	P(p, "TimeOut     =  %d\n", es_event->TimeOut);
      }
      break;
      
    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
    case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
      {
	const struct Upnp_Event_Subscribe* const es_event =
	  (struct Upnp_Event_Subscribe *) event;
	
	P(p, "ErrCode     =  %d\n", es_event->ErrCode);
	P(p, "SID         =  %s\n", NN(es_event->Sid));
	P(p, "PublisherURL=  %s\n", NN(es_event->PublisherUrl));
	P(p, "TimeOut     =  %d\n", es_event->TimeOut);
      }
      break;
      
    case UPNP_EVENT_AUTORENEWAL_FAILED:
    case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
      {
	const struct Upnp_Event_Subscribe* const es_event =
	  (struct Upnp_Event_Subscribe *) event;
	
	P(p, "ErrCode     =  %d\n", es_event->ErrCode);
	P(p, "SID         =  %s\n", NN(es_event->Sid));
	P(p, "PublisherURL=  %s\n", NN(es_event->PublisherUrl));
	P(p, "TimeOut     =  %d\n", es_event->TimeOut);
      }
      break;
      
    }
  }

  P(p, "----------------------------------------------------------------------\n");
  P(p, "======================================================================\n\n");
  
#undef P

  // Delete all temporary strings
  talloc_free (tmp_ctx);

  return p;
}



/*****************************************************************************
 * UpnpUtil_PrintEvent
 *****************************************************************************/
void
UpnpUtil_PrintEvent (Log_Level level,
		     IN Upnp_EventType eventType,
		     IN const void* event )
{
  char* const s = UpnpUtil_GetEventString (NULL, eventType, event);
  Log_Print (level, s);
  talloc_free (s);
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

