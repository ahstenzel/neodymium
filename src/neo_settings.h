/**
 * @file neo_settings.h
 * @brief Definitions for editor settings.
 */

#ifndef NEO_SETTINGS_H
#define NEO_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Default capacity for a newly initialized settings struct.
 */
#define NEO_SETTINGS_DEFAULT_CAPACITY 8

/**
 * @brief Union for different values a setting can have.
 */
typedef union {
	char* val_str;
	unsigned int val_uint;
	int val_int;
	bool val_bool;
} neo_settings_val_t;

/**
 * @brief Map of key-value pairs representing different settings.
 */
typedef struct {
	char** keys;
	neo_settings_val_t* vals;
	size_t length;
	size_t capacity;
} neo_settings_t;

/**
 * @brief Key-value pair for settings.
 */
typedef struct {
	const char* key;
	neo_settings_val_t val;
} neo_settings_entry_t;

/**
 * @brief Default settings values.
 */
extern const neo_settings_entry_t _neo_settings_defaults[];

/**
 * @brief Initialize a settings structure.
 * @details
 * This takes an existing settings structure that has been default constructed and allocates memory
 * for it, populating it with default values. It is considered valid at this point, and must be 
 * cleared before the program ends to prevent memory leaks.
 * @param settings Settings pointer
 * @return True if successful
 */
bool neo_settings_init(neo_settings_t* settings);

/**
 * @brief Deinitialize a settings structure.
 * @details
 * This frees all memory associated with the menu group. The object becomes invalid after this, and
 * must be re-initialized if you want to use it again.
 * @param settings Settings pointer
 */
void neo_settings_clear(neo_settings_t* settings);

/**
 * @brief Insert a new settings value into the structure, overwriting a previous value if it exists.
 * @param settings Settings pointer
 * @param key Setting name
 * @param val Setting value
 * @return True if successful
 */
bool neo_settings_insert(neo_settings_t* settings, const char* key, neo_settings_val_t val);

/**
 * @brief Remove a settings value from the structure.
 * @param settings Settings pointer
 * @param key Setting name
 */
void neo_settings_remove(neo_settings_t* settings, const char* key);

/**
 * @brief Retreive a value from the settings structure if it exists.
 * @details
 * This will check if the given key exists in the settings structure. If so, and if a valid
 * destination pointer is given for `val`, it will copy the value to that destination. The
 * function will return true if the value exists and was coped successfully. If NULL is
 * given for `val`, this will simply return true if the value exists.
 * @param settings Settings pointer
 * @param key Setting name
 * @param val Setting value destination (or NULL)
 * @return True if exists
 */
bool neo_settings_get(neo_settings_t* settings, const char* key, neo_settings_val_t* val);

/**
 * @brief Clear all settings values and reload defaults.
 * @param settings Settings pointer
 * @return True if successful
 */
bool neo_settings_load_defaults(neo_settings_t* settings);

#endif // NEO_SETTINGS_H