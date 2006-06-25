/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id$
 *
 * Test VFS object.
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

#include "vfs_p.h"
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "talloc_util.h"
#include "log.h"





/*****************************************************************************
 * TestFS
 *****************************************************************************/

OBJECT_DECLARE_CLASS(TestFS,VFS);

OBJECT_DEFINE_STRUCT(TestFS, /**/ );

OBJECT_DEFINE_METHODS(TestFS, /**/ );



static VFS_BrowseStatus
BrowseSubTest (const VFS* const vfs, const char* const path, 
	       const VFS_Query* query, void* tmp_ctx)
{
	BROWSE_BEGIN(path, query) {
		
		DIR_BEGIN("test") {
			DIR_BEGIN("a1") {
			} DIR_END;

			struct tm t = { .tm_year = 104, .tm_mon = 5, 
					.tm_mday = 25};
			
			DIR_BEGIN("a2") {
				DIR_BEGIN("b1") {
					FILE_BEGIN("f1") {
						const char* str = "essais";
						FILE_SET_STRING (str, false);
						t.tm_hour = 14;
						VFS_SET_TIME (mktime (&t));
					} FILE_END;
				} DIR_END;
				
				DIR_BEGIN("b2") {
					DIR_BEGIN ("c1") {
					} DIR_END;
				} DIR_END;
			} DIR_END;
			
			DIR_BEGIN("a3") {
				DIR_BEGIN("b3") {
				} DIR_END;
				
				int i;
				for (i = 4; i < 10; i++) {
					char buffer [10];
					sprintf (buffer, "b%d", i);
					DIR_BEGIN(buffer) {
						t.tm_hour = t.tm_min = i-1;
						VFS_SET_TIME (mktime (&t));
						DIR_BEGIN ("toto") {
						} DIR_END;
					} DIR_END;
				}
			}DIR_END;
		} DIR_END;
	} BROWSE_END;
	return BROWSE_RESULT;
}

static VFS_BrowseStatus
BrowseTest (VFS* const vfs, const char* const path, 
	    const VFS_Query* query, void* tmp_ctx)
{
	BROWSE_BEGIN(path, query) {
		DIR_BEGIN("atest") {
			BROWSE_SUB (BrowseSubTest (vfs, BROWSE_PTR,
						   query, tmp_ctx));
			SYMLINK_BEGIN("link_to_test") {
				SYMLINK_SET_PATH("test");
			} SYMLINK_END;
		} DIR_END;

		BROWSE_SUB (BrowseSubTest (vfs, BROWSE_PTR, query, tmp_ctx));

		FILE_BEGIN("void_file") {
		} FILE_END;
		
		SYMLINK_BEGIN("broken_link") {
			SYMLINK_SET_PATH("broken/link");
		} SYMLINK_END;
		
		DIR_BEGIN("zetest") {
			BROWSE_SUB (BrowseSubTest (vfs, BROWSE_PTR, 
						   query, tmp_ctx));
			SYMLINK_BEGIN("link_to_test") {
				SYMLINK_SET_PATH("../test");
			} SYMLINK_END;
		} DIR_END;

		// should be ignored
		DIR_BEGIN("") {
		} DIR_END;

		// should be ignored
		FILE_BEGIN(NULL) {
		} FILE_END;

	} BROWSE_END;

	VFS_BrowseStatus const s = BROWSE_RESULT;
	printf ("BROWSE_RESULT : %d (%s) : path='%s', stops at='%s'\n", 
		s.rc, strerror (-s.rc), path, s.ptr);
	
	return s;
}


static void
init_testfs_class (TestFS_Class* const isa)
{
  CLASS_SUPER_CAST(isa)->browse_root = BrowseTest;
}

OBJECT_INIT_CLASS(TestFS, VFS, init_testfs_class);

TestFS*
TestFS_Create (void* talloc_context)
{
	OBJECT_SUPER_CONSTRUCT (TestFS, VFS_Create, talloc_context, true);
	return self;
}


static VFS* g_vfs = NULL;



/*****************************************************************************
 * FUSE Operations
 *****************************************************************************/

static int 
fs_getattr (const char* path, struct stat* stbuf)
{
	*stbuf = (struct stat) { .st_mode = 0 };
	
	VFS_Query const q = { .path = path, .stbuf = stbuf };
	int rc = VFS_Browse (g_vfs, &q);
	
	return rc;
}

static int 
fs_readlink (const char* path, char* buf, size_t size)
{
	VFS_Query const q = { .path = path, 
			      .lnk_buf = buf, .lnk_bufsiz = size };
	int rc = VFS_Browse (g_vfs, &q);
	return rc;
}

static int 
fs_getdir (const char* path, fuse_dirh_t h, fuse_dirfil_t filler)
{
	VFS_Query q = { .path = path, .h = h, .filler = filler };
	int rc = VFS_Browse (g_vfs, &q);
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
	VFS_Query q = { .path = path, .talloc_context = context, 
			.file = &file };
	int rc = VFS_Browse (g_vfs, &q);
	if (rc) {
		talloc_free (file);
		file = NULL;
	}
	fi->fh = (intptr_t) file;

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
	
	// print raw message
	puts (msg);
	
	Log_EndColor (level, stdout);
}



/*****************************************************************************
 * Main
 *****************************************************************************/

static void
bad_usage (const char* progname, ...)
{
	fprintf (stderr, "%s: ", progname);
	va_list ap;
	va_start (ap, progname);
	const char* const format = va_arg (ap, const char*);
	vfprintf (stderr, format, ap);
	va_end (ap);
	exit (EXIT_FAILURE); // ---------->
}

int 
main (int argc, char *argv[])
{
	int rc;

	talloc_enable_leak_report();

	// Create a working context for temporary strings
	void* const tmp_ctx = talloc_autofree_context();

	rc = Log_Initialize (stdout_print);
	if (rc != 0) {
		fprintf (stderr, "%s : Error initialising Logger", argv[0]);
		exit (rc); // ---------->
	}  
	Log_Colorize (true);
	Log_SetMaxLevel (LOG_DEBUG);
	
	/*
	 * Handle options
	 */
	
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
		if (*o != '-') { 
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
				FUSE_ARG ("-o");
				FUSE_ARG (talloc_strdup (tmp_ctx, s));
			}
			free (options_copy);
			Log_Printf (LOG_INFO, "  Mount options = %s", options);
			
		} else {
			bad_usage (argv[0], "unrecognized option '%s'", 
				   o); // ---------->
		}
	}


	/* 
	 * Create virtual file system
	 */
	g_vfs = TestFS_ToVFS (TestFS_Create (tmp_ctx));
	if (g_vfs == NULL) {
		Log_Printf (LOG_ERROR, "Failed to create virtual file system");
		exit (EXIT_FAILURE); // ---------->
	}


	/*
	 * Initialise FUSE
	 */
	
	// Force Read-only (write operations not implemented yet)
	FUSE_ARG ("-r"); 

#if FUSE_VERSION >= 23
	// try to fill in d_ino in readdir
	FUSE_ARG ("-o");
	FUSE_ARG ("readdir_ino");
#endif

	fuse_argv[fuse_argc] = NULL; // End FUSE arguments list
	rc = fuse_main (fuse_argc, fuse_argv, &fs_oper);
	if (rc != 0) {
		Log_Printf (LOG_ERROR, "Error in FUSE main loop = %d", rc);
	}
	
	Log_Printf (LOG_DEBUG, "Shutting down ...");
	
	Log_Finish();

	return rc; 
}
