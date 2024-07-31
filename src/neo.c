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
	if (at + len >= buf->size) { len -= (at + len) - buf->size; }

	// Shift data around
	if (at + len < (buf->size - 1)) { memmove(&buf->data[at], &buf->data[at + len], buf->size - (at + len) + 1); }
	else { buf->data[at + 1] = '\0'; }
	buf->size -= len;
}

void strbufAppend(strbuf* buf, const char* str, unsigned int len) {
	if (!buf || !str || len == 0) { return; }

	// Resize if necessary
	if (buf->size + len > (buf->capacity - 1)) { strbufGrow(buf); }

	// Copy to end of buffer
	memcpy(&buf->data[buf->size], str, len);
	buf->size += len;
	buf->data[buf->size] = '\0';
}

void strbufInsert(strbuf* buf, const char* str, unsigned int len, unsigned int at) {
	if (!buf || !str || len == 0) { return; }

	// Resize if necessary
	if (len + buf->size > (buf->capacity - 1)) { strbufGrow(buf); }

	// Shift data around
	memmove(&buf->data[at + len], &buf->data[at], (buf->size - at) + 1);

	// Copy to buffer
	memcpy(&buf->data[at], str, len);
	buf->size += len;
}

void strbufSet(strbuf* buf, const char* str, unsigned int len, unsigned int at) {
	if (!buf || !str || len == 0) { return; }

	// Resize if necessary
	if (at + len > (buf->capacity - 1)) { strbufGrow(buf); }

	// Overwrite data
	memcpy(&buf->data[at], str, len);
	if (at + len > buf->size) { buf->size += (at + len) - buf->size; }
	buf->data[buf->size] = '\0';
}

void strbufAddChar(strbuf* buf, char c) {
	if (!buf) { return; }

	if (buf->size >= (buf->capacity - 1)) { strbufGrow(buf); }
	buf->data[buf->size++] = c;
	buf->data[buf->size] = '\0';
}

void strbufDelChar(strbuf* buf) {
	if (!buf) { return; }

	buf->data[--buf->size] = '\0';
}

char strbufGetChar(strbuf* buf, int at) {
	if (!buf) { return '\0'; }
	if (at < 0) { at += buf->size; }
	if (at < 0 || at >= buf->size) { return '\0'; }
	return buf->data[at];
}

void strbufGrow(strbuf* buf) {
	unsigned int newCapacity = (buf->capacity == 0) ? 40 : buf->capacity * 2;
	char* newData = realloc(buf->data, newCapacity);
	if (!newData) { return; }
	buf->data = newData;
	buf->capacity = newCapacity;
	buf->data[buf->size] = '\0';
}

void rowInit(editorRow* row) {
	if (!row) { return; }

	strbufInit(&row->text, 1);
	strbufInit(&row->rtext, 1);
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
	int ntabs = 0;
	for(int i=0; i<row->text.size; ++i) {
		if (row->text.data[i] == '\t') { ntabs++; }
	}

	// Allocate render buffer
	strbufClear(&row->rtext);
	strbufInit(&row->rtext, row->text.size + (ntabs * (ctx->settingTabStop - 1)) + 1);

	// Render text
	for(int j=0; j<row->text.size; ++j) {
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
	if (at < 0 || at > page->numRows) { at = page->numRows; }

	// Resize if necessary
	if (page->numRows >= page->maxRows) { pageGrowRows(page); }

	// Shift rows down
	if (at < page->numRows) {
		memmove(&page->rows[at + 1], &page->rows[at], sizeof(*page->rows) * (page->numRows - at));
	}

	// Copy text buffer to row
	editorRow* row = &page->rows[at];
	rowInit(row);
	strbufSet(&row->text, str, len, 0);
	row->dirty = true;

	// Update state
	page->numRows++;
	page->flags |= EF_DIRTY;
}

void pageDeleteRow(editorPage* page, int at) {

}

void pageMoveCursor(editorPage* page, int dir, int num) {

}

void pageSetCursorRow(editorPage* page, int at) {

}

void pageSetCursorCol(editorPage* page, int at) {

}

void pageSave(editorPage* page) {

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
	editorPage* page = EDITOR_CURR_PAGE(ctx);
	page->rx = 0;
	if (page->cy < page->numRows) {
		editorRow* row = &page->rows[page->cy];
		for(int i=0; i<page->cx; ++i) {
			if (strbufGetChar(&row->text, i) == '\t') { 
				page->rx += (ctx->settingTabStop - 1) - (page->rx % ctx->settingTabStop); 
			}
			page->rx++;
		}
	}
	page->ry = page->cy + NEO_HEADER;

	// Calculate row & column offsets
	if (page->cy < page->rowOff) { page->rowOff = page->cy; }
	if (page->cy >= page->rowOff + ctx->screenRows) { page->rowOff = page->cy - ctx->screenRows + 1; }
	if (page->rx < page->colOff) { page->colOff = page->rx; }
	if (page->rx >= page->colOff + ctx->screenCols) { page->colOff = page->rx - ctx->screenCols + 1; }

	// Write header
	strbuf screenBuffer;
	strbufInit(&screenBuffer, ctx->screenRows * ctx->screenCols);
	strbufAppend(&screenBuffer, "File\n", 5);
	strbufAppend(&screenBuffer, "Tabs\n", 5);

	// Write page
	for(int y=0; y<ctx->screenRows; ++y) {
		int rowIdx = y + page->rowOff;
		editorRow* row = &page->rows[rowIdx];

		if (rowIdx >= page->numRows) {
			strbufAppend(&screenBuffer, "~\n", 2);
		} else {
			int len = row->rtext.size - page->colOff;
			if (len < 0) { len = 0; }
			if (len > ctx->screenCols) { len = ctx->screenCols; }
			strbufAppend(&screenBuffer, row->rtext.data, len);
			strbufAddChar(&screenBuffer, '\n');
		}
	}

	// Write footer
	strbufAppend(&screenBuffer, "Status\n", 8);
	strbufAppend(&screenBuffer, "\n", 1);

	// Write buffer to terminal
	move(0, 0);
	clear();
	printw(screenBuffer.data);

	// Cleanup
	move(page->ry - page->rowOff, page->rx - page->colOff + 1);
	strbufClear(&screenBuffer);
}

void editorHandleInput(editorContext* ctx, int key) {

}

void editorOpenPage(editorContext* ctx, char* filename) {
	if (!ctx) { return; }
	
	// Create page
	if (ctx->numPages >= ctx->maxPages) { editorGrowPages(ctx); }
	ctx->currPage = ctx->numPages;
	ctx->numPages++;
	if (!filename) { return; }

	// Populate page with file contents
	editorPage* page = EDITOR_CURR_PAGE(ctx);
	page->filename = strdup(filename);
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
	if (at >=0 && at < ctx->numPages) { ctx->currPage = at; }
	else if (at < 0 && at >= -(ctx->numPages)) { ctx->currPage = ctx->numPages - at; }
}