/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "content_dir_p.h"
#include "device_list.h"
#include "xml_util.h"
#include <string.h>
#include <inttypes.h>
#include <upnp/upnp.h>
#include "service_p.h"
#include "cache.h"
#include "log.h"



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


// Special 'criteria' for BrowseOrSearchAction 
// (use the pointer values)
static const char* const CRITERIA_BROWSE_METADATA = "BrowseMetadata";
static const char* const CRITERIA_BROWSE_CHILDREN = "BrowseDirectChildren";

static inline bool is_browse (const char* const criteria) 
{
	return (criteria == CRITERIA_BROWSE_METADATA ||
		criteria == CRITERIA_BROWSE_CHILDREN);
}


/******************************************************************************
 * DestroyChildren
 *
 * Description:
 *      DIDL Object destructor, automatically called by "talloc_free".
 *
 *****************************************************************************/
static int
DestroyChildren (ContentDir_Children* const children)
{
	if (children) {
#if CONTENT_DIR_HAVE_CHILDREN_MUTEX
		ithread_mutex_destroy (&children->mutex);
#endif	
		// Other "talloc'ed" fields will be deleted automatically :
		// nothing to do
	}
	return 0; // ok -> deallocate memory
}


/******************************************************************************
 * int_to_string
 *
 * 	Note: when finished, result should not be freed directly. instead,
 *	parent context "result_context" should be freed using "talloc_free".
 *	
 *****************************************************************************/
static const char*
int_to_string (void* result_context, intmax_t val)
{
	// hardcode some common values for "BrowseAction"
	switch (val) {
	case 0:	  return "0";
	case 1:	  return "1";
	default:  return talloc_asprintf (result_context, "%" PRIdMAX, val);
	}
}


/******************************************************************************
 * BrowseAction
 *****************************************************************************/
static int
BrowseOrSearchAction (ContentDir* cds,
		      void* result_context,
		      const char* objectId, 
		      const char* criteria,
		      Index starting_index,
		      Count requested_count,
		      Count* nb_matched,
		      Count* nb_returned,
		      PtrArray* objects)
{
	if (cds == NULL || objectId == NULL || criteria == NULL) {
		Log_Printf (LOG_ERROR, 
			    "BrowseOrSearchAction NULL parameter");
		return UPNP_E_INVALID_PARAM; // ---------->
	}
	
	// Create a working context for temporary allocations
	void* tmp_ctx = talloc_new (NULL);
	
	const bool browse = is_browse (criteria);
	IXML_Document* doc = NULL;
	int rc = Service_SendActionVa
		(OBJECT_SUPER_CAST(cds), &doc, 
		 (browse ? "Browse" : "Search"),
		 (browse ? "ObjectID" : "ContainerID"),		objectId,
		 (browse ? "BrowseFlag" : "SearchCriteria"),	criteria,
		 "Filter", 	      "*",
		 "StartingIndex",     int_to_string (tmp_ctx, starting_index),
		 "RequestedCount",    int_to_string (tmp_ctx, requested_count),
		 "SortCriteria",      "",
		 NULL, 		      NULL);
	if (doc == NULL && rc == UPNP_E_SUCCESS)
		rc = UPNP_E_BAD_RESPONSE;
	if (rc != UPNP_E_SUCCESS) {
		Log_Printf (LOG_ERROR, "BrowseOrSearchAction ObjectId='%s'",
			    NN(objectId));
		goto cleanup; // ---------->
	}
	
	const char* s = XMLUtil_GetFirstNodeValue ((IXML_Node*) doc, 
						   "TotalMatches", true);
	STRING_TO_INT (s, *nb_matched, 0);
	
	s = XMLUtil_GetFirstNodeValue ((IXML_Node*) doc, "NumberReturned",
				       true);
	STRING_TO_INT (s, *nb_returned, 0);
	
	Log_Printf (LOG_DEBUG, "+++BROWSE RESULT+++\n%s\n", 
		    XMLUtil_GetDocumentString (tmp_ctx, doc));
	
	char* const resstr = XMLUtil_GetFirstNodeValue ((IXML_Node*)doc,
							"Result", true);
	if (resstr == NULL) {
		Log_Printf (LOG_ERROR, "BrowseOrSearchAction ObjectId=%s : "
			    "can't get 'Result' in doc=%s",
			    objectId, 
			    XMLUtil_GetDocumentString (tmp_ctx, doc));
		rc = UPNP_E_BAD_RESPONSE;
		goto cleanup; // ---------->
	}

	IXML_Document* subdoc = ixmlParseBuffer (resstr);
	if (subdoc == NULL) {
		Log_Printf (LOG_ERROR, "BrowseOrSearchAction ObjectId=%s : "
			    "can't parse 'Result'=%s", objectId, resstr);
		rc = UPNP_E_BAD_RESPONSE;
	} else {
		IXML_NodeList* containers = ixmlDocument_getElementsByTagName
			(subdoc, "container"); 
		ContentDir_Count const nb_containers = 
			ixmlNodeList_length (containers);
		IXML_NodeList* items =
			ixmlDocument_getElementsByTagName (subdoc, "item"); 
		ContentDir_Count const nb_items = ixmlNodeList_length (items);
		if (nb_containers + nb_items != *nb_returned) {
			Log_Printf (LOG_ERROR, 
				    "BrowseOrSearchAction ObjectId=%s "
				    "got %d containers + %d items, "
				    "expected %d", objectId, 
				    (int) nb_containers, (int) nb_items,
				    (int) *nb_returned);
			*nb_returned = nb_containers + nb_items;
		}
		if (criteria == CRITERIA_BROWSE_METADATA && *nb_returned != 1){
			Log_Printf (LOG_ERROR, "ContentDir_Browse Metadata : "
				    "not 1 result exactly ! Id=%s", 
				    NN(objectId));
		}

		ContentDir_Count i; 
		for (i = 0; i < *nb_returned; i++) {
			bool const is_container = (i < nb_containers);
			IXML_Element* const elem = (IXML_Element*) 
				ixmlNodeList_item
				(is_container ? containers : items, 
				 is_container ? i : i - nb_containers);
			DIDLObject* o = DIDLObject_Create (result_context, 
							   elem, is_container);
			if (o) {
				PtrArray_Append (objects, o);
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
 * BrowseOrSearchAll
 *****************************************************************************/
static ContentDir_Children*
BrowseOrSearchAll (ContentDir* cds,
		   void* result_context, 
		   const char* objectId, 
		   const char* const criteria)
{
	ContentDir_Children* result = talloc (result_context, 
					      ContentDir_Children);
	if (result == NULL)
		return NULL; // ---------->

	PtrArray* objects = PtrArray_Create (result);
	if (objects == NULL) 
		goto FAIL; // ---------->

	*result = (ContentDir_Children) {
		.objects = objects
	};
		
#if CONTENT_DIR_HAVE_CHILDREN_MUTEX
	ithread_mutexattr_t attr;
	ithread_mutexattr_init (&attr);
	ithread_mutexattr_setkind_np (&attr, ITHREAD_MUTEX_RECURSIVE_NP);
	ithread_mutex_init (&result->mutex, &attr);
	ithread_mutexattr_destroy (&attr);
#endif

        talloc_set_destructor (result, DestroyChildren);

	// Request all objects
	Count nb_matched  = 0;
	Count nb_returned = 0;
	
	int rc = BrowseOrSearchAction (cds,
				       objects,
				       objectId, 
				       criteria,
				       /* starting_index  => */ 0,
				       /* requested_count => */ 0,
				       &nb_matched,
				       &nb_returned,
				       objects);
	if (rc != UPNP_E_SUCCESS) 
		goto FAIL; // ---------->
	
	// Loop if missing entries
	// (this is not normal : "RequestedCount" == 0 means to request 
	// all entries according to ContentDirectory specification).
	// Note: it is allowed to have nb_matched == 0 if it cannot be
	// computed by the CDS.
	int nb_retry = 0;
	while (PtrArray_GetSize (objects) < nb_matched && nb_retry++ < 2) {
		Log_Printf (LOG_WARNING, 
			    "ContentDir_BrowseId ObjectId=%s : "
			    "got %d results, expected %d. Retry %d ...",
			    objectId, (int) PtrArray_GetSize (objects), 
			    (int) nb_matched, nb_retry);
    
		// Workaround : request missing entries.
		rc = BrowseOrSearchAction 
			(cds,
			 objects,
			 objectId, 
			 criteria,
			 /* starting_index  => */ PtrArray_GetSize (objects),
			 /* requested_count => */ 
			 nb_matched - PtrArray_GetSize (objects),
			 &nb_matched,
			 &nb_returned,
			 objects);
		// Stop if error, or no more results (to prevent infinite loop)
		if (rc != UPNP_E_SUCCESS || nb_returned == 0)
			break; // ---------->
	}
	
	return result;

FAIL:
	talloc_free (result);
	return NULL; 
}


/******************************************************************************
 * cache_free_expired_data
 *****************************************************************************/
static void 
cache_free_expired_data (const char* key, void* data)
{
	Children* const children = (Children*) data;

	// Un-reference old cached data (Children). Will be really freed
	// by talloc only when its reference count drops to zero.
	if (talloc_free (children) == 0) {
		Log_Printf (LOG_DEBUG, "ContentDir CACHE_FREE (key='%s')", 
			    key);
	}
}


/******************************************************************************
 * DestroyResult
 *****************************************************************************/
static int 
DestroyResult (BrowseResult* const br)
{
	if (br) {
		if (br->cds && br->cds->cache)
			ithread_mutex_lock (&br->cds->cache_mutex);
		
		// Cached data will be really freed by talloc 
		// when its reference count drops to zero.
		if (talloc_free (br->children) == 0)
			Log_Printf (LOG_DEBUG, "ContentDir CACHE_FREE");
		
		if (br->cds && br->cds->cache)
			ithread_mutex_unlock (&br->cds->cache_mutex);
		
		*br = (BrowseResult) { };
	}
	
	return 0;
}


/******************************************************************************
 * BrowseOrSearchWithCache
 *****************************************************************************/
static const ContentDir_BrowseResult*
BrowseOrSearchWithCache (ContentDir* cds, void* result_context, 
			 const char* objectId, const char* const criteria)
{
	if (cds == NULL || objectId == NULL || criteria == NULL)
		return NULL; // ---------->

	BrowseResult* br = talloc (result_context, BrowseResult);
	if (br == NULL)
		return NULL; // ---------->
	*br = (BrowseResult) { .cds = cds };

	if (cds->cache == NULL) {
		/*
		 * No cache
		 */
		br->children = BrowseOrSearchAll (cds, br, objectId, criteria);
	} else {
		/*
		 * Lookup and/or update cache 
		 */   
		ithread_mutex_lock (&cds->cache_mutex);

		char key_buffer [strlen(objectId) + strlen(criteria) + 2 ];
		const char* key;
		if (criteria == CRITERIA_BROWSE_CHILDREN) {
			key = objectId;
		} else {
			// criteria == "BrowseMetadata" or Search criteria
			sprintf (key_buffer, "%s\t%s", objectId, criteria);
			key = key_buffer;
		}

		Children** cp = (Children**) Cache_Get (cds->cache, key);
		if (cp) {
			if (*cp) {
				// cache hit
				br->children = *cp;
			} else {
				// cache new (or expired)

				// Note: use the cache as parent context for 
				// allocation of result.
				br->children = BrowseOrSearchAll
					(cds, cds->cache, objectId, criteria);
				// set cache
				*cp = br->children;
			}
		}
		// Add a reference to the cached result before returning it
		if (br->children) {
			talloc_increase_ref_count (br->children);    
			talloc_set_destructor (br, DestroyResult);
		}
		
		ithread_mutex_unlock (&cds->cache_mutex);
	}

	if (br->children == NULL) {
		talloc_free (br);
		br = NULL;
	}
	
	return br;
}


/******************************************************************************
 * ContentDir_Browse
 *****************************************************************************/
const ContentDir_BrowseResult*
ContentDir_Browse (ContentDir* cds, void* result_context, 
		   const char* objectId, ContentDir_BrowseFlag browse_flag)
{
	return BrowseOrSearchWithCache
		(cds, result_context, objectId, 
		 (browse_flag == CONTENT_DIR_BROWSE_METADATA) ? 
		 CRITERIA_BROWSE_METADATA : CRITERIA_BROWSE_CHILDREN);
}


/*****************************************************************************
 * ContentDir_GetSearchCapabilities
 *****************************************************************************/
const char*
ContentDir_GetSearchCapabilities (ContentDir* self, void* unused)
{
	if (self == NULL)
		return NULL; // ---------->
	
	// Send Action if result not already cached
	if (self->search_caps == NULL) {

		IXML_Document* doc = NULL;
		int rc = Service_SendActionVa
			(OBJECT_SUPER_CAST(self), &doc,
			 "GetSearchCapabilities",
			 NULL, NULL);
		if (rc == UPNP_E_SUCCESS && doc != NULL) {
			self->search_caps = talloc_strdup 
				(self, XMLUtil_GetFirstNodeValue 
				 ((IXML_Node*) doc, "SearchCaps", true));
			
			Log_Printf (LOG_DEBUG, 
				    "ContentDir_GetSearchCapabilities = '%s'",
				    NN(self->search_caps));
		}
		ixmlDocument_free (doc);
	}
	
	return self->search_caps;
}


/*****************************************************************************
 * ContentDir_Search
 *****************************************************************************/
const ContentDir_BrowseResult*
ContentDir_Search (ContentDir* cds, void* result_context, 
		   const char* objectId, const char* criteria)
{
	Log_Printf (LOG_DEBUG, "ContentDir_Search objectId='%s' criteria='%s'",
		    NN(objectId), NN(criteria));
	return BrowseOrSearchWithCache (cds, result_context, 
					objectId, criteria);
}


/*****************************************************************************
 * get_status_string
 *****************************************************************************/
static char*
get_status_string (const Service* serv, 
		   void* result_context, bool debug, const char* spacer) 
{
	ContentDir* const cds = (ContentDir*) serv;

	// Call superclass' method
	char* p = CLASS_METHOD (Service, get_status_string)
		(serv, result_context, debug, spacer);

	// Add ContentDir specific status
	if (spacer == NULL)
		spacer = "";
	
	// Create a working context for temporary strings
	void* const tmp_ctx = talloc_new (NULL);
	
	tpr (&p, "%s+- Browse Cache\n", spacer);
	tpr (&p, "%s", Cache_GetStatusString 
	     (cds->cache, tmp_ctx, talloc_asprintf (tmp_ctx, "%s      ",
						    spacer)));
	
	// Delete all temporary strings
	talloc_free (tmp_ctx);

	return p;
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

	if (cds && cds->cache) {
		ithread_mutex_destroy (&cds->cache_mutex);
	}
	
	// Other "talloc'ed" fields will be deleted automatically : 
	// nothing to do 
}


/*****************************************************************************
 * OBJECT_INIT_CLASS
 *****************************************************************************/

// Initialize class methods
static void 
init_class (ContentDir_Class* const isa) 
{ 
	CLASS_BASE_CAST(isa)->finalize = finalize;
	CLASS_SUPER_CAST(isa)->get_status_string = get_status_string;

	// Class-specific initialization :
	// Increase maximum permissible content-length for SOAP 
	// messages, because "Browse" answers can be very large 
	// if contain lot of objects.
	UpnpSetMaxContentLength (MAX_CONTENT_LENGTH);
}

OBJECT_INIT_CLASS(ContentDir, Service, init_class);


/*****************************************************************************
 * ContentDir_Create
 *****************************************************************************/
ContentDir* 
ContentDir_Create (void* talloc_context, 
		   UpnpClient_Handle ctrlpt_handle, 
		   IXML_Element* serviceDesc, 
		   const char* base_url)
{
	OBJECT_SUPER_CONSTRUCT (ContentDir, Service_Create, talloc_context,
				ctrlpt_handle, serviceDesc, base_url);
	if (self == NULL)
		goto error; // ---------->
	
	if (CACHE_SIZE > 0 && CACHE_TIMEOUT > 0) {
		self->cache = Cache_Create (self, CACHE_SIZE, CACHE_TIMEOUT,
					    cache_free_expired_data);
		if (self->cache == NULL)
			goto error; // ---------->
		ithread_mutex_init (&self->cache_mutex, NULL);
	}
	
	return self; // ---------->
	
error:
	Log_Print (LOG_ERROR, "ContentDir_Create error");
	if (self) 
		talloc_free (self);
	return NULL;
}

