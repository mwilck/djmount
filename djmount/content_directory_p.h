/* $Id$
 *
 * UPnP Content Directory Service implementation (private / protected).
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

#include "content_directory.h"
#include "service_p.h"


/******************************************************************************
 *
 *	private / protected implementation ; do not include directly.
 *
 *****************************************************************************/

union _ContentDirectoryClass {

  ObjectClass o;

  struct {
    // Inherit parent fields
    ServiceClass _; 
    // No additional methods
  } m;

};




union _ContentDirectory {
  
  const ContentDirectoryClass* isa;

  struct {
    // Inherit parent fields
    Service _; 

    // No additional fields
  } m;
};

