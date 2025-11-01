/**
 * @file neo_editor.h
 * @brief Definitions for editor functionality.
 */

#ifndef NEO_EDITOR_H
#define NEO_EDITOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "neo_string.h"
#include "neo_settings.h"

#define NEO_HEADER 2
#define NEO_FOOTER 2
#define NEO_SCROLL_MARGIN 1

/**
 * @brief Default number of rows for a newly initialized editor page.
 */
#define NEO_EDIT_PAGE_DEFAULT_CAPACITY 8

/**
 * @brief Default number of pages for a newly initialized editor context.
 */
#define NEO_EDIT_CTX_DEFAULT_PAGE_CAPACITY 4

/**
 * @brief Maximum number of files that can be open at once.
 */
#define NEO_EDIT_MAX_OPEN_FILES FOPEN_MAX

/**
 * @brief Flags that define properties of a page in the editor.
 */
typedef enum {
	NPAGE_FLAG_DIRTY =    0x01,   // File has been modified and should be saved before closing.
	NPAGE_FLAG_READONLY = 0x02    // File is marked as read-only and cannot be modified or saved.
} neo_page_flag_t;

/**
 * @brief Current state of the editor.
 */
typedef enum {
	NSTATE_INVALID = 0,    // Default state before context is initialized.
	NSTATE_OPEN,           // Normal state for reading user input & drawing to the screen.
	NSTATE_PROMPT,         // Prompting user for input on the status bar.
	NSTATE_MENU,           // Selecting an option from a menu group.
	NSTATE_SHOULD_CLOSE    // Editing has finished and the program should clean up & terminate.
} neo_state_t;

/**
 * @brief Shorthand for cursor direction in the editor.
 */
typedef enum {
	NDIR_UP    = 0x01,
	NDIR_DOWN  = 0x02,
	NDIR_LEFT  = 0x04,
	NDIR_RIGHT = 0x08,
} neo_dir_t;

struct neo_edit_page_t;
struct neo_edit_ctx_t;

/**
 * @brief A single line of text in a file.
 */
struct neo_edit_row_t {
	struct neo_edit_page_t* page;
	string_t content;
	string_t rcontent;
	bool dirty;
};

/**
 * @brief A single file in the editor.
 */
struct neo_edit_page_t {
	struct neo_edit_ctx_t* context;
	struct neo_edit_row_t* rows;
	string_t filename;
	size_t num_rows;
	size_t max_rows;
	size_t num_cols;
	size_t row_off, col_off;
	size_t cursor_x, cursor_y, rcursor_x, rcursor_y;
	neo_page_flag_t flags;
};

/**
 * @brief Top-level state of the editor.
 */
struct neo_edit_ctx_t {
	struct neo_edit_page_t* pages;
	neo_settings_t settings;
	size_t num_pages;
	size_t max_pages;
	size_t curr_page;
	neo_state_t state;
};

typedef struct neo_edit_row_t neo_edit_row_t;
typedef struct neo_edit_page_t neo_edit_page_t;
typedef struct neo_edit_ctx_t neo_edit_ctx_t;

/**
 * @brief Initialize a row structure.
 * @details
 * This takes an existing row structure that has been default constructed and allocates memory
 * for it. It is considered valid at this point, and must be cleared before the program ends to 
 * prevent memory leaks.
 * @param row Row pointer
 * @param page Parent page pointer
 * @return True if successful
 */
bool neo_edit_row_init(neo_edit_row_t* row, neo_edit_page_t* page);

/**
 * @brief Deinitialize a row structure.
 * @details
 * This frees all memory associated with the row. The object becomes invalid after this, and
 * must be re-initialized if you want to use it again.
 * @param row Row pointer
 */
void neo_edit_row_clear(neo_edit_row_t* row);

/**
 * @brief Render the special characters in the rows text.
 * @param row Row pointer
 * @return True if successful
 */
bool neo_edit_row_update(neo_edit_row_t* row);

/**
 * @brief Calculate the cursors correct position in the rows rendered text.
 * @param row Row pointer
 * @param cx Absolute cursor position
 * @return Rendered cursor position (or SIZE_MAX on error)
 */
size_t neo_edit_row_cursor_update(neo_edit_row_t* row, size_t cx);

/**
 * @brief Get the number of characters in the row of text.
 * @param row Row pointer
 * @return Row length (or SIZE_MAX on error)
 */
size_t neo_edit_row_length(neo_edit_row_t* row);

/**
 * @brief Initialize a page structure.
 * @details
 * This takes an existing page structure that has been default constructed and allocates memory
 * for it according to the NEO_EDIT_PAGE_DEFAULT_CAPACITY. It is considered valid at this point, and
 * must be cleared before the program ends to prevent memory leaks.
 * @param page Page pointer
 * @param context Parent context pointer
 * @return True if successful
 */
bool neo_edit_page_init(neo_edit_page_t* page, neo_edit_ctx_t* context);

/**
 * @brief Deinitialize a page structure.
 * @details
 * This frees all memory associated with the page. The object becomes invalid after this, and
 * must be re-initialized if you want to use it again.
 * @param page Page pointer
 */
void neo_edit_page_clear(neo_edit_page_t* page);

/**
 * @brief Update all rows within the page.
 * @param page Page pointer
 * @return True if successful
 */
bool neo_edit_page_update(neo_edit_page_t* page);

/**
 * @brief Get the row at the given position.
 * @param page Page pointer
 * @param at Row index (or -1 for the last row)
 * @return Row pointer (or NULL on error)
 */
neo_edit_row_t* neo_edit_page_get_row(neo_edit_page_t* page, int at);

/**
 * @brief Insert a new row at the given position.
 * @param page Page pointer
 * @param at Row index (or -1 for the end)
 * @return New row pointer (or NULL on error)
 */
neo_edit_row_t* neo_edit_page_insert_row(neo_edit_page_t* page, int at);

/**
 * @brief Remove the row at the given position.
 * @param page Page pointer
 * @param at Row index (or -1 for the last row)
 */
void neo_edit_page_remove_row(neo_edit_page_t* page, int at);

/**
 * @brief Set the Y position of the cursor.
 * @param page Page pointer
 * @param row Row index (or -1 for the end)
 */
void neo_edit_page_set_cursor_row(neo_edit_page_t* page, int row);

/**
 * @brief Set the X position of the cursor.
 * @param page Page pointer
 * @param col Column index (or -1 for the end)
 */
void neo_edit_page_set_cursor_col(neo_edit_page_t* page, int col);

/**
 * @brief Move the cursor on the page relatively.
 * @param page Page pointer
 * @param dir Cursor direction
 * @param num Number of characters to move
 */
void neo_edit_page_move_cursor(neo_edit_page_t* page, neo_dir_t dir, size_t num);

/**
 * @brief Set the filename for the page.
 * @param page Page pointer
 * @param filename Full filename
 */
void neo_edit_page_set_filename(neo_edit_page_t* page, string_t filename);

/**
 * @brief Get the index of the page in the list of pages.
 * @param page Page pointer
 * @return Page index (or SIZE_MAX on error)
 */
size_t neo_edit_page_get_index(neo_edit_page_t* page);

/**
 * @brief Initialize a context structure.
 * @details
 * This takes an existing context structure that has been default constructed and allocates memory
 * for it. It is considered valid at this point, and must be cleared before the program ends to 
 * prevent memory leaks.
 * @param context Context pointer
 * @return True if successful
 */
bool neo_edit_ctx_init(neo_edit_ctx_t* context);

/**
 * @brief Deinitialize a context structure.
 * @details
 * This frees all memory associated with the context. The object becomes invalid after this, and
 * must be re-initialized if you want to use it again.
 * @param context Context pointer
 */
void neo_edit_ctx_clear(neo_edit_ctx_t* context);

/**
 * @brief Update all pages within the context.
 * @param context Context pointer
 * @return True if successful
 */
bool neo_edit_ctx_update(neo_edit_ctx_t* context);

#endif // NEO_EDITOR_H