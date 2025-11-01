/**
 * @file neo_common.h
 * @brief Declarations for global functionality.
 */
#ifndef NEO_COMMON_H
#define NEO_COMMON_H

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE


// ============================================== includes

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <libgen.h>
#include <ctype.h>
#include <assert.h>
#include <ncurses.h>
#include <menu.h>
#include <panel.h>


// ============================================== defines

#ifndef NEO_MALLOC
#define NEO_MALLOC malloc
#endif

#ifndef NEO_REALLOC
#define NEO_REALLOC realloc
#endif

#ifndef NEO_FREE
#define NEO_FREE free
#endif


// ============================================== error logging

/**
 * @brief Status codes for editor operation.
 */
enum neo_error_t {
	NERROR_SUCCESS = 0,      // No error.
	NERROR_GENERIC,          // Unspecified runtime error.
	NERROR_INVALID_PARAM,    // Invalid function parameter.
	NERROR_BAD_ALLOC,        // Memory operation failed.
	NERROR_BAD_SETTING,      // Invalid settings value.
	NERROR_OUT_OF_SPACE      // Container has reached maximum size.
};

/**
 * @brief Maximum length for status message buffers.
 */
#define NEO_MSG_BUFLEN 1024

extern int _neo_error_code;
extern char _neo_error_msg[NEO_MSG_BUFLEN];

/**
 * @brief Status code of most recently completed function.
 */
#define NEO_ERRORNO _neo_error_code

/**
 * @brief Error message for most recently completed function.
 */
#define NEO_ERRORMSG _neo_error_msg

/**
 * @brief Set NEO_ERRORNO and set the error message with a custom addition.
 * @param x Error status
 * @param msg Error message
 */
#define NEO_THROW_ERROR_MSG(x, msg) do { \
	NEO_ERRORNO = x; \
	snprintf(NEO_ERRORMSG, sizeof(NEO_ERRORMSG), "Error in function (%s): %s [%s]", __func__, #x, msg); \
} while(0)

/**
 * @brief Set NEO_ERRORNO and set the error message.
 * @param x Error status
 */
#define NEO_THROW_ERROR(x) do { \
	NEO_ERRORNO = x; \
	snprintf(NEO_ERRORMSG, sizeof(NEO_ERRORMSG), "Error in function (%s): %s", __func__, #x); \
} while(0)

/**
 * @brief Reset NEO_ERRORNO to NERROR_SUCCESS and clear the error message.
 */
#define NEO_CLEAR_ERROR do { \
	NEO_ERRORNO = NERROR_SUCCESS; \
	NEO_ERRORMSG[0] = '\0'; \
} while(0)


// ============================================== meta functionality

/**
 * @brief Initialize the ncurses library.
 * @return True if successful
 */
bool ncurses_init();

/**
 * @brief Deinitialize the ncurses library.
 */
void ncurses_clear();

/**
 * @brief General signal handler.
 * @param sig Signal value
 */
void signal_handler(int sig);

extern bool _neo_flag_resized;

#endif // NEO_COMMON_H