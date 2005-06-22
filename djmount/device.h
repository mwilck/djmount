/* $Id$
 *
 * UPnP Device
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

#ifndef DEVICE_INCLUDED
#define DEVICE_INCLUDED 1


#include <upnp/upnp.h>
#include <upnp/ixml.h>


#ifdef __cplusplus
extern "C" {
#endif


// Forward declaration
struct ServiceStruct;


/******************************************************************************
 * @var Device
 *
 *	This opaque type encapsulates access to the UPnP device.
 *	
 *      NOTE THAT THE FUNCTION API IS NOT THREAD SAFE. Function should 
 *      be called through the global functions in "upnp_devicelist.h", 
 *	who lock the global device list.
 *
 *****************************************************************************/

struct DeviceStruct;
typedef struct DeviceStruct Device;


/******************************************************************************
 * @brief Creates a new device.
 *	This routine parse the DOM representation document for the device.
 *	The returned Device* can be destroyed with "talloc_free".
 *
 * @param context        the talloc parent context
 * @param ctrlpt_handle  the UPnP client handle
 * @param location 	 the location of the description document 
 * @param descDoc 	 the description document (copied internally)
 *****************************************************************************/
Device* 
Device_Create (void* context, 
	       UpnpClient_Handle ctrlpt_handle, 
	       const char* location, 
	       IXML_Document* descDoc);


/** TBD 
 * The return string should be copied if necessary e.g. if the Device
 *	is to be destroyed.	
 */
//TBD
char*
Device_GetDescDocItem (const Device* dev, const char* item);



/**
 * @brief returns the pointer to a given Service.
 * 	Given a list number, returns the pointer to the service
 *      node at that position in the device. Note
 *      that this function is not thread safe.  It must be called 
 *      from a function that has locked the global device list.
 *
 * @param servnum   The service number (order in the list, starting with 0)
 */
struct ServiceStruct* 
Device_GetService (const Device* dev, int servnum);

// TBD
enum GetFrom { FROM_SID, FROM_CONTROL_URL, FROM_EVENT_URL };
struct ServiceStruct* 
Device_GetServiceFrom (const Device* dev, const char*, enum GetFrom);


/******************************************************************************
 * @brief Returns a string describing the state of the device.
 * 	  The returned string should be freed using "talloc_free".
 *****************************************************************************/
char*
Device_GetStatusString (const Device* dev);




#ifdef __cplusplus
}; // extern "C"
#endif


#endif /* DEVICE_INCLUDED */




