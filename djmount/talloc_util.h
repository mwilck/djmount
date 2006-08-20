/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * "talloc" utilities.
 * This file is also a wrapper around "talloc.h", so that it can be 
 * included directly without errors due to missing definitions 
 * (missing #include in "talloc.h").
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


#ifndef TALLOC_UTIL_H_INCLUDED
#define TALLOC_UTIL_H_INCLUDED


/* those include are currently missing from "talloc.h" */
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STDARG_H
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


#include "talloc.h"
#include <stdbool.h>


/*****************************************************************************
 * This function is equivalent to "talloc_asprintf_append" but has a different
 * prototype : the given string is in/out parameter (as in "asprintf"),
 * and the function returns true if ok (false if failure).
 *****************************************************************************/
bool
tpr (char** s, const char* fmt, ...) PRINTF_ATTRIBUTE(2,3);




#ifdef __cplusplus
}; // extern "C"
#endif


#endif /* TALLOC_UTIL_H_INCLUDED */





