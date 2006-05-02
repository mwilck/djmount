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


int 
main(int argc, char * argv[])
{
	talloc_enable_leak_report();
	
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

	int bytes = talloc_total_size (NULL);
	assert (bytes == 0);
	
	exit (0);
}



