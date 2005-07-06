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


#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <stdarg.h>	/* missing from "talloc.h" */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

#include "device_list.h"
#include "log.h"
#include "upnp_util.h"
#include "string_util.h"
#include "djfs.h"


#define MY_FUSE_VERSION (FUSE_MAJOR_VERSION * 10 + FUSE_MINOR_VERSION)




/*****************************************************************************
 * CONFIGURATION
 *****************************************************************************/





// TBD in StringUtil.h

// Returned struct must be deallocated with "talloc_free"
static StringArray*
split_path (void* context, const char* path)
{
  if ( path == 0 || path[0] != '/' ) {
    return 0; // ---------->
    
  } else {
    // Parse the path
    // 1) find maximum number of components
    //    (may be one or two less if begining or ending /)
    int nb_max = 0;
    const char* p = path;
    while (*p) {
      if (*p++ == '/')
	nb_max++;
    }
    StringArray* ret = StringArray_talloc (context, nb_max);
    
    // 2) find components
    char* path_copy = talloc_strdup (ret, path); // overwritten by "strtok"
    char* tokptr = 0;
    char* s;
    for (s = strtok_r (path_copy, "/", &tokptr); s != 0; 
	 s = strtok_r (0, "/", &tokptr)) {
      if (*s) { // Keep only non-empty components
	ret->str[ret->nb++] = s; 
      }
    }
    return ret; // ---------->
  }
}



enum FileMode {
    
  FILE_NONE = 0,
  FILE_READ_STRING
  
};

typedef struct _FileHandle {
    
  enum FileMode mode;
  
  char*  string;
  size_t length;
  
} FileHandle;





/*****************************************************************************
 * FsObject cache
 *****************************************************************************/


// TBD 



/*****************************************************************************
 * FUSE Operations
 *****************************************************************************/

static int 
fs_getattr (const char* path, struct stat* stbuf)
{
  *stbuf = (struct stat) { .st_mode = 0 };
  
  int rc = DJFS_Browse (path, stbuf, NULL, NULL, NULL, NULL);

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
  int rc = DJFS_Browse (path, NULL, h, filler, NULL, NULL);
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
  FileHandle* fh = talloc (context, FileHandle);
  if (fh == NULL) 
    return -ENOMEM; // ---------->
  
  char* content = NULL;
  int rc = DJFS_Browse (path, NULL, NULL, NULL, fh, &content);
  if (rc == 0) {
    *fh = (struct _FileHandle) { 
      .mode   = FILE_READ_STRING,
      .string = content,
      .length = content ? strlen (content) : 0
    };
    fi->fh = (intptr_t) fh;
  }
  if (rc)
    talloc_free (fh);
  return rc;
}


static int
fs_read (const char* path, char* buf, size_t size, off_t offset,
	 struct fuse_file_info* fi)
{
  int rc;
  FileHandle* const fh = (FileHandle*) fi->fh;
  
  if (buf == NULL) {
    rc = -EFAULT;

  } else if (fh == NULL) {
    rc = -EIO; // should not happen

  } else {
    switch (fh->mode) {
    case FILE_READ_STRING:
      if (offset >= fh->length) {
	rc = 0; // EOF // TBD return error ??? XXX
      } else {
	size_t n = min (size, fh->length - offset);
	memcpy (buf, fh->string + offset, n);
	rc = n;
      }
      break;
    default:
      rc = -EIO; // should not happen
      break;
    }
  }
  
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
  FileHandle* const fh = (FileHandle*) fi->fh;
  
  if (fh) {
    fh->mode = FILE_NONE;
    talloc_free (fh);
    fi->fh = 0;
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
  .getdir	= fs_getdir,
  .mknod	= fs_mknod,
  .mkdir	= fs_mkdir,
  .symlink	= fs_symlink,
  .unlink	= fs_unlink,
  .rmdir	= fs_rmdir,
  .rename	= fs_rename,
  .link		= fs_link,
  .chmod	= fs_chmod,
  .chown	= fs_chown,
  .truncate	= fs_truncate,
  .utime	= fs_utime,
  .open		= fs_open,
  .read		= fs_read,
  .write	= fs_write,
  .statfs	= fs_statfs,
  .flush      	= NULL,
  .release	= fs_release,
  .fsync	= fs_fsync,
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
stdout_print (Log_Level level, const char *msg)
{
  switch (level) {
  case LOG_ERROR:	printf ("[E] "); break;
  case LOG_WARNING:	printf ("[W] "); break;
  case LOG_INFO:	printf ("[I] "); break;
  case LOG_DEBUG:	printf ("[D] "); break;
  default:
    printf ("[%d] ", (int) level);
    break;
  }
  puts (msg);
}

/*****************************************************************************
 * Usage
 *****************************************************************************/

#ifdef DEBUG
#define DEBUG_DEFAULT_LEVELS	"upnpall,debug,fuse,leak"
#else
#define DEBUG_DEFAULT_LEVELS	"debug,fuse,leak"
#endif

static void
usage (const char* progname)
{
  fprintf 
    (stderr, 
     "usage: %s [options] mountpoint\n"
     "Options:\n"
     "    -h                     print help\n"
     "    -d[levels]             enable debug output (implies -f)\n"
     "    -f                     foreground operation\n"
     "\n"
     "Debug levels are one or more comma separated words :\n"
#ifdef DEBUG
     "    upnperr, upnpall : increasing level of UPnP traces\n"
#endif
     "    error, warn, info, debug : increasing level of djmount traces\n"
     "    fuse : activates FUSE traces\n"
     "    leak, leakfull : enable talloc leak reports at exit\n"
     "'-d' alone defaults to '" DEBUG_DEFAULT_LEVELS "' i.e. all traces.\n"
     "\n",
     progname);
  exit (1);
}


/*****************************************************************************
 * Main
 *****************************************************************************/

int 
main (int argc, char *argv[])
{
  int rc;

  rc = Log_Initialize (stdout_print);
  if (rc != 0) {
    fprintf (stderr, "%s : Error initialising Logger", argv[0]);
    exit (rc); // ---------->
  }  
#ifdef DEBUG
  SetLogFileNames ("/dev/null", "/dev/null");
#endif

  /*
   * Handle options
   */
  char* fuse_argv[32] = { argv[0] };
  int fuse_argc = 1;

#define FUSE_ARG(OPT)					\
  if (fuse_argc >= 31) usage (argv[0]) ;		\
  Log_Printf (LOG_DEBUG, "  Fuse option = %s", OPT);	\
  fuse_argv[fuse_argc++] = OPT

  int opt = 1;
  char* o;
  while ((o = argv[opt++])) {
    if (strcmp(o, "-f") == 0) {
      FUSE_ARG (o);

    } else if (*o != '-') { 
      // mount point
      FUSE_ARG (o);

    } else if (strncmp (o, "-d", 2) == 0) {
      FUSE_ARG ("-f");
      // Parse debug levels
      const char* const levels = (o[2] ? o+2 : DEBUG_DEFAULT_LEVELS);
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
#ifdef DEBUG
	} else if (strcmp (s, "upnperr") == 0) {
	  SetLogFileNames ("/dev/stdout", "/dev/null");
	} else if (strcmp (s, "upnpall") == 0) {
	  SetLogFileNames ("/dev/stdout", "/dev/stdout");
#endif
	} else {
	  fprintf (stderr, "%s : unknown debug level '%s'\n\n", argv[0], s);
	  usage (argv[0]); // ---------->
	}
      }
      free (levels_copy);
      Log_Printf (LOG_DEBUG, "  Debug options = %s", levels);
    } else {
      if (strcmp (o, "-h") != 0)
	fprintf (stderr, "%s : unknown option '%s'\n\n", argv[0], o);
      usage (argv[0]); // ---------->
    }
  }

  // Force Read-only (write operations not implemented yet)
  FUSE_ARG ("-r"); 

#if MY_FUSE_VERSION >= 23
  // try to fill in d_ino in readdir
  FUSE_ARG ("-o");
  FUSE_ARG ("readdir_ino");
#endif

  fuse_argv[fuse_argc] = NULL;


  /*
   * Initialise UPnP Control point and starts FUSE file system
   */

  rc = DeviceList_Start ("urn:schemas-upnp-org:service:ContentDirectory:1",
			 NULL);
  if (rc != UPNP_E_SUCCESS) {
    Log_Printf (LOG_ERROR, "Error starting UPnP Control Point");
    exit (rc); // ---------->
  }

  rc = fuse_main (fuse_argc, fuse_argv, &fs_oper);
  if (rc != 0) {
    Log_Printf (LOG_ERROR, "Error in FUSE main loop = %d", rc);
  }

  Log_Printf (LOG_DEBUG, "Shutting down ...");
  DeviceList_Stop();
  Log_Finish();

  return rc; 
}
