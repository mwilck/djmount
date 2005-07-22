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
    // TBD handle unicode names ? XXX
    o->title = String_CleanFileName (o, XMLUtil_GetFirstNodeValue
				     (node, "dc:title"));
    o->cds_class = String_StripSpaces (o, XMLUtil_GetFirstNodeValue
				       (node, "upnp:class"));
    Log_Printf 
      (LOG_DEBUG,
       "new ContentDir_Object : %s : id='%s' title='%s' class='%s'",
       (is_container ? "container" : "item"), 
       NN(o->id), NN(o->title), NN(o->cds_class));
  }

  // Register destructor
  talloc_set_destructor (o, DestroyObject);

  return o;
}


/******************************************************************************
 * ContentDir_BrowseId
 *****************************************************************************/
static ContentDir_Children*
ContentDir_BrowseId (const ContentDir* cds,
		     void* result_context, 
		     const char* objectId, 
		     enum BrowseFlag metadata)
{
  if (cds == NULL || objectId == NULL) {
    Log_Printf (LOG_ERROR, "ContentDir_BrowseId NULL parameter");
    return NULL; // ---------->
  }
  
  IXML_Document* doc = NULL;
  int rc = Service_SendActionVa
    (SUPER_CAST(cds), &doc, "Browse", 
     "ObjectID", 	objectId,
     "BrowseFlag", 	( (metadata == BROWSE_METADATA) ? 
			  "BrowseMetadata" : "BrowseDirectChildren"),
     "Filter", 	      	"*",
     "StartingIndex", 	"0",
     "RequestedCount", 	"0", // 0 = all
     "SortCriteria", 	"",
     NULL, 		NULL);
  
  if (rc != UPNP_E_SUCCESS || doc == NULL) {
    Log_Printf (LOG_ERROR, "ContentDir_BrowseId ObjectId='%s'",
		NN(objectId));
    return NULL; // ---------->
  }

  ContentDir_Children* result = NULL;
    
  // Create a working context for temporary allocations
  void* tmp_ctx = talloc_new (NULL);

  ContentDir_Count nb_total = 
    XMLUtil_GetFirstNodeValueInteger ((IXML_Node*) doc, "TotalMatches", 0);
  ContentDir_Count nb = 
    XMLUtil_GetFirstNodeValueInteger ((IXML_Node*) doc, "NumberReturned", 0);
    
  if (nb < nb_total) {
    Log_Printf (LOG_ERROR, 
		"ContentDir_BrowseId ObjectId=%s : got %d results, "
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
       "ContentDir_BrowseId ObjectId=%s : can't get 'Result' in doc=%s",
       objectId, XMLUtil_GetDocumentString (tmp_ctx, doc));
  } else {
    IXML_Document* subdoc = ixmlParseBuffer (resstr);
    if (subdoc == NULL) {
      Log_Printf 
	(LOG_ERROR, 		  
	 "ContentDir_BrowseId ObjectId=%s : can't parse 'Result'=%s",
	 objectId, resstr);
    } else {
      IXML_NodeList* containers =
	ixmlDocument_getElementsByTagName (subdoc, "container"); 
      ContentDir_Count const nb_containers = ixmlNodeList_length (containers);
      IXML_NodeList* items =
	ixmlDocument_getElementsByTagName (subdoc, "item"); 
      ContentDir_Count const nb_items = ixmlNodeList_length (items);
      if (nb_containers + nb_items != nb) {
	Log_Printf (LOG_ERROR, 
		    "ContentDir_BrowseId ObjectId=%s got %d containers "
		    "+ %d items, expected %d",
		    objectId, nb_containers, nb_items, nb);
	nb = nb_containers + nb_items;
      }
      
      result = talloc (result_context, ContentDir_Children);
      *result = (struct _ContentDir_Children) { 
	.nb_containers 	= 0,
	.nb_items 	= 0,
	.objects 	= NULL
      };
      ContentDir_Count i; 
      for (i = 0; i < nb; i++) {
	bool const is_container = (i < nb_containers);
	IXML_Element* const elem = (IXML_Element*)
	  ixmlNodeList_item (is_container ? containers : items, 
			     is_container ? i : i - nb_containers);
	ContentDir_Object* o = CreateObject (result, elem, is_container);
	if (o) {
	  o->next = result->objects;
	  result->objects = o;
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
    br->children = ContentDir_BrowseId (cds, br, objectId, BROWSE_CHILDREN);

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
      br->children = ContentDir_BrowseId (cds, cds->m.cache, objectId, 
					  BROWSE_CHILDREN);
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
ContentDir_BrowseMetadata (const ContentDir* cds,
			   void* result_context, 
			   const char* objectId)
{
  // TBD: no cache in BrowseMetadata method for the time being
  
  ContentDir_Object* res = NULL;
  Children* children = ContentDir_BrowseId (cds, NULL, objectId, 
					    BROWSE_METADATA);
  if (children) {
    if (children->objects && children->objects->next == NULL) {
      res = talloc_steal (result_context, children->objects);
    } else {
      Log_Printf (LOG_ERROR, 
		  "ContentDir_BrowseMetadata : not 1 result exactly Id=%s",
		  NN(objectId));
    }
    talloc_free (children);
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

