/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * Testing String utilities.
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

#include "string_util.h"
#include <stdio.h>
#include "talloc_util.h"
#include <stdint.h>


#undef NDEBUG
#include <assert.h>


static void	
test_string_to_int()
{
	int8_t i8 = 99;
	int32_t i32 = 99;
	uint32_t u32 = 99;
	intmax_t imax = 99;
	char buffer [64];

	STRING_TO_INT("xx", i8, -1);
	assert (i8 == -1);

	STRING_TO_INT("127", i8, -1);
	assert (i8 == 127);
	
	STRING_TO_INT("128", i8, -1);
	assert (i8 == -1);
	
	STRING_TO_INT("  -0", i8, -1);
	assert (i8 == 0);

	STRING_TO_INT("\t+77", i8, -1);
	assert (i8 == 77);

	STRING_TO_INT("  88 \t", i8, -1);
	assert (i8 == 88);

	STRING_TO_INT(" 99 100", i32, -1);
	assert (i32 == -1);

	STRING_TO_INT("128", i32, -1);
	assert (i32 == 128);

	STRING_TO_INT("128", u32, 0);
	assert (u32 == 128);

	STRING_TO_INT("-128", u32, 0);
	assert (u32 == 0);

	sprintf (buffer, "%" PRIdMAX, (intmax_t) INT32_MAX);

	STRING_TO_INT(buffer, i32, -1);
	assert (i32 == INT32_MAX);

	STRING_TO_INT(buffer, i8, -1);
	assert (i8 == -1);

	STRING_TO_INT(buffer, u32, -1);
	assert (u32 == INT32_MAX);

	sprintf (buffer, "%" PRIdMAX, INTMAX_C(0x80000000));

	STRING_TO_INT(buffer, i32, -1);
	assert (i32 == -1);

	STRING_TO_INT(buffer, u32, -1);
	assert (u32 == INTMAX_C(0x80000000));

	sprintf (buffer, "%" PRIdMAX, (intmax_t) INT32_MIN);

	STRING_TO_INT(buffer, i32, -1);
	assert (i32 == INT32_MIN);

	STRING_TO_INT(buffer, u32, 0);
	assert (u32 == 0);

	sprintf (buffer, "%" PRIdMAX, (intmax_t) INTMAX_MAX);
	
	STRING_TO_INT(buffer, imax, -1);
	assert (imax == INTMAX_MAX);
}

static void
test_string_stream()
{
	size_t slen = 0;
	char* s = NULL;

	// Create a working context for memory allocations
	void* const ctx = talloc_new (NULL);
	
	StringStream* ss1 = StringStream_Create (ctx);
	assert (ss1 != NULL);

	FILE* f1 = StringStream_GetFile (ss1);
	assert (f1 != NULL);

	fprintf (f1, "Hell%c ", 'o');
	fprintf (f1, "World!\n");
	s = StringStream_GetSnapshot (ss1, ctx, NULL);
	assert (s != NULL);
	assert (strcmp (s, "Hello World!\n") == 0);

	StringStream* ss2 = StringStream_Create (ctx);
	assert (ss2 != NULL);

	FILE* f2 = StringStream_GetFile (ss2);
	assert (f2 != NULL);

	fprintf (f2, "Hello %d", 1);
	s = StringStream_GetSnapshot (ss2, ctx, &slen);
	assert (s != NULL);
	assert (strcmp (s, "Hello 1") == 0);
	assert (slen == strlen (s));

	fprintf (f2, " Hello %d", 2);
	s = StringStream_GetSnapshot (ss2, ctx, &slen);
	assert (s != NULL);
	assert (strcmp (s, "Hello 1 Hello 2") == 0);
	assert (slen == strlen (s));

	talloc_free (ss1);
	talloc_free (ss2);

	// Delete all storage
	talloc_free (ctx);
}


int 
main(int argc, char * argv[])
{
	talloc_enable_leak_report();

	test_string_to_int();
	test_string_stream();

	int bytes = talloc_total_size (NULL);
	assert (bytes == 0);
	
	exit (0);
}



