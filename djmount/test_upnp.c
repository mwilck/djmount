/* $Id$
 *
 * Testing of UPnP classes.
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
 

#include "device_list.h"
#include "log.h"
#include "content_dir.h"
#include "djfs.h"
#include "log.h"
#include "charset.h"

#include <talloc.h>

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#ifdef HAVE_READLINE
#    include <readline/readline.h>
#    include <readline/history.h>
#endif


// UPnP search target
#define UPNP_TARGET	"ssdp:all"


/*
 * Tags for valid commands issued at the command prompt 
 */
typedef enum CommandType {
  CMD_HELP = 0, 
  CMD_LOGLEVEL, 
  CMD_LEAK, 
  CMD_LEAK_FULL,
  CMD_BROWSE, 
  CMD_METADATA, 
  CMD_LS,
  CMD_ACTION, 
  CMD_PRINTDEV, 
  CMD_LISTDEV, 
  CMD_REFRESH, 
  CMD_EXIT
} CommandType;

/*
 * Data structure for parsing commands from the command line 
 */
struct CommandStruct {
  const char*  str;             // the string 
  CommandType  cmdnum;         	// the command
  int          numargs;		// the number of arguments
  const char*  args;            // the args
};


/*
 * Mappings between command text names, command tag,
 * and required command arguments for command line commands 
 */
static const struct CommandStruct CMDLIST[] = {
  { "help", 	CMD_HELP, 1, ""},
  { "loglevel", CMD_LOGLEVEL, 2, "<max log level (0-3)>"},
  { "leak", 	CMD_LEAK, 1, ""},
  { "leakfull", CMD_LEAK_FULL, 1, ""},
  { "listdev", 	CMD_LISTDEV, 1, ""},
  { "refresh", 	CMD_REFRESH, 1, ""},
  { "printdev", CMD_PRINTDEV, 2, "<devname>"},
  { "browse", 	CMD_BROWSE, 3, "<devname> <objectId>"},
  { "metadata", CMD_METADATA, 3, "<devname> <objectId>"},
  { "ls", 	CMD_LS, 3, "<devname> <path>"},
  { "action", 	CMD_ACTION, 4, "<devname> <serviceId> <actionName>"},
  { "exit", 	CMD_EXIT, 1, ""}
};

static const int CMDNUM = sizeof(CMDLIST)/sizeof(CMDLIST[0]);


/*****************************************************************************
 * @fn 		stdout_print 
 * @brief 	Output log messages.
 *
 * Parameters:
 * 	See Log_PrintFunction prototype.
 *
 *****************************************************************************/

// Special "internal" log level for main module
#define LOG_MAIN 	LOG_RESERVED


static void
stdout_print (Log_Level level, const char* const msg)
{
  // ANSI Color codes
#define VT(CODES)		"\033[" CODES "m"
#define VT_NORMAL 		VT("0")
#define VT_DIM			VT("2")
#define VT_RED			VT("31")
#define VT_RED_BRIGHT		VT("31;1")
#define VT_GREEN 		VT("32")
#define VT_YELLOW_BRIGHT	VT("33;1")
#define VT_BLUE 		VT("34")
#define VT_MAGENTA_BRIGHT	VT("35;1")


  // colorize ?
  const bool col = isatty (fileno (stdout)); 

  switch (level) {
  case LOG_ERROR:    printf ("%s[E] ", (col ? VT_RED_BRIGHT : ""));     break;
  case LOG_WARNING:  printf ("%s[W] ", (col ? VT_MAGENTA_BRIGHT : "")); break;
  case LOG_INFO:     printf ("%s[I] ", (col ? VT_BLUE : ""));           break;
  case LOG_DEBUG:    printf ("%s[D] ", (col ? VT_DIM : ""));            break;
  case LOG_MAIN:     break; // Do not tag internal message from main module
  default:
    printf ("%s[%d] ", (col ? VT_RED_BRIGHT : ""), (int) level);
    break;
  }

  // TBD print thread id ?

  // Convert message to display charset 
  const size_t size = Charset_FromUtf8Size (msg);
  char buffer [size];
  const char* const str = Charset_FromUtf8 (msg, buffer, size);

  printf ("%s%s\n", (str ? str : msg), (col ? VT_NORMAL : ""));
}



/*****************************************************************************
 * print_commands
 *
 * Description: 
 *       Print the list of valid command line commands to the user
 *****************************************************************************/
static void
print_commands()
{
  int i;
  
  Log_Lock();
  Log_Printf (LOG_MAIN, "Valid Commands:");
  for (i = 0; i < CMDNUM; i++) {
    Log_Printf (LOG_MAIN, "  %-14s %s", CMDLIST[i].str, CMDLIST[i].args );
  }
  Log_Print (LOG_MAIN, "");
  Log_Unlock();
}

/*****************************************************************************
 * process_command
 *
 * Description: 
 *	Parse a command line and calls the appropriate functions
 *****************************************************************************/
static int
process_command (const char* cmdline)
{
  int rc = UPNP_E_SUCCESS;  

  // Create a working context for temporary memory allocations
  void* tmp_ctx = talloc_new (NULL);

  // Convert from display charset to UTF-8
  cmdline = Charset_ToUtf8_talloc (tmp_ctx, cmdline);

  char cmd[100];
  char strarg1[100];
  char strarg2[100];
  char strarg3[100];
  int validargs = sscanf (cmdline, "%99s %99s %99s %99s", 
			  cmd, strarg1, strarg2, strarg3);
  int invalidargs = 0;
  int cmdnum = -1;

  int i;
  for (i = 0; i < CMDNUM; i++) {
    if (strcasecmp (cmd, CMDLIST[i].str) == 0 ) {
      cmdnum = CMDLIST[i].cmdnum;
      if (validargs != CMDLIST[i].numargs) {
	invalidargs++;
	goto cleanup; // ---------->
      }
    }
  }
  
  switch (cmdnum) {
  case CMD_HELP:
    print_commands();
    break;
    
  case CMD_LOGLEVEL: 
    {
      // Parse log level
      int level;
      if (sscanf (strarg1, "%d", &level) == 1)
	Log_SetMaxLevel ((Log_Level) level);
      else
	invalidargs++;
    }
    break;

  case CMD_LEAK:
    talloc_report (NULL, stdout);
    break;

  case CMD_LEAK_FULL:
    talloc_report_full (NULL, stdout);
    break;

  case CMD_BROWSE:
    {
      const ContentDir_BrowseResult* res = NULL;
      DEVICE_LIST_CALL_SERVICE (res, strarg1, CONTENT_DIR_SERVICE_ID,
				ContentDir, BrowseChildren, 
				tmp_ctx, strarg2);
      if (res) {
	const ContentDir_Object* o = res->children->objects;
	while (o) {
	  Log_Printf (LOG_MAIN, "  %s", NN(o->title));
	  o = o->next;
	}
      }
    }
    break;

  case CMD_METADATA:
    {
      const ContentDir_Object* o = NULL;
      DEVICE_LIST_CALL_SERVICE (o, strarg1, CONTENT_DIR_SERVICE_ID,
				ContentDir, BrowseMetadata,
				tmp_ctx, strarg2);
      if (o) {
	Log_Printf (LOG_MAIN, "  %s", NN(o->title));
      }
    }
    break;

  case CMD_LS:
    {
      size_t nb_matched = 0;
      const ContentDir_BrowseResult* res = 
	_DJFS_BrowseCDS (tmp_ctx, strarg1, strarg2, &nb_matched);
      if (res) {
	const ContentDir_Object* o = res->children->objects;
	while (o) {
	  Log_Printf (LOG_MAIN, "  %s", NN(o->title));
	  o = o->next;
	}
      }
    }
    break;

  case CMD_ACTION:
    rc = DeviceList_SendActionAsync (strarg1, strarg2, strarg3, 0, NULL);
    break;
    
  case CMD_PRINTDEV:
    rc = DeviceList_PrintDeviceStatus (LOG_MAIN, strarg1);
    break;
    
  case CMD_LISTDEV:
    {
      char* s = DeviceList_GetStatusString (tmp_ctx);
      Log_Printf (LOG_MAIN, "DeviceList:\n%s", NN(s));
    }
    break;
    
  case CMD_REFRESH:
    rc = DeviceList_RefreshAll (UPNP_TARGET);
    break;
    
  case CMD_EXIT:
    rc = DeviceList_Stop();
    exit (rc); // ---------->
    break;

  default:
    cmdnum = -1;
    break;
  }

 cleanup:
  
  if (cmdnum < 0) {
    Log_Printf (LOG_ERROR, "Command not found: '%s' ; try 'Help'", cmd);
    rc = UPNP_E_INVALID_PARAM;

  } else if (invalidargs) { 
    Log_Printf (LOG_ERROR, "Invalid args in command; see 'Help'" );
    rc = UPNP_E_INVALID_PARAM;

  } else if (rc != UPNP_E_SUCCESS) {
    Log_Printf (LOG_ERROR, "Error executing '%s' : %d (%s)",
		cmd, rc, UpnpGetErrorMessage (rc));
  }
    
  // Delete all temporary storage
  talloc_free (tmp_ctx);
  tmp_ctx = NULL;

  return rc;
}


/*****************************************************************************
 * CommandLoop
 *
 * Description: 
 *       Thread that receives commands from the user at the command prompt,
 *	 and calls the appropriate functions for those commands.
 *
 *****************************************************************************/
static void*
CommandLoop (void* arg)
{
#ifdef HAVE_READLINE
  rl_inhibit_completion = true;
  while (1) {
    char* line = readline (">> ");
    char* s = String_StripSpaces (talloc_autofree_context(), line);
    if (s) {
      if (*s) {
	add_history (s); // Add to history only non blank lines
	process_command (s);
      }
      talloc_free (s);
    }
    free (line);
  }
#else
  while (1) {
    char cmdline[100];
    Log_Printf (LOG_MAIN, "\n>> " );
    fgets (cmdline, sizeof (cmdline), stdin);
    if (*cmdline)
      process_command (cmdline);
  }
#endif
}


int
main (int argc, char** argv)
{
  int rc;
  ithread_t cmdloop_thread;
  int sig;

  talloc_enable_leak_report();
  
  rc = Log_Initialize (stdout_print);
  if ( rc != 0 ) {
    Log_Printf (LOG_ERROR, "Error initialising Control Point");
    exit (rc); // ---------->
  }  
  Log_SetMaxLevel (LOG_DEBUG);

  rc = Charset_Initialize (NULL);
  if (rc) 
    Log_Printf (LOG_ERROR, "Error initialising charset");

  rc = DeviceList_Start (UPNP_TARGET, NULL);
  if( rc != UPNP_E_SUCCESS ) {
    Log_Printf (LOG_ERROR, "Error starting UPnP Control Point");
    exit (rc); // ---------->
  }
  // start a command loop thread
  rc = ithread_create (&cmdloop_thread, NULL, CommandLoop, NULL);
  
  /*
   * Catch Ctrl-C and properly shutdown 
   */
  sigset_t sigs_to_catch;
  sigemptyset( &sigs_to_catch );
  sigaddset( &sigs_to_catch, SIGINT );
  sigwait( &sigs_to_catch, &sig );

#ifdef HAVE_READLINE
  // Manually reset readline (we have overridden its SIGINT handler)
  rl_cleanup_after_signal();
#endif
  
  Log_Printf (LOG_WARNING, "Shutting down on signal %d...", sig);
  rc = DeviceList_Stop();

  Charset_Finish();
  Log_Finish();

  exit (rc);
}
