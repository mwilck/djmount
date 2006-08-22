/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * Testing Device object.
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
 
#include <config.h>

#include "device.h"
#include <stdio.h>
#include "talloc_util.h"
#include "log.h"


#undef NDEBUG
#include <assert.h>



static void
stdout_print (Log_Level level, const char* const msg)
{
	Log_BeginColor (level, stdout);
	if (level == LOG_ERROR)
		printf ("[E] ");
	else 
		printf ("[%d] ", (int) level);
	
	puts (msg);
	Log_EndColor (level, stdout);
}


int 
main (int argc, char * argv[])
{
	talloc_enable_leak_report();

	(void) Log_Initialize (stdout_print);
	Log_Colorize (true);
	Log_SetMaxLevel (LOG_ERROR);

	// Manually initialise UPnP logs because UpnpInit() is not called
	assert (InitLog() == UPNP_E_SUCCESS);

	char buffer [10 * 1024];
	char* const descDocText = fgets (buffer, sizeof (buffer), stdin);
	assert (descDocText != NULL);
	
	// Makes the XML parser more tolerant to malformed text
	ixmlRelaxParser ('?');

	// Test Device creation
	Device* dev = Device_Create (NULL, (UpnpClient_Handle) -1, 
				     "http://test.url", descDocText);
	assert (dev != NULL);

	char* s = Device_GetStatusString (dev, dev, true);
	assert (s != NULL && strlen (s) > 1);
	puts (s);

	talloc_free (dev);
	dev = NULL;

	size_t bytes = talloc_total_size (NULL);
	assert (bytes == 0);
	
	exit (0);
}



