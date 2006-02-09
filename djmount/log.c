/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
#include <stdbool.h>


/*
 * Function pointer to use for displaying formatted
 * strings. Set on Initialization of device. 
 */
static Log_PrintFunction gPrintFun = NULL;


/*
 * Mutex to control displaying of events
 */
static bool g_initialized = false;
static ithread_mutex_t g_log_mutex;


/*
 * Current log level
 */
static Log_Level g_max_level = LOG_INFO;


/*
 * Authorize colorizing log output
 */
static bool g_colorize = false;

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

static const char* const COLORS[] = {
	[LOG_ERROR]   = VT_RED_BRIGHT,
	[LOG_WARNING] = VT_MAGENTA_BRIGHT,
	[LOG_INFO]    = VT_BLUE,
	[LOG_DEBUG]   = VT_DIM,
};
static int const NB_COLORS = sizeof(COLORS)/sizeof(COLORS[0]);

static const char* const COLOR_UNKNOWN_LEVEL = VT_RED_BRIGHT;



/*****************************************************************************
 * Log_Initialize
 *****************************************************************************/

int
Log_Initialize (Log_PrintFunction print_function )
{
	if (! g_initialized) {
		ithread_mutexattr_t attr;
		
		ithread_mutexattr_init (&attr);
		ithread_mutexattr_setkind_np (&attr,
					      ITHREAD_MUTEX_RECURSIVE_NP);
		ithread_mutex_init (&g_log_mutex, &attr);
		ithread_mutexattr_destroy (&attr);
		g_initialized = true;
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
		g_initialized = false;
		ithread_mutex_destroy (&g_log_mutex);
	}
	return 0;
}


/*****************************************************************************
 * Log_IsActivated
 *****************************************************************************/
bool
Log_IsActivated (Log_Level level) 
{
	return ( g_initialized && gPrintFun && level <= g_max_level );
}


/*****************************************************************************
 * Log_Print 
 *****************************************************************************/
int
Log_Print (Log_Level level, const char* msg)
{
	if (Log_IsActivated (level) && msg) { 
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
	if (Log_IsActivated (level) && fmt) { 
		va_list ap;
		char buf[4096] = "";
		
		va_start (ap, fmt);
		int rc = vsnprintf (buf, sizeof(buf), fmt, ap);
		va_end (ap);
		
		if (rc >= 0) {
			ithread_mutex_lock (&g_log_mutex);
			gPrintFun (level, buf);
			ithread_mutex_unlock (&g_log_mutex);
		}
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


/*****************************************************************************
 * Log_Colorize / Log_BeginColor / Log_EndColor
 *****************************************************************************/
void
Log_Colorize (bool colorize)
{
	g_colorize = colorize;
}

void 
Log_BeginColor (Log_Level level, FILE* stream)
{
	if (Log_IsActivated (level) && stream) { 
		// colorize ?
		const bool colorize = g_colorize && isatty (fileno (stream)); 
		if (colorize) {
			if (level >= 0 && level <= NB_COLORS && COLORS[level])
				fputs (COLORS[level], stream);
			else
				fputs (COLOR_UNKNOWN_LEVEL, stream);
		}
	}
}

void 
Log_EndColor (Log_Level level, FILE* stream)
{
	if (Log_IsActivated (level) && stream) { 
		const bool colorize = g_colorize && isatty (fileno (stream)); 
		if (colorize) {
			fputs (VT_NORMAL, stream);
		}
	}
}

