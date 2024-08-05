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
#include <signal.h>
#include <time.h>
#include <libgen.h>
#include <ncurses.h>
#include <ctype.h>
#include <assert.h>


// ============================================== defines

#define NEO_VERSION "0.0.1"
#define NEO_HEADER 2
#define NEO_FOOTER 2

enum editorFlag {
	EF_DIRTY =    0x01,		// File has been modified and should be saved before closing.
	EF_READONLY = 0x02		// File is marked as read-only and cannot be modified or saved.
};

enum editorState {
	ES_OPEN = 1,			// Normal state for reading user input & drawing to the screen.
	ES_PROMPT,				// Prompting user for input on the status bar.
	ES_MENU,				// Selecting an option from a menu group.
	//ES_MENU_FILE,			// Selecting file menu option.
	//ES_MENU_EDIT,			// Selecting edit menu option.
	//ES_MENU_HELP,			// Selecting help menu option.
	ES_SHOULD_CLOSE			// Editing has finished and the program should clean up & terminate.
};

enum editorDirection {
	ED_UP    = 0x01,
	ED_DOWN  = 0x02,
	ED_LEFT  = 0x04,
	ED_RIGHT = 0x08,
};


// ============================================== meta functionality

/// @brief Initialize ncurses library.
void cursesInit();

/// @brief General signal handler.
/// @param sig Signal id
void signalHandler(int sig);

extern bool _neo_flag_resized;


// ============================================== context menus

/// @brief Callback function for a menu entry.
typedef void (*fptrMenuCallback)(void*, int);

/// @brief Entry for a context menu.
typedef struct {
	char* name;
	char shortcut;
	fptrMenuCallback callback;
} menuEntry;

/// @brief List of menu entries.
typedef struct {
	char* name;
	menuEntry* entries;
	int numEntries;
	int maxEntries;
	int selected;
} menuGroup;

/// @brief Initialize a menu group structure.
/// @param grp Menu group pointer
void menuGroupInit(menuGroup* grp);

/// @brief Free all memory associated with the menu group.
/// @param grp Menu group pointer
void menuGroupClear(menuGroup* grp);

/// @brief Add a new entry to the menu group.
/// @param grp Menu group pointer
/// @param at Position to insert at (or -1 for the end)
/// @param entry Entry to add (NULL name treated as seperator)
void menuGroupInsert(menuGroup* grp, int at, menuEntry entry);

/// @brief Remove the entry from the menu group.
/// @param grp Menu group pointer
/// @param at Position to delete (or -1 for the end)
void menuGroupDelete(menuGroup* grp, int at);

// ============================================== text buffers

/// @brief Dynamically resizing null-terminated text buffer.
typedef struct {
	char* data;
	unsigned int size;
	unsigned int capacity;
} strbuf;

/// @brief Initialize a string buffer structure.
/// @param buf String buffer pointer
/// @param capacity Initial buffer capacity
void strbufInit(strbuf* buf, unsigned int capacity);

/// @brief Free all memory associated with the string buffer.
/// @param buf String buffer pointer
void strbufClear(strbuf* buf);

/// @brief Remove text from the string buffer.
/// @param buf String buffer pointer
/// @param at Starting position
/// @param len Number of characters to remove (or -1 to remove until the end)
void strbufDelete(strbuf* buf, unsigned int at, int len);

/// @brief Append text to the string buffer.
/// @param buf String buffer pointer
/// @param str String to append
/// @param len String length
void strbufAppend(strbuf* buf, const char* str, unsigned int len);

/// @brief Insert text into the string buffer.
/// @param buf String buffer pointer
/// @param str String to insert
/// @param len String length
/// @param at Starting position in buffer
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
/// @param min_size Minimum new size the buffer should be
void strbufGrow(strbuf* buf, unsigned int min_size);

/// @brief Number of non-terminating characters in the string buffer.
/// @param buf String buffer pointer
/// @return String length
int strbufLength(strbuf* buf);


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
	char* fullFilename;
	int maxRows;
	int numRows, numCols;
	int cx, cy;
	int rx, ry;
	int rowOff, colOff;
	int flags;
} editorPage;

/// @brief Top level container for open files and editor settings.
typedef struct {
	editorPage* pages;
	menuGroup* menus;
	char statusMsg[80];
	time_t statusMsgTime;
	int maxPages;
	int numPages;
	int currPage;
	int screenCols, screenRows;
	int state;
	int pageOff;
	int settingTabStop;
	int numMenus;
	int currMenu;
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

/// @brief Calculate the rendered cursor position based on it's current position.
/// @param ctx Render context pointer
/// @param row Row pointer
/// @param cx Cursor X position
/// @return Rendered X position
int rowCxToRx(editorContext* ctx, editorRow* row, int cx);

/// @brief Insert text into the row.
/// @param row Row pointer
/// @param at Position to insert at (or -1 for the end)
/// @param str String to insert
/// @param len String length
void rowInsert(editorRow* row, int at, const char* str, unsigned int len);

/// @brief Remove text from the row.
/// @param row Row pointer
/// @param at Position to delete at
/// @param len Number of characters to delete (or -1 to delete till the end)
void rowDelete(editorRow* row, int at, int len);


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
/// @return Inserted row
editorRow* pageInsertRow(editorPage* page, int at, char* str, unsigned int len);

/// @brief Delete the row of text from the page.
/// @param page Page poitner
/// @param at Row to remove
void pageDeleteRow(editorPage* page, int at);

/// @brief Move the cursor on the page by a relative amount.
/// @param ctx Editor context pointer
/// @param page Page pointer
/// @param dir Direction to move in
/// @param num Number of spaces to move
void pageMoveCursor(editorContext* ctx, editorPage* page, int dir, int num);

/// @brief Set the cursor row on the page.
/// @param page Page pointer
/// @param at Row number (or -1 for last row)
void pageSetCursorRow(editorPage* page, int at);

/// @brief Set the cursor column on the row.
/// @param page Page pointer
/// @param at Column number (or -1 for last character)
void pageSetCursorCol(editorPage* page, int at);

/// @brief Save the pages contents to file.
/// @param ctx Context pointer
/// @param page Page pointer
void pageSave(editorContext* ctx, editorPage* page);

/// @brief Set the complete filename of a page.
/// @param page Page pointer
/// @param filename Full filename, including path and basename
void pageSetFullFilename(editorPage* page, char* fullFilename);

/// @brief Get whichever number the page is in the tab list.
/// @param ctx Context pointer
/// @param page Page pointer
/// @return Page number (or -1 on error)
int pageGetNumber(editorContext* ctx, editorPage* page);


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

/// @brief Print the menu group to the screen.
/// @param ctx Context pointer
/// @param grp Menu group pointer
/// @param off Horizontal offset
void editorPrintMenu(editorContext* ctx, menuGroup* grp, int off);

/// @brief Respond to keyboard input.
/// @param ctx Context pointer
/// @param key Keyboard code
void editorHandleInput(editorContext* ctx, int key);

/// @brief Open a new page in the editor and make it the current page.
/// @param ctx Context pointer
/// @param filename File to open (or NULL for a blank page)
/// @param internal If filename is NULL and this is >= 0, open an internal document instead of a blank page
/// @return Created page (or NULL on error)
editorPage* editorOpenPage(editorContext* ctx, char* filename, int internal);

/// @brief Set the currently visible page in the editor.
/// @param ctx Context pointer
/// @param at Page number (or -1 for the last page)
void editorSetPage(editorContext* ctx, int at);

/// @brief Set the message displayed at the bottom of the screen.
/// @param ctx Context pointer
/// @param fmt Formatted message
/// @param ... printf-style arguments
/// @return Length of printed message
int editorSetMessage(editorContext* ctx, const char* fmt, ...);

/// @brief Close a certain page, asking if the user wants to save first.
/// @param ctx Context pointer
/// @param at Page number (or -1 for last page)
/// @param save Save the page if it's not a new file
void editorClosePage(editorContext* ctx, int at, bool save);

/// @brief Close all pages and exit the program.
/// @param ctx Context pointer
/// @return True if pages close, False if cancelled
bool editorCloseAll(editorContext* ctx);

/// @brief Use the status bar to prompt for user input. Note that this function
/// @brief takes an uninitialied strbuf and will initialize it; it will be up to
/// @brief the user to clear it once they are done using it.
/// @param ctx Context pointer
/// @param buf Destination string buffer (uninitialized)
/// @param prompt Formatted strings
void editorPrompt(editorContext* ctx, strbuf* buf, const char* prompt);


// ============================================== menu actions

/// @brief Callback function for the Help->About menu entry.
void cbMenuHelpAbout(void* data, int num);

extern const char _help_docs_contents[];
extern const char _help_docs_filename[];


// ============================================== functional macros

#if !__STRICT_ANSI__ && __GNUC__ >= 3
	/// @brief Get the currently viewed page.
	#define EDITOR_CURR_PAGE(ctx) ({ __typeof__ (ctx) _ctx=(ctx); &(_ctx->pages[_ctx->currPage]); })

	/// @brief Get the currently active menu group.
	#define EDITOR_CURR_MENU(ctx) ({ __typeof__ (ctx) _ctx=(ctx); &(_ctx->menus[_ctx->currMenu]); })

	/// @brief Get the row where the cursor is.
	#define PAGE_CURR_ROW(page) ({ __typeof__ (page) _page=(page); (_page->cy < _page->numRows) ? &(_page->rows[_page->cy]) : NULL; })

	#define MIN(a,b) ({ __typeof__ (a) _a=(a); __typeof__ (b) _b=(b); _a<_b ? _a : _b; })
	#define MAX(a,b) ({ __typeof__ (a) _a=(a); __typeof__ (b) _b=(b); _a>_b ? _a : _b; })
#else
	/// @brief Get the currently viewed page.
	#define EDITOR_CURR_PAGE(ctx) &((ctx)->pages[(ctx)->currPage])
	
	/// @brief Get the currently active menu group.
	#define EDITOR_CURR_MENU(ctx) &((ctx)->menus[(ctx)->currMenu])
	
	/// @brief Get the row where the cursor is.
	#define PAGE_CURR_ROW(page) (((page)->cy < (page)->numRows) ? (&(page)->rows[(page)->cy]) : NULL)
	
	#define MIN(a, b) ((a) < (b)) ? (a) : (b)
	#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#endif

#define NEO_SCROLL_MARGIN 1

#define CTRL_KEY(x) ((x) & 0x1f)

#define PAGE_FLAG_ISSET(p, f) (((p)->flags & (f)) != 0)

#define PAGE_FLAG_ISCLEAR(p, f) (((p)->flags & (f)) == 0)

#define PAGE_FLAG_SET(p, f) ((p)->flags |= (f))

#define PAGE_FLAG_CLEAR(p, f) ((p)->flags &= ~(f))

#define STR_TOLOWER(str) do { \
	for(char* _c = (str); *_c; ++_c) { *_c = tolower(*_c); } \
} while(0)

#define STR_TOUPPER(str) do { \
	for(char* _c = (str); *_c; ++_c) { *_c = toupper(*_c); } \
} while(0)

#endif // NEO_H