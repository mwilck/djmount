/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * djfs : file system implementation for djmount.
 * This file is part of djmount.
 *
 * (C) Copyright 2005-2006 Rémi Turboult <r3mi@users.sourceforge.net>
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



#ifndef DJFS_H_INCLUDED
#define DJFS_H_INCLUDED 1

#include <fuse.h>
#include "vfs.h"



/******************************************************************************
 * @var DJFS
 *      This opaque type encapsulates access to a djfs file system.
 *      This type is derived from the "VFS" type : all VFS_* methods
 *      can be used on DJFS.
 *
 *****************************************************************************/

OBJECT_DECLARE_CLASS(DJFS,VFS);



/*****************************************************************************
 * Flags for DJFS_Create
 *****************************************************************************/
typedef enum _DJFS_Flags {
	
	DJFS_SHOW_DEBUG    = 001, // show debug directory
	DJFS_USE_PLAYLISTS = 002, // use playlists for AV files (default:files)
	DJFS_SHOW_METADATA = 004, // show XML files containing DIDL metadata

} DJFS_Flags;


/*****************************************************************************
 * @fn 		DJFS_Create
 * @brief	create a djfs file system object.
 *
 * @param flags		     	see DJFS_Flags
 * @param search_history_size	size of search history, set to 0 to disable 
 *				"search" sub-directories
 *****************************************************************************/
DJFS*
DJFS_Create (void* talloc_context, DJFS_Flags flags, 
	     size_t search_history_size);



#endif // DJFS_H_INCLUDED

