/**
 * @file neo_string.h
 * @brief Declarations for string buffer functions.
 */
#ifndef NEO_STRING_H
#define NEO_STRING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Default capacity for a newly initialized string.
 */
#define NEO_STRING_DEFAULT_CAPACITY 128

/**
 * @brief Dynamically resizing null-terminated text buffer.
 */
typedef struct {
	char* data;         /** Character data. */
	size_t length;      /** Length of the string (not counting the null termintor). */ 
	size_t capacity;    /** Internal size of the character buffer. */
} string_t;

/**
 * @brief Initialize a string structure.
 * @details
 * This takes an existing string structure that has been default constructed and allocates memory
 * for it according to the NEO_STRING_DEFAULT_CAPACITY. It is considered valid at this point, and
 * must be cleared before the program ends to prevent memory leaks.
 * @param str String pointer
 * @return True if successful
 */
bool string_init(string_t* str);

/**
 * @brief Deinitialize a string stricture.
 * @details
 * This frees all memory associated with the string. The string becomes invalid after this, and
 * must be re-initialized if you want to use it again.
 * @param str String pointer
 */
void string_clear(string_t* str);

/**
 * @brief Insert text into the string.
 * @param str String poitner
 * @param pos Position to insert at
 * @param insert Text to insert
 * @param len Number of characters to insert
 * @return True if successful
 */
bool string_insert(string_t* str, size_t pos, const char* insert, size_t len);

/**
 * @brief Erase characters from the string.
 * @param str String pointer
 * @param pos Position to erase at
 * @param len Number of characters to erase
 * @return True if successful
 */
bool string_erase(string_t* str, size_t pos, size_t len);

/**
 * @brief Erase all characters from the string.
 * @details
 * This is a shorthand for erase() that erases all characters in the string. This is different from
 * clear() in that it does not deallocate memory, and leaves the string in a valid state to
 * perform operations later.
 * @param str String pointer
 * @return True if successful
 */
bool string_erase_all(string_t* str);

/**
 * @brief Append characters to the end of the string.
 * @param str String pointer
 * @param insert Text to insert
 * @param len Number of characters to insert
 * @return True if successful
 */
bool string_append(string_t* str, const char* insert, size_t len);

/**
 * @brief Overwrite text in the string.
 * @details
 * Starting at the given position, this will overwrite characters in the string with the given
 * text. If the text exceeds the length of the string from that starting point, it will extend
 * the string as necessary.
 * @param str String pointer
 * @param pos Position to write at
 * @param insert Text to write
 * @param len Number of characters to overwrite (if < 0, will calculate with strlen)
 * @return True if successful
 */
bool string_set(string_t* str, size_t pos, const char* insert, int len);

/**
 * @brief Add one character to the end of the string.
 * @param str String pointer
 * @param insert Character to add
 * @return True if successful
 */
bool string_push_back(string_t* str, char insert);

/**
 * @brief Remove one character from the end of the string.
 * @param str String pointer
 * @return True if successful
 */
bool string_pop_back(string_t* str);

/**
 * @brief Add one character to the start of the string.
 * @param str String pointer
 * @param insert Character to add
 * @return True if successful
 */
bool string_push_front(string_t* str, char insert);

/**
 * @brief Remove one character from the start of the string.
 * @param str String pointer
 * @return True if successful
 */
bool string_pop_front(string_t* str);

/**
 * @brief Get a character at a position in the string.
 * @param str String pointer
 * @param pos Position to check
 * @return Character (or \0 if out of bounds)
 */
char string_at(string_t* str, size_t pos);

/**
 * @brief Check if the string has no characters.
 * @param str String pointer
 * @return True if length == 0
 */
bool string_empty(string_t* str);

/**
 * @brief Get the character at the start of the string.
 * @param str String pointer
 * @return Character at position 0 (or \0 for an empty string)
 */
char string_front(string_t* str);

/**
 * @brief Get the character at the end of the string.
 * @param str String pointer
 * @return Character at position (length - 1) (or \0 for an empty string)
 */
char string_back(string_t* str);

/**
 * @brief Duplicate the strings contents.
 * @details
 * This will copy every character from src into dst, overwriting whatever was there before. Note
 * that the destination must be an initialized string, and its contents will only be overwritten
 * if this function is successful. This does not guarentee the strings will have the same capacity,
 * nor the same contents past the null terminator; rather, this only guarentees that the two strings
 * have the same length, and the first (src->length + 1) bytes of both src and dst are identical.
 * @param src Source string pointer
 * @param dst Destination string pointer
 * @return True if successful
 */
bool string_duplicate(string_t* src, string_t* dst);

/**
 * @brief Create a new string from part of another string.
 * @details
 * This will copy a portion of src into dst, overwriting whatever was there before. Note that the 
 * destination must be an initialized string, and its contents will only be overwritten if this 
 * function is successful.
 * @param src Source string pointer
 * @param pos Position to start at
 * @param len Length of substring
 * @param dst Destination string pointer
 * @return True if successful
 */
bool string_substr(string_t* src, size_t pos, size_t len, string_t* dst);

/**
 * @brief Check if the contents of the strings match.
 * @details
 * Check if the two strings are the same length and have a strcmp() value of 0. Note that 
 * this does not check if the capacities of the two strings are the same, nor does it check 
 * any characters beyond the null terminator.
 * @param str1 First string pointer
 * @param str2 Second string pointer
 * @return True if equal
 */
bool string_equal(string_t* str1, string_t* str2);

/**
 * @brief Resize the strings buffer to be at least greater than the given capacity.
 * @param str String pointer
 * @param new_capacity New capacity
 * @return New capacity (or -1 on error)
 */
int string_reserve(string_t* str, size_t new_capacity);

/**
 * @brief Insert newlines into the string to not go over the given max width.
 * @param str String pointer
 * @param max_width Max length before a newline
 * @return Number of lines the string takes up (or -1 on error)
 */
int string_wrap(string_t* str, size_t max_width);

#endif // NEO_STRING_H