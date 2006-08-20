/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * Object base class.
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

#include "object_p.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <upnp/ithread.h>

#include "log.h"
#include "talloc_util.h"



/*
 * Mutex to control initialisation of static 'Class' singletons
 * in multi-threaded environements (must be recursive because one 
 * class initialisation chains into its superclass initialisation).
 */
static bool g_mutex_initialized = false;
static pthread_mutex_t g_class_mutex;


// Magic number for sanity checks
#define CLASS_MAGIC 	0xacebabe


/******************************************************************************
 * destroy
 *
 * Description: 
 *	Object destructor, automatically called by "talloc_free".
 *
 *****************************************************************************/
static int
destroy (Object* const obj)
{
	if (obj) {
		/*
		 * Chain all 'finalize' functions
		 */
		const Object_Class* const objclass = OBJECT_GET_CLASS(obj);
		const Object_Class* c = objclass;
		while (c) {
			if (c->finalize)
				c->finalize (obj);  
			c = c->super;
		}
		
		// Reset all data to 0 
		if (objclass && objclass->initializer)
			memcpy (obj, objclass->initializer, objclass->size);
		
		// Delete type information
		_OBJECT_SET_CLASS (obj, NULL);
	}
	return 0; // 0 -> ok for talloc to deallocate memory
}


/*****************************************************************************
 * _ObjectClass_Lock
 *****************************************************************************/
int
_ObjectClass_Lock()
{
  if (! g_mutex_initialized) {
    ithread_mutexattr_t attr;
    
    ithread_mutexattr_init (&attr);
    ithread_mutexattr_setkind_np (&attr, ITHREAD_MUTEX_RECURSIVE_NP );
    ithread_mutex_init (&g_class_mutex, &attr);
    ithread_mutexattr_destroy (&attr);
    g_mutex_initialized = true;
  }
  return pthread_mutex_lock (&g_class_mutex);
}

/*****************************************************************************
 * _ObjectClass_Unlock
 *****************************************************************************/
inline int
_ObjectClass_Unlock()
{
  return pthread_mutex_unlock (&g_class_mutex);
}


/*****************************************************************************
 * _Object_IsA
 *****************************************************************************/
bool
_Object_IsA (const void* const objptr, const Object_Class* searched_class)
{
	if (objptr && searched_class) {
		const Object* const obj = (const Object*) objptr;
		const Object_Class* const objclass = OBJECT_GET_CLASS(obj);

		if (objclass == NULL || objclass->magic != CLASS_MAGIC) {
			Log_Printf (LOG_ERROR, 
				    "Object_IsA : not an object ; "
				    "memory might be corrupted !!");
			return false; // ---------->
		}
    
#if 0
		Log_Printf (LOG_DEBUG, "Object_IsA : '%s' isa '%s' ?", 
			    NN(objclass->name), NN(searched_class->name));
#endif

		register const Object_Class* c = objclass;
		while (c) {
			if (c == searched_class)
				return true; // ---------->
			c = c->super;
		}  
		
		Log_Printf (LOG_ERROR, "Object_IsA : '%s' is not a '%s' ", 
			    NN(objclass->name), NN(searched_class->name));
	}
	return false;
}

/*****************************************************************************
 * OBJECT_CLASS_PTR(Object)
 *****************************************************************************/

const Object_Class* OBJECT_CLASS_PTR(Object)
{
	static Object_Class the_class = {
		.magic	        = CLASS_MAGIC,
		.name 	        = "Object",
		.super	        = NULL,
		.size	        = sizeof (Object),
		.initializer 	= NULL,
		.finalize	= NULL,
	};
	static const Object the_default_object = { ._.isa = &the_class };
	
	if (the_class.initializer == NULL) {
		_ObjectClass_Lock();
		the_class.initializer = &the_default_object;
		_ObjectClass_Unlock();
	}
	
	return &the_class;
}

const Object_Class* OBJECT_BASE_CLASS_PTR(Object)
{					
	return OBJECT_CLASS_PTR(Object);
}



/*****************************************************************************
 * _Object_check_alloc
 *****************************************************************************/

static const char* const NAME_UNDER_CONSTRUCTION = "under construction";

Object*
_Object_check_alloc (void* talloc_context, const Object_Class* isa)
{
	/*
	 * 1) check if we have a "real" context, or an object already 
	 *    allocated and under construction.
	 */
	if (talloc_get_name (talloc_context) == NAME_UNDER_CONSTRUCTION) {
		return talloc_context; // ---------->
	} 

	/*
	 * 2) Allocate a new object
	 */

	if (isa == NULL) {
		Log_Print (LOG_ERROR, "Object CreateBase NULL isa class");
		return NULL; // ---------->
	}
	
	Object* obj = talloc_named_const (talloc_context, isa->size, 
					  NAME_UNDER_CONSTRUCTION);
	if (obj == NULL) {
		Log_Print (LOG_ERROR, "Object CreateBase Out of Memory");
		return NULL; // ---------->
	}

	// Init fields to default values
	if (isa->initializer)
		memcpy (obj, isa->initializer, isa->size);
	_OBJECT_SET_CLASS (obj, isa);
	
	// Register destructor
	talloc_set_destructor (obj, destroy);
	
	return obj;
}

/*****************************************************************************
 * Object_Create
 *****************************************************************************/

Object* 
Object_Create (void* parent_context, void* unused)
{
	(void) unused;

	Object* self = _Object_check_alloc (parent_context, 
					    OBJECT_BASE_CLASS_PTR (Object));
	
	// Finalize construction
	if (self != NULL) 
		talloc_set_name_const (self, OBJECT_GET_CLASS_NAME (self));
		
	return self;
}


