/* $Id$
 *
 * UPnP Service implementation (private / protected).
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

#ifndef SERVICE_P_INCLUDED
#define SERVICE_P_INCLUDED 1


#include "service.h"
#include "object_p.h"

#include <upnp/LinkedList.h>


/******************************************************************************
 *
 *	Service private / protected implementation ; do not include directly.
 *
 *****************************************************************************/

union _ServiceClass {

  ObjectClass o;

  struct {
    // Inherit parent fields
    ObjectClass _;

    // Addition Virtual methods
    void  (*update_variable) (Service*, const char* name, const char* value);
    char* (*get_status_string) (const Service* serv, void* result_context, 
				const char* spacer1, const char* spacer);
  } m;
  
};



union _Service {

  const ServiceClass* isa;

  struct {
    Object _;

    char* serviceId;
    char* serviceType;
    char* eventURL;
    char* controlURL;
    char* sid;
    
    // TBD XXX to replace by hashtable XXX
    LinkedList	variables;
    
    UpnpClient_Handle ctrlpt_handle;
  } m;
};


/******************************************************************************
 *
 * 	Initialize a Service object (must be created beforehand
 *	with _Object_talloc). Return 0 if ok, or error code.
 *
 *****************************************************************************/

int
_Service_Initialize (Service* serv,
		     UpnpClient_Handle ctrlpt_handle, 
		     IXML_Element* serviceDesc, 
		     const char* base_url);



#endif /* SERVICE_P_INCLUDED */




