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

/*
#ifndef ND_DISABLE_CURSES
#include <ncurses.h>
#endif
*/

/* ==================================== Defines */

#define ND_VERSION "0.0.1"
#define ND_TAB_STOP 4
#define ND_HEADER 2
#define ND_FOOTER 2

#define CTRL_KEY(k) ((k) & 0x1f)

#define ESC_SCREEN_CLEAR     "\x1b[2J",   4
#define ESC_LINE_CLEAR       "\x1b[K",    3
#define ESC_HOME_CURSOR      "\x1b[H",    3
#define ESC_SHOW_CURSOR      "\x1b[?25h", 6
#define ESC_HIDE_CURSOR      "\x1b[?25l", 6
#define ESC_FORMAT_INVERT    "\x1b[7m",   4
#define ESC_FORMAT_UNDERLINE "\x1b[4m",   4
#define ESC_FORMAT_DEFAULT   "\x1b[m",    3

#define WRITE_ESC(e) write(STDOUT_FILENO, e)
#define STRBUF_ESC(b, e) strAppend(b, e)

enum editorKey {
	EK_BACKSPACE = 127,
	EK_LEFT = 1000,
	EK_RIGHT,
	EK_UP,
	EK_DOWN,
	EK_PAGE_UP,
	EK_PAGE_DOWN,
	EK_HOME,
	EK_END,
	EK_DEL
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
	int screen_rows, screen_cols;
	int row_off, col_off;
	int flags;
	char status_msg[80];
	time_t status_msg_time;
	struct termios orig_termios;
} editorConfig;


/* ==================================== Terminal operations */

void fail(const char* str);

void disableRawMode();

void enableRawMode();

int editorReadKey();

int getCursorPosition(int *rows, int *cols);

int getWindowSize(int *rows, int *cols);


/* ==================================== Row operations */

int editorRowCxToRx(editorRow* row, int cx);

void editorUpdateRow(editorRow* row);

void editorInsertRow(int at, char *str, size_t len);

void editorFreeRow(editorRow* row);

void editorDelRow(int at);

void editorRowInsertChar(editorRow* row, int at, int c);

void editorRowAppendString(editorRow* row, char* str, size_t len);

void editorRowDelChar(editorRow* row, int at);


/* ==================================== Editor operations */

void editorInsertChar(int c);

void editorInsertNewline();

void editorDelChar();


/* ==================================== String buffers */

typedef struct {
	char* buf;
	int len;
} strBuf;

#define STRBUF_INIT {NULL, 0}

void strAppend(strBuf *sb, const char* str, int len);

void strFree(strBuf *sb);


/* ==================================== Input operations */

char* editorPrompt(char* prompt);

void editorMoveCursor(int key);

void editorProcessKeypress();


/* ==================================== Output operations */

void editorScroll();

void editorDrawHeader(strBuf *sb);

void editorDrawRows(strBuf *sb);

void editorDrawFooter(strBuf *sb);

void editorRefreshScreen();

void editorSetStatusMessage(const char* fmt, ...);


/* ==================================== File operations */

void editorOpen(char* filename);

char* editorRowsToString(int* buflen);

void editorSave();


/* ==================================== Initialization operations */

void initEditor();

#endif