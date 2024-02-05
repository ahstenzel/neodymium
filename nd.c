/**********************************************************
* neodymium
*
* Created by Alex Stenzel (2024)
**********************************************************/
/* ================ Includes */

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
#include <termios.h>
#include <ncurses.h>
#include <sys/ioctl.h>
#include <sys/types.h>

/* ================ Defines */

#define ND_VERSION "0.0.1"
#define ND_TAB_STOP 4

#define CTRL_KEY(k) ((k) & 0x1f)

#define ESC_SCREEN_CLEAR   "\x1b[2J",   4
#define ESC_LINE_CLEAR     "\x1b[K",    3
#define ESC_HOME_CURSOR    "\x1b[H",    3
#define ESC_SHOW_CURSOR    "\x1b[?25h", 6
#define ESC_HIDE_CURSOR    "\x1b[?25l", 6
#define ESC_INVERT_COLOR   "\x1b[7m",   4
#define ESC_DEFAULT_FORMAT "\x1b[m",    3

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

void editorSetStatusMessage(const char* fmt, ...);
void editorSave();
void editorRefreshScreen();
char* editorPrompt(char* prompt);

/* ================ Globals */

typedef struct {
	char* buf;
	char* rbuf;
	int len;
	int rlen;
} editorRow;

struct editorConfig {
	char* filename;
	editorRow* rows;
	int num_rows;
	int cx, cy;
	int rx;
	int screen_rows, screen_cols;
	int row_off, col_off;
	int flags;
	char status_msg[80];
	time_t status_msg_time;
	struct termios orig_termios;
};

struct editorConfig CFG;

/* ================ Terminal control */

void fail(const char* str) {
	WRITE_ESC(ESC_SCREEN_CLEAR);
	WRITE_ESC(ESC_HOME_CURSOR);
	perror(str);
	write(STDOUT_FILENO, "\r", 2);
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

int editorRowCxToRx(editorRow* row, int cx) {
	int rx = 0;
	for (int j=0; j<cx; j++) {
		if (row->buf[j] == '\t') rx += (ND_TAB_STOP - 1) - (rx % ND_TAB_STOP);
		rx++;
	}
	return rx;
}

void editorUpdateRow(editorRow* row) {
	int tabs = 0;
	for (int j=0; j<row->len; ++j) {
		if (row->buf[j] == '\t') tabs++;
	}

	free(row->rbuf);
	row->rbuf = malloc(row->len + (tabs * (ND_TAB_STOP - 1)) + 1);

	int idx = 0;
	for (int j=0; j<row->len; ++j) {
		if (row->buf[j] == '\t') {
			row->rbuf[idx++] = ' ';
			while(idx % ND_TAB_STOP != 0) row->rbuf[idx++] = ' ';
		} else {
			row->rbuf[idx++] = row->buf[j];
		}
	}
	row->rbuf[idx] = '\0';
	row->rlen = idx;
}

void editorInsertRow(int at, char *str, size_t len) {
	if (at < 0 || at > CFG.num_rows) return;

	CFG.rows = realloc(CFG.rows, sizeof *(CFG.rows) * (CFG.num_rows + 1));
	memmove(&CFG.rows[at + 1], &CFG.rows[at], sizeof *(CFG.rows) * (CFG.num_rows - at));

	CFG.rows[at].len = len;
	CFG.rows[at].buf = malloc(len + 1);
	memcpy(CFG.rows[at].buf, str, len);
	CFG.rows[at].buf[len] = '\0';

	CFG.rows[at].rlen = 0;
	CFG.rows[at].rbuf = NULL;
	editorUpdateRow(&CFG.rows[at]);

	CFG.num_rows++;
	CFG.flags |= EF_DIRTY;
}

void editorFreeRow(editorRow* row) {
	free(row->buf);
	free(row->rbuf);
}

void editorDelRow(int at) {
	if (at < 0 || at >= CFG.num_rows) return;
	editorFreeRow(&CFG.rows[at]);
	memmove(&CFG.rows[at], &CFG.rows[at + 1], sizeof *(CFG.rows) * (CFG.num_rows - at - 1));
	CFG.num_rows--;
	CFG.flags |= EF_DIRTY;
}

void editorRowInsertChar(editorRow* row, int at, int c) {
	if (at < 0 || at > row->len) at = row->len;
	row->buf = realloc(row->buf, row->len + 2);
	memmove(&row->buf[at + 1], &row->buf[at], row->len - at + 1);
	row->len++;
	row->buf[at] = c;
	editorUpdateRow(row);
	CFG.flags |= EF_DIRTY;
}

void editorRowAppendString(editorRow* row, char* str, size_t len) {
	row->buf = realloc(row->buf, row->len + len + 1);
	memcpy(&row->buf[row->len], str, len);
	row->len += len;
	row->buf[row->len] = '\0';
	editorUpdateRow(row);
	CFG.flags |= EF_DIRTY;
}

void editorRowDelChar(editorRow* row, int at) {
	if (at < 0 || at >= row->len) return;
	memmove(&row->buf[at], &row->buf[at + 1], row->len - at);
	row->len--;
	editorUpdateRow(row);
	CFG.flags |= EF_DIRTY;
}

/* ================ Editor operations */

void editorInsertChar(int c) {
	if (CFG.cy == CFG.num_rows) editorInsertRow(CFG.num_rows, "", 0);
	editorRowInsertChar(&CFG.rows[CFG.cy], CFG.cx, c);
	CFG.cx++;
}

void editorInsertNewline() {
	if (CFG.cx == 0) {
		editorInsertRow(CFG.cy, "", 0);
	} else {
		editorRow* row = &CFG.rows[CFG.cy];
		editorInsertRow(CFG.cy + 1, &row->buf[CFG.cx], row->len - CFG.cx);
		row = &CFG.rows[CFG.cy];
		row->len = CFG.cx;
		row->buf[row->len] = '\0';
	}
	CFG.cy++;
	CFG.cx = 0;
}

void editorDelChar() {
	if (CFG.cy == CFG.num_rows) return;
	if (CFG.cx == 0 && CFG.cy == 0) return;

	editorRow* row = &CFG.rows[CFG.cy];
	if (CFG.cx > 0) {
		editorRowDelChar(row, CFG.cx - 1);
		CFG.cx--;
	} else {
		CFG.cx = CFG.rows[CFG.cy - 1].len;
		editorRowAppendString(&CFG.rows[CFG.cy - 1], row->buf, row->len);
		editorDelRow(CFG.cy);
		CFG.cy--;
	}
}

/* ================ Buffers */

typedef struct {
	char* buf;
	int len;
} strBuf;

#define STRBUF_INIT {NULL, 0}

void strAppend(strBuf *sb, const char* str, int len) {
	char* new = realloc(sb->buf, sb->len + len);

	if (!new) return;
	memcpy(&new[sb->len], str, len);
	sb->buf = new;
	sb->len += len;
}

void strFree(strBuf *sb) {
	free(sb->buf);
	sb->len = 0;
}

/* ================ Input handling */

char* editorPrompt(char* prompt) {
	size_t bufsize = 128;
	char* buf = malloc(bufsize);

	size_t buflen = 0;
	buf[0] = '\0';

	while(1) {
		editorSetStatusMessage(prompt, buf);
		editorRefreshScreen();

		int c = editorReadKey();
		if (c == EK_DEL || c == CTRL_KEY('h') || c == EK_BACKSPACE) {
			if (buflen != 0) buf[--buflen] = '\0';
		} else if (c == '\x1b') {
			editorSetStatusMessage("");
			free(buf);
			return NULL;
		} else if (c == '\r') {
			if (buflen != 0) {
				editorSetStatusMessage("");
				return buf;
			}
		} else if (!iscntrl(c) && c < 128) {
			if (buflen == bufsize - 1) {
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}
	}
}

void editorMoveCursor(int key) {
	editorRow* row = (CFG.cy >=  CFG.num_rows) ? NULL : &CFG.rows[CFG.cy];
	
	switch (key) {
		case EK_LEFT:
			if (CFG.cx != 0) {
				CFG.cx--;
			} else if (CFG.cy > 0) {
				CFG.cy--;
				CFG.cx = CFG.rows[CFG.cy].len;
			}
			break;
		case EK_RIGHT:
			if (row && CFG.cx < row->len) {
				CFG.cx++;
			} else if (row && CFG.cx == row->len) {
				CFG.cy++;
				CFG.cx = 0;
			}
			break;
		case EK_UP:
			if (CFG.cy != 0) CFG.cy--;
			break;
		case EK_DOWN:
			if (CFG.cy < CFG.num_rows) CFG.cy++;
			break;
	}

	/* Snap cursor to line endings */
	row = (CFG.cy >=  CFG.num_rows) ? NULL : &CFG.rows[CFG.cy];
	int row_len = row ? row->len : 0;
	if (CFG.cx > row_len) CFG.cx = row_len;
}

void editorProcessKeypress() {
	static int quit_confirm = 1;
	int c = editorReadKey();

	switch(c) {
		case '\r':
			editorInsertNewline();
			break;
		case CTRL_KEY('q'):
			if (CFG.flags & EF_DIRTY && quit_confirm != 0) {
				editorSetStatusMessage("WARNING- unsaved changes! Press Ctrl-Q again to quit without saving.");
				quit_confirm = 0;
				return;
			}
			WRITE_ESC(ESC_SCREEN_CLEAR);
			WRITE_ESC(ESC_HOME_CURSOR);
			exit(0);
			break;
		case CTRL_KEY('s'):
			editorSave();
			break;
		case EK_PAGE_UP:
		case EK_PAGE_DOWN:
			{
				if (c == EK_PAGE_UP) {
					CFG.cy = CFG.row_off;
				} else if (c == EK_PAGE_DOWN) {
					CFG.cy = CFG.row_off + CFG.screen_rows - 1;
					if (CFG.cy > CFG.num_rows) CFG.cy = CFG.num_rows;
				}

				int n = CFG.screen_rows;
				while (n--) {
					editorMoveCursor(c == EK_PAGE_UP ? EK_UP : EK_DOWN);
				}
			}
			break;
		case EK_HOME:
			CFG.cx = 0;
			break;
		case EK_END:
			if (CFG.cy < CFG.num_rows) CFG.cx = CFG.rows[CFG.cy].len;
			break;
		case EK_BACKSPACE:
		case CTRL_KEY('h'):
		case EK_DEL:
			if (c == EK_DEL) editorMoveCursor(EK_RIGHT);
			editorDelChar();
			break;
		case EK_UP:
		case EK_DOWN:
		case EK_LEFT:
		case EK_RIGHT:
			editorMoveCursor(c);
			break;
		case CTRL_KEY('l'):
		case '\x1b':
			break;
		default:
			editorInsertChar(c);
			break;
	}

	quit_confirm = 1;
}

/* ================ Output handling */

void editorScroll() {
	CFG.rx = 0;
	if (CFG.cy < CFG.num_rows) {
		CFG.rx = editorRowCxToRx(&CFG.rows[CFG.cy], CFG.cx);
	}

	if (CFG.cy < CFG.row_off) {
		CFG.row_off = CFG.cy;
	}
	if (CFG.cy >= CFG.row_off + CFG.screen_rows) {
		CFG.row_off = CFG.cy - CFG.screen_rows + 1;
	}
	if (CFG.rx < CFG.col_off) {
		CFG.col_off = CFG.rx;
	}
	if (CFG.rx >= CFG.col_off + CFG.screen_cols) {
		CFG.col_off = CFG.rx - CFG.screen_cols + 1;
	}
}

void editorDrawHeader(strBuf *sb) {
	STRBUF_ESC(sb, ESC_LINE_CLEAR);
	strAppend(sb, "\r\n", 2);
}

void editorDrawRows(strBuf *sb) {
	for (int y=0; y<CFG.screen_rows; ++y) {
		int row = y + CFG.row_off;
		if (row >= CFG.num_rows) {
			strAppend(sb, "~", 1);
		} else {
			int len = CFG.rows[row].rlen - CFG.col_off;
			if (len < 0) len = 0;
			if (len > CFG.screen_cols) len = CFG.screen_cols;
			strAppend(sb, &CFG.rows[row].rbuf[CFG.col_off], len);
		}

		STRBUF_ESC(sb, ESC_LINE_CLEAR);
		strAppend(sb, "\r\n", 2);
	}
}

void editorDrawFooter(strBuf *sb) {
	/* Draw status bar */
	STRBUF_ESC(sb, ESC_INVERT_COLOR);

	char lstatus[80], rstatus[80];
	int llen = snprintf(lstatus, sizeof(lstatus), "%.20s%s - %d lines",
		CFG.filename ? CFG.filename : "[No Name]", 
		CFG.flags & EF_DIRTY ? "*" : "",
		CFG.num_rows);
	if (llen > CFG.screen_cols) llen = CFG.screen_cols;
	int rlen = snprintf(rstatus, sizeof(rstatus), "Ln %d, Col %d",
		CFG.cy + 1, CFG.rx + 1);
	strAppend(sb, lstatus, llen);
	while (llen < CFG.screen_cols) {
		if ((CFG.screen_cols - llen == rlen) && (CFG.cy < CFG.num_rows)) {
			strAppend(sb, rstatus, rlen);
			break;
		} else {
			strAppend(sb, " ", 1);
			llen++;
		}
	}
	
	STRBUF_ESC(sb, ESC_DEFAULT_FORMAT);
	strAppend(sb, "\r\n", 2);

	/* Draw status message */
	STRBUF_ESC(sb, ESC_LINE_CLEAR);
	int msg_len = strlen(CFG.status_msg);
	if (msg_len > CFG.screen_cols) msg_len = CFG.screen_cols;
	if (msg_len && time(NULL) - CFG.status_msg_time < 5) {
		strAppend(sb, CFG.status_msg, msg_len);
	}
}

void editorRefreshScreen() {
	editorScroll();

	strBuf screen = STRBUF_INIT;
	
	/* Write contents to screen buffer */
	STRBUF_ESC(&screen, ESC_HIDE_CURSOR);
	STRBUF_ESC(&screen, ESC_HOME_CURSOR);

	//editorDrawHeader(&screen);
	editorDrawRows(&screen);
	editorDrawFooter(&screen);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (CFG.cy - CFG.row_off)+1, (CFG.rx - CFG.col_off)+1);
	strAppend(&screen, buf, strlen(buf));
	
	STRBUF_ESC(&screen, ESC_SHOW_CURSOR);

	/* Write buffer to terminal */
	write(STDOUT_FILENO, screen.buf, screen.len);
	strFree(&screen);
}

void editorSetStatusMessage(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(CFG.status_msg, sizeof(CFG.status_msg), fmt, ap);
	va_end(ap);
	CFG.status_msg_time = time(NULL);
}

/* ================ File operations */

void editorOpen(char* filename) {
	free(CFG.filename);
	CFG.filename = strdup(filename);
	FILE *fp = fopen(filename, "r");
	if (!fp) fail("fopen");

	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
			linelen--;
		}
		editorInsertRow(CFG.num_rows, line, linelen);
	}
	free(line);
	fclose(fp);
	CFG.flags &= ~(EF_DIRTY);
}

char* editorRowsToString(int* buflen) {
	int len = 0;
	for (int j=0; j<CFG.num_rows; ++j) {
		len += CFG.rows[j].len + 1;
	}
	*buflen = len;

	char* buf = malloc(len);
	char* p = buf;
	for (int j=0; j<CFG.num_rows; ++j) {
		memcpy(p, CFG.rows[j].buf, CFG.rows[j].len);
		p += CFG.rows[j].len;
		*p = '\n';
		p++;
	}

	return buf;
}

void editorSave() {
	if (CFG.filename == NULL) {
		CFG.filename = editorPrompt("Save as: %s (ESC to cancel)");
		if (!CFG.filename) return;
	}

	int len;
	char* buf = editorRowsToString(&len);

	int fd = open(CFG.filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1) {
		if (ftruncate(fd, len) != -1) {
			if (write(fd, buf, len) == len) {
				close(fd);
				free(buf);
				CFG.flags &= ~(EF_DIRTY);
				editorSetStatusMessage("%d bytes written to disk", len);
				return;
			}
		}
		close(fd);
	}
	free(buf);
	editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/* ================ Init */

void initEditor() {
	CFG.cx = 0;
	CFG.cy = 0;
	CFG.rx = 0;
	CFG.num_rows = 0;
	CFG.row_off = 0;
	CFG.col_off = 0;
	CFG.rows = NULL;
	CFG.filename = NULL;
	CFG.status_msg[0] = '\0';
	CFG.status_msg_time = 0;
	CFG.flags = 0;
	if (getWindowSize(&CFG.screen_rows, &CFG.screen_cols) == -1) fail("getWindowSize");
	CFG.screen_rows -= 2;
}

int main(int argc, char* argv[]) {
	enableRawMode();
	initEditor();
	if (argc >= 2) {
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit");

	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}