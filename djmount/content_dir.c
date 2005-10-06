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

#include "content_dir_p.h"
#include "device_list.h"
#include "xml_util.h"
#include <string.h>
#include <inttypes.h>
#include <upnp/upnp.h>
#include "service_p.h"



/******************************************************************************
 * Internal parameters
 *****************************************************************************/

// Cache timeout, in seconds
#define CACHE_TIMEOUT	60

// Number of cached entries. Set to zero to deactivate caching.
#define CACHE_SIZE	1024

// Maximum permissible content-length for SOAP messages, in bytes
// (taking into account that "Browse" answers can be very large 
// if contain lot of objects).
#define MAX_CONTENT_LENGTH	(1024 * 1024) 



/******************************************************************************
 * Local types
 *****************************************************************************/

// Shortcuts
#define BrowseResult	ContentDir_BrowseResult
#define Children	ContentDir_Children
#define Count		ContentDir_Count
#define Index		ContentDir_Index


enum BrowseFlag {
  BROWSE_CHILDREN,
  BROWSE_METADATA
};

typedef struct _ContentDir_CacheEntry {
  
  // "objectId" is either NULL or points to a valid talloc'ed string
  // (independantly of cached data being valid or not)
  char* 		objectId;
  String_HashType 	hash;

  // Cached data "children" is valid iff current time <= limit. 
  // In particular:
  //  - "children == NULL" might be a valid cached result,
  //  - "limit == 0" always means cached data is invalid.
  time_t		limit;
  Children*		children;

} CacheEntry;



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
    ContentDir_Object* const o = (ContentDir_Object*) ptr;

    ixmlElement_free (o->element);
   
    // The "talloc'ed" strings will be deleted automatically 
  }
  return 0; // ok -> deallocate memory
}


/******************************************************************************
 * CreateObject
 *****************************************************************************/
static ContentDir_Object*
CreateObject (IN void* talloc_context, IN IXML_Element* elem, 
	      IN bool is_container) 
{
  ContentDir_Object* o = talloc(talloc_context, ContentDir_Object);
  if (o) {
    *o = (struct _ContentDir_Object) { 
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
    o->title = String_CleanFileName (o, XMLUtil_GetFirstNodeValue
				     (node, "dc:title"));
    o->cds_class = String_StripSpaces (o, XMLUtil_GetFirstNodeValue
				       (node, "upnp:class"));
    Log_Printf 
      (LOG_DEBUG,
       "new ContentDir_Object : %s : id='%s' title='%s' class='%s'",
       (is_container ? "container" : "item"), 
       NN(o->id), NN(o->title), NN(o->cds_class));
    
    // Register destructor
    talloc_set_destructor (o, DestroyObject);
  }
  return o;
}


/******************************************************************************
 * int_to_string
 *
 * 	Note: when finished, result should not be freed directly. instead,
 *	parent context "result_context" should be freed using "talloc_free".
 *	
 *****************************************************************************/
static char*
int_to_string (void* result_context, intmax_t val)
{
  // hardcode some common values for "BrowseAction"
  switch (val) {
  case 0:	return "0";
  case 1:	return "1";
  default:	return talloc_asprintf (result_context, "%" PRIdMAX, val);
  }
}


/******************************************************************************
 * BrowseAction
 *****************************************************************************/
static int
BrowseAction (ContentDir* cds,
	      void* result_context, 
	      const char* objectId, 
	      enum BrowseFlag metadata,
	      Index starting_index,
	      Count requested_count,
	      Count* nb_matched,
	      Count* nb_returned,
	      ContentDir_Object** objects)
{
  if (cds == NULL || objectId == NULL) {
    Log_Printf (LOG_ERROR, "ContentDir_BrowseAction NULL parameter");
    return UPNP_E_INVALID_PARAM; // ---------->
  }

  // Create a working context for temporary allocations
  void* tmp_ctx = talloc_new (NULL);

  IXML_Document* doc = NULL;
  int rc = Service_SendActionVa
    (SUPER_CAST(cds), &doc, "Browse", 
     "ObjectID", 	objectId,
     "BrowseFlag", 	( (metadata == BROWSE_METADATA) ? 
			  "BrowseMetadata" : "BrowseDirectChildren"),
     "Filter", 	      	"*",
     "StartingIndex", 	int_to_string (tmp_ctx, starting_index),
     "RequestedCount", 	int_to_string (tmp_ctx, requested_count),
     "SortCriteria", 	"",
     NULL, 		NULL);
  if (doc == NULL && rc == UPNP_E_SUCCESS)
    rc = UPNP_E_BAD_RESPONSE;
  if (rc != UPNP_E_SUCCESS) {
    Log_Printf (LOG_ERROR, "ContentDir_BrowseAction ObjectId='%s'",
		NN(objectId));
    goto cleanup; // ---------->
  }
  
  *nb_matched = 
    XMLUtil_GetFirstNodeValueInteger ((IXML_Node*) doc, "TotalMatches", 0);
  *nb_returned = 
    XMLUtil_GetFirstNodeValueInteger ((IXML_Node*) doc, "NumberReturned", 0);
    
  Log_Printf (LOG_DEBUG, "+++BROWSE RESULT+++\n%s\n", 
	      XMLUtil_GetDocumentString (tmp_ctx, doc));
  
  char* const resstr = XMLUtil_GetFirstNodeValue ((IXML_Node*)doc, "Result");
  if (resstr == NULL) {
    Log_Printf (LOG_ERROR,        
		"BrowseAction ObjectId=%s : can't get 'Result' in doc=%s",
		objectId, XMLUtil_GetDocumentString (tmp_ctx, doc));
    rc = UPNP_E_BAD_RESPONSE;
    goto cleanup; // ---------->
  }

  IXML_Document* subdoc = ixmlParseBuffer (resstr);
  if (subdoc == NULL) {
    Log_Printf (LOG_ERROR, 		  
		"BrowseAction ObjectId=%s : can't parse 'Result'=%s",
		objectId, resstr);
    rc = UPNP_E_BAD_RESPONSE;
  } else {
    IXML_NodeList* containers =
      ixmlDocument_getElementsByTagName (subdoc, "container"); 
    ContentDir_Count const nb_containers = ixmlNodeList_length (containers);
    IXML_NodeList* items =
      ixmlDocument_getElementsByTagName (subdoc, "item"); 
    ContentDir_Count const nb_items = ixmlNodeList_length (items);
    if (nb_containers + nb_items != *nb_returned) {
      Log_Printf (LOG_ERROR, 
		  "BrowseAction ObjectId=%s got %d containers + %d items, "
		  "expected %d", objectId, (int) nb_containers, (int) nb_items,
		  (int) *nb_returned);
      *nb_returned = nb_containers + nb_items;
    }
    
    ContentDir_Count i; 
    for (i = 0; i < *nb_returned; i++) {
      bool const is_container = (i < nb_containers);
      IXML_Element* const elem = (IXML_Element*)
	ixmlNodeList_item (is_container ? containers : items, 
			   is_container ? i : i - nb_containers);
      ContentDir_Object* o = CreateObject (result_context, elem, is_container);
      if (o) {
	o->next = *objects;
	*objects = o;
      }
    }
      
    if (containers)
      ixmlNodeList_free (containers);
    if (items)
      ixmlNodeList_free (items);
    ixmlDocument_free (subdoc);
  }

 cleanup:

  ixmlDocument_free (doc);
  doc = NULL;

  // Delete all temporary storage
  talloc_free (tmp_ctx);
  tmp_ctx = NULL;

  if (rc != UPNP_E_SUCCESS)
    *nb_returned = *nb_matched = 0;
  
  return rc;
}


/******************************************************************************
 * BrowseId
 *****************************************************************************/
static ContentDir_Children*
BrowseAll (ContentDir* cds,
	   void* result_context, 
	   const char* objectId, 
	   enum BrowseFlag metadata)
{
  ContentDir_Children* result = talloc (result_context, ContentDir_Children);
  if (result == NULL) 
    return NULL; // ---------->

  *result = (ContentDir_Children) { 
    .nb_objects = 0,
    .objects 	= NULL
  };
  
  // Request all objects
  Count nb_matched  = 0;
  Count nb_returned = 0;
  
  int rc = BrowseAction (cds,
			 result,
			 objectId, 
			 metadata,
			 /* starting_index  => */ 0,
			 /* requested_count => */ 0,
			 &nb_matched,
			 &nb_returned,
			 &(result->objects));
  if (rc != UPNP_E_SUCCESS) {
    talloc_free (result);
    return NULL; // ---------->
  }
  
  result->nb_objects = nb_returned;
  // Loop if missing entries
  while (result->nb_objects < nb_matched) {
    // This is not normal : "RequestedCount" == 0 means to request all entries
    // according to ContentDirectory specification.
    Log_Printf 
      (LOG_WARNING, 
       "ContentDir_BrowseId ObjectId=%s : got %d results, expected %d",
       objectId, (int) result->nb_objects, (int) nb_matched);
    
    // Workaround : request missing entries.
    rc = BrowseAction(cds,
		      result,
		      objectId, 
		      metadata,
		      /* starting_index  => */ result->nb_objects,
		      /* requested_count => */ nb_matched - result->nb_objects,
		      &nb_matched,
		      &nb_returned,
		      &(result->objects));
    // Stop if error, or no more results (to prevent infinite loop)
    if (rc != UPNP_E_SUCCESS || nb_returned == 0)
      break; // ---------->
    
    result->nb_objects += nb_returned;
  }
  
  return result;
}


/******************************************************************************
 * DestroyResult
 *****************************************************************************/
static int 
DestroyResult (void* ptr)
{
  BrowseResult* br = ptr;
  
  if (br) {
    if (br->cds && br->cds->m.cache)
      ithread_mutex_lock (&br->cds->m.cache_mutex);

    // Cached data will be really freed by talloc 
    // when its reference count drops to zero.
    if (talloc_free (br->children) == 0)
      Log_Printf (LOG_DEBUG, "ContentDir CACHE_FREE");
    
    if (br->cds && br->cds->m.cache)
      ithread_mutex_unlock (&br->cds->m.cache_mutex);

    *br = (BrowseResult) { };
  }

  return 0;
}

/******************************************************************************
 * ContentDir_BrowseChildren
 *****************************************************************************/
const ContentDir_BrowseResult*
ContentDir_BrowseChildren (ContentDir* cds,
			   void* result_context, 
			   const char* objectId)
{
  if (cds == NULL || objectId == NULL)
    return NULL; // ---------->

  BrowseResult* br = talloc (result_context, BrowseResult);
  if (br == NULL)
    return NULL; // ---------->
  *br = (BrowseResult) { .cds = cds };

  if (cds->m.cache == NULL) {
    /*
     * No cache
     */
    br->children = BrowseAll (cds, br, objectId, BROWSE_CHILDREN);

  } else {
    /*
     * Lookup and/or update cache 
     */   
    ithread_mutex_lock (&cds->m.cache_mutex);

    String_HashType const h = String_Hash (objectId);
    unsigned int const idx  = h % CACHE_SIZE;
    CacheEntry* const ce    = cds->m.cache + idx;
    
    bool const same_object_id = (ce->objectId && ce->hash == h && 
				 strcmp (ce->objectId, objectId) == 0);
    if (same_object_id && time(NULL) <= ce->limit) {
      Log_Printf (LOG_DEBUG, "ContentDir CACHE_HIT (id='%s', idx=%u)",
		  objectId, idx);
      br->children = ce->children; 
    } else {
      // Use the cache as parent context for allocation of result
      br->children = BrowseAll (cds, cds->m.cache, objectId, BROWSE_CHILDREN);
      if (same_object_id) {
	Log_Printf (LOG_DEBUG, 
		    "ContentDir CACHE_EXPIRED (new='%s', idx=%u)",
		    objectId, idx);
      } else {
	if (ce->objectId) {
	  Log_Printf (LOG_DEBUG, 
		      "ContentDir CACHE_COLLIDE (old='%s', new='%s', idx=%u)",
		      ce->objectId, objectId, idx);
	} else {
	  Log_Printf (LOG_DEBUG, "ContentDir CACHE_NEW (new='%s', idx=%u)",
		      objectId, idx);
	}
	talloc_free (ce->objectId);
	ce->objectId = talloc_strdup (cds->m.cache, objectId);
	ce->hash = h;
      }
      if (ce->children) {
	// Un-reference old cached data ; will be really freed
	// by talloc when its reference count drops to zero.
	if (talloc_free (ce->children) == 0)
	  Log_Printf (LOG_DEBUG, "ContentDir CACHE_FREE (new='%s', idx=%u)",
		      objectId, idx);
      }
      ce->children = br->children;
      ce->limit    = time(NULL) + CACHE_TIMEOUT;
    }
    // Add a reference to the cached result before returning it
    if (br->children) {
      talloc_increase_ref_count (br->children);    
      talloc_set_destructor (br, DestroyResult);
    }

    ithread_mutex_unlock (&cds->m.cache_mutex);
  }

  if (br->children == NULL) {
    talloc_free (br);
    br = NULL;
  }

  return br;
}



/******************************************************************************
 * ContentDir_BrowseMetadata
 *****************************************************************************/
ContentDir_Object*
ContentDir_BrowseMetadata (ContentDir* cds,
			   void* result_context, 
			   const char* objectId)
{
  // TBD: no cache in BrowseMetadata method for the time being
  
  ContentDir_Object* res = NULL;
  Count nb_matched  = 0;
  Count nb_returned = 0;
  int rc = BrowseAction (cds, result_context, objectId, BROWSE_METADATA,
			 0, 1, &nb_matched, &nb_returned, &res);
  if (rc == UPNP_E_SUCCESS && nb_returned != 1) {
    Log_Printf (LOG_ERROR, 
		"ContentDir_BrowseMetadata : not 1 result exactly Id=%s",
		NN(objectId));
  }
  return res;
}


/******************************************************************************
 * finalize
 *
 * Description: 
 *	ContentDir destructor
 *
 *****************************************************************************/
static void
finalize (Object* obj)
{
  ContentDir* const cds = (ContentDir*) obj;

  if (cds && cds->m.cache)
    ithread_mutex_destroy (&cds->m.cache_mutex);

  // Other "talloc'ed" fields will be deleted automatically : nothing to do 
}


/*****************************************************************************
 * OBJECT_CLASS_PTR(ContentDir)
 *****************************************************************************/

const ContentDirClass* OBJECT_CLASS_PTR(ContentDir)
{
  static ContentDirClass the_class = { .o.size = 0 };
  static const ContentDir the_default_object = { .isa = &the_class };

  // Initialize non-const fields on first call 
  if (the_class.o.size == 0) {

    _ObjectClass_Lock();

    // 1. Copy superclass methods
    const ServiceClass* super = OBJECT_CLASS_PTR(Service);
    the_class.m._ = *super;

    // 2. Initialize specific fields
    the_class.o = (ObjectClass) {
      .magic		= super->o.magic,
      .name 		= "ContentDir",
      .super		= &super->o,
      .size		= sizeof (ContentDir),
      .initializer 	= &the_default_object,
      .finalize 	= finalize,
    };

    // Class-specific initialization :
    // Increase maximum permissible content-length for SOAP messages
    // because "Browse" answers can be very large if contain lot of objects.
    UpnpSetMaxContentLength (MAX_CONTENT_LENGTH);

    _ObjectClass_Unlock();
  }
  
  return &the_class;
}


/*****************************************************************************
 * ContentDir_Create
 *****************************************************************************/
ContentDir* 
ContentDir_Create (void* talloc_context, 
		   UpnpClient_Handle ctrlpt_handle, 
		   IXML_Element* serviceDesc, 
		   const char* base_url)
{
  ContentDir* cds = _OBJECT_TALLOC (talloc_context, ContentDir);
  if (cds == NULL)
    goto error; // ---------->

  int rc = _Service_Initialize (SUPER_CAST(cds), ctrlpt_handle, 
				serviceDesc, base_url);
  if (rc)
    goto error; // ---------->
  
  if (CACHE_SIZE > 0 && CACHE_TIMEOUT > 0) {
    cds->m.cache = talloc_array (cds, CacheEntry, CACHE_SIZE);
    if (cds->m.cache == NULL)
      goto error; // ---------->
    int i;
    for (i = 0; i < CACHE_SIZE; i++)
      cds->m.cache[i] = (CacheEntry) { .objectId = NULL, .limit = 0 };
    ithread_mutex_init (&cds->m.cache_mutex, NULL);
  }

  return cds; // ---------->
  
 error:
  Log_Print (LOG_ERROR, "ContentDir_Create error");
  // TBD there might be a leak here,
  // TBD but don't try to call talloc_free on partialy initialized object
  return NULL;
}

