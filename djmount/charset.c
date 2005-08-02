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

#include "charset.h"

#include "log.h"

#include <stdio.h>	// Needed to compile "talloc.h"
#include <stdarg.h>	// Needed to compile "talloc.h"
#include <stdlib.h>	// Needed to compile "talloc.h"
#include "talloc.h"
#include <string.h>

#include <locale.h>
#include <langinfo.h>

#ifdef HAVE_ICONV
TBD;
#else
#  include "charset_internal.h"
#endif


/*****************************************************************************
 * Charset_Initialize
 *****************************************************************************/
int
Charset_Initialize (const char* charset)
{
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
   * (cf. http://mirror.hamakor.org.il/archives/linux-il/01-2004/7759.html )
   *
   * GTK+ 1 / GNOME 1 :
   *	GTK+ 1 will treat filenames according to the locale. If you use 
   *	a UTF-8 locale (check with "locale charmap"), GTK+ 1 will consider the 
   *	file names on disk to be in UTF-8 charset. 
   *
   * KDE :
   *	KDE will treat filenames according to the locale. You can force UTF8 
   *	file names despite a non-UTF8 locale by setting the KDE_UTF8_FILENAMES
   *	environment variable : see
   *	http://wiki.kde.org/tiki-index.php?page=Environment%20Variables
   *
   * GTK+ 2 / GNOME 2 : 
   *	GTK+ 2 considers file names to be always in UTF-8 encoding, unless
   *	the G_FILENAME_ENCODING or G_BROKEN_FILENAMES environment variables 
   *	are set : see
   * 	http://developer.gnome.org/doc/API/2.0/glib/glib-running.html
   *
   *	G_FILENAME_ENCODING : this environment variable can be set to a 
   *	comma-separated list of character set names. GLib assumes that 
   *	filenames are encoded in the first character set from that list 
   *	rather than in UTF-8. The special token "@locale" can be used 
   *	to specify the character set for the current locale. 
   *
   *	G_BROKEN_FILENAMES : if this environment variable is set, GLib 
   *	assumes that filenames are in the locale encoding rather than in UTF-8.
   *	G_FILENAME_ENCODING takes priority over G_BROKEN_FILENAMES. 
   *
   */

  int rc = 1;
  char buffer [128] = "";
  // b) find if specific desktop settings
  if (charset == NULL || *charset == 0) {
    char* s = getenv ("KDE_UTF8_FILENAMES");
    if (s) {
      charset = "UTF-8";
    } else {
      // Ignore GNOME settings, except if explicitely set, because the 
      // default behaviour (UTF-8) is the inverse of all other conventions 
      // so we have no way of finding the default.
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
  if (charset == NULL || *charset == 0) {
    setlocale (LC_ALL, "");
    strncpy (buffer, nl_langinfo (CODESET), sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    charset = buffer;
  }
  // d) else, UTF-8
  if (charset == NULL || *charset == 0) {
    charset = "UTF-8";
  }
  
#ifdef HAVE_ICONV
  TBD;
#else
  rc = !init_charset (charset);
#endif

  if (rc)
    Log_Printf (LOG_ERROR, "Charset : error initialising charset='%s'",
		NN(charset));
  else
    Log_Printf (LOG_INFO, "Charset : successfully initialised charset='%s'",
		NN(charset));

  return rc;
}


/*****************************************************************************
 * Charset_ToUtf8Size
 *****************************************************************************/
size_t
Charset_ToUtf8Size (const char* src)
{
#ifdef HAVE_ICONV
  TBD;
#else
  return ascii2utf_size (src);
#endif
}


/*****************************************************************************
 * Charset_ToUtf8
 *****************************************************************************/
char*
Charset_ToUtf8 (const char* src, char* dest, size_t dest_size)
{
  if (dest == NULL || dest_size <= 0)
    return NULL; // ---------->
  if (src == NULL) {
    *dest = '\0';
    return NULL; // ---------->
  }

#ifdef HAVE_ICONV
  TBD;
#else
  return ascii2utf (src, dest, dest_size);
#endif
}


/*****************************************************************************
 * Charset_ToUtf8_talloc
 *****************************************************************************/
char*
Charset_ToUtf8_talloc (void* talloc_context, const char* str)
{
  if (str == NULL)
    return NULL; // ---------->

  size_t const size = Charset_ToUtf8Size (str);
  char* const dest = talloc_size (talloc_context, size);
  char* const result = Charset_ToUtf8 (str, dest, size);
  if (result == NULL)
    talloc_free (dest);
  return result;
}


/*****************************************************************************
 * Charset_FromUtf8
 *****************************************************************************/
size_t
Charset_FromUtf8Size (const char* src)
{
#ifdef HAVE_ICONV
  TBD;
#else
  return utf2ascii_size (src);
#endif
}


/*****************************************************************************
 * Charset_FromUtf8
 *****************************************************************************/
char*
Charset_FromUtf8 (const char* src, char* dest, size_t dest_size)
{
  if (dest == NULL || dest_size <= 0)
    return NULL; // ---------->
  if (src == NULL) {
    *dest = '\0';
    return NULL; // ---------->
  }

#ifdef HAVE_ICONV
  TBD;
#else
  return utf2ascii (src, dest, dest_size);
#endif
}

/*****************************************************************************
 * Charset_FromUtf8_talloc
 *****************************************************************************/
char*
Charset_FromUtf8_talloc  (void* talloc_context, const char* str)
{
  if (str == NULL)
    return NULL; // ---------->

  size_t const size = Charset_FromUtf8Size (str);
  char* const dest = talloc_size (talloc_context, size);
  char* const result = Charset_FromUtf8 (str, dest, size);
  if (result == NULL)
    talloc_free (dest);
  return result;
}


/*****************************************************************************
 * Charset_Finish
 *****************************************************************************/
int
Charset_Finish()
{
#ifdef HAVE_ICONV
  TBD;
#else
  (void) init_charset ("");
  return 0;
#endif
}



 

