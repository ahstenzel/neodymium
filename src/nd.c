/**********************************************************
* nd.h
*
* Definitions for editor functionality.
**********************************************************/
#include "nd.h"

editorConfig ED;

#define ESC_SCREEN_CLEAR     ED.esc_screen_clear
#define ESC_LINE_CLEAR       ED.esc_line_clear
#define ESC_HOME_CURSOR      ED.esc_home_cursor
#define ESC_SHOW_CURSOR      ED.esc_show_cursor
#define ESC_HIDE_CURSOR      ED.esc_hide_cursor
#define ESC_FORMAT_INVERT    ED.esc_format_invert
#define ESC_FORMAT_UNDERLINE ED.esc_format_underline
#define ESC_FORMAT_BOLD      ED.esc_format_bold
#define ESC_FORMAT_DEFAULT   ED.esc_format_default
#define ESC_NEWLINE          "\n\r"

#define ESC_WRITE_SCREEN(e)    write(STDOUT_FILENO, e, strlen(e))
#define ESC_WRITE_STRBUF(b, e) strAppend(b, e, strlen(e))

#define ED_CURR_TAB &ED.etabs[ED.curr_tab]

void fail(const char* str) {
	ESC_WRITE_SCREEN(ESC_SCREEN_CLEAR);
	ESC_WRITE_SCREEN(ESC_HOME_CURSOR);
	perror(str);
	write(STDOUT_FILENO, "\r", 2);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &ED.orig_termios) == -1) fail("tcsetattr");
}

void enableRawMode() {
	/* Initialize terminfo database */
	if (setupterm(getenv("TERM"), STDOUT_FILENO, NULL) == -1) fail("setupterm");

	/* Save original terminal state for later */
	if (tcgetattr(STDIN_FILENO, &ED.orig_termios) == -1) fail("tcgetattr");
	atexit(disableRawMode);

	/* Modify terminal state */
	struct termios raw = ED.orig_termios;
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

	if (c == EK_ESCAPE) {
		/* Parse escaped character */
		char seq[5];
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return EK_ESCAPE;
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return EK_ESCAPE;
		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return EK_ESCAPE;
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
				} else if (seq[2] == ';') {
					if (read(STDIN_FILENO, &seq[3], 1) != 1) return EK_ESCAPE;
					if (read(STDIN_FILENO, &seq[4], 1) != 1) return EK_ESCAPE;
					if (seq[3] == '5') {
						switch(seq[4]) {
							case 'D': return EK_TABL; break;
							case 'C': return EK_TABR; break;
						}
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

		return EK_ESCAPE;
	} else {
		return c;
	}
}

int getWindowSize(int *rows, int *cols) {
	int num_rows, num_cols;
	if ((num_rows = tigetnum("lines")) == -2) return -1;
	if ((num_cols = tigetnum("cols")) == -2) return -1;
	*rows = num_rows;
	*cols = num_cols;
	return 0;
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
	int ntabs = 0;
	for (int j=0; j<row->len; ++j) {
		if (row->buf[j] == '\t') ntabs++;
	}

	free(row->rbuf);
	row->rbuf = malloc(row->len + (ntabs * (ND_TAB_STOP - 1)) + 1);

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

void editorInsertRow(editorTab* etab, int at, char *str, size_t len) {
	if (at < 0 || at > etab->num_rows) return;

	etab->rows = realloc(etab->rows, sizeof *(etab->rows) * (etab->num_rows + 1));
	memmove(&etab->rows[at + 1], &etab->rows[at], sizeof *(etab->rows) * (etab->num_rows - at));

	etab->rows[at].len = len;
	etab->rows[at].buf = malloc(len + 1);
	memcpy(etab->rows[at].buf, str, len);
	etab->rows[at].buf[len] = '\0';

	etab->rows[at].rlen = 0;
	etab->rows[at].rbuf = NULL;
	editorUpdateRow(&etab->rows[at]);

	etab->num_rows++;
	etab->flags |= EF_DIRTY;
}

void editorFreeRow(editorRow* row) {
	free(row->buf);
	free(row->rbuf);
}

void editorDelRow(editorTab* etab, int at) {
	if (at < 0 || at >= etab->num_rows) return;
	editorFreeRow(&etab->rows[at]);
	memmove(&etab->rows[at], &etab->rows[at + 1], sizeof *(etab->rows) * (etab->num_rows - at - 1));
	etab->num_rows--;
	etab->flags |= EF_DIRTY;
}

void editorRowInsertChar(editorTab* etab, editorRow* row, int at, int c) {
	if (at < 0 || at > row->len) at = row->len;
	row->buf = realloc(row->buf, row->len + 2);
	memmove(&row->buf[at + 1], &row->buf[at], row->len - at + 1);
	row->len++;
	row->buf[at] = c;
	editorUpdateRow(row);
	etab->flags |= EF_DIRTY;
}

void editorRowAppendString(editorTab* etab, editorRow* row, char* str, size_t len) {
	row->buf = realloc(row->buf, row->len + len + 1);
	memcpy(&row->buf[row->len], str, len);
	row->len += len;
	row->buf[row->len] = '\0';
	editorUpdateRow(row);
	etab->flags |= EF_DIRTY;
}

void editorRowDelChar(editorTab* etab, editorRow* row, int at) {
	if (at < 0 || at >= row->len) return;
	memmove(&row->buf[at], &row->buf[at + 1], row->len - at);
	row->len--;
	editorUpdateRow(row);
	etab->flags |= EF_DIRTY;
}

void editorInsertTab(editorTab* etab, int at) {
	if (at < 0) at = ED.num_tabs;

	ED.etabs = realloc(ED.etabs, sizeof(*etab) * (ED.num_tabs + 1));
	if (at < ED.num_tabs) {
		memmove(&ED.etabs[at + 1], &ED.etabs[at], sizeof(*etab) * (ED.num_tabs - at));
	}
	memcpy(&ED.etabs[at], etab, sizeof(*etab));
	ED.num_tabs++;
	ED.curr_tab = at;
}

void editorCloseTab(int at, int newtab) {
	/* Clear tab resources */
	if (at < 0 || at > ED.num_tabs - 1) return;
	editorTab* etab = &ED.etabs[at];
	free(etab->filename);
	for(int i=0; i<etab->num_rows; ++i) {
		editorFreeRow(&etab->rows[i]);
	}
	free(etab->rows);

	/* Remove from global struct */
	if (at < (ED.num_tabs - 1)) {
		memmove(&ED.etabs[at], &ED.etabs[at + 1], sizeof(*etab) * (ED.num_tabs - at - 1));
	}
	ED.num_tabs--;
	if (ED.curr_tab >= ED.num_tabs) ED.curr_tab = ED.num_tabs - 1;

	/* Add blank tab if the last tab is closed */
	if (ED.num_tabs == 0 && newtab == 1) {
		editorOpen(NULL);
	}
}

void editorSetTab(int at) {
	if (at < 0 || at > ED.num_tabs - 1) at = ED.num_tabs - 1;
	ED.curr_tab = at;
}

void editorInsertChar(editorTab* etab, int c) {
	if (etab->cy == etab->num_rows) editorInsertRow(etab, etab->num_rows, "", 0);
	editorRowInsertChar(etab, &etab->rows[etab->cy], etab->cx, c);
	etab->cx++;
}

void editorInsertNewline(editorTab* etab) {
	if (etab->cx == 0) {
		editorInsertRow(etab, etab->cy, "", 0);
	} else {
		editorRow* row = &etab->rows[etab->cy];
		editorInsertRow(etab, etab->cy + 1, &row->buf[etab->cx], row->len - etab->cx);
		row = &etab->rows[etab->cy];
		row->len = etab->cx;
		row->buf[row->len] = '\0';
		editorUpdateRow(row);
	}
	etab->cy++;
	etab->cx = 0;
}

void editorDelChar(editorTab* etab) {
	if (etab->cy == etab->num_rows) return;
	if (etab->cx == 0 && etab->cy == 0) return;

	editorRow* row = &etab->rows[etab->cy];
	if (etab->cx > 0) {
		editorRowDelChar(etab, row, etab->cx - 1);
		etab->cx--;
	} else {
		etab->cx = etab->rows[etab->cy - 1].len;
		editorRowAppendString(etab, &etab->rows[etab->cy - 1], row->buf, row->len);
		editorDelRow(etab, etab->cy);
		etab->cy--;
	}
}

void strAppend(strBuf *sb, const char* str, int len) {
	char* new = realloc(sb->buf, sb->len + len);

	if (!new) return;
	memcpy(&new[sb->len], str, len);
	sb->buf = new;
	sb->len += len;
}

void strSet(strBuf *sb, const char* str, int len, int start) {
	if (start > sb->len) return;
	if (start < 0) start = sb->len - len - (-start) + 1;
	if (start + len > sb->len) len = sb->len - start;
	memcpy(&sb->buf[start], str, len);
}

void strFree(strBuf *sb) {
	free(sb->buf);
	sb->len = 0;
}

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
		} else if (c == EK_ESCAPE) {
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

void editorMoveCursor(editorTab* etab, int key) {
	editorRow* row = (etab->cy >= etab->num_rows) ? NULL : &etab->rows[etab->cy];
	
	switch (key) {
		case EK_LEFT:
			if (etab->cx != 0) {
				etab->cx--;
			} else if (etab->cy > 0) {
				etab->cy--;
				etab->cx = etab->rows[etab->cy].len;
			}
			break;
		case EK_RIGHT:
			if (row && etab->cx < row->len) {
				etab->cx++;
			} else if (row && etab->cx == row->len) {
				etab->cy++;
				etab->cx = 0;
			}
			break;
		case EK_UP:
			if (etab->cy != 0) etab->cy--;
			break;
		case EK_DOWN:
			if (etab->cy < etab->num_rows) etab->cy++;
			break;
	}

	/* Snap cursor to line endings */
	row = (etab->cy >=  etab->num_rows) ? NULL : &etab->rows[etab->cy];
	int row_len = row ? row->len : 0;
	if (etab->cx > row_len) etab->cx = row_len;
}

void editorProcessKeypress() {
	editorTab* etab = ED_CURR_TAB;
	int c = editorReadKey();
	static int escapeCounter = 0;

	switch(c) {
		case '\r':
			editorInsertNewline(etab);
			break;
		case CTRL_KEY('q'):
			if (etab->flags & EF_DIRTY) {
				while (1) {
					char* res = editorPrompt("WARNING! Quit without saving? (y/n): %s");
					if (res) {
						for (char* r = res; *r; ++r) *r = tolower(*r);
						if (strcmp(res, "y") == 0) {
							if (!etab->filename) {
								editorQuit();
							} else {
								editorCloseTab(ED.curr_tab, 1);
								break;
							}
						} else if (strcmp(res, "n") == 0) {
							return;
						}
					}
				}
			} else {
				if (!etab->filename && ED.num_tabs == 1) {
					editorQuit();
				} else {
					editorCloseTab(ED.curr_tab, 1);
				}
			}
			break;
		case CTRL_KEY('s'):
			if (editorSave(etab) != 0) {
				editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
			}
			break;
		case CTRL_KEY('o'):
			{
				char* filename = editorPrompt("Open file: %s (ESC to cancel)");
				if (filename) {
					if (editorOpen(filename) != 0) {
						editorSetStatusMessage("Failed to open file!");
					} else {
						editorSetTab(-1);
					}
				}
				break;
			}
		case CTRL_KEY('n'):
			editorOpen(NULL);
			break;
		case EK_PAGE_UP:
		case EK_PAGE_DOWN:
			{
				if (c == EK_PAGE_UP) {
					etab->cy = etab->row_off;
				} else if (c == EK_PAGE_DOWN) {
					etab->cy = etab->row_off + ED.screen_rows - 1;
					if (etab->cy > etab->num_rows) etab->cy = etab->num_rows;
				}

				int n = ED.screen_rows;
				while (n--) {
					editorMoveCursor(etab, c == EK_PAGE_UP ? EK_UP : EK_DOWN);
				}
			}
			break;
		case EK_HOME:
			etab->cx = 0;
			break;
		case EK_END:
			if (etab->cy < etab->num_rows) etab->cx = etab->rows[etab->cy].len;
			break;
		case EK_BACKSPACE:
		case CTRL_KEY('h'):
		case EK_DEL:
			if (c == EK_DEL) editorMoveCursor(etab, EK_RIGHT);
			editorDelChar(etab);
			break;
		case EK_UP:
		case EK_DOWN:
		case EK_LEFT:
		case EK_RIGHT:
			editorMoveCursor(etab, c);
			break;
		case EK_TABL:
			ED.curr_tab--;
			if (ED.curr_tab < 0) ED.curr_tab = ED.num_tabs - 1;
			break;
		case EK_TABR:
			ED.curr_tab++;
			if (ED.curr_tab > (ED.num_tabs - 1)) ED.curr_tab = 0;
			break;
		case CTRL_KEY('l'):
		case EK_ESCAPE:
			escapeCounter++;
			if (escapeCounter < 2) {
				editorSetStatusMessage("Press Esc again to quit");
				return;
			} else {
				editorQuit();
			}
			break;
		default:
			editorInsertChar(etab, c);
			break;
	}

	if (escapeCounter != 0) {
		escapeCounter = 0;
		editorSetStatusMessage("");
	}
}

void editorScroll(editorTab* etab) {
	/* Calculate rendered cursor position */
	etab->rx = 0;
	if (etab->cy < etab->num_rows) {
		etab->rx = editorRowCxToRx(&etab->rows[etab->cy], etab->cx);
	}
	etab->ry = etab->cy + ND_HEADER;

	/* Calculate offset values based on cursor position */
	if (etab->cy < etab->row_off) {
		etab->row_off = etab->cy;
	}
	if (etab->cy >= etab->row_off + ED.screen_rows) {
		etab->row_off = etab->cy - ED.screen_rows + 1;
	}
	if (etab->rx < etab->col_off) {
		etab->col_off = etab->rx;
	}
	if (etab->rx >= etab->col_off + ED.screen_cols) {
		etab->col_off = etab->rx - ED.screen_cols + 1;
	}
}

void editorDrawHeader(strBuf *sb) {
	/* Draw file bar */
	ESC_WRITE_STRBUF(sb, ESC_LINE_CLEAR);
	ESC_WRITE_STRBUF(sb, ESC_NEWLINE);

	/* Draw tab bar */
	ESC_WRITE_STRBUF(sb, ESC_LINE_CLEAR);
	ESC_WRITE_STRBUF(sb, ESC_FORMAT_INVERT);

	int line_len = 0;
	for(int i=0; i<ED.num_tabs; ++i) {
		editorTab* etab = &ED.etabs[i];

		/* Underline & bold current tab */
		if (i == ED.curr_tab) {
			ESC_WRITE_STRBUF(sb, ESC_FORMAT_UNDERLINE);
			ESC_WRITE_STRBUF(sb, ESC_FORMAT_BOLD);
		}

		/* Write tab name */
		char* tab_str = malloc(ED.screen_cols + 1);
		int tab_len = snprintf(tab_str, ED.screen_cols, "%s%-8.20s", 
			etab->flags & EF_DIRTY ? "*" : "",
			etab->filename ? basename(etab->filename) : "<No Name>");
		strAppend(sb, tab_str, tab_len);
		ESC_WRITE_STRBUF(sb, ESC_FORMAT_DEFAULT);
		ESC_WRITE_STRBUF(sb, ESC_FORMAT_INVERT);
		strAppend(sb, "|", 1);
		tab_len++;
		line_len += tab_len;
	}
	while(line_len < ED.screen_cols) {
		strAppend(sb, " ", 1);
		line_len++;
	}

	ESC_WRITE_STRBUF(sb, ESC_FORMAT_DEFAULT);
	ESC_WRITE_STRBUF(sb, ESC_NEWLINE);
}

void editorDrawRows(editorTab* etab, strBuf *sb) {
	for (int y=0; y<ED.screen_rows; ++y) {
		/* Copy rendered text to screen buffer */
		int row = y + etab->row_off;
		
		if (row >= etab->num_rows) {
			strAppend(sb, "~", 1);
		} else {
			int len = etab->rows[row].rlen - etab->col_off;
			if (len < 0) len = 0;
			if (len > ED.screen_cols) len = ED.screen_cols;
			strAppend(sb, &etab->rows[row].rbuf[etab->col_off], len);
		}

		/* Draw scroll bar */
		if (ED.screen_rows < (etab->num_rows + 1)) {
			int len = (row >= etab->num_rows) ? 1 : etab->rows[row].rlen - etab->col_off;
			/* Fill in line with spaces */
			for(int x=len; x<ED.screen_cols - 1; ++x) {
				strAppend(sb, " ", 1);
			}

			if (y == 0) {
				/* Draw top arrow */
				ESC_WRITE_STRBUF(sb, ESC_FORMAT_INVERT);
				strAppend(sb, "^", 1);
				ESC_WRITE_STRBUF(sb, ESC_FORMAT_DEFAULT);
			} else if (y == (ED.screen_rows - 1)) {
				/* Draw bottom arrow */
				ESC_WRITE_STRBUF(sb, ESC_FORMAT_INVERT);
				strAppend(sb, "v", 1);
				ESC_WRITE_STRBUF(sb, ESC_FORMAT_DEFAULT);
			} else {
				float bar_ratio = ED.screen_rows / (float)(etab->num_rows + 1);
				int bar_height = (int)(bar_ratio * (ED.screen_rows - 2)) - 1;
				float row_ratio = etab->row_off / (float)((etab->num_rows + 1) - ED.screen_rows);
				int bar_offset = (int)(row_ratio * (ED.screen_rows - 2 - (bar_height + 1))) + 1;

				if (y < bar_offset || y > (bar_offset + bar_height)) {
					/* Draw bar background */
					strAppend(sb, "|", 1);
				} else {
					/* Draw bar */
					ESC_WRITE_STRBUF(sb, ESC_FORMAT_INVERT);
					strAppend(sb, " ", 1);
					ESC_WRITE_STRBUF(sb, ESC_FORMAT_DEFAULT);
				}
			}
		}

		ESC_WRITE_STRBUF(sb, ESC_LINE_CLEAR);
		ESC_WRITE_STRBUF(sb, ESC_NEWLINE);
	}
}

void editorDrawFooter(editorTab* etab, strBuf *sb) {
	/* Draw status bar */
	ESC_WRITE_STRBUF(sb, ESC_FORMAT_INVERT);

	/* Draw left message */
	char lstatus[80], rstatus[80];
	int llen = snprintf(lstatus, sizeof(lstatus), "%d lines", etab->num_rows);
	if (llen > ED.screen_cols) llen = ED.screen_cols;

	/* Draw right message */
	int rlen = snprintf(rstatus, sizeof(rstatus), "Ln %d, Col %d",
		etab->cy + 1, 
		etab->rx + 1);
	strAppend(sb, lstatus, llen);
	while (llen < ED.screen_cols) {
		if ((ED.screen_cols - llen == rlen) && (etab->cy < etab->num_rows)) {
			strAppend(sb, rstatus, rlen);
			break;
		} else {
			strAppend(sb, " ", 1);
			llen++;
		}
	}
	
	ESC_WRITE_STRBUF(sb, ESC_FORMAT_DEFAULT);
	ESC_WRITE_STRBUF(sb, ESC_NEWLINE);

	/* Draw status message */
	ESC_WRITE_STRBUF(sb, ESC_LINE_CLEAR);
	int msg_len = strlen(ED.status_msg);
	if (msg_len > ED.screen_cols) msg_len = ED.screen_cols;
	if (msg_len && time(NULL) - ED.status_msg_time < 5) {
		strAppend(sb, ED.status_msg, msg_len);
	}
}

void editorRefreshScreen() {
	editorTab* etab = ED_CURR_TAB;
	/* Set scroll offsets */
	editorScroll(etab);
	
	/* Write contents to screen buffer */
	strBuf screen = STRBUF_INIT;
	ESC_WRITE_STRBUF(&screen, ESC_HIDE_CURSOR);
	ESC_WRITE_STRBUF(&screen, ESC_HOME_CURSOR);

	editorDrawHeader(&screen);
	editorDrawRows(etab, &screen);
	editorDrawFooter(etab, &screen);

	/* Set cursor position */
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (etab->ry - etab->row_off)+1, (etab->rx - etab->col_off)+1);
	strAppend(&screen, buf, strlen(buf));
	ESC_WRITE_STRBUF(&screen, ESC_SHOW_CURSOR);

	/* Write buffer to terminal */
	write(STDOUT_FILENO, screen.buf, screen.len);
	strFree(&screen);
}

void editorSetStatusMessage(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(ED.status_msg, sizeof(ED.status_msg), fmt, ap);
	va_end(ap);
	ED.status_msg_time = time(NULL);
}

int editorOpen(char* filename) {
	/* Create etab struct */
	editorTab* etab = malloc(sizeof(*etab));
	initTab(etab);
	editorInsertTab(etab, -1);
	if (!filename) {
		return 0;
	} else {
		free(etab);
		etab = ED_CURR_TAB;
	}

	/* Read contents */
	etab->filename = strdup(filename);
	FILE *fp = fopen(filename, "r");
	if (!fp) return 1;
	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
			linelen--;
		}
		editorInsertRow(etab, etab->num_rows, line, linelen);
	}
	free(line);
	fclose(fp);

	/* Check file flags */
	etab->flags &= ~(EF_DIRTY);
	return 0;
}

char* editorRowsToString(editorTab* etab, int* buflen) {
	int len = 0;
	for (int j=0; j<etab->num_rows; ++j) {
		len += etab->rows[j].len + 1;
	}
	*buflen = len;

	char* buf = malloc(len);
	char* p = buf;
	for (int j=0; j<etab->num_rows; ++j) {
		memcpy(p, etab->rows[j].buf, etab->rows[j].len);
		p += etab->rows[j].len;
		*p = '\n';
		p++;
	}

	return buf;
}

int editorSave(editorTab* etab) {
	if (etab->filename == NULL) {
		etab->filename = editorPrompt("Save as: %s (ESC to cancel)");
		if (!etab->filename) return 0;
	}

	int len;
	char* buf = editorRowsToString(etab, &len);

	int fd = open(etab->filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1) {
		if (ftruncate(fd, len) != -1) {
			if (write(fd, buf, len) == len) {
				close(fd);
				free(buf);
				etab->flags &= ~(EF_DIRTY);
				return 0;
			}
		}
		close(fd);
	}
	free(buf);
	return 1;
}

void editorQuit() {
	/* Check for unsaved tabs */
	int saveAll = -1;
	for(int i=0; i<ED.num_tabs; ++i) {
		editorTab* etab = &ED.etabs[i];
		if (etab->flags & EF_DIRTY) {
			/* Ask if the user wants to save all open files */
			if (saveAll == -1) {
				while (saveAll == -1) {
					char* res = editorPrompt("Save all files? (y/N): %s");
					if (res) {
						for (char* r = res; *r; ++r) *r = tolower(*r);
						if (strcmp(res, "y") == 0) {
							saveAll = 1;
						} else if (strcmp(res, "n") == 0) {
							saveAll = 0;
						}
					}
				}
			} else if (saveAll == 1) {
				/* Try to save file */
				while(1) {
					ED.curr_tab = i;
					if (editorSave(etab) != 0) {
						/* I/O error; ask the user what to do */
						int action = -1;
						while(action == -1) {
							char* err = editorPrompt("Failed to save file! Retry, skip, or abort? (r/s/a): %s");
							if (err) {
								for (char* r = err; *r; ++r) *r = tolower(*r);
								if (strcmp(err, "r") == 0) {
									action = 0;
								} else if (strcmp(err, "s") == 0) {
									action = 1;
								} else if (strcmp(err, "a") == 0) {
									action = 2;
								}
							}
						}
						if (action == 0) continue;
						else if (action == 1) break;
						else if (action == 2) return;
					} else break;
				}
			}
		}
	}

	/* Close all tabs */
	for(int i=0; i<ED.num_tabs; ++i) {
		editorCloseTab(i, 0);
	}

	/* Clear screen & exit */
	ESC_WRITE_SCREEN(ESC_SCREEN_CLEAR);
	ESC_WRITE_SCREEN(ESC_HOME_CURSOR);
	exit(0);
}

void initEditor() {
	ED.etabs = NULL;
	ED.num_tabs = 0;
	ED.curr_tab = 0;
	ED.status_msg[0] = '\0';
	ED.status_msg_time = 0;
	if (getWindowSize(&ED.screen_rows, &ED.screen_cols) == -1) fail("getWindowSize");
	ED.screen_rows -= (ND_HEADER + ND_FOOTER);

	/* Record escape codes */
	ESC_SCREEN_CLEAR     = tigetstr("clear");
	ESC_LINE_CLEAR       = tigetstr("el");
	ESC_HOME_CURSOR      = tigetstr("home");
	ESC_SHOW_CURSOR      = tigetstr("cvvis");
	ESC_HIDE_CURSOR      = tigetstr("civis");
	ESC_FORMAT_INVERT    = tigetstr("rev");
	ESC_FORMAT_UNDERLINE = tigetstr("smul");
	ESC_FORMAT_BOLD      = tigetstr("bold");
	ESC_FORMAT_DEFAULT   = tigetstr("sgr0");
}

void initTab(editorTab* etab) {
	if (!etab) return;
	etab->filename = NULL;
	etab->rows = NULL;
	etab->num_rows = 0;
	etab->cx = 0;
	etab->cy = 0;
	etab->rx = 0;
	etab->ry = ND_HEADER;
	etab->row_off = 0;
	etab->col_off = 0;
	etab->flags = 0;
}