/**
 * neo.h
 * 
 * Declarations for editor functionality.
 */
#ifndef NEO_H
#define NEO_H

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE


// ============================================== includes

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <libgen.h>
#include <ctype.h>
#include <assert.h>
#include <ncurses.h>
#include <menu.h>


// ============================================== defines

#define NEO_HEADER 2
#define NEO_FOOTER 2
#define NEO_SCROLL_MARGIN 1

#ifndef NEO_MALLOC
#define NEO_MALLOC malloc
#endif

#ifndef NEO_REALLOC
#define NEO_REALLOC realloc
#endif

#ifndef NEO_FREE
#define NEO_FREE free
#endif

/// @brief Flags that define properties of a page in the editor.
enum neo_page_flag_t {
	NPAGE_FLAG_DIRTY =    0x01,   // File has been modified and should be saved before closing.
	NPAGE_FLAG_READONLY = 0x02    // File is marked as read-only and cannot be modified or saved.
};

/// @brief Current state of the editor.
enum neo_state_t {
	NSTATE_OPEN = 1,       // Normal state for reading user input & drawing to the screen.
	NSTATE_PROMPT,         // Prompting user for input on the status bar.
	NSTATE_MENU,           // Selecting an option from a menu group.
	NSTATE_SHOULD_CLOSE    // Editing has finished and the program should clean up & terminate.
};

/// @brief Shorthand for cursor direction in the editor.
enum neo_dir_t {
	NDIR_UP    = 0x01,
	NDIR_DOWN  = 0x02,
	NDIR_LEFT  = 0x04,
	NDIR_RIGHT = 0x08,
};

/// @brief Status codes for editor operation.
enum neo_error_t {
	NERROR_SUCCESS = 0,      // No error.
	NERROR_GENERIC,          // Generic runtime error.
	NERROR_INVALID_PARAM,    // Invalid function parameter.
	NERROR_BAD_ALLOC         // Memory operation failed.
};

extern int _neo_error_code;
extern char _neo_error_msg[1024];
extern const char* _neo_error_desc[];

#define NEO_ERROR_MSG(x, msg) do { \
	_neo_error_code = x; \
	snprintf(_neo_error_msg, sizeof(_neo_error_msg), "Error in function (%s): %s [%s]", __func__, #x, msg); \
} while(0)

#define NEO_ERROR(x) do { \
	_neo_error_code = x; \
	snprintf(_neo_error_msg, sizeof(_neo_error_msg), "Error in function (%s): %s", __func__, #x); \
} while(0)

#define NEO_ERROR_CLEAR do { \
	_neo_error_code = NERROR_SUCCESS; \
	_neo_error_msg[0] = '\0'; \
} while(0)

// ============================================== meta functionality

/// @brief Initialize ncurses library.
void ncurses_init();

/// @brief Deinitialize the ncurses library.
void ncurses_clear();

/// @brief General signal handler.
/// @param sig Signal id
void signal_handler(int sig);

extern bool _neo_flag_resized;


// ============================================== text buffers

#define NEO_STR_DEFAULT_CAPACITY 128

/// @brief Dynamically resizing null-terminated text buffer.
typedef struct {
	char* data;         // Character buffer.
	size_t length;      // Length of the string (not counting the null termintor).
	size_t capacity;    // Internal size of the character buffer.
} string_t;

/// @brief Initialize a string structure.
/// @param str String pointer
void string_init(string_t* str);

/// @brief Free all memory associated with the string.
/// @param str String pointer
void string_clear(string_t* str);

/// @brief Insert text into the string.
/// @param str String pointer
/// @param pos Position to insert at
/// @param insert Text to insert
/// @param len Number of characters to insert
void string_insert(string_t* str, size_t pos, const char* insert, size_t len);

/// @brief Erase characters from the string.
/// @param str String pointer
/// @param pos Position to erase at
/// @param len Number of characters to erase
void string_erase(string_t* str, size_t pos, size_t len);

/// @brief Append text to the end of the string.
/// @param str String pointer
/// @param insert Text to insert
/// @param len Number of characters to insert
void string_append(string_t* str, const char* insert, size_t len);

/// @brief Overwrite text in the string.
/// @param str String pointer
/// @param pos Position to write at
/// @param insert Text to insert
/// @param len Number of characters to overwrite
void string_set(string_t* str, size_t pos, const char* insert, size_t len);

/// @brief Add one character to the end of the string.
/// @param str String pointer
/// @param insert Character to add
void string_push_back(string_t* str, char insert);

/// @brief Remove the last character in the string.
/// @param str String pointer
void string_pop_back(string_t* str);

/// @brief Add one character to the start of the string.
/// @param str String pointer
/// @param insert Character to add
void string_push_front(string_t* str, char insert);

/// @brief Remove the first character in the string.
/// @param str String pointer
void string_pop_front(string_t* str);

/// @brief Get a character in the string.
/// @param str String pointer
/// @param pos Position to check
/// @return Character at that position (or \0 if out of bounds)
char string_at(string_t* str, size_t pos);

/// @brief Check if the string is empty.
/// @param str String pointer
/// @return True if length == 0
bool string_empty(string_t* str);

/// @brief Get the first character in the string.
/// @param str String pointer
/// @return Character at position 0 (or \0 for an empty string)
char string_front(string_t* str);

/// @brief Get the last character in the string.
/// @param str String pointer
/// @return Character at position (length - 1) (or \0 for an empty string)
char string_back(string_t* str);

/// @brief Duplicate the strings contents into a new string.
/// @param str String pointer
/// @return String (must be cleared later)
string_t string_duplicate(string_t* str);

/// @brief Create a new string from part of another string.
/// @param str String pointer
/// @param pos Position to start at
/// @param len Length of substring
/// @return String (must be cleared later)
string_t string_substr(string_t* str, size_t pos, size_t len);

/// @brief Check if the contents of the strings match.
/// @param str1 First string pointer
/// @param str2 Second string pointer
/// @return True if strcmp == 0
bool string_equal(string_t* str1, string_t* str2);

/// @brief Internal function for resizing a strings contents if needed.
/// @param str String pointer
/// @param len The proposed additional length for the string
bool _string_check_resize(string_t* str, size_t len);

/// @brief Internal function for checking if a string is in a valid state.
/// @param str String pointer
/// @return True if memory is still valid & internal values are in expected ranges
bool _string_valid(string_t* str);

#endif // NEO_H