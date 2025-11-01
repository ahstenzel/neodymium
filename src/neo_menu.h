/**
 * @file neo_menu.h
 * @brief Definitions for drop-down menu functionality.
 */

#ifndef NEO_MENU_H
#define NEO_MENU_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "neo_string.h"

/**
 * @brief Callback function for a menu entry.
 */
typedef void (*neo_menu_callback_fptr)(void*, int);

/**
 * @brief Default number of menu entries for a newly initialized menu group.
 */
#define NEO_MENU_GROUP_DEFAULT_CAPACITY 4

/**
 * @brief Default number of menu groups for a newly initialized menu bar.
 */
#define NEO_MENU_BAR_DEFAULT_CAPACITY 4

/**
 * @brief Entry for a context menu.
 */
struct neo_menu_entry_t {
	string_t name;
	char shortcut;
	neo_menu_callback_fptr callback;
	bool seperator;
};
typedef struct neo_menu_entry_t neo_menu_entry_t;

/**
 * @brief List of menu entries.
 */
struct neo_menu_group_t {
	string_t name;
	neo_menu_entry_t* entries;
	size_t num_entries;
	size_t max_entries;
	int selected;
	char shortcut;
};
typedef struct neo_menu_group_t neo_menu_group_t;

/**
 * @brief Top-level collection of menu groups.
 */
struct neo_menu_bar_t {
	neo_menu_group_t* groups;
	size_t num_groups;
	size_t max_groups;
	int selected;
};
typedef struct neo_menu_bar_t neo_menu_bar_t;

/**
 * @brief Initialize a menu group structure.
 * @details
 * This takes an existing menu group structure that has been default constructed and allocates memory
 * for it according to the NEO_MENU_GROUP_DEFAULT_CAPACITY. It is considered valid at this point, and
 * must be cleared before the program ends to prevent memory leaks.
 * @param group Menu group pointer
 * @return True if successful
 */
bool neo_menu_group_init(neo_menu_group_t* group);

/**
 * @brief Deinitialize a menu group stricture.
 * @details
 * This frees all memory associated with the menu group. The object becomes invalid after this, and
 * must be re-initialized if you want to use it again.
 * @param group Menu group pointer
 */
void neo_menu_group_clear(neo_menu_group_t* group);

/**
 * @brief Create a new menu entry and insert it into the group.
 * @param group Menu group pointer
 * @param position Position to insert at (or -1 for the end)
 * @param entry_name Name of the entry
 * @param entry_shortcut Shortcut for the entry
 * @param entry_callback Callback function for the entry
 * @return Menu entry pointer (or NULL on error)
 */
neo_menu_entry_t* neo_menu_group_insert_entry(neo_menu_group_t* group, int position, const char* entry_name, char entry_shortcut, neo_menu_callback_fptr entry_callback);

/**
 * @brief Insert a seperator into the menu group
 * @param group Menu group pointer
 * @param position Position to insert at (or -1 for the end)
 */
void neo_menu_group_insert_seperator(neo_menu_group_t* group, int position);

/**
 * @brief Get the menu entry at the given position.
 * @param group Menu group pointer
 * @param position Menu position (or -1 for the end)
 * @return Menu entry pointer (or NULL for a spacer or out-of-bounds)
 */
neo_menu_entry_t* neo_menu_group_get_entry(neo_menu_group_t* group, int position);

/**
 * @brief Get the position of the menu entry with the given name.
 * @param group Menu group pointer
 * @param name Entry name
 * @return Entry position (or -1 on error)
 */
int neo_menu_group_get_entry_position(neo_menu_group_t* group, const char* name);

/**
 * @brief Remove the menu entry at the given position.
 * @param group Menu group pointer
 * @param position Position to remove at (or -1 for the end)
 */
void neo_menu_group_remove_entry(neo_menu_group_t* group, int position);

/**
 * @brief Initialize a menu bar structure.
 * @details
 * This takes an existing menu bar structure that has been default constructed and allocates memory
 * for it according to the NEO_MENU_BAR_DEFAULT_CAPACITY. It is considered valid at this point, and
 * must be cleared before the program ends to prevent memory leaks.
 * @param bar Menu bar pointer
 * @return True if successful
 */
bool neo_menu_bar_init(neo_menu_bar_t* bar);

/**
 * @brief Deinitialize a menu bar structure.
 * @details
 * This frees all memory associated with the menu bar. The object becomes invalid after this, and
 * must be re-initialized if you want to use it again.
 * @param bar Menu bar pointer
 */
void neo_menu_bar_clear(neo_menu_bar_t* bar);

/**
 * @brief Create a new menu group and insert it into the bar.
 * @param bar Menu bar pointer
 * @param position Position to insert at (or -1 for the end)
 * @param group_name Name of the group
 * @param group_shortcut Shortcut for the entry
 * @return Menu group pointer (or NULL on error)
 */
neo_menu_group_t* neo_menu_bar_insert_group(neo_menu_bar_t* bar, int position, const char* group_name, char group_shortcut);

/**
 * @brief Get the menu group at the given position.
 * @param bar Menu bar pointer
 * @param position Menu position (or -1 for the end)
 * @return Menu group pointer (or NULL for out-of-bounds)
 */
neo_menu_group_t* neo_menu_bar_get_group(neo_menu_bar_t* bar, int position);

/**
 * @brief Get the position of the menu group with the given name.
 * @param bar Menu bar pointer
 * @param group_name Group name
 * @return Group position (or -1 on error)
 */
int neo_menu_bar_get_group_position(neo_menu_bar_t* bar, const char* name);

/**
 * @brief Remove the menu group at the given position.
 * @param bar Menu bar pointer
 * @param position Position to remove at (or -1 for the end)
 */
void neo_menu_bar_remove_group(neo_menu_bar_t* bar, int position);

#endif // NEO_MENU_H