/* $Id$
 *
 * "CDS" = Content Directory Service 
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

#include "cds.h"
#include "device_list.h"
#include "xml_util.h"
#include <string.h>
#include <upnp/upnp.h>



#define CONTENT_DIRECTORY_SERVICE_ID "urn:upnp-org:serviceId:ContentDirectory"



/******************************************************************************
 * DestroyObject
 *
 * Description: 
 *	DIDL Object destructor, automatically called by "talloc_free".
 *
 *****************************************************************************/
static int
DestroyObject (void* ptr)
{
  if (ptr) {
    CDS_Object* const o = (CDS_Object*) ptr;

    ixmlElement_free (o->element);
   
    // The "talloc'ed" strings will be deleted automatically 
  }
  return 0; // ok -> deallocate memory
}


/******************************************************************************
 * CreateObject
 *****************************************************************************/
static CDS_Object*
CreateObject (IN void* talloc_context, IN IXML_Element* elem, 
	      IN bool is_container) 
{
  CDS_Object* o = talloc (talloc_context, CDS_Object);
  if (o) {
    *o = (struct CDS_ObjectStruct) { 
      .is_container = is_container,
      .next = NULL
    };

    IXML_Node* node = NULL;
    /* Steal the node from its parent (instead of copying it, given
     * that the parent document is going to be deallocated anyway)
     */
    ixmlNode_removeChild (ixmlNode_getParentNode ((IXML_Node*) elem), 
			  (IXML_Node*) elem, &node);
    o->element = (IXML_Element*) node;

    o->id = talloc_strdup (o, ixmlElement_getAttribute (o->element, "id"));
    // TBD handle unicode names ? XXX
    o->title = String_CleanFileName (o, XMLUtil_GetFirstNodeValue
				     (node, "dc:title"));
    o->cds_class = String_StripSpaces (o, XMLUtil_GetFirstNodeValue
				       (node, "upnp:class"));
    Log_Printf (LOG_DEBUG,
		"new CDS_Object : %s : id='%s' title='%s' class='%s'",
		(is_container ? "container" : "item"), 
		NN(o->id), NN(o->title), NN(o->cds_class));
  }

  // Register destructor
  talloc_set_destructor (o, DestroyObject);

  return o;
}


/******************************************************************************
 * CDS_BrowseId
 *****************************************************************************/
static CDS_BrowseResult*
CDS_BrowseId (void* result_context, const char* deviceName, 
	      const char* objectId, bool metadata)
{
  CDS_BrowseResult* result = NULL;


  // TBD put elsewhere ??
  // Increase maximum permissible content-length for SOAP messages
  // because "Browse" answers can be very large if contain lot of objects.
  UpnpSetMaxContentLength (1024 * 1024); // 1 Mbytes
  // TBD XXX

  IXML_Document* doc = DeviceList_SendActionVa
    (deviceName, CONTENT_DIRECTORY_SERVICE_ID, "Browse", 
     "ObjectID", 	objectId,
     "BrowseFlag", 	(metadata ? "BrowseMetadata" : "BrowseDirectChildren"),
     "Filter", 	      	"*",
     "StartingIndex", 	"0",
     "RequestedCount", 	"0", // 0 = all
     "SortCriteria", 	"",
     NULL, 		NULL);

  // Create a working context for temporary allocations
  void* tmp_ctx = talloc_new (NULL);

  if (doc == NULL) {
    Log_Printf (LOG_ERROR, "CDS_BrowseId device='%s' Id='%s'",
		NN(deviceName), NN(objectId));
  } else {
    CDS_Count nb_total = XMLUtil_GetFirstNodeValueInteger ((IXML_Node*) doc,
							   "TotalMatches", 0);
    CDS_Count nb = XMLUtil_GetFirstNodeValueInteger ((IXML_Node*) doc, 
						     "NumberReturned", 0);
    
    if (nb < nb_total) {
      Log_Printf (LOG_ERROR, 
		  "CDS_BrowseId '%s' Id=%s : got %d results, expected %d "
		  "in doc=%s",
		  deviceName, objectId, nb, nb_total,
		  XMLUtil_GetDocumentString (tmp_ctx, doc));
    }
    Log_Printf (LOG_DEBUG, "+++BROWSE RESULT+++\n%s\n", 
		XMLUtil_GetDocumentString (tmp_ctx, doc));

    char* const resstr = XMLUtil_GetFirstNodeValue ((IXML_Node*)doc, "Result");
    if (resstr == NULL) {
      Log_Printf (LOG_ERROR, 		  
		  "CDS_BrowseId '%s' Id=%s : can't get 'Result' in doc=%s",
		  deviceName, objectId, 
		  XMLUtil_GetDocumentString (tmp_ctx, doc));
    } else {
      IXML_Document* subdoc = ixmlParseBuffer (resstr);
      if (subdoc == NULL) {
	Log_Printf (LOG_ERROR, 		  
		    "CDS_BrowseId '%s' Id=%s : can't parse 'Result'=%s",
		    deviceName, objectId, resstr);
      } else {
	IXML_NodeList* containers =
	  ixmlDocument_getElementsByTagName (subdoc, "container"); 
	CDS_Count const nb_containers = ixmlNodeList_length (containers);
	IXML_NodeList* items =
	  ixmlDocument_getElementsByTagName (subdoc, "item"); 
	CDS_Count const nb_items = ixmlNodeList_length (items);
	if (nb_containers + nb_items != nb) {
	  Log_Printf 
	    (LOG_ERROR, 
	     "CDS_BrowseId '%s' Id=%s got %d containers + %d items, "
	     "expected %d",
	     deviceName, objectId, nb_containers, nb_items, nb);
	  nb = nb_containers + nb_items;
	}
	
	result = talloc (result_context, CDS_BrowseResult);
	*result = (struct CDS_BrowseResultStruct) { 
	  .nb_containers = 0,
	  .nb_items =      0,
	  .children =      NULL
	};
	CDS_Count i; 
	for (i = 0; i < nb; i++) {
	  bool const is_container = (i < nb_containers);
	  IXML_Element* const elem = (IXML_Element*)
	    ixmlNodeList_item (is_container ? containers : items, 
			       is_container ? i : i - nb_containers);
	  CDS_Object* o = CreateObject (result, elem, is_container);
	  if (o) {
	    o->next = result->children;
	    result->children = o;
	    is_container ? result->nb_containers++ : result->nb_items++;
	  }
	}
	
	if (containers)
	  ixmlNodeList_free (containers);
	if (items)
	  ixmlNodeList_free (items);
	ixmlDocument_free (subdoc);
      }
    }
    ixmlDocument_free (doc);
  }  

  // Delete all temporary storage
  talloc_free (tmp_ctx);
  tmp_ctx = NULL;

  return result;
}

/******************************************************************************
 * CDS_BrowseChildren
 *****************************************************************************/
CDS_BrowseResult*
CDS_BrowseChildren (void* result_context, 
		    const char* deviceName, const char* objectId)
{
  return CDS_BrowseId (result_context, deviceName, objectId, 
		       /* metadata => */ false);
}


/******************************************************************************
 * CDS_BrowseMetadata
 *****************************************************************************/
CDS_Object*
CDS_BrowseMetadata (void* result_context, 
		    const char* deviceName, const char* objectId)
{
  CDS_Object* res = NULL;
  CDS_BrowseResult* br = CDS_BrowseId (NULL, deviceName, objectId, 
				       /* metadata => */ true);
  if (br) {
    if (br->children && br->children->next == NULL) {
      res = talloc_steal (result_context, br->children);
    } else {
      Log_Printf (LOG_ERROR, 
		  "CDS_BrowseMetadata '%s' : not 1 result exactly Id=%s",
		  deviceName, NN(objectId));
    }
    talloc_free (br);
  }
  return res;
}




