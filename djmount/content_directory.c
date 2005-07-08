/* $Id$
 *
 * UPnP Content Directory Service 
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

#include "content_directory_p.h"
#include "device_list.h"
#include "xml_util.h"
#include <string.h>
#include <upnp/upnp.h>
#include "service_p.h"




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
    ContentDirectory_Object* const o = (ContentDirectory_Object*) ptr;

    ixmlElement_free (o->element);
   
    // The "talloc'ed" strings will be deleted automatically 
  }
  return 0; // ok -> deallocate memory
}


/******************************************************************************
 * CreateObject
 *****************************************************************************/
static ContentDirectory_Object*
CreateObject (IN void* talloc_context, IN IXML_Element* elem, 
	      IN bool is_container) 
{
  ContentDirectory_Object* o = talloc(talloc_context, ContentDirectory_Object);
  if (o) {
    *o = (struct _ContentDirectory_Object) { 
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
    Log_Printf 
      (LOG_DEBUG,
       "new ContentDirectory_Object : %s : id='%s' title='%s' class='%s'",
       (is_container ? "container" : "item"), 
       NN(o->id), NN(o->title), NN(o->cds_class));
  }

  // Register destructor
  talloc_set_destructor (o, DestroyObject);

  return o;
}


/******************************************************************************
 * ContentDirectory_BrowseId
 *****************************************************************************/
static ContentDirectory_BrowseResult*
ContentDirectory_BrowseId (const ContentDirectory* cds,
			   void* result_context, 
			   const char* objectId, 
			   bool metadata)
{
  IXML_Document* doc = NULL;
  int rc = Service_SendActionVa
    (SUPER_CAST(cds), &doc, "Browse", 
     "ObjectID", 	objectId,
     "BrowseFlag", 	(metadata ? "BrowseMetadata" : "BrowseDirectChildren"),
     "Filter", 	      	"*",
     "StartingIndex", 	"0",
     "RequestedCount", 	"0", // 0 = all
     "SortCriteria", 	"",
     NULL, 		NULL);
  
  if (rc != UPNP_E_SUCCESS || doc == NULL) {
    Log_Printf (LOG_ERROR, "ContentDirectory_BrowseId ObjectId='%s'",
		NN(objectId));
    return NULL; // ---------->
  }

  ContentDirectory_BrowseResult* result = NULL;
    
  // Create a working context for temporary allocations
  void* tmp_ctx = talloc_new (NULL);

  ContentDirectory_Count nb_total = 
    XMLUtil_GetFirstNodeValueInteger ((IXML_Node*) doc, "TotalMatches", 0);
  ContentDirectory_Count nb = 
    XMLUtil_GetFirstNodeValueInteger ((IXML_Node*) doc, "NumberReturned", 0);
    
  if (nb < nb_total) {
    Log_Printf (LOG_ERROR, 
		"ContentDirectory_BrowseId ObjectId=%s : got %d results, "
		"expected %d in doc=%s",
	        objectId, nb, nb_total,
		XMLUtil_GetDocumentString (tmp_ctx, doc));
  }
  Log_Printf (LOG_DEBUG, "+++BROWSE RESULT+++\n%s\n", 
	      XMLUtil_GetDocumentString (tmp_ctx, doc));
  
  char* const resstr = XMLUtil_GetFirstNodeValue ((IXML_Node*)doc, "Result");
  if (resstr == NULL) {
    Log_Printf 
      (LOG_ERROR, 		  
       "ContentDirectory_BrowseId ObjectId=%s : can't get 'Result' in doc=%s",
       objectId, XMLUtil_GetDocumentString (tmp_ctx, doc));
  } else {
    IXML_Document* subdoc = ixmlParseBuffer (resstr);
    if (subdoc == NULL) {
      Log_Printf 
	(LOG_ERROR, 		  
	 "ContentDirectory_BrowseId ObjectId=%s : can't parse 'Result'=%s",
	 objectId, resstr);
    } else {
      IXML_NodeList* containers =
	ixmlDocument_getElementsByTagName (subdoc, "container"); 
      ContentDirectory_Count const nb_containers = 
	ixmlNodeList_length (containers);
      IXML_NodeList* items =
	ixmlDocument_getElementsByTagName (subdoc, "item"); 
      ContentDirectory_Count const nb_items = ixmlNodeList_length (items);
      if (nb_containers + nb_items != nb) {
	Log_Printf (LOG_ERROR, 
		    "ContentDirectory_BrowseId ObjectId=%s got %d containers "
		    "+ %d items, expected %d",
		    objectId, nb_containers, nb_items, nb);
	nb = nb_containers + nb_items;
      }
      
      result = talloc (result_context, ContentDirectory_BrowseResult);
      *result = (struct _ContentDirectory_BrowseResult) { 
	.nb_containers 	= 0,
	.nb_items 	= 0,
	.children 	= NULL
      };
      ContentDirectory_Count i; 
      for (i = 0; i < nb; i++) {
	bool const is_container = (i < nb_containers);
	IXML_Element* const elem = (IXML_Element*)
	  ixmlNodeList_item (is_container ? containers : items, 
			     is_container ? i : i - nb_containers);
	ContentDirectory_Object* o = 
	  CreateObject (result, elem, is_container);
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
  doc = NULL;

  // Delete all temporary storage
  talloc_free (tmp_ctx);
  tmp_ctx = NULL;

  return result;
}

/******************************************************************************
 * ContentDirectory_BrowseChildren
 *****************************************************************************/
ContentDirectory_BrowseResult*
ContentDirectory_BrowseChildren (const ContentDirectory* cds,
				 void* result_context, 
				 const char* objectId)
{
  return ContentDirectory_BrowseId (cds, result_context, objectId, 
				    /* metadata => */ false);
}


/******************************************************************************
 * ContentDirectory_BrowseMetadata
 *****************************************************************************/
ContentDirectory_Object*
ContentDirectory_BrowseMetadata (const ContentDirectory* cds,
				 void* result_context, 
				 const char* objectId)
{
  ContentDirectory_Object* res = NULL;
  ContentDirectory_BrowseResult* br = 
    ContentDirectory_BrowseId (cds, NULL, objectId, /* metadata => */ true);
  if (br) {
    if (br->children && br->children->next == NULL) {
      res = talloc_steal (result_context, br->children);
    } else {
      Log_Printf 
	(LOG_ERROR, 
	 "ContentDirectory_BrowseMetadata : not 1 result exactly Id=%s",
	 NN(objectId));
    }
    talloc_free (br);
  }
  return res;
}




/*****************************************************************************
 * _ContentDirectoryClass_Get
 *****************************************************************************/

const ContentDirectoryClass* OBJECT_CLASS_PTR(ContentDirectory)
{
  static ContentDirectoryClass the_class = { .o.size = 0 };
  static const ContentDirectory the_default_object = { .isa = &the_class };

  // Initialize non-const fields on first call 
  if (the_class.o.size == 0) {

    _ObjectClass_Lock();

    // 1. Copy superclass methods
    const ServiceClass* super = OBJECT_CLASS_PTR(Service);
    the_class.m._ = *super;

    // 2. Initialize specific fields
    the_class.o = (ObjectClass) {
      .magic		= super->o.magic,
      .name 		= "ContentDirectory",
      .super		= &super->o,
      .size		= sizeof (ContentDirectory),
      .initializer 	= &the_default_object,
      .finalize 	= NULL,
    };

    // Class-specific initialization :
    // Increase maximum permissible content-length for SOAP messages
    // because "Browse" answers can be very large if contain lot of objects.
    UpnpSetMaxContentLength (1024 * 1024); // 1 Mbytes

    _ObjectClass_Unlock();
  }
  
  return &the_class;
}


/*****************************************************************************
 * ContentDirectory_Create
 *****************************************************************************/
ContentDirectory* 
ContentDirectory_Create (void* talloc_context, 
			 UpnpClient_Handle ctrlpt_handle, 
			 IXML_Element* serviceDesc, 
			 const char* base_url)
{
  ContentDirectory* cds = _OBJECT_TALLOC (talloc_context, ContentDirectory);

  if (cds) {
    int rc = _Service_Initialize (SUPER_CAST(cds), ctrlpt_handle, 
				  serviceDesc, base_url);
    if (rc)
      cds = NULL;
  }
  
  return cds;
}

