/**
 * neo_string.h
 * 
 * Declarations for string buffer functions.
 */
#ifndef NEO_STRING_H
#define NEO_STRING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define NEO_STR_DEFAULT_CAPACITY 128

/// @brief Dynamically resizing null-terminated text buffer.
typedef struct {
	char* data;         // Character buffer.
	size_t length;      // Length of the string (not counting the null termintor).
	size_t capacity;    // Internal size of the character buffer.
} string_t;

/// @brief Initialize a string structure.
/// @param str String pointer
/// @return True if successful
bool string_init(string_t* str);

/// @brief Free all memory associated with the string. The string becomes invalid after this, and must be
/// @brief re-initialized if you want to use it again.
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

/// @brief Erase all characters from the string. This is different from clear() in that it leaves
/// @brief the string in a valid state to perform operations again later.
/// @param str String pointer
void string_erase_all(string_t* str);

/// @brief Append text to the end of the string.
/// @param str String pointer
/// @param insert Text to insert
/// @param len Number of characters to insert
void string_append(string_t* str, const char* insert, size_t len);

/// @brief Overwrite text in the string. This may extend the string if the position and length
/// @brief go beyond its original end.
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

/// @brief Duplicate the strings contents into a new string. Note that the destination must be an initialized
/// @brief string, and its contents will only be overwritten if this function is successful. This does not
/// @brief guarentee the two strings have the same capacity or contents beyond the null terminator; rather, this only
/// @brief guarentees the two strings have the same length, and that the first (src->length + 1) bytes of both src 
/// @brief and dst are identical.
/// @param src Source string pointer
/// @param dst Destination string pointer
/// @return True if successful
bool string_duplicate(string_t* src, string_t* dst);

/// @brief Create a new string from part of another string. Note that the destination string must be an initialized
/// @brief string, and its contents will only be overwritten if this function is successful.
/// @param src Source string pointer
/// @param pos Position to start at
/// @param len Length of substring
/// @param dst Destination string pointer
/// @return True if successful
bool string_substr(string_t* src, size_t pos, size_t len, string_t* dst);

/// @brief Check if the contents of the strings match. Note that this does not check if the capacities of the
/// @brief strings match, nor their contents beyond the null terminator; rather, this only checks if the two strings
/// @brief have the same length, and if so, if the first (str1->length) characters have a strcmp() value of 0.
/// @param str1 First string pointer
/// @param str2 Second string pointer
/// @return True if equal
bool string_equal(string_t* str1, string_t* str2);

/// @brief Internal function for resizing a strings contents if needed.
/// @param str String pointer
/// @param len The proposed additional length for the string
bool _string_check_resize(string_t* str, size_t len);

/// @brief Internal function for checking if a string is in a valid state.
/// @param str String pointer
/// @return True if memory is still valid & internal values are in expected ranges
bool _string_valid(string_t* str);

#endif // NEO_STRING_H