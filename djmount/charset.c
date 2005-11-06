/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * charset : charset management for djmount.
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

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "charset.h"

#include "log.h"
#include "talloc_util.h"
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <inttypes.h>
#include "minmax.h"

#ifdef HAVE_LOCALE_H
#	include <locale.h>
#endif
#ifdef HAVE_LANGINFO_H
#	include <langinfo.h>
#endif

#ifdef HAVE_ICONV
#	include <iconv.h>
#	include <upnp/ithread.h>
#else
#	include "charset_internal.h"
#endif


static enum { 
	NOT_INITIALIZED = 0,
	INITIALIZED_UTF8,
	INITIALIZED_NOT_UTF8

} g_state = NOT_INITIALIZED;


#ifdef HAVE_ICONV

typedef struct _Converter {
	iconv_t cd;
	ithread_mutex_t mutex;
	bool const to_utf8;
} Converter;
static Converter g_converters[] = {
	[CHARSET_TO_UTF8]   = { .cd = (iconv_t) -1, .to_utf8 = true },
	[CHARSET_FROM_UTF8] = { .cd = (iconv_t) -1, .to_utf8 = false },
};

#else

typedef const struct _Converter {
	size_t (*size) (const char* src);
	int (*convert) (const char** inbuf, size_t* inbytesleft,
			char** outbuf, size_t* outbytesleft);
} Converter;
static Converter g_converters[] = {
	[CHARSET_TO_UTF8]   = { ascii2utf_size, ascii2utf },
	[CHARSET_FROM_UTF8] = { utf2ascii_size, utf2ascii },
};

#endif

static const int NB_CONVERTERS =  sizeof(g_converters)/sizeof(g_converters[0]);




/*****************************************************************************
 * Charset_Initialize
 *****************************************************************************/
int
Charset_Initialize (const char* charset)
{
	if (g_state != NOT_INITIALIZED)
		return EALREADY; // ---------->

	/*
	 * charset selection:
	 * a) if charset is provided, use this one ;
	 * b) else, use charset according to the current desktop settings
	 *    (e.g. KDE or GNOME, see below) ;
	 * c) else, use charset according to the current locale (same as 
	 *    "locale charmap" command) ;
	 * d) if everything fails, stay in UTF-8 i.e. no conversions.
	 *
	 *
	 * Specific desktop environment settings :
	 * ( http://mirror.hamakor.org.il/archives/linux-il/01-2004/7759.html )
	 *
	 * GTK+ 1 / GNOME 1 :
	 *	GTK+ 1 will treat filenames according to the locale. If you 
	 *	use a UTF-8 locale (check with "locale charmap"), GTK+ 1 will 
	 *	consider the file names on disk to be in UTF-8 charset. 
	 *
	 * KDE :
	 *	KDE will treat filenames according to the locale. You can 
	 *	force UTF8 file names despite a non-UTF8 locale by setting 
	 *	the KDE_UTF8_FILENAMES environment variable : see
	 *	http://wiki.kde.org/tiki-index.php?page=Environment%20Variables
	 *
	 * GTK+ 2 / GNOME 2 : 
	 *	GTK+ 2 considers file names to be always in UTF-8 encoding, 
	 *	unless the G_FILENAME_ENCODING or G_BROKEN_FILENAMES 
	 *	environment variables are set : see
	 * 	http://developer.gnome.org/doc/API/2.0/glib/glib-running.html
	 *
	 *	G_FILENAME_ENCODING : this environment variable can be set to 
	 *	a comma-separated list of character set names. GLib assumes 
	 *	that filenames are encoded in the first character set from 
	 *	that list rather than in UTF-8. The special token "@locale"
	 *	can be used to specify the character set for the current 
	 *	locale. 
	 *
	 *	G_BROKEN_FILENAMES : if this environment variable is set, 
	 *	GLib assumes that filenames are in the locale encoding rather 
	 *	than in UTF-8.
	 *	G_FILENAME_ENCODING takes priority over G_BROKEN_FILENAMES. 
	 *
	 */

	char buffer [128] = "";
	// b) find if specific desktop settings
	if (charset == NULL || *charset == 0) {
		char* s = getenv ("KDE_UTF8_FILENAMES");
		if (s) {
			charset = "UTF-8";
		} else {
			// Ignore GNOME settings, except if explicitely set, 
			// because the default behaviour (UTF-8) is the
			// inverse of all other conventions, so we have no way
			// of finding the default !
			s = getenv ("G_FILENAME_ENCODING");
			if (s) {
				strncpy (s, buffer, sizeof(buffer)-1);
				buffer[sizeof(buffer)-1] = '\0';
				buffer[strcspn (buffer, ",;:")] = '\0';
				if (strcmp (buffer, "@locale") != 0) {
					charset = buffer;
				}
			}
		}
	}
	// c) else, use locale
#if defined(HAVE_LANGINFO_CODESET) && defined(HAVE_SETLOCALE)
	if (charset == NULL || *charset == 0) {
		setlocale (LC_ALL, "");
		strncpy (buffer, nl_langinfo (CODESET), sizeof(buffer)-1);
		buffer[sizeof(buffer)-1] = '\0';
		charset = buffer;
	}
#else
#	warning "TBD no setlocale or nl_langinfo(CODESET) implemented"
#endif
	// d) else, UTF-8
	if (charset == NULL || *charset == 0) {
		charset = "UTF-8";
	}
	
	bool const utf8 = (strcasecmp (charset, "UTF-8") == 0);
	int rc = 0;
	
#ifdef HAVE_ICONV
	if (!utf8) {
		int i;
		for (i = 0; i < NB_CONVERTERS; i++) {
			Converter* const cvt = g_converters + i;
			if (i == CHARSET_TO_UTF8)
				cvt->cd = iconv_open ("UTF-8", charset);
			else if ( i == CHARSET_FROM_UTF8)
				cvt->cd = iconv_open (charset, "UTF-8");
			else
				rc = ENOSYS;
			if (cvt->cd == (iconv_t) -1)
				rc = errno;
			if (rc == 0) 
				ithread_mutex_init (&cvt->mutex, NULL);
		}
	}
#else
	if (init_charset (charset) == 0)
		rc = EINVAL;
#endif
	
	if (rc)
		Log_Printf (LOG_ERROR,
			    "Charset : error initialising charset='%s' : %s",
			    NN(charset), strerror (rc));
	else {
		Log_Printf (LOG_INFO, 
			    "Charset : successfully initialised charset='%s'",
			    NN(charset));
		g_state = (utf8 ? INITIALIZED_UTF8 : INITIALIZED_NOT_UTF8);
	}
	return rc;
}


/*****************************************************************************
 * Charset_IsConverting
 *****************************************************************************/
bool
Charset_IsConverting() 
{
	return (g_state == INITIALIZED_NOT_UTF8);
}


/*****************************************************************************
 * convert
 *	This function is similar to iconv(3) but handles incorrect
 *	character sequences internally.
 *	Returns 0 if ok, or E2BIG if insufficient output space.
 *****************************************************************************/
static int
convert (Converter* const cvt,
	 const char** const inbuf, size_t* const inbytesleft,
	 char** const outbuf, size_t* const outbytesleft)
{
#ifdef HAVE_ICONV
	if (iconv (cvt->cd, (ICONV_CONST char**) inbuf, inbytesleft, 
		   outbuf, outbytesleft) != (size_t) -1) 
		return 0; // ---------->
	if (errno != EILSEQ && errno != EINVAL) 
		return errno; // ---------->

	// Error undefined or invalid sequence : store appropriate 
	// error character (if sufficient output buffer space)
	int save_errno = errno;

	if (cvt->to_utf8) {
		// store Unicode 'REPLACEMENT CHARACTER' 0xFFFD 
		// (encoded in UTF-8)
		if (*outbytesleft < 3) 
			return E2BIG; // ---------->
		*(*outbuf)++ = 0xEF;
		*(*outbuf)++ = 0xBF;
		*(*outbuf)++ = 0xBD;
		(*outbytesleft) -= 3;
	} else {
		// store '?' (properly encoded, if possible)
		const char ERROR_CHAR = '?';
		const char* ibuf = &ERROR_CHAR;
		size_t ileft = 1;
		if (iconv (cvt->cd, (ICONV_CONST char**) &ibuf, &ileft, 
			   outbuf, outbytesleft) == (size_t) -1 || ileft > 0) {
			if (errno == E2BIG || *outbytesleft < 1)
				return E2BIG; // ---------->
			*(*outbuf)++ = ERROR_CHAR;
			(*outbytesleft)--;
		}
	}

	// Skip invalid input
	if (*inbytesleft > 0) {
		if (save_errno == EINVAL) {
			// skip all remaining chars
			(*inbuf) += (*inbytesleft);
			(*inbytesleft) = 0;
		} else {
			// skip 1 char
			(*inbuf)++;
			(*inbytesleft)--;
		}
	}
	
	return 0;

#else // ! HAVE_ICONV
	return cvt->convert (inbuf, inbytesleft, outbuf, outbytesleft);
#endif
}



/*****************************************************************************
 * Charset_ConvertString
 *****************************************************************************/
char*
Charset_ConvertString (Charset_Direction dir, const char* const str, 
		       char* buffer, size_t bufsize,
		       void* const talloc_context)
{
	if (str == NULL)
		return NULL; // ---------->
	if (g_state != INITIALIZED_NOT_UTF8)
		return (char*) str; // ---------->

	if (dir < 0 || dir >= NB_CONVERTERS)
		return NULL; // ----------> 
	Converter* const cvt = g_converters + dir;

	char* result = NULL;

#ifdef HAVE_ICONV
	ithread_mutex_lock (&cvt->mutex);
	(void) iconv (cvt->cd, NULL, NULL, NULL, NULL);
#endif


	const char* inbuf = str;
#ifdef HAVE_ICONV
	size_t inbytesleft = strlen(str); // convert _excluding_ final '\0'
	const size_t extra = 16; // estimate needed for final flush and 0s
	const size_t needed_size = inbytesleft * 2 + extra; // initial guess
#else
	size_t inbytesleft = strlen(str) + 1; // convert _including_ final '\0'
	const size_t extra = 0; // no need for extra bytes after conversion
	const size_t needed_size = cvt->size (str); // exact size
	if (bufsize < needed_size) 
		buffer = NULL; // don't bother trying buffer if too small
#endif
	result = buffer;
	if (buffer == NULL) {
		bufsize = needed_size;
		result = talloc_size (talloc_context, bufsize);
		if (result == NULL)
			goto FAIL; // ---------->
	}
	char* outbuf = result;
	size_t outbytesleft = bufsize;

	while (inbytesleft > 0) {
		int save_errno = convert (cvt,
					  &inbuf, &inbytesleft,
					  &outbuf, &outbytesleft);
		if (save_errno != 0 && save_errno != E2BIG) {
			if (result != buffer)
				talloc_free (result);
			result = NULL;
			goto FAIL;
		}
		// Grow result string, if insufficient outbuf size
		// (even if not E2BIG, because needs at least space for iconv 
		// flush and final 0)
		if (save_errno == E2BIG || outbytesleft < extra) {
			size_t const converted = outbuf - result;
			size_t const incr = inbytesleft * 2 + extra;
			bufsize += incr;
			if (result == buffer) {
				char* const ptr = talloc_size (talloc_context,
							       bufsize);
				if (ptr == NULL)
					goto FAIL; // ---------->
				result = memcpy (ptr, result, converted);
			} else {
				result = talloc_realloc_size (talloc_context, 
							      result, bufsize);
			}
			if (result == NULL)
				goto FAIL; // ---------->
			outbytesleft += incr;
			outbuf = result + converted;
		}
	}
	
#ifdef HAVE_ICONV
	// Flush iconv conversion
	(void) iconv (cvt->cd, NULL, NULL, &outbuf, &outbytesleft);

	/* Terminate string.
	 * Note: not all charsets can be nul-terminated with a single nul byte.
	 * UCS2, for example, needs 2 nul bytes and UCS4 needs 4. 
	 * 4 nul bytes should be enough to terminate all multibyte charsets ...
	 */
	if (outbytesleft < 1)
		result[bufsize-1] = '\0'; // error, truncate result
	else
		memset (outbuf, '\0', MIN (outbytesleft, 4));
#endif

        // If necessary, shrink result to save space
	if (result != buffer && outbytesleft > 32) {
		result = talloc_realloc_size (talloc_context, result,
					      outbuf - result);
	}
FAIL:
#ifdef HAVE_ICONV
	ithread_mutex_unlock (&cvt->mutex);
#endif
	return result;
}


/*****************************************************************************
 * Charset_PrintString
 *****************************************************************************/
int
Charset_PrintString (Charset_Direction dir, const char* const str, 
		     FILE* const stream)
{
	if (str == NULL || stream == NULL)
		return EOF; // ---------->
	if (g_state != INITIALIZED_NOT_UTF8)
		return fputs (str, stream); // ---------->

	if (dir < 0 || dir >= NB_CONVERTERS)
		return EOF; // ----------> 
	Converter* const cvt = g_converters + dir;

#ifdef HAVE_ICONV
	ithread_mutex_lock (&cvt->mutex);
	(void) iconv (cvt->cd, NULL, NULL, NULL, NULL);
#endif
	int rc = 0;

	const char* inbuf  = str;
	size_t inbytesleft = strlen(str); // convert excluding final '\0'
	
	union {
		intmax_t _align;
		char bytes [BUFSIZ];
	} buffer;
	while (inbytesleft > 0) {
		char* outbuf = buffer.bytes;
		size_t outbytesleft = sizeof(buffer.bytes);
		int save_errno = convert (cvt,
					  &inbuf, &inbytesleft,
					  &outbuf, &outbytesleft);
		if (outbuf > buffer.bytes) {
			rc = fwrite (buffer.bytes, outbuf - buffer.bytes, 1,
				     stream);
			if (rc < 1) {
				rc = EOF;
				break; // ---------->
			}
		}
		if (save_errno != 0 && save_errno != E2BIG) {
			rc = EOF;
			break; // ---------->
		}
	}
#ifdef HAVE_ICONV
	if (rc >= 0) {
		// Flush iconv conversion
		char* outbuf = buffer.bytes;
		size_t outbytesleft = sizeof(buffer.bytes);
		(void) iconv (cvt->cd, NULL, NULL, &outbuf, &outbytesleft);
		if (outbuf > buffer.bytes) {
			rc = fwrite (buffer.bytes, outbuf - buffer.bytes, 1, 
				     stream);
			if (rc < 1)
				rc = EOF;
		}
	}
	ithread_mutex_unlock (&cvt->mutex);
#endif
	return rc;
}


/*****************************************************************************
 * Charset_Finish
 *****************************************************************************/
int
Charset_Finish()
{
	if (g_state == NOT_INITIALIZED)
		return EALREADY; // ---------->
	
	int rc = 0;
#ifdef HAVE_ICONV
	int i;
	for (i = 0; i < NB_CONVERTERS; i++) {
		Converter* const cvt = g_converters + i;
		ithread_mutex_destroy (&cvt->mutex);
		if (iconv_close (cvt->cd))
			rc = errno;
		cvt->cd = (iconv_t) -1;
	}
#else
	(void) init_charset ("");
#endif
	g_state = NOT_INITIALIZED;
	return rc;
}



 

