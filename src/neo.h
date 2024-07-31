/**
 * nd.h
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
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>


// ============================================== defines

#define NEO_VERSION "0.0.1"
#define NEO_HEADER 2
#define NEO_FOOTER 2

enum editorFlag {
	EF_DIRTY = 0x01			// File has been modified and should be saved before closing.
};

enum editorState {
	ES_OPEN = 1,			// Normal state for reading user input & drawing to the screen.
	ES_SHOULD_CLOSE			// Editing has finished and the program should clean up & terminate.
};

enum editorDirection {
	ED_UP    = 0x01,
	ED_DOWN  = 0x02,
	ED_LEFT  = 0x04,
	ED_RIGHT = 0x08,
};


// ============================================== curses functionality

/// @brief Initialize ncurses library.
void cursesInit();


// ============================================== text buffers

/// @brief Dynamically resizing null-terminated text buffer.
typedef struct {
	char* data;
	unsigned int size;
	unsigned int capacity;
} strbuf;

/// @brief Initialize a string buffer structure, freeing any memory it had before.
/// @param buf String buffer pointer
/// @param capacity Initial buffer capacity
void strbufInit(strbuf* buf, unsigned int capacity);

/// @brief Free all memory associated with the string buffer.
/// @param buf String buffer pointer
void strbufClear(strbuf* buf);

/// @brief Remove text from the string buffer.
/// @param buf String buffer pointer
/// @param at Starting position
/// @param len Number of characters to remove
void strbufDelete(strbuf* buf, unsigned int at, unsigned int len);

/// @brief Append text to the string buffer.
/// @param buf String buffer pointer
/// @param str String to append
/// @param len String length
void strbufAppend(strbuf* buf, const char* str, unsigned int len);

/// @brief Insert text into the string buffer.
/// @param buf String buffer pointer
/// @param str String to insert
/// @param len String length
/// @param at Starting position in buffer.
void strbufInsert(strbuf* buf, const char* str, unsigned int len, unsigned int at);

/// @brief Overwrite the text in the string buffer.
/// @param buf String buffer pointer
/// @param str String to insert
/// @param len String length
/// @param at Starting position in buffer
void strbufSet(strbuf* buf, const char* str, unsigned int len, unsigned int at);

/// @brief Add a character to the end of the string buffer.
/// @param buf String buffer pointer
/// @param c Character
void strbufAddChar(strbuf* buf, char c);

/// @brief Remove the last character from the string buffer.
/// @param buf String buffer pointer
void strbufDelChar(strbuf* buf);

/// @brief Get a character in the string buffer.
/// @param buf String buffer pointer
/// @param at Position (or -1 for last char)
/// @return Character (or '\0' for invalid position)
char strbufGetChar(strbuf* buf, int at);

/// @brief Grow internal array of characters.
/// @param buf String buffer pointer
void strbufGrow(strbuf* buf);


// ============================================== editor objects

/// @brief Single row of text.
typedef struct {
	strbuf text;
	strbuf rtext;
	bool dirty;
} editorRow;

/// @brief Single open file containing many rows of text.
typedef struct {
	editorRow* rows;
	char* filename;
	int maxRows;
	int numRows;
	int cx, cy;
	int rx, ry;
	int rowOff, colOff;
	int flags;
} editorPage;

/// @brief Top level container for open files and editor settings.
typedef struct {
	editorPage* pages;
	char statusMsg[80];
	time_t statusMsgTime;
	int maxPages;
	int numPages;
	int currPage;
	int screenCols, screenRows;
	int state;

	int settingTabStop;
} editorContext;

/// @brief Initialize a row structure.
/// @param row Row pointer
void rowInit(editorRow* row);

/// @brief Free all memeory associated with the row.
/// @param row Row pointer
void rowClear(editorRow* row);

/// @brief Render a text row according to cursor position, tab stops, etc.
/// @param ctx Context pointer
/// @param row Row pointer
void rowUpdate(editorContext* ctx, editorRow* row);



/// @brief Initialize a page structure.
/// @param page Page pointer
void pageInit(editorPage* page);

/// @brief Free all memory associated with the page.
/// @param page Page pointer
void pageClear(editorPage* page);

/// @brief Render all rows of text in the page.
/// @param ctx Context pointer
/// @param page Page pointer
void pageUpdate(editorContext* ctx, editorPage* page);

/// @brief Increase the size of the internal array of rows.
/// @param page Page pointer
void pageGrowRows(editorPage* page);

/// @brief Insert a new row of text into the page.
/// @param page Page pointer
/// @param at Row to insert at (or -1 for the end)
/// @param str Row initial text
/// @param len Text length
void pageInsertRow(editorPage* page, int at, char* str, unsigned int len);

/// @brief Delete the row of text from the page.
/// @param page Page poitner
/// @param at Row to remove
void pageDeleteRow(editorPage* page, int at);

/// @brief Move the cursor on the page by a relative amount.
/// @param page Page pointer
/// @param dir Direction to move in
/// @param num Number of spaces to move
void pageMoveCursor(editorPage* page, int dir, int num);

/// @brief Set the cursor row on the page.
/// @param page Page pointer
/// @param at Row number (or -1 for last row)
void pageSetCursorRow(editorPage* page, int at);

/// @brief Set the cursor column on the row.
/// @param page Page pointer
/// @param at Column number (or -1 for last character)
void pageSetCursorCol(editorPage* page, int at);

/// @brief Save the pages contents to file.
/// @param page Page pointer
void pageSave(editorPage* page);



/// @brief Initialize an editor context structure.
/// @param ctx Context pointer
void editorInit(editorContext* ctx);

/// @brief Free all memory associated with the editor context.
/// @param ctx Context pointer
void editorClear(editorContext* ctx);

/// @brief Update all the pages in the editor.
/// @param ctx Context pointer
void editorUpdate(editorContext* ctx);

/// @brief Increase the size of the internal array of pages.
/// @param ctx Context pointer
void editorGrowPages(editorContext* ctx);

/// @brief Get the current state of the editor.
/// @param ctx Context pointer
/// @return State
int editorGetState(editorContext* ctx);

/// @brief Print the editor to the screen.
/// @param ctx Context pointer
void editorPrint(editorContext* ctx);

/// @brief Respond to keyboard input.
/// @param ctx Context pointer
/// @param key Keyboard code
void editorHandleInput(editorContext* ctx, int key);

/// @brief Open a new page in the editor and make it the current page.
/// @param ctx Context pointer
/// @param filename File to open (or NULL for a blank page)
void editorOpenPage(editorContext* ctx, char* filename);

/// @brief Set the currently visible page in the editor.
/// @param ctx Context pointer
/// @param at Page number (or -1 for the last page)
void editorSetPage(editorContext* ctx, int at);


// ============================================== functional macros

#define EDITOR_CURR_PAGE(ctx) &((ctx)->pages[(ctx)->currPage])

#endif // NEO_H