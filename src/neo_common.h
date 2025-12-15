/**
 * @file neo_common.h
 * @brief Declarations for global functionality.
 */
#ifndef NEO_COMMON_H
#define NEO_COMMON_H

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif


// ============================================== version strings

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X)
#define MAKE_VERSION_STR(major, minor, patch) (STRINGIFY(major) "." STRINGIFY(minor) "." STRINGIFY(patch))

#ifndef NEO_VERSION_MAJOR
#define NEO_VERSION_MAJOR 0
#endif
#ifndef NEO_VERSION_MINOR
#define NEO_VERSION_MINOR 0
#endif
#ifndef NEO_VERSION_PATCH
#define NEO_VERSION_PATCH 0
#endif

#define NEO_VERSION_STR MAKE_VERSION_STR(NEO_VERSION_MAJOR, NEO_VERSION_MINOR, NEO_VERSION_PATCH)


// ============================================== includes

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <libgen.h>
#include <ctype.h>
#include <assert.h>
#include <locale.h>
//#define NEO_USE_WCHAR
#ifdef NEO_USE_WCHAR
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif
#include <menu.h>
#include <panel.h>


// ============================================== defines

#ifdef NEO_USE_WCHAR

#include <wchar.h>

/**
 * @brief Character type used for strings.
 */
#define NEO_CHAR_T wchar_t

#define NEO_STR_CAST(s) L ## s

#define neo_memmove wmemmove
#define neo_memcpy wmemcpy
#define neo_strlen wcslen
#define neo_strcmp wcscmp
#define neo_strncmp wcsncmp
#define neo_snprintf swprintf
#define neo_vsnprintf vswprintf
#define neo_getline getwline
#define neo_strchr wcschr
#define neo_strrchr wcsrchr

#define neo_waddnstr waddnwstr
#define neo_waddch waddch
#define neo_wvline wvline
#define neo_hvline whline

#else

/**
 * @brief Character type used for strings.
 */
#define NEO_CHAR_T char

#define NEO_STR_CAST(s) s

#define neo_memmove memmove
#define neo_memcpy memcpy
#define neo_strlen strlen
#define neo_strcmp strcmp
#define neo_strncmp strncmp
#define neo_snprintf snprintf
#define neo_vsnprintf vsnprintf
#define neo_getline getline
#define neo_strchr strchr
#define neo_strrchr strrchr

#define neo_waddnstr waddnstr
#define neo_waddch waddch
#define neo_wvline wvline
#define neo_hvline whline

#endif

#ifndef NEO_MALLOC
#define NEO_MALLOC malloc
#endif

#ifndef NEO_REALLOC
#define NEO_REALLOC realloc
#endif

#ifndef NEO_FREE
#define NEO_FREE free
#endif

#define NEO_SIZE_MENU_BAR 2
#define NEO_SIZE_FILE_BAR 3
#define NEO_SIZE_HEADER (NEO_SIZE_MENU_BAR+NEO_SIZE_FILE_BAR)
#define NEO_SIZE_FOOTER 2
#define NEO_SCROLL_MARGIN 3


// ============================================== functional macros

#if !__STRICT_ANSI__ && __GNUC__ >= 3
	#define MIN(a,b) ({ __typeof__ (a) _a=(a); __typeof__ (b) _b=(b); _a<_b ? _a : _b; })
	#define MAX(a,b) ({ __typeof__ (a) _a=(a); __typeof__ (b) _b=(b); _a>_b ? _a : _b; })
	#define CLAMP(a, l, u) ({ __typeof__ (a) _a=(a); __typeof__ (l) _l=(l); __typeof__ (u) _u=(u); _a>_u ? _u : (_a<_l ? _l : _a); })
#else
	#define MIN(a, b) ((a) < (b)) ? (a) : (b)
	#define MAX(a, b) ((a) > (b)) ? (a) : (b)
	#define CLAMP(a, l, u) ((a) > (u)) ? (u) : (((a) < (l)) ? (l) : (a))
#endif

#define UNUSED(x) ((void)(x))

#define COUNT_OF(a) (sizeof(a)/sizeof(a[0]))

#define CTRL_KEY(x) ((x) & 0x1f)

/**
 * @brief Shortcut to move the ncurses window cursor one row down.
 */
#define wmove_cursor_down(win, n) ({ int _x, _y; getyx((win), _y, _x); wmove((win), _y + (n), _x); })

/**
 * @brief Shortcut to move the ncurses window cursor one row up.
 */
#define wmove_cursor_up(win, n) ({ int _x, _y; getyx((win), _y, _x); wmove((win), _y - (n), _x); })

/**
 * @brief Shortcut to move the ncurses window cursor one column left.
 */
#define wmove_cursor_left(win, n) ({ int _x, _y; getyx((win), _y, _x); wmove((win), _y, _x - (n)); })

/**
 * @brief Shortcut to move the ncurses window cursor one column right.
 */
#define wmove_cursor_right(win, n) ({ int _x, _y; getyx((win), _y, _x); wmove((win), _y, _x + (n)); })


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

extern int _neo_ext_signal;

typedef enum {
	NCHOICE_OK = 0,
	NCHOICE_NO,
	NCHOICE_CANCEL
} neo_choice_t;

#include "neo_string.h"

bool neo_dialog_message(string_t* message);

bool neo_dialog_choice(string_t* message, neo_choice_t* result);

bool neo_dialog_file(string_t* message, string_t* filename);


// ============================================== error logging

/**
 * @brief Status codes for editor operation.
 */
enum neo_error_t {
	NERROR_SUCCESS = 0,      // No error.
	NERROR_GENERIC,          // Unspecified runtime error.
	NERROR_UNIMPLEMENTED,    // Unimplemented feature.
	NERROR_INVALID_PARAM,    // Invalid function parameter.
	NERROR_BAD_SETTING,      // Invalid settings value.
	NERROR_OUT_OF_SPACE,     // Container has reached maximum size.
	NERROR_BAD_ALLOC         // Memory operation failed.
};

/**
 * @brief Maximum length for status message buffers.
 */
#define NEO_MSG_BUFLEN 1023

extern int _neo_error_code;
extern NEO_CHAR_T _neo_error_msg[NEO_MSG_BUFLEN + 1];
extern bool _neo_error_enable;

/**
 * @brief Status code of most recently completed function.
 */
#define NEO_ERRORNO _neo_error_code

/**
 * @brief Error message for most recently completed function.
 */
#define NEO_ERRORMSG _neo_error_msg

/**
 * @brief If false, suppress all error messages.
 */
#define NEO_ERROREN _neo_error_enable

/**
 * @brief Set NEO_ERRORNO, set the error message string, and pop up an error dialog.
 * @param err Error status
 * @param msg Error message
 */
#define NEO_THROW_ERROR_MSG(err, msg) do { \
	if (NEO_ERROREN) { \
		if (err > NEO_ERRORNO) NEO_ERRORNO = err; \
		size_t err_len_ = neo_snprintf(NEO_ERRORMSG, NEO_MSG_BUFLEN, "Error in function (%s): %s [%s]", __func__, #err, msg); \
		if (NEO_ERRORNO != NERROR_BAD_ALLOC) { \
			bool prev_erren_ = NEO_ERROREN; \
			NEO_ERROREN = false; \
			string_t err_message_ = { 0 }; \
			string_init(&err_message_); \
			string_set(&err_message_, 0, &NEO_ERRORMSG[0], MIN(NEO_MSG_BUFLEN, err_len_)); \
			neo_dialog_message(&err_message_); \
			string_clear(&err_message_); \
			NEO_ERROREN = prev_erren_; \
		} \
		else { fprintf(stderr, "%s\n", NEO_ERRORMSG); } \
	} \
} while(0)

/**
 * @brief Set NEO_ERRORNO and set the error message string.
 * @param err Error status
 */
#define NEO_THROW_ERROR(err, msg) do { \
	if (err > NEO_ERRORNO) NEO_ERRORNO = err; \
	neo_snprintf(NEO_ERRORMSG, sizeof(NEO_ERRORMSG), "Error in function (%s): %s [%s]", __func__, #err, msg); \
} while(0)

/**
 * @brief Reset NEO_ERRORNO to NERROR_SUCCESS and clear the error message.
 */
#define NEO_CLEAR_ERROR do { \
	NEO_ERRORNO = NERROR_SUCCESS; \
	NEO_ERRORMSG[0] = '\0'; \
} while(0)

#endif // NEO_COMMON_H