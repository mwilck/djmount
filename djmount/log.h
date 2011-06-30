/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* $Id: log.h 157 2006-02-09 21:33:30Z r3mi $
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

#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>


#ifdef PRINTF_ATTRIBUTE
#  undef PRINTF_ATTRIBUTE
#endif
#if (__GNUC__ >= 3)
/** Use gcc attribute to check printf fns.  a1 is the 1-based index of
 * the parameter containing the format, and a2 the index of the first
 * argument. Some gcc 2.x versions don't handle this properly ?
 */
#  define PRINTF_ATTRIBUTE(a1, a2) __attribute__((format (__printf__, a1, a2)))
#else
#  define PRINTF_ATTRIBUTE(a1, a2)
#endif


/** "Not Null" - small macro to properly print a NULL string */
#define NN(s) ((s)?(s):"(null)")


/*****************************************************************************
 * Types
 *****************************************************************************/

/**
 * @brief The 4 predefined log levels
 */
typedef enum Log_Level {

	LOG_RESERVED = -1, // Internal use
	
	LOG_ERROR    = 0,
	LOG_WARNING  = 1,
	LOG_INFO     = 2,
	LOG_DEBUG    = 3

} Log_Level;

#define LOG_LEVEL_MIN	-1
#define LOG_LEVEL_MAX 	3



/**
 * @brief Prototype for displaying strings. 
 * 	All printing ultimately uses this to display strings to the user. 
 *	However, this function is only called when a given log message 
 *	is allowed by the current log level (see Log_SetMaxLevel).
 *	For console output, this function should behave similarly to "puts"
 *	and append a carriage-return to the display.
 *	The string is encoded in UTF-8 (see "charset.h") : it is the
 *	responsibility of this print function to translate to another
 *	charset if necessary.
 */
typedef void (*Log_PrintFunction) (Log_Level level, const char* string);


/*****************************************************************************
 * Macros
 *****************************************************************************/

/** 
 * @brief  Predicate to test for debug ouput.
 * - if debug messages are compiled in the application (-DDEBUG), this
 *   is a dynamic test against the current log level ;
 * - else this is false (constant)
 */
#ifdef DEBUG
#	define LOG_IS_DEBUG_ACTIVATED	Log_IsActivated (LOG_DEBUG)
#else
#	define LOG_IS_DEBUG_ACTIVATED	false
#endif


/*****************************************************************************
 * Functions
 *****************************************************************************/

/**
 * @brief Initializes the logger. 
 *	Must be called before any log functions. 
 *	May be called multiple times.
 *
 *  @param print_function print function to use 
 */
int Log_Initialize (Log_PrintFunction print_function);


/**
 * @brief Releases resources held by logger.
 */
int Log_Finish (void);


/**
 * @brief Log a message
 *	Log a message, if allowed by current log level (see Log_SetMaxLevel).
 *	Ultimately calls the registered print function with the string.
 *
 * @param level log level for the message	
 * @param msg message string. Should not contain the final carriage return.
 */
int Log_Print  (Log_Level level, const char* msg);


/**
 * @brief Log a message
 *	Similar to "Log_Print", but with arguments emulating printf.
 *
 * @param level log level for the message	
 * @param fmt format (see printf), followed by variable number of arguments.
 */
int Log_Printf (Log_Level level, const char* fmt, ...) PRINTF_ATTRIBUTE(2,3);


/**
 * @brief Set the maximum log level
 * 	This function is used to set the maximum log level that is output
 *	(inclusive). Other messages are discarded.
 *	Example:
 *	    Log_SetMaxLevel (LOG_ERROR) -> only ERROR is output
 *	    Log_SetMaxLevel (LOG_INFO)  -> ERROR, WARNING and INFO are output
 *
 *	Default is LOG_INFO i.e. LOG_DEBUG messages are discarded.	
 *
 * @param max_level maximum log level
 */
void Log_SetMaxLevel (Log_Level max_level);


/**
 * @brief Predicate : tests if a log level is output or not
 *	  (see Log_SetMaxLevel).
 *
 * @param level  	log level to test
 */
bool Log_IsActivated (Log_Level level);


/**
 * @brief Functions to lock / unlock the logger.
 *	Functions to lock / unlock the logger, to prevent other threads 
 *	of intermixing their own outputs.
 *	NOTE : a lock is automatically performed for each individual 
 *	"Log_Print", therefore explicit "lock" and "unlock" are only 
 *	necessary when a thread wants to display atomically a large amount 
 *	of text in several "Log_Print" calls.
 */

int Log_Lock (void);
int Log_Unlock (void);


/**
 * @brief Authorize colorizing log output (default : false).
 */
void Log_Colorize (bool colorize);


/**
 * @brief Starts colorizing log output (depending on log severity)
 *	The color is applied only if authorised (see Log_Colorize), 
 *	and if the output stream is a terminal (tty).
 */
void Log_BeginColor (Log_Level level, FILE* stream);


/**
 * @brief Stops colorizing log output. Must match Log_BeginColor().
 */
void Log_EndColor (Log_Level level, FILE* stream);


/*****************************************************************************
 * C++ extensions
 *****************************************************************************/

#ifdef __cplusplus

}; // extern "C"

/**
 * @def LOG_COUT
 * @brief Log macro taking C++ style stream output
 *	Log macro taking C++ style stream output, that ultimately calls 
 *	the registered print function with the formatted string.
 *	Example:
 *		LOG_COUT (LOG_INFO, "a=" << a << ", b=" << b);
 *
 * @param LVL log level
 * @param ARGS C++ stream output
 */

#include <sstream>

#define LOG_COUT(LVL,ARGS) \
{ std::ostringstream oo__ ; oo__ << ARGS ; Log_Print(LVL,oo__.str().c_str()); }

#endif // __cplusplus


#endif // LOG_H_INCLUDED
