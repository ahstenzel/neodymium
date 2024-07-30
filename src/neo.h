/**
 * nd.h
 * 
 * Declarations for editor functionality.
 */
#ifndef NEO_H
#define NEO_H

// ============================================== includes

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
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

// ============================================== text buffer functionality

/// @brief Single row of text.
typedef struct {
	char* buf;
	char* rbuf;
	int len;
	int rlen;
} editorRow;

/// @brief Single open file containing many rows of text.
typedef struct {
	editorRow* rows;
	char* filename;
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
	int numPages;
	int currPage;
	int screenCols, screenRows;
	int state;
} editorContext;

/// @brief Create a new structure containing a row of text.
/// @return Row pointer
editorRow* rowInit();

/// @brief Render a text row according to cursor position, tab stops, etc.
/// @param row Row pointer
void rowUpdate(editorRow* row);

/// @brief Create a new structure containing a page full of text rows.
/// @return Page pointer
editorPage* pageInit();

/// @brief Render all rows of text in the page.
/// @param page Page pointer
void pageUpdate(editorPage* page);

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

/// @brief Create a new top level editor context.
/// @return Context pointer
editorContext* editorInit();

/// @brief Update all the pages in the editor.
/// @param ctx Context pointer
void editorUpdate(editorContext* ctx);

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

/// @brief Open a new page in the editor.
/// @param ctx Context pointer
/// @param filename File to open (or NULL for a blank page)
void editorOpenPage(editorContext* ctx, char* filename);

/// @brief Set the currently visible page in the editor.
/// @param ctx Context pointer
/// @param at Page number (or -1 for the last page)
void editorSetPage(editorContext* ctx, int at);

#endif // NEO_H