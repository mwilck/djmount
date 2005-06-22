/* $Id$
 *
 * Log facilities.
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


#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <upnp/ithread.h>



/*
 * Function pointer to use for displaying formatted
 * strings. Set on Initialization of device. 
 */
static Log_PrintFunction gPrintFun = NULL;


/*
 * Mutex to control displaying of events
 */
static int g_initialized = 0;
static ithread_mutex_t g_log_mutex;


/*
 * Current log level
 */
static Log_Level g_max_level = LOG_INFO;



/*****************************************************************************
 * Log_Initialize
 *****************************************************************************/

int
Log_Initialize (Log_PrintFunction print_function )
{
  if (! g_initialized) {
    ithread_mutexattr_t attr;
    
    ithread_mutexattr_init (&attr);
    ithread_mutexattr_setkind_np (&attr, ITHREAD_MUTEX_RECURSIVE_NP );
    ithread_mutex_init (&g_log_mutex, &attr);
    ithread_mutexattr_destroy (&attr);
    g_initialized = 1;
  }
  gPrintFun = print_function;
  return 0;
}


/*****************************************************************************
 * Log_Finish
 *****************************************************************************/
int
Log_Finish ()
{
  gPrintFun = NULL;
  if (g_initialized) {
    g_initialized = 0;
    ithread_mutex_destroy (&g_log_mutex);
  }
  return 0;
}

/*****************************************************************************
 * Log_Print 
 *****************************************************************************/
int
Log_Print (Log_Level level, const char* msg)
{
  if (g_initialized && gPrintFun && level <= g_max_level) { 
    
    ithread_mutex_lock (&g_log_mutex);
    gPrintFun (level, msg);
    ithread_mutex_unlock (&g_log_mutex);
  }
  return 0;
}


/*****************************************************************************
 * Log_Printf
 *****************************************************************************/
int
Log_Printf (Log_Level level, const char* fmt, ... )
{
  if (g_initialized && gPrintFun && level <= g_max_level) { 
    va_list ap;
    char buf[4096];
    int rc;
    
    va_start( ap, fmt );
    rc = vsnprintf( buf, sizeof(buf), fmt, ap );
    va_end( ap );
    
    ithread_mutex_lock (&g_log_mutex);
    gPrintFun (level, buf);
    ithread_mutex_unlock (&g_log_mutex);
    
    return rc;
  }
  return -1;
}



/*****************************************************************************
 * Log_SetMaxLevel
 *****************************************************************************/
void 
Log_SetMaxLevel (Log_Level max_level)
{
  g_max_level = max_level;
}


/*****************************************************************************
 * Log_Lock / Log_Unlock
 *****************************************************************************/

int
Log_Lock()
{
  return ithread_mutex_lock (&g_log_mutex);
}

int
Log_Unlock()
{
  return ithread_mutex_unlock (&g_log_mutex);
}
