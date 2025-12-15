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
#include "neo_common.h"
#include "neo_string.h"
#include "neo_settings.h"
#include "neo_menu.h"

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
 * @brief Maximum status message length.
 */
#define NEO_EDIT_MAX_STATUS_MESSAGE_LEN 79u

/**
 * @brief Flags that define properties of a page in the editor.
 */
typedef enum {
	NPAGE_FLAG_DIRTY =    0x01,   // File has been modified and should be saved before closing.
	NPAGE_FLAG_READONLY = 0x02    // File is marked as read-only and cannot be modified or saved.
} neo_page_flag_t;

/**
 * @brief Check if a flag for the page is set to 1.
 */
#define PAGE_FLAG_ISSET(p, f) (((p)->flags & (f)) != 0)

/**
 * @brief Check if a flag for the page is set to 0
 */
#define PAGE_FLAG_ISCLEAR(p, f) (((p)->flags & (f)) == 0)

/**
 * @brief Set a flag for the page to 1.
 */
#define PAGE_FLAG_SET(p, f) ((p)->flags |= (f))

/**
 * @brief Set a flag for the page to 0.
 */
#define PAGE_FLAG_CLEAR(p, f) ((p)->flags &= ~(f))

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
	WINDOW* nc_window;
	PANEL* nc_panel;
	string_t filename;
	string_t filename_base;
	size_t num_rows;
	size_t max_rows;
	size_t num_cols;
	size_t row_off, col_off;
	size_t cursor_x, cursor_y, rcursor_x, rcursor_y;
	size_t window_rows;
	size_t window_cols;
	neo_page_flag_t flags;
};

#define PAGE_GET_CURR_ROW(page) ((page) && (page)->cursor_y < (page)->num_rows) ? &(page)->rows[(page)->cursor_y] : NULL

/**
 * @brief Top-level state of the editor.
 */
struct neo_edit_ctx_t {
	struct neo_edit_page_t* pages;
	WINDOW* nc_window;
	PANEL* nc_panel;
	neo_settings_t settings;
	neo_menu_bar_t menu_bar;
	time_t status_timer;
	string_t status_message;
	size_t num_pages;
	size_t max_pages;
	size_t curr_page;
	size_t window_rows;
	size_t window_cols;
	neo_state_t state;
};

#define EDITOR_GET_CURR_PAGE(ctx) (((ctx) && (ctx)->curr_page < (ctx)->num_pages) ? &(ctx)->pages[(ctx)->curr_page] : NULL)

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
 * @brief Insert text into the row at the given position.
 * @param row Row pointer
 * @param position Position to insert at (or -1 for the end)
 * @param insert Text to insert
 * @param len Number of characters to insert
 * @return True if successful
 */
bool neo_edit_row_insert_text(neo_edit_row_t* row, int position, const NEO_CHAR_T* insert, size_t len);

/**
 * @brief Overwrite the text of the row.
 * @param row Row pointer
 * @param insert Text to insert
 * @param len Number of characters to insert
 * @return True if successful
 */
bool neo_edit_row_set_text(neo_edit_row_t* row, const NEO_CHAR_T* insert, size_t len);

/**
 * @brief Erase text from the row at the given position.
 * @param row Row pointer
 * @param position Position to erase at
 * @param len Number of characters to erase
 * @return True if successful
 */
bool neo_edit_row_erase_text(neo_edit_row_t* row, size_t position, size_t len);

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
 * @brief Draw the contents of the page to its window.
 * @param page Page pointer
 * @return True if successful
 */
bool neo_edit_page_draw(neo_edit_page_t* page);

/**
 * @brief Get the row at the given position.
 * @param page Page pointer
 * @param position Row index (or -1 for the last row)
 * @return Row pointer (or NULL on error)
 */
neo_edit_row_t* neo_edit_page_get_row(neo_edit_page_t* page, int position);

/**
 * @brief Insert a new row at the given position.
 * @param page Page pointer
 * @param position Row index (or -1 for the end)
 * @return New row pointer (or NULL on error)
 */
neo_edit_row_t* neo_edit_page_insert_row(neo_edit_page_t* page, int position);

/**
 * @brief Remove the row at the given position.
 * @param page Page pointer
 * @param position Row index (or -1 for the last row)
 */
void neo_edit_page_remove_row(neo_edit_page_t* page, int position);

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
 * @return True if successful
 */
bool neo_edit_page_set_filename(neo_edit_page_t* page, NEO_CHAR_T* filename);

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

/**
 * @brief Render the full context, including menu bar and pages.
 * @param context Context pointer
 * @return True if successful
 */
bool neo_edit_ctx_draw(neo_edit_ctx_t* context);

/**
 * @brief Open a file as a new page.
 * @param context Context pointer
 * @param filename Full filename
 * @return Index of new page (or SIZE_MAX on error)
 */
size_t neo_edit_ctx_open_page(neo_edit_ctx_t* context, char* filename);

/**
 * @brief Write the contents of a page to file.
 * @param context Context pointer
 * @param position Index of page (or -1 for the last page)
 * @return True if successful
 */
bool neo_edit_ctx_write_page(neo_edit_ctx_t* context, int position);

/**
 * @brief Open a new blank page.
 * @param context Context pointer
 * @return Index of new page (or SIZE_MAX on error)
 */
size_t neo_edit_ctx_new_page(neo_edit_ctx_t* context);

/**
 * @brief Close a page.
 * @param context Context pointer
 * @param position Index of page (or -1 for the last page)
 * @return True if successful
 */
bool neo_edit_ctx_close_page(neo_edit_ctx_t* context, int position);

/**
 * @brief Set which page is currently visible.
 * @param context Context pointer
 * @param position Index of page (or -1 for the last page)
 * @return True if successful
 */
bool neo_edit_ctx_set_page(neo_edit_ctx_t* context, int position);

/**
 * @brief Respond to keyboard input.
 * @param context Context pointer
 * @return True if successful
 */
bool neo_edit_ctx_handle_input(neo_edit_ctx_t* context);

/**
 * @brief Set the status message at the bottom of the screen.
 * @param context Context pointer
 * @param fmt Formatted message
 * @param ... printf-style arguments
 * @return Length of printed message
 */
int neo_edit_ctx_status(neo_edit_ctx_t* context, const NEO_CHAR_T* fmt, ...);

#endif // NEO_EDITOR_H