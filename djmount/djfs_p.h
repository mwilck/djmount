/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * djfs : file system implementation for djmount
 * (private / protected part).
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

#ifndef DJFS_P_INCLUDED
#define DJFS_P_INCLUDED 1

#include "djfs.h"
#include "vfs_p.h"



/******************************************************************************
 *
 *      DJFS private / protected implementation ; do not include directly.
 *
 *****************************************************************************/

OBJECT_DEFINE_STRUCT(DJFS,
		     
		     DJFS_Flags flags;
		     
                     );


OBJECT_DEFINE_METHODS(DJFS,
		      
		      // No additional methods

                      );


#endif // DJFS_P_INCLUDED


