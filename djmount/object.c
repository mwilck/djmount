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

#include "object_p.h"

#include <string.h>
#include <stdarg.h>	/* missing from "talloc.h" */
#include <stdio.h>
#include <stdbool.h>
#include <upnp/ithread.h>

#include "log.h"

#include <talloc.h>



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
destroy (void* ptr)
{
  if (ptr) {
    /*
     * Chain all 'finalize' functions
     */
    Object* const obj = (Object*) ptr;
    const ObjectClass* objclass = obj->isa;
    while (objclass) {
      if (objclass->finalize)
	objclass->finalize (obj);  
      objclass = objclass->super;
    }

    // Reset all data to 0 
    if (obj->isa && obj->isa->initializer)
      memcpy (obj, obj->isa->initializer, obj->isa->size);

    // Delete type information
    obj->isa = NULL;
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
_Object_IsA (const Object* const obj, const ObjectClass* searched_class)
{
  if (obj && searched_class) {
    register const ObjectClass* objclass = obj->isa;

    if (objclass == NULL || objclass->magic != CLASS_MAGIC) {
      Log_Printf (LOG_ERROR, 
		  "Object_IsA : not an object ; memory might be corrupted !!");
      return false; // ---------->
    }
    
#if 0
    Log_Printf (LOG_DEBUG, "Object_IsA : '%s' isa '%s' ?", 
                NN(objclass->name), NN(searched_class->name));
#endif

    while (objclass) {
      if (objclass == searched_class)
	return true; // ---------->
      objclass = objclass->super;
    }  

    Log_Printf (LOG_ERROR, "Object_IsA : '%s' is not a '%s' ", 
		NN(obj->isa->name), NN(searched_class->name));
  }
  return false;
}

/*****************************************************************************
 * _ObjectClass_Get
 *****************************************************************************/

const ObjectClass* OBJECT_CLASS_PTR(Object)
{
  static ObjectClass the_class = {
    .magic	        = CLASS_MAGIC,
    .name 	        = "Object",
    .super	        = NULL,
    .size	        = sizeof (Object),
    .initializer 	= NULL,
    .finalize		= NULL,
  };
  static const Object the_default_object = { .isa = &the_class };

  if (the_class.initializer == NULL) {
    _ObjectClass_Lock();
    the_class.initializer = &the_default_object;
    _ObjectClass_Unlock();
  }

  return &the_class;
}


/*****************************************************************************
 * _Object_talloc
 *****************************************************************************/
Object*
_Object_talloc (void* talloc_context, const ObjectClass* isa)
{
  if (isa == NULL) {
    Log_Print (LOG_ERROR, "Object CreateBase NULL isa class");
    return NULL; // ---------->
  }

  Object* obj = talloc_named_const (talloc_context, isa->size, isa->name);
  if (obj == NULL) {
    Log_Print (LOG_ERROR, "Object CreateBase Out of Memory");
    return NULL; // ---------->
  }

  // Init fields to default values
  if (isa->initializer)
    memcpy (obj, isa->initializer, isa->size);
  obj->isa = isa;

  // Register destructor
  talloc_set_destructor (obj, destroy);

  return obj;
}

