/**********************************************************
* nd.h
*
* Declarations for editor functionality.
**********************************************************/
#ifndef ND_H
#define ND_H

/* ==================================== Includes */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <libgen.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <term.h>
#include <ncurses.h>

/* ==================================== Defines */

#define ND_VERSION "0.0.1"
#define ND_TAB_STOP 4
#define ND_HEADER 2
#define ND_FOOTER 2

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
	EK_ESCAPE = 27,
	EK_BACKSPACE = 127,
	EK_LEFT = 1000,
	EK_RIGHT,
	EK_UP,
	EK_DOWN,
	EK_PAGE_UP,
	EK_PAGE_DOWN,
	EK_HOME,
	EK_END,
	EK_DEL,
	EK_TABL,
	EK_TABR
};

enum editorFlag {
	EF_DIRTY = 1
};


/* ==================================== Global structures */

typedef struct {
	char* buf;
	char* rbuf;
	int len;
	int rlen;
} editorRow;

typedef struct {
	char* filename;
	editorRow* rows;
	int num_rows;
	int cx, cy;
	int rx, ry;
	int row_off, col_off;
	int flags;
} editorTab;

typedef struct {
	editorTab* etabs;
	int num_tabs;
	int curr_tab;
	int screen_cols, screen_rows;
	char status_msg[80];
	time_t status_msg_time;
	struct termios orig_termios;

	char* esc_screen_clear;
	char* esc_line_clear;
	char* esc_home_cursor;
	char* esc_show_cursor;
	char* esc_hide_cursor;
	char* esc_format_invert;
	char* esc_format_underline;
	char* esc_format_bold;
	char* esc_format_default;
} editorConfig;


/* ==================================== Terminal operations */

void fail(const char* str);

void disableRawMode();

void enableRawMode();

int editorReadKey();

int getWindowSize(int *rows, int *cols);


/* ==================================== Row operations */

int editorRowCxToRx(editorRow* row, int cx);

void editorUpdateRow(editorRow* row);

void editorInsertRow(editorTab* etab, int at, char *str, size_t len);

void editorFreeRow(editorRow* row);

void editorDelRow(editorTab* etab, int at);

void editorRowInsertChar(editorTab* etab, editorRow* row, int at, int c);

void editorRowAppendString(editorTab* etab, editorRow* row, char* str, size_t len);

void editorRowDelChar(editorTab* etab, editorRow* row, int at);


/* ==================================== Tab operations */

void editorInsertTab(editorTab* etab, int at);

void editorCloseTab(int at, int newtab);

void editorSetTab(int at);


/* ==================================== Editor operations */

void editorInsertChar(editorTab* etab, int c);

void editorInsertNewline(editorTab* etab);

void editorDelChar(editorTab* etab);


/* ==================================== String buffers */

typedef struct {
	char* buf;
	int len;
} strBuf;

#define STRBUF_INIT {NULL, 0}

void strAppend(strBuf *sb, const char* str, int len);

void strSet(strBuf *sb, const char* str, int len, int start);

void strFree(strBuf *sb);


/* ==================================== Input operations */

char* editorPrompt(char* prompt);

void editorMoveCursor(editorTab* etab, int key);

void editorProcessKeypress();


/* ==================================== Output operations */

void editorScroll(editorTab* etab);

void editorDrawHeader(strBuf *sb);

void editorDrawRows(editorTab* etab, strBuf *sb);

void editorDrawFooter(editorTab* etab, strBuf *sb);

void editorRefreshScreen();

void editorSetStatusMessage(const char* fmt, ...);


/* ==================================== File operations */

int editorOpen(char* filename);

char* editorRowsToString(editorTab* etab, int* buflen);

int editorSave(editorTab* etab);

void editorQuit();


/* ==================================== Initialization operations */

void initEditor();

void initTab(editorTab* etab);

#endif
