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

#ifndef OBJECT_INCLUDED
#define OBJECT_INCLUDED 1

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 * @var Object
 *
 *	This opaque type encapsulates access to a generic object.
 *
 *****************************************************************************/

struct _Object;
typedef struct _Object Object;



/******************************************************************************
 * @fn		OBJECT_DECLARE_CLASS
 * @brief	Declare a new class, deriving from an existing one.
 *		Shall be used in public interface file.
 *
 * Example in "Service.h" : 
 *	OBJECT_DECLARE_CLASS(Service,Object);
 * will declare the new type "Service".
 *
 *****************************************************************************/

// Returns the class singleton for a given type
#define OBJECT_CLASS_PTR(TYPE)	_ ## TYPE ## Class_Get()

typedef struct _ObjectClass ObjectClass;
extern const ObjectClass* OBJECT_CLASS_PTR(Object);


#define OBJECT_DECLARE_CLASS(OBJTYPE,SUPERTYPE)			\
typedef union _ ## OBJTYPE OBJTYPE;				\
typedef union _  ## OBJTYPE ## Class OBJTYPE ## Class;		\
extern const OBJTYPE ## Class* OBJECT_CLASS_PTR(OBJTYPE)



/******************************************************************************
 * @fn		OBJECT_IS_A
 * @brief	Dynamic type checking	
 *
 * Example : 
 *	Object*  obj  = ...;
 *	if (OBJECT_IS_A(obj, Service)) { ... }
 *
 *****************************************************************************/

extern bool
_Object_IsA (const Object* obj, const ObjectClass* searched_class);

// GCC-specific optimisations for compile-time checks :
// checks if object already of the target type
#ifdef __GNUC__
#  define _OBJECT_IF_COMPILE_CHECK_TYPE(OBJ,TYPE,THEN,ELSE)	\
  __builtin_choose_expr						\
  (__builtin_types_compatible_p(typeof(OBJ), TYPE), THEN, ELSE)
#else
#  define _OBJECT_IF_COMPILE_CHECK_TYPE(OBJ,TYPE,THEN,ELSE) ELSE
#endif


#define OBJECT_IS_A(OBJ,TYPE) _OBJECT_IF_COMPILE_CHECK_TYPE	\
  (OBJ, TYPE*, true,						\
   _Object_IsA((const Object*) OBJ,				\
	       (const ObjectClass*) OBJECT_CLASS_PTR(TYPE)))
  


/******************************************************************************
 * @fn		OBJECT_DYNAMIC_CAST
 * @brief	Dynamic type casting
 *
 * Example : 
 *	Object*  obj  = xx;
 *	Service* serv = OBJECT_DYNAMIC_CAST(obj, Service)
 *
 *****************************************************************************/

#define OBJECT_DYNAMIC_CAST(OBJ,TYPE) _OBJECT_IF_COMPILE_CHECK_TYPE	\
  (OBJ, TYPE*, OBJ,							\
   _Object_IsA((const Object*) OBJ,					\
	       (const ObjectClass*) OBJECT_CLASS_PTR(TYPE)) ? (TYPE*)OBJ :NULL)



#ifdef __cplusplus
}; /* extern "C" */
#endif


#endif /* OBEJCT_INCLUDED */



