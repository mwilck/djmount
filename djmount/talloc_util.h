/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * "talloc" utilities.
 * This file is also a wrapper around "talloc.h", so that it can be 
 * included directly without errors due to missing definitions 
 * (missing #include in "talloc.h").
 *
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
#include <sys/types.h>		// Import "off_t"


#ifdef __cplusplus
extern "C" {
#endif


#include "talloc.h"


#ifdef __cplusplus
}; // extern "C"
#endif


#endif /* TALLOC_UTIL_H_INCLUDED */





