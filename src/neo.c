#include "neo.h"

void cursesInit() {
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, true);
}

void strbufInit(strbuf* buf, unsigned int capacity) {
	if (!buf) { return; }

	buf->data = malloc(capacity);
	buf->size = 0;
	buf->capacity = capacity;
	if (capacity > 0) { buf->data[0] = '\0'; }
}

void strbufClear(strbuf* buf) {
	if (!buf || !buf->data) { return; }
	
	free(buf->data);
	buf->data = NULL;
	buf->size = 0;
	buf->capacity = 0;
}

void strbufDelete(strbuf* buf, unsigned int at, unsigned int len) {
	if (!buf || at >= buf->size || len == 0) { return; }

	// Adjust boundaries
	if (at + len >= buf->size) { 
		len -= (at + len) - buf->size; 
	}

	// Shift data around
	if (at + len + 1 < buf->size) { 
		memmove(&buf->data[at], &buf->data[at + len], buf->size - (at + len) + 1); 
	} else { 
		buf->data[at + 1] = '\0'; 
	}
	buf->size -= len;
}

void strbufAppend(strbuf* buf, const char* str, unsigned int len) {
	if (!buf || !str || len == 0) { return; }

	// Resize if necessary
	if (buf->size + len + 1 > buf->capacity) { 
		strbufGrow(buf, buf->capacity + len + 1); 
	}

	// Copy to end of buffer
	memcpy(&buf->data[buf->size], str, len);
	buf->size += len;
	buf->data[buf->size] = '\0';
}

void strbufInsert(strbuf* buf, const char* str, unsigned int len, unsigned int at) {
	if (!buf || !str || len == 0) { return; }

	// Resize if necessary
	if (len + buf->size + 1 > buf->capacity) { 
		strbufGrow(buf, buf->capacity + len + 1); 
	}

	// Shift data around
	memmove(&buf->data[at + len], &buf->data[at], (buf->size - at) + 1);

	// Copy to buffer
	memcpy(&buf->data[at], str, len);
	buf->size += len;
}

void strbufSet(strbuf* buf, const char* str, unsigned int len, unsigned int at) {
	if (!buf || !str || len == 0) { return; }

	// Resize if necessary
	if (at + len + 1 > buf->capacity) { 
		strbufGrow(buf, at + len + 1); 
	}

	// Overwrite data
	memcpy(&buf->data[at], str, len);
	if (at + len > buf->size) { 
		buf->size += (at + len) - buf->size; 
	}
	buf->data[buf->size] = '\0';
}

void strbufAddChar(strbuf* buf, char c) {
	if (!buf) { return; }

	if (buf->size + 1 >= buf->capacity) { 
		strbufGrow(buf, buf->capacity + 1); 
	}
	buf->data[buf->size++] = c;
	buf->data[buf->size] = '\0';
}

void strbufDelChar(strbuf* buf) {
	if (!buf) { return; }

	if (buf->size > 0) { 
		buf->data[--buf->size] = '\0'; 
	}
}

char strbufGetChar(strbuf* buf, int at) {
	if (!buf) { return '\0'; }

	if (at < 0) { 
		at = buf->size; 
	}
	if ((unsigned int)(at) >= buf->size) { 
		return '\0'; 
	}
	return buf->data[at];
}

void strbufGrow(strbuf* buf, unsigned int min_size) {
	unsigned int newCapacity = buf->capacity;
	while (newCapacity < min_size) { 
		newCapacity = (newCapacity == 0) ? 40 : newCapacity * 2; 
	}
	char* newData = realloc(buf->data, newCapacity);
	if (!newData) { return; }
	buf->data = newData;
	buf->capacity = newCapacity;
}

void rowInit(editorRow* row) {
	if (!row) { return; }

	strbufInit(&row->text, 0);
	strbufInit(&row->rtext, 0);
	row->dirty = false;
}

void rowClear(editorRow* row) {
	if (!row) { return; }

	strbufClear(&row->text);
	strbufClear(&row->rtext);
}

void rowUpdate(editorContext* ctx, editorRow* row) {
	if (!row || !row->dirty) { return; }

	// Count tabs
	unsigned int ntabs = 0;
	for(unsigned int i=0; i<row->text.size; ++i) {
		if (strbufGetChar(&row->text, i) == '\t') { ntabs++; }
	}

	// Allocate render buffer
	strbufDelete(&row->rtext, 0, row->rtext.size);
	strbufInit(&row->rtext, row->text.size + (ntabs * (ctx->settingTabStop - 1)) + 1);

	// Render text
	for(unsigned int j=0; j<row->text.size; ++j) {
		if (strbufGetChar(&row->text, j) == '\t') {
			strbufAddChar(&row->rtext, ' ');
			while(row->rtext.size % ctx->settingTabStop != 0) { 
				strbufAddChar(&row->rtext, ' '); 
			}
		} else {
			strbufAddChar(&row->rtext, strbufGetChar(&row->text, j));
		}
	}
	row->dirty = false;
}

int rowCxToRx(editorContext* ctx, editorRow* row, int cx) {
	if (!ctx || !row) { return 0; }

	int rx = 0;
	for(int i=0; i<cx; ++i) {
		if (strbufGetChar(&row->text, i) == '\t') { 
			rx += (ctx->settingTabStop - 1) - (rx % ctx->settingTabStop); 
		}
		rx++;
	}
	return rx;
}

void pageInit(editorPage* page) {
	if (!page) { return; }

	page->rows = malloc(0);
	page->filename = NULL;
	page->maxRows = 0;
	page->numRows = 0;
	page->cx = 0;
	page->cy = 0;
	page->rx = 0;
	page->ry = 0;
	page->rowOff = 0;
	page->colOff = 0;
	page->flags = 0;
}

void pageClear(editorPage* page) {
	if (!page) { return; }

	for(int i=0; i<page->numRows; ++i) { 
		rowClear(&page->rows[i]); 
	}
	free(page->filename);
	free(page->rows);
}

void pageUpdate(editorContext* ctx, editorPage* page) {
	if (!page) { return; }

	for(int i=0; i<page->numRows; ++i) { 
		rowUpdate(ctx, &page->rows[i]); 
	}
}

void pageGrowRows(editorPage* page) {
	if (!page) { return; }

	int newSize = (page->maxRows == 0) ? 48 : (page->maxRows * 2);
	editorRow* newRows = realloc(page->rows, newSize * sizeof(*newRows));
	if (!newRows) { return; }
	for(int i=page->numRows; i<newSize; ++i) { 
		rowInit(&newRows[i]); 
	}
	page->rows = newRows;
	page->maxRows = newSize;
}

void pageInsertRow(editorPage* page, int at, char* str, unsigned int len) {
	if (!page) { return; }
	
	// Check boundaries
	if (at < 0 || at > page->numRows) { 
		at = page->numRows; 
	}

	// Resize if necessary
	if (page->numRows >= page->maxRows) { 
		pageGrowRows(page); 
	}

	// Shift rows down
	if (at < page->numRows) {
		memmove(&page->rows[at + 1], &page->rows[at], sizeof(*page->rows) * (page->numRows - at));
	}

	// Copy text buffer to row
	editorRow* row = &page->rows[at];
	strbufDelete(&row->text, 0, row->text.size);
	strbufSet(&row->text, str, len, 0);
	row->dirty = true;

	// Update state
	page->numRows++;
	page->flags |= EF_DIRTY;
}

void pageDeleteRow(editorPage* page, int at) {
	(void)(page);
	(void)(at);
}

void pageMoveCursor(editorContext* ctx, editorPage* page, int dir, int num) {
	(void)(page);
	(void)(dir);
	(void)(num);
	editorRow* currRow = (page->cy >= page->numRows) ? NULL : &page->rows[page->cy];
	editorRow* nextRow = currRow;

	switch(dir) {
		case ED_DOWN:
			if (page->cy < page->numRows) { 
				// Correct for tabs
				nextRow = (page->cy + 1 >= page->numRows) ? NULL : &page->rows[page->cy + 1];
				if (nextRow && page->cx > 0) {
					int currTabs = 0;
					for(int i=0; i<page->cx; ++i) {
						if (strbufGetChar(&currRow->text, i) == '\t') { currTabs++; }
					}
					int nextTabs = 0;
					for(int i=0; i<page->cx; ++i) {
						if (strbufGetChar(&nextRow->text, i) == '\t') { nextTabs++; }
					}
					if (currTabs != nextTabs) {
						int currRx = rowCxToRx(ctx, currRow, page->cx);
						int nextRx = 0;
						int nextCx = 0;
						for(; nextCx<page->cx && nextRx < currRx; ++nextCx) {
							if (strbufGetChar(&nextRow->text, nextCx) == '\t') { nextRx += (ctx->settingTabStop - 1); }
							nextRx++;
						}
						if (strbufGetChar(&nextRow->text, nextCx - 1) == '\t') {
							page->cx = nextCx - 1;
						} else {
							page->cx -= (nextTabs - currTabs) * (ctx->settingTabStop - 1);
						}
					}
				}

				// Move line
				page->cy++; 
			}
		break;
		case ED_UP:
			if (page->cy != 0) { 
				// Correct for tabs
				nextRow = (page->cy - 1 >= page->numRows) ? NULL : &page->rows[page->cy - 1];
				if (nextRow && page->cx > 0) {
					int currTabs = 0;
					for(int i=0; i<page->cx; ++i) {
						if (strbufGetChar(&currRow->text, i) == '\t') { currTabs++; }
					}
					int nextTabs = 0;
					for(int i=0; i<page->cx; ++i) {
						if (strbufGetChar(&nextRow->text, i) == '\t') { nextTabs++; }
					}
					if (currTabs != nextTabs) {
						int currRx = rowCxToRx(ctx, currRow, page->cx);
						int nextRx = 0;
						int nextCx = 0;
						for(; nextCx<page->cx && nextRx < currRx; ++nextCx) {
							if (strbufGetChar(&nextRow->text, nextCx) == '\t') { nextRx += (ctx->settingTabStop - 1); }
							nextRx++;
						}
						if (strbufGetChar(&nextRow->text, nextCx - 1) == '\t') {
							page->cx = nextCx - 1;
						} else {
							page->cx -= (nextTabs - currTabs) * (ctx->settingTabStop - 1);
						}
					}
				}
				
				// Move line
				page->cy--; 
			}
		break;
		case ED_LEFT:
			if (page->cx != 0) { page->cx--; }
			else if (page->cy > 0) {
				page->cy--;
				page->cx = page->rows[page->cy].text.size;
			}
		break;
		case ED_RIGHT:
			if (currRow && (unsigned int)(page->cx) < currRow->text.size) { page->cx++; }
			else if (currRow && (unsigned int)(page->cx) == currRow->text.size) {
				page->cy++;
				page->cx = 0;
			}
		break;
	}

	// Snap cursor to line endings
	currRow = (page->cy >= page->numRows) ? NULL : &page->rows[page->cy];
	int rowLen = currRow ? currRow->text.size : 0;
	if (page->cx > rowLen) { page->cx = rowLen; }
}

void pageSetCursorRow(editorPage* page, int at) {
	(void)(page);
	(void)(at);
}

void pageSetCursorCol(editorPage* page, int at) {
	(void)(page);
	(void)(at);
}

void pageSave(editorPage* page) {
	(void)(page);
}

void editorInit(editorContext* ctx) {
	if (!ctx) { return; }

	ctx->pages = malloc(0);
	ctx->statusMsg[0] = '\0';
	ctx->statusMsgTime = 0;
	ctx->maxPages = 0;
	ctx->numPages = 0;
	ctx->currPage = -1;
	getmaxyx(stdscr, ctx->screenRows, ctx->screenCols);
	ctx->screenRows -= (NEO_HEADER + NEO_FOOTER);
	ctx->state = ES_OPEN;
	ctx->settingTabStop = 4;
}

void editorClear(editorContext* ctx) {
	if (!ctx) { return; }

	for(int i=0; i<ctx->numPages; ++i) { 
		pageClear(&ctx->pages[i]); 
	}
	free(ctx->pages);
}

void editorUpdate(editorContext* ctx) {
	if (!ctx) { return; }

	for(int i=0; i<ctx->numPages; ++i) { 
		pageUpdate(ctx, &ctx->pages[i]); 
	}
}

void editorGrowPages(editorContext* ctx) {
	if (!ctx) { return; }

	int newSize = (ctx->maxPages == 0) ? 1 : (ctx->maxPages * 2);
	editorPage* newPages = realloc(ctx->pages, newSize * sizeof(*newPages));
	if (!newPages) { return; }
	for(int i=ctx->numPages; i<newSize; ++i) { 
		pageInit(&newPages[i]); 
	}
	ctx->pages = newPages;
	ctx->maxPages = newSize;
}

int editorGetState(editorContext* ctx) {
	if (!ctx) { return ES_SHOULD_CLOSE; }
	return ctx->state;
}

void editorPrint(editorContext* ctx) {
	if (!ctx) { return; }

	// Calculate rendered cursor position
	editorPage* currPage = EDITOR_CURR_PAGE(ctx);
	if (currPage->cy < currPage->numRows) {
		editorRow* row = &currPage->rows[currPage->cy];
		currPage->rx = rowCxToRx(ctx, row, currPage->cx);
	} else { 
		currPage->rx = 0; 
	}
	currPage->ry = currPage->cy + NEO_HEADER;

	// Calculate row & column offsets
	if (currPage->cy < currPage->rowOff) { currPage->rowOff = currPage->cy; }
	if (currPage->cy >= currPage->rowOff + ctx->screenRows) { currPage->rowOff = currPage->cy - ctx->screenRows + 1; }
	if (currPage->rx < currPage->colOff) { currPage->colOff = currPage->rx; }
	if (currPage->rx >= currPage->colOff + ctx->screenCols) { currPage->colOff = currPage->rx - ctx->screenCols + 1; }

	// Write file bar
	move(0, 0);
	attron(A_UNDERLINE); printw("F"); attroff(A_UNDERLINE); printw("ile    ");
	attron(A_UNDERLINE); printw("E"); attroff(A_UNDERLINE); printw("dit    ");
	attron(A_UNDERLINE); printw("H"); attroff(A_UNDERLINE); printw("elp\n");

	// Calculate page tab scroll
	int pageIdx = 0;
	int total = 0;
	for(int i=1; i<ctx->numPages; ++i) {
		editorPage* lastPage = &ctx->pages[i - 1];
		if (lastPage->filename) { 
			total += strlen(lastPage->filename) + 3; 
		} else { 
			total += 13; 
		}
		if (total >= (ctx->screenCols / 2)) {
			total = 3;
			pageIdx++;
		}
	}

	// Write page tabs
	attron(A_REVERSE);
	int charIdx = 0;
	while(charIdx < ctx->screenCols) {
		editorPage* page = &ctx->pages[pageIdx];

		// Write start of line
		if (pageIdx > 0 && charIdx == 0) {
			charIdx += 3;
			addstr("< |");
		}

		// Write filename
		if (pageIdx == ctx->currPage) { 
			attron(A_UNDERLINE | A_STANDOUT); 
		}
		if (page->flags & EF_DIRTY) { 
			charIdx++;
			addch('*'); 
		}
		if (!page->filename) { 
			charIdx += 10;
			addnstr("<untitled>", ctx->screenCols - charIdx - 2); 
		} else { 
			charIdx += strlen(page->filename);
			addnstr(page->filename, ctx->screenCols - charIdx - 2);
		}

		// Write end of line
		if (ctx->screenCols - charIdx < 2) {
			attroff(A_STANDOUT | A_UNDERLINE);
			while(ctx->screenCols - charIdx < 5) { 
				delch(); 
				charIdx--;
			}
			charIdx += 5;
			addstr("... >");
		} else {
			charIdx += 2;
			addch(' ');
			if (pageIdx == ctx->currPage) { attroff(A_UNDERLINE | A_STANDOUT); }
			addch('|');
		}

		// Fill spaces
		pageIdx++;
		if (pageIdx >= ctx->numPages) {
			while(charIdx < ctx->screenCols) {
				charIdx++;
				addch(' ');
			}
		}
	}
	attroff(A_REVERSE);

	// Write page
	for(int i=0; i<ctx->screenRows; ++i) {
		int rowIdx = i + currPage->rowOff;
		editorRow* row = &currPage->rows[rowIdx];

		if (rowIdx >= currPage->numRows) {
			printw("~\n");
		} else {
			int len = row->rtext.size - currPage->colOff;
			if (len < 0) { len = 0; }
			if (len > ctx->screenCols) { len = ctx->screenCols; }
			addnstr(row->rtext.data + currPage->colOff, len);
			if (len < ctx->screenCols) { addch('\n'); }
		}
	}

	// Write status bar
	attron(A_REVERSE);
	addstr(ctx->statusMsg);
	int statusIdx = strlen(ctx->statusMsg);
	char linePos[128];
	int lineLen = snprintf(linePos, sizeof(linePos), "Ln %d, Col %d", currPage->cy + 1, currPage->rx + 1);
	for(int i=statusIdx; i<ctx->screenCols - lineLen; ++i) { 
		addch(' '); 
	}
	addstr(linePos);
	attroff(A_REVERSE);

	// Write bottom bar
	move(currPage->ry - currPage->rowOff, currPage->rx - currPage->colOff);
}

void editorHandleInput(editorContext* ctx, int key) {
	editorPage* page = EDITOR_CURR_PAGE(ctx);
	switch(ctx->state) {
		case ES_OPEN:
			switch(key) {
				case KEY_LEFT: pageMoveCursor(ctx, page, ED_LEFT, 1); break;
				case KEY_RIGHT: pageMoveCursor(ctx, page, ED_RIGHT, 1); break;
				case KEY_UP: pageMoveCursor(ctx, page, ED_UP, 1); break;
				case KEY_DOWN: pageMoveCursor(ctx, page, ED_DOWN, 1); break;
			}
		break;
	}
}

void editorOpenPage(editorContext* ctx, char* filename) {
	if (!ctx) { return; }
	
	// Create page
	if (ctx->numPages >= ctx->maxPages) { 
		editorGrowPages(ctx); 
	}
	ctx->currPage = ctx->numPages;
	ctx->numPages++;
	editorPage* page = EDITOR_CURR_PAGE(ctx);
	if (!filename) { 
		pageInsertRow(page, -1, "", 0);
		return; 
	}

	// Populate page with file contents
	page->filename = strdup(basename(filename));
	FILE *fp = fopen(filename, "r");
	if (!fp) { return; }
	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while((linelen = getline(&line, &linecap, fp)) != -1) {
		// Trim off newlines
		while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
			linelen--;
		}
		pageInsertRow(page, -1, line, linelen);
	}
	free(line);
	fclose(fp);

	// Set file flags
	page->flags &= ~(EF_DIRTY);
}

void editorSetPage(editorContext* ctx, int at) {
	if (!ctx) { return; }
	if (at >=0 && at < ctx->numPages) { 
		ctx->currPage = at; 
	}
	else if (at < 0 && at >= -(ctx->numPages)) { 
		ctx->currPage = ctx->numPages - at; 
	}
}