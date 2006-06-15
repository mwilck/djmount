/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * vfs : virtual file system implementation for djmount.
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



#ifndef VFS_H_INCLUDED
#define VFS_H_INCLUDED 1

#include "object.h"
#include <fuse.h>
#include "file_buffer.h"


/******************************************************************************
 * @var VFS
 *      This opaque type encapsulates access to a virtual file system.
 *
 *      NOTE THAT THE FUNCTION API IS NOT THREAD SAFE. Callers should
 *      take care of the necessary locks if an object is shared between
 *      multiple threads.
 *
 *****************************************************************************/

OBJECT_DECLARE_CLASS(VFS, Object);



/*****************************************************************************
 * @typedef	VFS_Query
 * @brief	query parameters for browse operations on the file system.
 *
 * 	This query groups all parameters for the 'browse' operations : 
 *	"stat", "get_dir", "read" and "readlink".
 *	The corresponding fields must be set to non-NULL to use a given 
 *	operation. It is possible to use several operation simultaneously 
 *	e.g. stat + read.
 *
 *****************************************************************************/

typedef struct _VFS_Query {

	/*
	 * for all operation : the original path to browse
	 */
	const char* path; 

	/*
	 * STAT 
	 */
	struct stat* stbuf; 
	
	/* 
	 * GETDIR 
	 */
	void* h; 
	fuse_dirfil_t filler; 

	/*
	 * READ
	 */
	void* talloc_context;
	FileBuffer** file;
	
	/*
	 * READLINK
	 */
	char* buffer;
	size_t bufsiz;

} VFS_Query;



/*****************************************************************************
 * @fn 		VFS_Create
 * @brief	create a virtual file system object.
 *
 *****************************************************************************/
VFS*
VFS_Create (void* talloc_context, bool show_debug_dir);


/*****************************************************************************
 * @fn 		VFS_Browse
 * @brief	browse the virtual file system.
 *
 * 	The 'browse' function allows to describe the filesystem structure
 *      into only one place, and groups the FUSE operations.
 *
 * @param self		the VFS object
 * @param query		query parameters
 * @return 		0 if success, or -errno if error.
 *
 *****************************************************************************/
int
VFS_Browse (VFS* self, const VFS_Query* query);



#endif // VFS_H_INCLUDED

