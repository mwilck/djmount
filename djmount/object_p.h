/* $Id$
 *
 * Object base class implementation (private / protected).
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

#ifndef OBJECT_P_INCLUDED
#define OBJECT_P_INCLUDED 1


#include "object.h"
#include <stdlib.h>
#include <inttypes.h>


/******************************************************************************
 *
 * Object private / protected implementation ; do not include directly.
 *
 *****************************************************************************/

struct _ObjectClass {
  
  uint32_t	magic;
  
  const char* 	name;

  const struct _ObjectClass* super;

  size_t      	size;

  const void*	initializer;

  /*
   * Class methods / specific methods
   */

  void  (*finalize) (Object*); // Automatically chained

  /*
   * Object virtual methods 
   */

  // ...

};


struct _Object {

  const ObjectClass* isa;

};





/*
 * Cast a derived object pointer to its superclass
 */
#define SUPER_CAST(O)	((O) ? &((O)->m._) : NULL)


/*****************************************************************************
 * _Object_LockClasses
 * _Object_UnlockClasses
 *	Allow to safely initilize the state of static 'Class' singletons
 *	in multi-threaded environements.
 *****************************************************************************/
int _ObjectClass_Lock();
int _ObjectClass_Unlock();


/******************************************************************************
 *
 * @fn  _OBJECT_TALLOC
 *
 * 	Allocate an object (or any derived class) with "talloc".
 *	The object data is not initialized : all derived classes 
 * 	shall be constructed afterwards.
 *	The returned object can be destroyed with "talloc_free".
 *
 * @param CTX		the talloc parent allocation context
 * @param TYPE		the C type e.g. Service
 *
 *****************************************************************************/

Object*
_Object_talloc (void* talloc_context, const ObjectClass* isa);

#define _OBJECT_TALLOC(CTX,TYPE)	\
  (TYPE*) _Object_talloc(CTX, &( OBJECT_CLASS_PTR(TYPE)->o ))


#endif /* OBJECT_P_INCLUDED */





