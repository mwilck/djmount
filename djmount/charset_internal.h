/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * charset_internal : 
 *	standalone character set conversions (do not require iconv)
 *	between UTF-8 <-> 1-byte charsets / codepages.
 *
 * File "charset.h" copied from "siefs" v0.5 
 * ( http://chaos.allsiemens.com/siefs/ )
 * (C) Dmitry Zakharov aka ChaoS <dmitry-z@mail.ru>
 *
 * Modified for djmount (C) 2005, Rémi Turboult <r3mi@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef CHARSET_INTERNAL_H_INCLUDED
#define CHARSET_INTERNAL_H_INCLUDED

#include <stddef.h>

int init_charset (const char *name);

size_t utf2ascii_size (const char* src);
int utf2ascii (const char** inbuf, size_t* inbytesleft,
	       char** outbuf, size_t* outbytesleft);

size_t ascii2utf_size (const char* src);
int ascii2utf (const char** inbuf, size_t* inbytesleft,
	       char** outbuf, size_t* outbytesleft);

#endif


