/**********************************************************
* neodymium
*
* Created by Alex Stenzel (2024)
**********************************************************/
/* ================ Includes */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/ioctl.h>
#include <string.h>

/* ================ Defines */

#define NEODYMIUM_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

#define ESC_SCREEN_CLEAR "\x1b[2J",   4
#define ESC_LINE_CLEAR   "\x1b[K",    3
#define ESC_HOME_CURSOR  "\x1b[H",    3
#define ESC_SHOW_CURSOR  "\x1b[?25h", 6
#define ESC_HIDE_CURSOR  "\x1b[?25l", 6

#define WRITE_ESC(e) write(STDOUT_FILENO, e)
#define STRBUF_ESC(b, e) strAppend(b, e)

enum editorKey {
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

/* ================ Globals */

struct editorConfig {
	int cx, cy;
	int screenrows, screencols;
	struct termios orig_termios;
};

struct editorConfig CFG;

/* ================ Terminal control */

void fail(const char* s) {
	WRITE_ESC(ESC_SCREEN_CLEAR);
	WRITE_ESC(ESC_HOME_CURSOR);
	perror(s);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &CFG.orig_termios) == -1) fail("tcsetattr");
}

void enableRawMode() {
	/* Save original terminal state for later */
	if (tcgetattr(STDIN_FILENO, &CFG.orig_termios) == -1) fail("tcgetattr");
	atexit(disableRawMode);

	/* Modify terminal state */
	struct termios raw = CFG.orig_termios;
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) fail("tcsettatr");
}

int editorReadKey() {
	/* Read raw input */
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) fail("read");
	}

	if (c == '\x1b') {
		/* Parse escaped character */
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return EK_HOME; break;
						case '3': return EK_DEL; break;
						case '4': return EK_END; break;
						case '5': return EK_PAGE_UP; break;
						case '6': return EK_PAGE_DOWN; break;
						case '7': return EK_HOME; break;
						case '8': return EK_END; break;
					}
				}
			} else {
				switch(seq[1]) {
					case 'A': return EK_UP; break;
					case 'B': return EK_DOWN; break;
					case 'C': return EK_RIGHT; break;
					case 'D': return EK_LEFT; break;
					case 'H': return EK_HOME; break;
					case 'F': return EK_END; break;
				}
			}
		} else if (seq[0] == 'O') {
			switch(seq[1]) {
				case 'H': return EK_HOME; break;
				case 'F': return EK_END; break;
			}
		}

		return '\x1b';
	} else {
		return c;
	}
}

int getCursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;

	/* Query the cursors position */
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

	/* Read the result from stdin into a buffer */
	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';

	/* Parse the values from the buffer */
	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
	return 0;
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/* ================ Buffers */

struct strbuf {
	char* buf;
	int len;
};

#define STRBUF_INIT {NULL, 0}

void strAppend(struct strbuf *b, const char* s, int len) {
	char* new = realloc(b->buf, b->len + len);

	if (!new) return;
	memcpy(&new[b->len], s, len);
	b->buf = new;
	b->len += len;
}

void strFree(struct strbuf *b) {
	free(b->buf);
	b->len = 0;
}

/* ================ Input handling */

void editorMoveCursor(int key) {
	switch (key) {
		case EK_LEFT:
			if (CFG.cx != 0) CFG.cx--;
			break;
		case EK_RIGHT:
			if (CFG.cx != CFG.screencols - 1) CFG.cx++;
			break;
		case EK_UP:
			if (CFG.cy != 0) CFG.cy--;
			break;
		case EK_DOWN:
			if (CFG.cy != CFG.screenrows - 1) CFG.cy++;
			break;
	}
}

void editorProcessKeypress() {
	int c = editorReadKey();

	switch(c) {
		case CTRL_KEY('q'):
			WRITE_ESC(ESC_SCREEN_CLEAR);
			WRITE_ESC(ESC_HOME_CURSOR);
			exit(0);
			break;
		case EK_PAGE_UP:
		case EK_PAGE_DOWN:
			{
				int n = CFG.screenrows;
				while (n--) {
					editorMoveCursor(c == EK_PAGE_UP ? EK_UP : EK_DOWN);
				}
			}
			break;
		case EK_HOME:
			CFG.cx = 0;
			break;
		case EK_END:
			CFG.cx = CFG.screencols - 1;
			break;
		case EK_UP:
		case EK_DOWN:
		case EK_LEFT:
		case EK_RIGHT:
			editorMoveCursor(c);
			break;
	}
}

/* ================ Output handling */

void editorDrawRows(struct strbuf *b) {
	int y;
	for (y=0; y<CFG.screenrows; ++y) {
		strAppend(b, "~", 1);
		STRBUF_ESC(b, ESC_LINE_CLEAR);
		if (y < CFG.screenrows - 1) {
			strAppend(b, "\r\n", 2);
		}
	}
}

void editorRefreshScreen() {
	struct strbuf screen = STRBUF_INIT;
	
	/* Write contents to screen buffer */
	STRBUF_ESC(&screen, ESC_HIDE_CURSOR);
	STRBUF_ESC(&screen, ESC_HOME_CURSOR);

	editorDrawRows(&screen);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", CFG.cy+1, CFG.cx+1);
	strAppend(&screen, buf, strlen(buf));
	
	STRBUF_ESC(&screen, ESC_SHOW_CURSOR);

	/* Write buffer to terminal */
	write(STDOUT_FILENO, screen.buf, screen.len);
	strFree(&screen);
}

/* ================ Init */

void initEditor() {
	CFG.cx = 10;
	CFG.cy = 0;
	if (getWindowSize(&CFG.screenrows, &CFG.screencols) == -1) fail("getWindowSize");
}

int main() {
	enableRawMode();
	initEditor();

	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}