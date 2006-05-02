/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * main FUSE interface.
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

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>
#ifdef HAVE_SETXATTR
#	include <sys/xattr.h>
#endif
#include <stdarg.h>	
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "talloc_util.h"
#include "device_list.h"
#include "log.h"
#include "upnp_util.h"
#include "string_util.h"
#include "djfs.h"
#include "content_dir.h"
#include "charset.h"
#include "minmax.h"



/*****************************************************************************
 * Configuration related to specific FUSE versions
 *****************************************************************************/

// Missing in earlier FUSE versions e.g. 2.2
#ifndef FUSE_VERSION
#	define FUSE_VERSION	(FUSE_MAJOR_VERSION * 10 + FUSE_MINOR_VERSION)
#endif

// "-o readdir_ino" option available ?
#if FUSE_VERSION >= 23
#	define HAVE_FUSE_O_READDIR_INO	1
#endif

// "-o nonempty" option available ?
#if FUSE_VERSION >= 24
#	define HAVE_FUSE_O_NONEMPTY	1
#endif

// per-file direct_io flag ?
#if FUSE_VERSION >= 24
#	define HAVE_FUSE_FILE_INFO_DIRECT_IO	1
#endif



/*****************************************************************************
 * Global djmount settings
 *****************************************************************************/

static DJFS_Flags g_djfs_flags = DJFS_METADATA;



/*****************************************************************************
 * Charset conversions (display <-> UTF-8) for filesystem
 *****************************************************************************/

typedef struct {
	fuse_dirh_t    h;
	fuse_dirfil_t  filler;
} my_dir_handle;

static int filler_from_utf8 (fuse_dirh_t h, const char *name, 
			     int type, ino_t ino)
{
	// Convert filename to display charset
	char buffer [NAME_MAX + 1];
	char* display_name = Charset_ConvertString (CHARSET_FROM_UTF8, 
						    name, 
						    buffer, sizeof (buffer),
						    NULL);
	my_dir_handle* const my_h = (my_dir_handle*) h;
	int rc = my_h->filler (my_h->h, display_name, type, ino);
	if (display_name != buffer && display_name != name)
		talloc_free (display_name);
	return rc;
}

static int
Browse (const char* path, 
	/* for STAT => */	struct stat* stbuf, 
	/* for GETDIR => */	fuse_dirh_t h, fuse_dirfil_t filler, 
	/* for READ => */	void* talloc_context, FileBuffer** file)
{
	int rc = -EIO;
	if (! Charset_IsConverting()) {
		rc = DJFS_Browse (path, g_djfs_flags, stbuf, h, filler, 
				  talloc_context, file);
	} else {
		// Convert filename from display charset 
		char buffer [PATH_MAX];
		char* const utf_path = Charset_ConvertString 
			(CHARSET_TO_UTF8, path, buffer, sizeof (buffer), NULL);
		my_dir_handle my_h = { .h = h, .filler = filler };
		
		rc = DJFS_Browse (utf_path, g_djfs_flags, stbuf, 
				  (filler ? (void*)&my_h : h), 
				  (filler ? filler_from_utf8 : NULL),
				  talloc_context, file);
		if (utf_path != buffer && utf_path != path)
			talloc_free (utf_path);
	}
	return rc;
}

/*****************************************************************************
 * FUSE Operations
 *****************************************************************************/

static int 
fs_getattr (const char* path, struct stat* stbuf)
{
	*stbuf = (struct stat) { .st_mode = 0 };
	
	int rc = Browse (path, stbuf, NULL, NULL, NULL, NULL);
	
	return rc;
}

#if 1
#  define fs_readlink	NULL
#else
static int fs_readlink (const char *path, char *buf, size_t size)
{
  int rc;
      
  // TBD not implemented yet
  rc = -EIO;

  return rc;
}
#endif


static int 
fs_getdir (const char* path, fuse_dirh_t h, fuse_dirfil_t filler)
{
	int rc = Browse (path, NULL, h, filler, NULL, NULL);
	return rc;
}  


static int 
fs_mknod (const char* path, mode_t mode, dev_t rdev)
{
  int rc;
  
  // TBD not implemented yet
  rc = -EIO;
  
  return rc;
}


#if 1
#  define fs_mkdir 	NULL
#else
static int 
fs_mkdir (const char* path, mode_t mode)
{
  int rc;
  
  if (path == 0) {
    res = -EFAULT;
  } else {
    // Parse the path :
    // directory creation only allowed in "/<device>/search/" sub-directories
    size_t pathlen = strlen (path);
    void* context  = talloc_new (0); // TBD
    char* device   = talloc_array (context, char, pathlen);
    char* criteria = talloc_array (context, char, pathlen);
    if (sscanf (path, "/%[^/]/search/%[^/]", device, criteria) != 2) {
      res = -EPERM;
    } else {
      
      // TBD not yet implemented !!
      res = -EIO;
    }
    talloc_free (context);
  }
  
  return rc;
}
#endif


static int 
fs_unlink (const char* path)
{
  // File creation / deletion is not allowed in this filesystem
  return -EPERM;
}


#if 1
#  define fs_rmdir 	NULL
#else
static int 
fs_rmdir (const char* path)
{
  int res;
  
  void* context = talloc_new (0); // TBD
  StringArray* p = split_path (context, path);
  if (p == 0) {
    res = -EFAULT;
  } else {
    // directory deletion only allowed in "/<device>/search/" sub-directories
    if (p->nb != 3 || strcmp (p->str[1], "search") != 0) {
      res = -EPERM;
    } else {
      // TBD not implemented yet
      res = -EIO;
    }
  }
  talloc_free (context);
  return res;
}
#endif


#if 1
#  define fs_symlink 	NULL
#else
static int 
fs_symlink (const char* from, const char* to)
{
  int rc;

  // TBD not implemented yet
  rc = -EIO;

  return rc;
}
#endif


#if 1
#  define fs_rename	NULL
#else
static int 
fs_rename (const char* from, const char* to)
{
  int res;
  
  res = rename(from, to);
  if(res == -1)
    return -errno;
  
  return 0;
}
#endif


#if 1
#  define fs_link	NULL
#else
static int 
fs_link (const char* from, const char* to)
{
  int res;
  
  res = link(from, to);
  if(res == -1)
    return -errno;
  
  return 0;
}
#endif


#if 1
#  define fs_chmod  	NULL
#else
static int fs_chmod (const char* path, mode_t mode)
{
  // Permission denied : no chmod can be performed on this filesystem
  return -EPERM;
}
#endif


#if 1
#  define fs_chown	NULL
#else
static int 
fs_chown (const char* path, uid_t uid, gid_t gid)
{
    int res;

    res = lchown(path, uid, gid);
    if(res == -1)
        return -errno;

    return 0;
}
#endif


#if 1
#  define fs_truncate	NULL
#else
static int fs_truncate (const char* path, off_t size)
{
  int res;
  
  res = truncate(path, size);
  if(res == -1)
    return -errno;
  
  return 0;
}
#endif


#if 1
#  define fs_utime	NULL
#else
static int 
fs_utime (const char *path, struct utimbuf *buf)
{
  int res;
  
  res = utime(path, buf);
  if(res == -1)
    return -errno;
  
  return 0;
}
#endif


static int 
fs_open (const char* path, struct fuse_file_info* fi)
{
	if (fi == NULL) {
		return -EIO; // ---------->
	} 
	if ((fi->flags & O_ACCMODE) != O_RDONLY) {
		return -EACCES; // ---------->
	} 
	
	void* context = NULL; // TBD
	FileBuffer* file = NULL;
	int rc = Browse (path, NULL, NULL, NULL, context, &file);
	if (rc) {
		talloc_free (file);
		file = NULL;
	}
	fi->fh = (intptr_t) file;

#if HAVE_FUSE_FILE_INFO_DIRECT_IO	
	/*
	 * Whenever possible, do not set the 'direct_io' flag on files : 
	 * this allow the 'mmap' operation to succeed on these files.
	 * However, in some case, we have to set the 'direct_io' :
	 * a) if the size of the file if not known in advance 
	 *    (e.g. if the DIDL-Lite attribute <res@size> is not set)
	 * b) or if the buffer does not guaranty to return exactly the
	 *    number of bytes requested in a read (except on EOF or error)
	 */
	fi->direct_io = ( FileBuffer_GetSize (file) < 0 ||
			  ! FileBuffer_HasExactRead (file) );
#endif

	return rc;
}


static int
fs_read (const char* path, char* buf, size_t size, off_t offset,
	 struct fuse_file_info* fi)
{
	FileBuffer* const file = (FileBuffer*) fi->fh;
	int rc = FileBuffer_Read (file, buf, size, offset);
	return rc;
}


#if 1
#  define fs_write	NULL
#else
static int 
fs_write (const char* path, const char* buf, size_t size,
	  off_t offset, struct fuse_file_info* fi)
{
  // Should not happen : no file is writable in this filesystem.
  return -EIO; 
}
#endif


#if 1
#  define fs_statfs	NULL
#else
static int 
fs_statfs (const char* path, struct statfs* stbuf)
{
  int res = -ENOSYS; // not supported
  
  *stbuf = (struct statfs) { }; // TBD
  
  return res;
}
#endif


static int 
fs_release (const char* path, struct fuse_file_info* fi)
{
	FileBuffer* const file = (FileBuffer*) fi->fh;
	
	if (file) {
		talloc_free (file);
		fi->fh = (intptr_t) NULL;
	}
	return 0;
}


static int 
fs_fsync (const char* path, int isdatasync, struct fuse_file_info* fi)
{
  /* Just a stub.  This method is optional and can safely be left
     unimplemented */

  (void) path;
  (void) isdatasync;
  (void) fi;
  return 0;
}


#ifdef HAVE_SETXATTR
/*
 * xattr operations are optional and can safely be left unimplemented 
 */
static int fs_setxattr (const char *path, const char *name, const char *value,
                        size_t size, int flags)
{
    int res = lsetxattr(path, name, value, size, flags);
    if(res == -1)
        return -errno;
    return 0;
}

static int fs_getxattr (const char *path, const char *name, char *value,
			size_t size)
{
    int res = lgetxattr(path, name, value, size);
    if(res == -1)
        return -errno;
    return res;
}

static int fs_listxattr (const char *path, char *list, size_t size)
{
    int res = llistxattr(path, list, size);
    if(res == -1)
        return -errno;
    return res;
}

static int fs_removexattr (const char *path, const char *name)
{
    int res = lremovexattr(path, name);
    if(res == -1)
        return -errno;
    return 0;
}
#endif /* HAVE_SETXATTR */


static struct fuse_operations fs_oper = {
	.getattr	= fs_getattr,
	.readlink	= fs_readlink,
	.getdir		= fs_getdir,
	.mknod		= fs_mknod,
	.mkdir		= fs_mkdir,
	.symlink	= fs_symlink,
	.unlink		= fs_unlink,
	.rmdir		= fs_rmdir,
	.rename		= fs_rename,
	.link		= fs_link,
	.chmod		= fs_chmod,
	.chown		= fs_chown,
	.truncate	= fs_truncate,
	.utime		= fs_utime,
	.open		= fs_open,
	.read		= fs_read,
	.write		= fs_write,
	.statfs		= fs_statfs,
	.flush      	= NULL,
	.release	= fs_release,
	.fsync		= fs_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= fs_setxattr,
	.getxattr	= fs_getxattr,
	.listxattr	= fs_listxattr,
	.removexattr	= fs_removexattr,
#endif
};


/*****************************************************************************
 * @fn 		stdout_print 
 * @brief 	Output log messages.
 *
 * Parameters:
 * 	See Log_PrintFunction prototype.
 *
 *****************************************************************************/

static void
stdout_print (Log_Level level, const char* msg)
{
	Log_BeginColor (level, stdout);
	switch (level) {
	case LOG_ERROR:		printf ("[E] "); break;
	case LOG_WARNING:	printf ("[W] "); break;
	case LOG_INFO:		printf ("[I] "); break;
	case LOG_DEBUG:		printf ("[D] "); break;
	default:
		printf ("[%d] ", (int) level);
		break;
	}
	
	// Convert message to display charset, and print
	Charset_PrintString (CHARSET_FROM_UTF8, msg, stdout);
	Log_EndColor (level, stdout);
	printf ("\n");
}


/*****************************************************************************
 * Usage
 *****************************************************************************/

#if UPNP_HAVE_DEBUG
#    define DEBUG_DEFAULT_LEVELS	"upnpall,debug,fuse,leak"
#else
#    define DEBUG_DEFAULT_LEVELS	"debug,fuse,leak"
#endif

static const char* const FUSE_ALLOWED_OPTIONS = \
	"    default_permissions    enable permission checking by kernel\n"
	"    allow_other            allow access to other users\n"
	"    allow_root             allow access to root\n"
	"    kernel_cache           cache files in kernel\n"
#if HAVE_FUSE_O_NONEMPTY
	"    nonempty               allow mounts over non-empty file/dir\n"
#endif
	"    fsname=NAME            set filesystem name in mtab\n";

static void
usage (FILE* stream, const char* progname)
{
  fprintf 
    (stdout,
     "usage: %s [options] mountpoint\n"
     "\n"
     "Options:\n"
     "    -h or --help           print this help, then exit\n"
     "    --version              print version number, then exit\n"
     "    -o [options]           mount options (see below)\n"
     "    -d[levels]             enable debug output (implies -f)\n"
     "    -f                     foreground operation (default: daemonized)\n"
     "\n"
     "Mount options (one or more comma separated options) :\n"
#if HAVE_CHARSET
     "    iocharset=<charset>    filenames encoding (default: environment)\n"
#endif
     "    playlists              use playlists for AV files, instead of plain files\n"
     "\n"
     "See FUSE documentation for the following mount options:\n"
     "%s\n"
     "Debug levels are one or more comma separated words :\n"
#if UPNP_HAVE_DEBUG
     "    upnperr, upnpall : increasing level of UPnP traces\n"
#endif
     "    error, warn, info, debug : increasing level of djmount traces\n"
     "    fuse : activates FUSE traces\n"
     "    leak, leakfull : enable talloc leak reports at exit\n"
     "'-d' alone defaults to '" DEBUG_DEFAULT_LEVELS "' i.e. all traces.\n"
     "\n"
     "Report bugs to <" PACKAGE_BUGREPORT ">.\n",
     progname, FUSE_ALLOWED_OPTIONS);
  exit (EXIT_SUCCESS); // ---------->
}


static void
bad_usage (const char* progname, ...)
{
	fprintf (stderr, "%s: ", progname);
	va_list ap;
	va_start (ap, progname);
	const char* const format = va_arg (ap, const char*);
	vfprintf (stderr, format, ap);
	va_end (ap);
	fprintf (stderr, "\nTry '%s --help' for more information.\n",
		 progname);
	exit (EXIT_FAILURE); // ---------->
}


static void
version (FILE* stream, const char* progname)
{
	fprintf (stream, 
		 "%s (" PACKAGE ") " VERSION "\n", progname);
	fprintf (stream, "Copyright (C) 2005 Rémi Turboult\n");
	fprintf (stream, "Compiled against: ");
	fprintf (stream, "FUSE %d.%d", FUSE_MAJOR_VERSION, 
		 FUSE_MINOR_VERSION);
#ifdef UPNP_VERSION_STRING
	fprintf (stream, ", libupnp %s", UPNP_VERSION_STRING);
#endif
	fputs ("\n\
This is free software. You may redistribute copies of it under the terms of\n\
the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n", stream);
	exit (EXIT_SUCCESS); // ---------->
}


/*****************************************************************************
 * Main
 *****************************************************************************/

int 
main (int argc, char *argv[])
{
	int rc;
	bool background = true;

	// Create a working context for temporary strings
	void* const tmp_ctx = talloc_autofree_context();

	rc = Log_Initialize (stdout_print);
	if (rc != 0) {
		fprintf (stderr, "%s : Error initialising Logger", argv[0]);
		exit (rc); // ---------->
	}  
	Log_Colorize (true);
#if UPNP_HAVE_DEBUG
	SetLogFileNames ("/dev/null", "/dev/null");
#endif
	
	/*
	 * Handle options
	 */
	char* charset = NULL;
	
	char* fuse_argv[32] = { argv[0] };
	int fuse_argc = 1;
	
#define FUSE_ARG(OPT)							\
	if (fuse_argc >= 31) bad_usage (argv[0], "too many args");	\
	fuse_argv[fuse_argc] = OPT;					\
	Log_Printf (LOG_DEBUG, "  Fuse option = %s", fuse_argv[fuse_argc]); \
	fuse_argc++

	int opt = 1;
	char* o;
	while ((o = argv[opt++])) {
		if (strcmp (o, "-h") == 0 || strcmp (o, "--help") == 0) {
			usage (stdout, argv[0]); // ---------->
			
		} else if (strcmp (o, "--version") == 0) {
			version (stdout, argv[0]); // ---------->
			
		} else if (strcmp(o, "-f") == 0) {
			background = false;

		} else if (*o != '-') { 
			// mount point
			FUSE_ARG (o);
			
		} else if ( strcmp (o, "-o") == 0 && argv[opt] ) { 
			// Parse mount options
			const char* const options = argv[opt++];
			char* options_copy = strdup (options);
			char* tokptr = 0;
			char* s;
			for (s = strtok_r (options_copy, ",", &tokptr); 
			     s != NULL; 
			     s = strtok_r (NULL, ",", &tokptr)) {
				if (strncmp (s,"playlists", 5) == 0) {
					g_djfs_flags |= DJFS_PLAYLISTS;
#if HAVE_CHARSET
				} else if (strncmp(s, "iocharset=", 10) == 0) {
					charset = talloc_strdup(tmp_ctx, s+10);
#endif
				} else if (strncmp(s, "fsname=", 7) == 0 ||
					   strstr (FUSE_ALLOWED_OPTIONS, s)) {
					FUSE_ARG ("-o");
					FUSE_ARG (talloc_strdup (tmp_ctx, s));
				} else {
					bad_usage (argv[0], 
						   "unknown mount option '%s'",
						   s); // ---------->
				}
			}
			free (options_copy);
			Log_Printf (LOG_INFO, "  Mount options = %s", options);
			
		} else if (strncmp (o, "-d", 2) == 0) {
			background = false;

			// Parse debug levels
			const char* const levels = 
				(o[2] ? o+2 : DEBUG_DEFAULT_LEVELS);
			char* levels_copy = strdup (levels);
			char* tokptr = 0;
			char* s;
			for (s = strtok_r (levels_copy, ",", &tokptr); 
			     s != NULL; 
			     s = strtok_r (NULL, ",", &tokptr)) {
				if (strcmp (s, "leak") == 0) {
					talloc_enable_leak_report();
				} else if (strcmp (s, "leakfull") == 0) {
					talloc_enable_leak_report_full();
				} else if (strcmp (s, "fuse") == 0) {
					FUSE_ARG ("-d");
				} else if (strcmp (s, "debug") == 0) {
					Log_SetMaxLevel (LOG_DEBUG);
				} else if (strcmp (s, "info") == 0) {
					Log_SetMaxLevel (LOG_INFO);
				} else if (strncmp (s, "warn", 4) == 0) {
					Log_SetMaxLevel (LOG_WARNING);
				} else if (strncmp (s, "error", 3) == 0) {
					Log_SetMaxLevel (LOG_ERROR);
#if UPNP_HAVE_DEBUG
				} else if (strcmp (s, "upnperr") == 0) {
					SetLogFileNames ("/dev/stdout", 
							 "/dev/null");
				} else if (strcmp (s, "upnpall") == 0) {
					SetLogFileNames ("/dev/stdout", 
							 "/dev/stdout");
#endif
				} else {
					bad_usage (argv[0],
						   "unknown debug level '%s'",
						   s); // ---------->
				}
			}
			free (levels_copy);
			Log_Printf (LOG_DEBUG, "  Debug options = %s", levels);
			
		} else {
			bad_usage (argv[0], "unrecognized option '%s'", 
				   o); // ---------->
		}
	}
	
	// Force Read-only (write operations not implemented yet)
	FUSE_ARG ("-r"); 

#if HAVE_FUSE_O_READDIR_INO
	// try to fill in d_ino in readdir
	FUSE_ARG ("-o");
	FUSE_ARG ("readdir_ino");
#endif
#if !HAVE_FUSE_FILE_INFO_DIRECT_IO	
	// Set global "direct_io" option, if not available per open file,
	// because we are not sure that every open file can be opened 
	// without this mode : see comment in fs_open() function.
	FUSE_ARG ("-o");
	FUSE_ARG ("direct_io");
#endif

	/*
	 * Set charset encoding
	 */
	rc = Charset_Initialize (charset);
	if (rc) {
		Log_Printf (LOG_ERROR, "Error initialising charset='%s'",
			    NN(charset));
	}

	/*
	 * Daemonize process if necessary (must be done before UPnP
	 * initialisation, so not relying on fuse_main function).
	 */
	FUSE_ARG ("-f");
	if (background) {
		// Avoid chdir, else a relative mountpoint given as 
		// argument to FUSE won't work.
		//  TBD FIXME  close stdout/stderr : how do we see errors 
		//  TBD FIXME  if UPnP or FUSE fails in background mode ?
	        rc = daemon (/* nochdir => */ 1, /* noclose => */ 0);
		if (rc == -1) {
			int const err = errno;
			Log_Printf (LOG_ERROR, 
				    "Failed to daemonize program : %s",
				    strerror (err));
			exit (err);
		}
	}
	
	/*
	 * Initialise UPnP Control point and starts FUSE file system
	 */
	
	rc = DeviceList_Start (CONTENT_DIR_SERVICE_TYPE, NULL);
	if (rc != UPNP_E_SUCCESS) {
		Log_Printf (LOG_ERROR, "Error starting UPnP Control Point");
		exit (rc); // ---------->
	}
	

	fuse_argv[fuse_argc] = NULL; // End FUSE arguments list
	rc = fuse_main (fuse_argc, fuse_argv, &fs_oper);
	if (rc != 0) {
		Log_Printf (LOG_ERROR, "Error in FUSE main loop = %d", rc);
	}
	
	Log_Printf (LOG_DEBUG, "Shutting down ...");
	DeviceList_Stop();
	
	(void) Charset_Finish();
	Log_Finish();

	return rc; 
}
