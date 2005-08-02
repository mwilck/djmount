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

#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#if (__GNUC__ >= 3)
/** Use gcc attribute to check printf fns.  a1 is the 1-based index of
 * the parameter containing the format, and a2 the index of the first
 * argument. Some gcc 2.x versions don't handle this properly ?
 */
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
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
