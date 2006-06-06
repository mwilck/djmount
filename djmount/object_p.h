/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

struct _Object_Class {
  
	uint32_t	magic;
  	const char* 	name;
	const struct _Object_Class* super;
	size_t      	size;
	const void*	initializer;

	/*
	 * Class methods / specific methods
	 */

	// Destructor : destroy specific fields if needed, or NULL.
	// All destructors are automatically chained.
	void  (*finalize) (Object*); 


	/*
	 * Object virtual methods 
	 */

	// ...

};


struct _Object {

	union {
		const Object_Class* baseclass;
		const Object_Class* isa;
	} _ ;
	
};


/*****************************************************************************
 * Define a class structure, which has been previously declared using 
 * OBJECT_DECLARE_CLASS.
 * Shall be used in private / protected implementation files.
 *
 * Example in "Service_p.h" : 
 *	OBJECT_DEFINE_STRUCT(Service,
 *		int a; // object property
 *	);
 *
 * @param OBJTYPE	object type (declared using OBJECT_DECLARE_CLASS)
 * @param FIELDS	object properties (struct fields)
 *****************************************************************************/

#define OBJECT_DEFINE_STRUCT(OBJTYPE,FIELDS)			\
struct _ ## OBJTYPE {						\
	union {							\
		const struct _Object_Class* baseclass;		\
		const struct _ ## OBJTYPE ## _Class* isa;	\
		_ ## OBJTYPE ## _SuperType super;		\
	} _ ;							\
	FIELDS							\
}


/*****************************************************************************
 * Define the methods of a class, which has been previously declared using 
 * OBJECT_DECLARE_CLASS. Must be called once, even if there is no methods.
 * Shall be used in private / protected implementation files.
 *
 * Example in "Service_p.h" : 
 *	OBJECT_DEFINE_METHODS(Service,
 *		void (*print) (FILE*); // virtual method
 *	);
 *
 * @param OBJTYPE	object type (declared using OBJECT_DECLARE_CLASS)
 * @param METHODS	object methods (function pointers)
 *****************************************************************************/

#define OBJECT_DEFINE_METHODS(OBJTYPE,METHODS)		\
struct _ ## OBJTYPE ## _Class {				\
	union {						\
		struct _Object_Class base;		\
		 _ ## OBJTYPE ## _SuperClass super;	\
	} _ ;						\
	METHODS						\
}


/*****************************************************************************
 * Initialise a class which have been previsouly defined using 
 * OBJECT_DEFINE_METHODS. The provided initialisation function can change
 * the method pointers in the ObjectClass to the desired values (else
 * the pointers inherit the superclass value, or defaults to NULL).
 *
 * Shall be called exactly once, in private implementation file (.c).
 *
 * Example in "Service.c" : 
 *      static void init_class (Service_Class* isa) 
 *	{ 
 *		CLASS_BASE_CAST(isa).finalize = finalize;
 *		isa.print = my_print_function;
 *	}
 *	OBJECT_INIT_CLASS(Service, Object, init_class);
 *
 * @param OBJTYPE	object type (declared using OBJECT_DECLARE_CLASS)
 * @param SUPERTYPE	parent type (same as in OBJECT_DECLARE_CLASS)
 * @param INITFUNCTION	initialisation function (can be NULL if nothing to do)
 *****************************************************************************/

#define OBJECT_INIT_CLASS(OBJTYPE,SUPERTYPE,INITFUNCTION)		\
const OBJTYPE ## _Class* OBJECT_CLASS_PTR(OBJTYPE)			\
{									\
	static OBJTYPE ## _Class the_class = { ._.base.size = 0 };	\
	static const OBJTYPE the_default_object =			\
		{ ._.isa = &the_class };				\
	/* Initialize non-const fields on first call */			\
	if (the_class._.base.size == 0) {				\
		_ObjectClass_Lock();					\
		if (the_class._.base.size == 0) {			\
			/* 1) Copy superclass methods */		\
			const _ ## OBJTYPE ## _SuperClass* super =	\
				OBJECT_CLASS_PTR(SUPERTYPE);		\
			the_class._.super = *super;			\
			/* 2) Overwrite specific fields */		\
			the_class._.base.name 	= #OBJTYPE;		\
			the_class._.base.super	= (Object_Class*)super;	\
			the_class._.base.size	= sizeof (OBJTYPE);	\
			the_class._.base.initializer = &the_default_object; \
			the_class._.base.finalize = NULL;		\
                        if (INITFUNCTION != NULL)			\
				INITFUNCTION(&the_class);		\
                }							\
		_ObjectClass_Unlock();					\
	}								\
	return &the_class;						\
}									\
const Object_Class* OBJECT_BASE_CLASS_PTR(OBJTYPE)			\
{									\
	return CLASS_BASE_CAST(OBJECT_CLASS_PTR(OBJTYPE));		\
}



/**
 * Get the object's class pointer
 * (no check for NULL pointer)
 */
#define OBJECT_GET_CLASS(OBJ)		((OBJ)->_.isa)

/**
 * Set the object's class pointer (internal use only)
 */
#define _OBJECT_SET_CLASS(OBJ,ISA)	(OBJ)->_.isa = ISA


/**
 * Get the object's class name
 */
#define OBJECT_GET_CLASS_NAME(OBJ)	\
	(((OBJ) && (OBJ)->_.baseclass) ? (OBJ)->_.baseclass->name : NULL)



/**
 * Cast a derived object pointer to its superclass
 * (no check for NULL pointer)
 */
#define OBJECT_SUPER_CAST(OBJ)		((OBJ) ? &((OBJ)->_.super) : NULL)


/**
 * Cast a derived class pointer to its superclass
 * (no check for NULL pointer)
 */
#define CLASS_SUPER_CAST(ISA)		(&((ISA)->_.super))


/**
 * Cast a class pointer to the base Object_Class
 * (no check for NULL pointer)
 */
#define CLASS_BASE_CAST(ISA)		(&((ISA)->_.base))


/*
 * Return the pointer to a virtual method
 */
#define OBJECT_METHOD(OBJ,METHOD) \
	( ((OBJ) && (OBJ)->_.isa) ? (OBJ)->_.isa->METHOD : NULL)

#define CLASS_METHOD(OBJTYPE,METHOD) \
	( OBJECT_CLASS_PTR(OBJTYPE)->METHOD)


/*****************************************************************************
 * _ObjectClass_Lock
 * _ObjectClass_Unlock
 *	Allow to safely initilize the state of static 'Class' singletons
 *	in multi-threaded environements.
 *****************************************************************************/
int _ObjectClass_Lock();
int _ObjectClass_Unlock();



/******************************************************************************
 * @fn  Object_Create
 *
 * 	Base constructor (normally called through OBJECT_SUPER_CONSTRUCT).
 *	The returned object can be destroyed with "talloc_free".
 *
 * @param parent_context	the talloc parent context
 * @param unused		unused parameter (needed for 
 *				OBJECT_SUPER_CONSTRUCT VA_ARGS syntax)
 *
 *****************************************************************************/

Object* Object_Create (void* parent_context, void* unused);


/******************************************************************************
 * @fn  OBJECT_SUPER_CONSTRUCT
 *
 *	Check that an object is not yet allocated, and alloctate if 
 *	necessary with "talloc". 
 *	The object data is initialized to default values.
 *	The returned object can be destroyed with "talloc_free".
 *	This macro allow to simply chain constructors, for example if
 *	B is derived from A derived from Object:
 *	
 *	A* A_Create (void* context, a) 
 *	{
 *		OBJECT_SUPER_CONSTRUCT (A, Object_Create, context, NULL);
 *		if (self != NULL) {
 *			if (some_check_is_ok)
 *				self->a = a;	
 *			else
 *				goto error;
 *		}
 *		return self;
 *	    error:
 *		talloc_free (self); 
 *		return NULL;
 *	}
 *
 *	B* B_Create (void* ctx, a, b) 
 *	{
 *		OBJECT_SUPER_CONSTRUCT (B, A_Create, context, a);
 *		if (self != NULL) {
 *			self->b = b;
 *		}
 *		return b;
 *	}
 *
 * @param OBJTYPE	object type (declared using OBJECT_DECLARE_CLASS)
 * @param SUPERCONS	name of the superclass constructor
 * @param CTX		talloc parent context (1st constructor parameter)
 * @param ...		other parameters for the superclass constructor 
 *			(at least 1)
 *
 *****************************************************************************/

Object*
_Object_check_alloc (void* talloc_context, const Object_Class* isa);


#define OBJECT_SUPER_CONSTRUCT(OBJTYPE,SUPERCONS,CTX,...)	\
	OBJTYPE* self = (OBJTYPE*) _Object_check_alloc		\
		(CTX, OBJECT_BASE_CLASS_PTR(OBJTYPE));		\
	if (self != NULL) {					\
		_ ## OBJTYPE ## _SuperType *__super =		\
			SUPERCONS (self, __VA_ARGS__);		\
		if (__super == NULL) {				\
			Log_Print (LOG_ERROR, #OBJTYPE		\
				   ": error in " #SUPERCONS);	\
			self = NULL; /* already freed */	\
		}						\
	}							\
	(void) self



#endif /* OBJECT_P_INCLUDED */





