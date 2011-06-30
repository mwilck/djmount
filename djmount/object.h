/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id: object.h 185 2006-06-06 20:54:31Z r3mi $
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
 * @fn		OBJECT_DECLARE_CLASS
 * @brief	Declare a new class, deriving from an existing one.
 *		Shall be used in public interface file.
 *
 * Example in "Service.h" : 
 *	OBJECT_DECLARE_CLASS(Service,Object);
 * will declare the new type "Service".
 *
 * The following public definitions are made available :
 *	typedef struct _Service Service;
 *	inline Object* Service_ToObject (Service*);
 *	inline const Object* Service_ToConstObject (const Service*);
 *	
 *****************************************************************************/

// Returns the class singleton for a given type
#define OBJECT_BASE_CLASS_PTR(OBJTYPE)	_ ## OBJTYPE ## _GetBaseClass()
#define OBJECT_CLASS_PTR(OBJTYPE)	_ ## OBJTYPE ## _GetClass()

#define _OBJECT_SUPERCLASS_PTR(OBJTYPE)	_ ## OBJTYPE ## _GetBaseSuperClass()

#define _OBJECT_DECLARE_CLASS(OBJTYPE)					\
	typedef struct _ ## OBJTYPE OBJTYPE;				\
	typedef struct _ ## OBJTYPE ## _Class OBJTYPE ## _Class;	\
	extern const OBJTYPE ## _Class* OBJECT_CLASS_PTR(OBJTYPE);	\
	extern const Object_Class* OBJECT_BASE_CLASS_PTR(OBJTYPE)	

#define OBJECT_DECLARE_CLASS(OBJTYPE,SUPERTYPE)				\
	_OBJECT_DECLARE_CLASS(OBJTYPE);					\
	typedef SUPERTYPE _ ## OBJTYPE ## _SuperType;			\
	static inline SUPERTYPE*					\
	OBJTYPE ## _To ## SUPERTYPE (OBJTYPE* obj)			\
	{ return (SUPERTYPE*) obj; }					\
	static inline const SUPERTYPE*					\
	OBJTYPE ## _ToConst ## SUPERTYPE (const OBJTYPE* obj)		\
	{ return (const SUPERTYPE*) obj; }				\
	typedef struct _ ## SUPERTYPE ## _Class _ ## OBJTYPE ## _SuperClass




/******************************************************************************
 * @var Object (topmost superclass)
 *
 *	This opaque type encapsulates access to a generic object.
 *
 *****************************************************************************/

_OBJECT_DECLARE_CLASS(Object);


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
_Object_IsA (const void* obj, const Object_Class* searched_class);

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
   _Object_IsA(OBJ, OBJECT_BASE_CLASS_PTR(TYPE)))
  


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
	(OBJ, TYPE*, OBJ,						\
	 _Object_IsA(OBJ, OBJECT_BASE_CLASS_PTR(TYPE)) ? (TYPE*)OBJ : NULL)



#ifdef __cplusplus
}; /* extern "C" */
#endif


#endif /* OBEJCT_INCLUDED */



