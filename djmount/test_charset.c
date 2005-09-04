/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * Testing of charset conversions.
 * This file is part of djmount.
 *
 * (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
 *
 * Part derived from libupnp example (libupnp/upnp/sample/tvctrlpt/linux/...)
 * Copyright (c) 2000-2003 Intel Corporation
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
 

#include "charset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "talloc.h"
#include <ctype.h>


/*****************************************************************************
 * dump
 *****************************************************************************/
void
dump (const unsigned char* ptr)
{
	int n = 0;
	while (*ptr) {
		if (n >= 16) {
			printf ("\n");
			n = 0;
		}
		printf (" %.2x(%c)", (int) *ptr, 
			(isprint(*ptr) ? *ptr : ' '));
		ptr++;
		n++;
	}
	printf ("\n");
}

/*****************************************************************************
 * Usage
 *****************************************************************************/
void
Usage (const char* progname) 
{
	fprintf (stderr, "Usage : %s [charset]\n", progname);
	exit(1);
}

/*****************************************************************************
 * main
 *****************************************************************************/
int
main (int argc, char** argv)
{
	talloc_enable_leak_report();
	
	if ( argc > 2 || (argc == 2 && strcmp (argv[1], "-help") == 0) ) {
		Usage (argv[0]);
	}
	const char* const charset = (argc > 1 ? argv[1] : NULL);
	int rc = Charset_Initialize (charset);
	if (rc) {
		fprintf (stderr, "Error initialising charset %s\n",
			 charset ? charset : "(default locale)");
		Usage (argv[0]);
	}

	while (1) {
		char cmdline [BUFSIZ];
		printf ("\n>> " );
		if (fgets (cmdline, sizeof (cmdline), stdin) == NULL)
			break; // ---------->
		if (*cmdline) {  
			// remove final '\n'
			size_t cmdlen = strlen(cmdline);
			if (cmdline[cmdlen-1] == '\n') 
				cmdline[--cmdlen] = '\0';

			printf ("len = %d\n", cmdlen);

			// Create a working context for temporary allocations
			void* tmp_ctx = talloc_new (NULL);


			printf ("\n1) ConvertString to UTF8 \n");
			char buffer1 [40];
			char* result1 = Charset_ConvertString 
				(CHARSET_TO_UTF8, cmdline, 
				 buffer1, sizeof(buffer1), tmp_ctx);
			if (result1) {
				dump (result1);
				printf ("len = %d\n", strlen(result1));
				if (result1 == buffer1)
					printf ("allocated on stack\n");
				else
					printf ("allocated by talloc\n");
			} else {
				printf (" ** NULL ! **\n");
			}


			printf ("\n2) PrintString UTF8 \n");
			rc = Charset_PrintString (CHARSET_FROM_UTF8, result1, 
						  stdout);
			if (rc < 0) 
				printf (" *** error printing string ***\n");
			printf ("\n");


			printf ("\n3) ConvertString back from UTF8 \n");
			char buffer2 [40];
			char* result2 = Charset_ConvertString 
				(CHARSET_FROM_UTF8, result1,
				 buffer2, sizeof (buffer2), tmp_ctx);
			if (result2) {
				printf ("str = '%s'\n", result2);
				dump (result2);
				printf ("len = %d\n", strlen(result2));
				if (result2 == buffer2)
					printf ("allocated on stack\n");
				else
					printf ("allocated by talloc\n");
			} else {
				printf (" ** NULL ! **\n");
			}	


			printf ("\n4) Compare strings \n");
			if (result2 && strcmp (cmdline, result2) == 0)
				printf (" ok\n");
			else
				printf (" *** error, differ ! *** \n");


			// Delete all temporary storage
			talloc_free (tmp_ctx);
			tmp_ctx = NULL;
		}
	}

	Charset_Finish();
	exit (0);
}
 
