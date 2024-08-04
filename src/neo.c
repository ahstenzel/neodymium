#include "neo.h"

void cursesInit() {
	initscr();
	noecho();
	cbreak();
	raw();
	keypad(stdscr, true);
}

void signalHandler(int sig) {
	switch(sig) {
		case SIGWINCH: {
			_neo_flag_resized = true; 
			endwin();
			refresh();
			clear();
		} break;
	}
}

void menuGroupInit(menuGroup* grp) {
	if (!grp) { return; }

	grp->name = NULL;
	grp->maxEntries = 0;
	grp->numEntries = 0;
	grp->selected = 0;
	grp->entries = malloc(0);
}

void menuGroupClear(menuGroup* grp) {
	if (!grp) { return; }

	free(grp->name);
	free(grp->entries);
	grp->entries = NULL;
	grp->numEntries = 0;
	grp->maxEntries = 0;
	grp->selected = 0;
}

void menuGroupInsert(menuGroup* grp, int at, menuEntry entry) {
	if (!grp) { return; }

	// Resize if necessary
	if (grp->numEntries >= grp->maxEntries) {
		int newCapacity = (grp->maxEntries == 0) ? 1 : (grp->maxEntries * 2);
		menuEntry* newEntries = realloc(grp->entries, newCapacity * sizeof(*grp->entries));
		if (!newEntries) { return; }
		grp->entries = newEntries;
		grp->maxEntries = newCapacity;
	}

	// Check boundaries
	if (at < 0 || at > grp->numEntries) { at = grp->numEntries; }

	// Shift elements
	if (at < grp->numEntries) {
		memmove(&grp->entries[at + 1], &grp->entries[at], (grp->numEntries - at) * sizeof(*grp->entries));
	}

	// Add to list
	memcpy(&grp->entries[at], &entry, sizeof(*grp->entries));
	grp->numEntries++;
}

void menuGroupDelete(menuGroup* grp, int at) {
	if (!grp || grp->numEntries == 0) { return; }

	// Check boundaries
	if (at < 0 || at >= grp->numEntries) { at = grp->numEntries - 1; }

	// Shift elements
	memmove(&grp->entries[at], &grp->entries[at + 1], (grp->numEntries - at - 1) * sizeof(*grp->entries));
	grp->numEntries--;
	if (grp->selected == at) { grp->selected = 0; }
}

bool _neo_flag_resized = false;

void strbufInit(strbuf* buf, unsigned int capacity) {
	if (!buf) { return; }
	assert(capacity >= 1);

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
		buf->data[buf->size] = '\0';
	}
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
		newCapacity = (newCapacity <= 1) ? 40 : newCapacity * 2; 
	}
	char* newData = realloc(buf->data, newCapacity);
	if (!newData) { return; }
	buf->data = newData;
	buf->capacity = newCapacity;
	buf->data[buf->size] = '\0';
}

int strbufLength(strbuf* buf) {
	if (!buf) { return 0; }
	return strlen(buf->data);
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

	// Clear render buffer
	strbufDelete(&row->rtext, 0, row->rtext.size);

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
	page->fullFilename = NULL;
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
	free(page->fullFilename);
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
		memmove(&page->rows[at + 1], &page->rows[at], (page->numRows - at) * sizeof(*page->rows));
	}

	// Copy text buffer to row
	editorRow* row = &page->rows[at];
	strbufDelete(&row->text, 0, row->text.size);
	strbufSet(&row->text, str, len, 0);
	row->dirty = true;

	// Update state
	page->numRows++;
	if (PAGE_FLAG_ISCLEAR(page, EF_READONLY)) {
		PAGE_FLAG_SET(page, EF_DIRTY);
	}
}

void pageDeleteRow(editorPage* page, int at) {
	(void)(page);
	(void)(at);
}

static void correctForTabs(editorContext* ctx, editorPage* page, editorRow* currRow, editorRow* nextRow) {
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

void pageMoveCursor(editorContext* ctx, editorPage* page, int dir, int num) {
	for(int r=0; r<num; ++r) {
		editorRow* currRow = (page->cy >= page->numRows) ? NULL : &page->rows[page->cy];
		editorRow* nextRow = currRow;

		// Move cursor
		switch(dir) {
			case ED_DOWN: {
				if (page->cy < page->numRows) { 
					// Correct for tabs
					nextRow = (page->cy + 1 >= page->numRows) ? NULL : &page->rows[page->cy + 1];
					if (nextRow && page->cx > 0) {
						correctForTabs(ctx, page, currRow, nextRow);
					}

					// Move line
					page->cy++; 
				}
			} break;
			case ED_UP: {
				if (page->cy != 0) { 
					// Correct for tabs
					nextRow = (page->cy - 1 >= page->numRows) ? NULL : &page->rows[page->cy - 1];
					if (nextRow && page->cx > 0) {
						correctForTabs(ctx, page, currRow, nextRow);
					}
					
					// Move line
					page->cy--; 
				}
			} break;
			case ED_LEFT: {
				if (page->cx != 0) { page->cx--; }
				else if (page->cy > 0) {
					nextRow = (page->cy - 1 >= page->numRows) ? NULL : &page->rows[page->cy - 1];
					page->cy--;
					page->cx = page->rows[page->cy].text.size;
				}
			} break;
			case ED_RIGHT: {
				if (currRow && (unsigned int)(page->cx) < currRow->text.size) { page->cx++; }
				else if (currRow && (unsigned int)(page->cx) == currRow->text.size) {
					nextRow = (page->cy + 1 >= page->numRows) ? NULL : &page->rows[page->cy + 1];
					page->cy++;
					page->cx = 0;
				}
			} break;
		}

		// Snap cursor to line endings
		int rowLen = nextRow ? nextRow->text.size : 0;
		if (page->cx > rowLen) { page->cx = rowLen; }
	}
}

void pageSetCursorRow(editorPage* page, int at) {
	(void)(page);
	(void)(at);
}

void pageSetCursorCol(editorPage* page, int at) {
	(void)(page);
	(void)(at);
}

void pageSave(editorContext* ctx, editorPage* page) {
	(void)(ctx);
	(void)(page);
	/*
	if (!page->filename) {
		editorSetPage(ctx, at);
		char* filename = editorPrompt(ctx, "File name: %s");
		if (filename) {
			page->filename = strdup(filename);
			free(filename);
			pageSave(ctx, page);
		}
	}
	*/
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
	ctx->pageOff = 0;
	ctx->settingTabStop = 4;

	// Hard code menu groups
	ctx->currMenu = 0;
	ctx->numMenus = 3;
	ctx->menus = malloc(ctx->numMenus * sizeof(*ctx->menus));
	for(int i=0; i<ctx->numMenus; ++i) { menuGroupInit(&ctx->menus[i]); }
	menuGroup* menuFile = &ctx->menus[0];
	menuGroup* menuEdit = &ctx->menus[1];
	menuGroup* menuHelp = &ctx->menus[2];
	menuFile->name = strdup("File");
	menuEdit->name = strdup("Edit");
	menuHelp->name = strdup("Help");

	menuEntry spacer = {
		.name = NULL,
		.shortcut = '\0',
		.callback = NULL
	};
	menuEntry entry = { 0 };
	entry.name = "New"; entry.shortcut = 'n'; menuGroupInsert(menuFile, -1, entry);
	entry.name = "Open"; entry.shortcut = 'o'; menuGroupInsert(menuFile, -1, entry);
	menuGroupInsert(menuFile, -1, spacer);
	entry.name = "Save"; entry.shortcut = 's'; menuGroupInsert(menuFile, -1, entry);
	entry.name = "Save All"; entry.shortcut = 'd'; menuGroupInsert(menuFile, -1, entry);
	menuGroupInsert(menuFile, -1, spacer);
	entry.name = "Next Tab"; entry.shortcut = 't'; menuGroupInsert(menuFile, -1, entry);
	entry.name = "Last Tab"; entry.shortcut = 'r'; menuGroupInsert(menuFile, -1, entry);
	entry.name = "Close Tab"; entry.shortcut = 'w'; menuGroupInsert(menuFile, -1, entry);
	entry.name = "Quit"; entry.shortcut = 'q'; menuGroupInsert(menuFile, -1, entry);

	entry.name = "Cut"; entry.shortcut = 'x'; menuGroupInsert(menuEdit, -1, entry);
	entry.name = "Copy"; entry.shortcut = 'c'; menuGroupInsert(menuEdit, -1, entry);
	entry.name = "Paste"; entry.shortcut = 'v'; menuGroupInsert(menuEdit, -1, entry);
	menuGroupInsert(menuEdit, -1, spacer);
	entry.name = "Undo"; entry.shortcut = 'z'; menuGroupInsert(menuEdit, -1, entry);
	entry.name = "Redo"; entry.shortcut = 'y'; menuGroupInsert(menuEdit, -1, entry);

	entry.name = "Docs"; entry.shortcut = '1'; menuGroupInsert(menuHelp, -1, entry);
	menuGroupInsert(menuHelp, -1, spacer);
	entry.name = "About"; entry.shortcut = '\0'; entry.callback = cbMenuHelpAbout; menuGroupInsert(menuHelp, -1, entry);
}

void editorClear(editorContext* ctx) {
	if (!ctx) { return; }

	for(int i=0; i<ctx->numPages; ++i) { 
		pageClear(&ctx->pages[i]); 
	}
	for(int i=0; i<ctx->numMenus; ++i) {
		menuGroupClear(&ctx->menus[i]);
	}
	free(ctx->pages);
	free(ctx->menus);
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
	ctx->pages = newPages;
	ctx->maxPages = newSize;
}

int editorGetState(editorContext* ctx) {
	if (!ctx) { return ES_SHOULD_CLOSE; }
	return ctx->state;
}

void editorPrint(editorContext* ctx) {
	if (!ctx) { return; }

	// Query terminfo for window size
	if (_neo_flag_resized) {
		getmaxyx(stdscr, ctx->screenRows, ctx->screenCols);
		ctx->screenRows -= (NEO_HEADER + NEO_FOOTER);
		_neo_flag_resized = false;
	}

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

	// Render full page line
	move(1, 0);
	strbuf pageLine;
	strbufInit(&pageLine, ctx->screenCols);
	for(int pageIdx=0; pageIdx<ctx->numPages; ++pageIdx) {
		editorPage* page = &ctx->pages[pageIdx];
		if (!page) { continue; }

		// Draw filename & decoration
		if (pageIdx == ctx->currPage) {
			strbufAddChar(&pageLine, '|');
		} else {
			strbufAddChar(&pageLine, ' ');
		}

		if (PAGE_FLAG_ISSET(page, EF_DIRTY)) {
			strbufAddChar(&pageLine, '*');
		}
		int segmentLen = 0;
		if (page->filename) {
			segmentLen = strlen(page->filename);
			strbufAppend(&pageLine, page->filename, segmentLen);
		} else {
			segmentLen = 10;
			strbufAppend(&pageLine, "<untitled>", segmentLen);
		}
		if (pageIdx == ctx->currPage) {
			strbufAddChar(&pageLine, '|');
		} else {
			strbufAddChar(&pageLine, ' ');
		}
		if (PAGE_FLAG_ISSET(page, EF_DIRTY))	{
			segmentLen++;
		}
		segmentLen += 2;

		// Scroll page offset
		if (pageIdx == ctx->currPage) {
			if (strbufLength(&pageLine) - segmentLen < ctx->pageOff + 5 && ctx->pageOff > 0) {
				ctx->pageOff = strbufLength(&pageLine) - segmentLen - 5;
			} else if (strbufLength(&pageLine) - ctx->pageOff > ctx->screenCols - 5) {
				ctx->pageOff = (strbufLength(&pageLine) - ctx->screenCols) + 5;
			}
		}
	}
	if (ctx->pageOff < 0) { 
		ctx->pageOff = 0; 
	}
	if ((ctx->pageOff + ctx->screenCols) > strbufLength(&pageLine) && strbufLength(&pageLine) > ctx->screenCols) {
		ctx->pageOff = strbufLength(&pageLine) - ctx->screenCols;
	}

	// Add scroll markers on either side
	if (ctx->pageOff > 0) {
		strbufSet(&pageLine, "< ...", 5, ctx->pageOff);
	}
	if (ctx->pageOff + ctx->screenCols < strbufLength(&pageLine)) {
		strbufSet(&pageLine, "... >", 5, ctx->pageOff + ctx->screenCols - 5);
	}

	// Trim page line to screen width
	attron(A_REVERSE);
	addnstr(&pageLine.data[ctx->pageOff], ctx->screenCols);
	int drawLen = strbufLength(&pageLine) - ctx->pageOff;
	while(drawLen < ctx->screenCols) {
		addch(' ');
		drawLen++;
	}
	attroff(A_REVERSE);
	strbufClear(&pageLine);

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

	// Write status message
	attron(A_REVERSE);
	int statusLen = strlen(ctx->statusMsg);
	if (statusLen > 0 && time(NULL) - ctx->statusMsgTime < 5) {
		addstr(ctx->statusMsg);
	} else {
		statusLen = 0;
	}

	// Write cursor position
	if (ctx->state == ES_PROMPT) {
		for(int i=statusLen; i<ctx->screenCols; ++i) { 
			addch(' '); 
		}
	} else {
		char linePos[16];
		int lineLen = snprintf(linePos, sizeof(linePos), "Ln %d, Col %d", currPage->cy + 1, currPage->rx + 1);
		for(int i=statusLen; i<ctx->screenCols - lineLen; ++i) { 
			addch(' '); 
		}
		addstr(linePos);
	}
	attroff(A_REVERSE);

	// Write bottom bar
	char fileInfo[32];
	int infoLen = snprintf(
		fileInfo, sizeof(fileInfo), "| Lines: %d%s", 
		currPage->numRows,
		PAGE_FLAG_ISSET(currPage, EF_READONLY) ? " (READ-ONLY)" : ""
	);
	int fullFilenameDraw = 0;
	int fullFilenameOff = 0;
	int fullFilenameLen = 0;
	if (currPage->fullFilename) {
		fullFilenameLen = strlen(currPage->fullFilename);
		if (fullFilenameLen > ctx->screenCols - infoLen) {
			fullFilenameOff = ctx->screenCols - infoLen;
		}
		fullFilenameDraw += fullFilenameLen - fullFilenameOff;
		addnstr(&currPage->fullFilename[fullFilenameOff], fullFilenameDraw);
	}
	for(int i=fullFilenameDraw; i<ctx->screenCols - infoLen; ++i) {
		addch(' ');
	}
	addnstr(fileInfo, infoLen);

	// Write file bar
	move(0, 0);
	for(int i=0; i<ctx->numMenus; ++i) {
		menuGroup* menu = &ctx->menus[i];
		if (!menu->name) { continue; }
		if (ctx->state == ES_MENU && i == ctx->currMenu) { 
			attron(A_REVERSE); 
		}
		attron(A_UNDERLINE); 
		addch(menu->name[0]);
		attroff(A_UNDERLINE);
		if (strlen(menu->name) > 1) {
			addnstr(&menu->name[1], 7);
		}
		for(int i=strlen(menu->name); i<8; ++i) {
			addch(' ');
		}
		if (ctx->state == ES_MENU && i == ctx->currMenu) { 
			attroff(A_REVERSE); 
		}
	}
	if (ctx->state == ES_MENU) {
		editorPrintMenu(ctx, EDITOR_CURR_MENU(ctx), 8 * ctx->currMenu);
	}

	// Move cursor
	if (ctx->state == ES_MENU) {
		curs_set(0);
	} else {
		curs_set(1);
	}
	move(currPage->ry - currPage->rowOff, currPage->rx - currPage->colOff);
}

void editorPrintMenu(editorContext* ctx, menuGroup* grp, int off) {
	(void)(ctx);
	if (!ctx || !grp || off < 0) { return; }

	attron(A_REVERSE);
	move(1, off);
	printw(",__________________,");
	int i=0;
	for(; i<grp->numEntries; ++i) {
		move(i + 2, off);
		menuEntry entry = grp->entries[i];
		if (!entry.name) {
			printw("|__________________|");
		} else {
			addch('|');
			if (grp->selected == i) { attroff(A_REVERSE); }
			int len = strlen(entry.name);
			addstr(entry.name);
			
			if (isalnum(entry.shortcut)) {
				if (isdigit(entry.shortcut)) {
					while(len < 14) { addch(' '); len++; }
					addstr("(F");
					addch(entry.shortcut);
					addch(')');
				} else {
					while(len < 10) { addch(' '); len++; }
					addstr("(Ctrl-");
					addch(toupper(entry.shortcut));
					addch(')');
				}
			} else {
				printw("             ");
			}
			if (grp->selected == i) { attron(A_REVERSE); }
			addch('|');
		}
	}
	move(i + 2, off);
	printw("|__________________|");
	attroff(A_REVERSE);
}

void editorHandleInput(editorContext* ctx, int key) {
	if (!ctx) { return; }

	editorPage* page = EDITOR_CURR_PAGE(ctx);
	switch(ctx->state) {
		case ES_OPEN: {
			switch(key) {
				case KEY_LEFT: pageMoveCursor(ctx, page, ED_LEFT, 1); break;
				case KEY_RIGHT: pageMoveCursor(ctx, page, ED_RIGHT, 1); break;
				case KEY_UP: pageMoveCursor(ctx, page, ED_UP, 1); break;
				case KEY_DOWN: pageMoveCursor(ctx, page, ED_DOWN, 1); break;
				case KEY_PPAGE: pageMoveCursor(ctx, page, ED_UP, ctx->screenRows); break;
				case KEY_NPAGE: pageMoveCursor(ctx, page, ED_DOWN, ctx->screenRows); break;
				case KEY_HOME: {
					page->cx = 0;
				} break;
				case KEY_END: {
					editorRow* row = (page->cy >= page->numRows) ? NULL : &page->rows[page->cy];
					page->cx = (!row) ? 0 : row->text.size; 
				} break;
				case KEY_SLEFT: {
					editorSetMessage(ctx, "Shift-Left");
				} break;
				case KEY_SRIGHT: {
					editorSetMessage(ctx, "Shift-Right");
				} break;
				case KEY_SR: {
					editorSetMessage(ctx, "Shift-Up");
				} break;
				case KEY_SF: {
					editorSetMessage(ctx, "Shift-Down");
				} break;
				case KEY_F(1): {
					editorOpenPage(ctx, NULL, 0);
				} break;
				case CTRL_KEY('r'): {
					ctx->currPage--;
					if (ctx->currPage < 0) {
						ctx->currPage = ctx->numPages - 1;
					}
				} break;
				case CTRL_KEY('t'): {
					ctx->currPage++;
					if (ctx->currPage > ctx->numPages - 1) {
						ctx->currPage = 0;
					}
				} break;
				case CTRL_KEY('q'): {
					if (editorCloseAll(ctx)) {
						ctx->state = ES_SHOULD_CLOSE;
					}
				} break;
				case CTRL_KEY('w'): {
					editorClosePage(ctx, ctx->currPage, true);
					if (ctx->numPages == 0) {
						editorOpenPage(ctx, NULL, -1);
					}
				} break;
				case CTRL_KEY('s'): {
					pageSave(ctx, EDITOR_CURR_PAGE(ctx));
				} break;
				case CTRL_KEY('n'): {
					editorOpenPage(ctx, NULL, -1);
				} break;
				case CTRL_KEY('o'): {
					strbuf input;
					editorPrompt(ctx, &input, "Open file: %s");
					if (input.data) {
						if (editorOpenPage(ctx, input.data, -1)) {
							if (ctx->numPages == 2) {
								editorPage* page = &ctx->pages[0];
								if (!page->filename && PAGE_FLAG_ISCLEAR(page, EF_DIRTY)) {
									editorClosePage(ctx, ctx->currPage, false);
								}
							}
						}
						strbufClear(&input);
					}
				} break;
				case CTRL_KEY('c'): {
					editorSetMessage(ctx, "Copy");
				} break;
				case CTRL_KEY('x'): {
					editorSetMessage(ctx, "Cut");
				} break;
				case CTRL_KEY('v'): {
					editorSetMessage(ctx, "Paste");
				} break;
				case CTRL_KEY('f'): { ctx->state = ES_MENU; ctx->currMenu = 0; } break;
				case CTRL_KEY('e'): { ctx->state = ES_MENU; ctx->currMenu = 1; } break;
				case CTRL_KEY('h'): { ctx->state = ES_MENU; ctx->currMenu = 2; } break;
			}
		} break;
		case ES_MENU: {
			menuGroup* menu = EDITOR_CURR_MENU(ctx);
			switch(key) {
				case CTRL_KEY('q'): { ctx->state = ES_OPEN; } break;
				case CTRL_KEY('f'): { 
					if (ctx->currMenu == 0) {
						ctx->state = ES_OPEN; 
					} else {
						ctx->currMenu = 0;
					}
				} break;
				case CTRL_KEY('e'): { 
					if (ctx->currMenu == 1) {
						ctx->state = ES_OPEN; 
					} else {
						ctx->currMenu = 1;
					}
				} break;
				case CTRL_KEY('h'): { 
					if (ctx->currMenu == 2) {
						ctx->state = ES_OPEN; 
					} else {
						ctx->currMenu = 2;
					}
				} break;
				case KEY_UP: {
					int count = 0;
					do {
						count++;
						menu->selected--;
						if (menu->selected < 0) {
							menu->selected = menu->numEntries - 1;
						}
					}
					while(!menu->entries[menu->selected].name && count < menu->numEntries);
				} break;
				case KEY_DOWN: {
					int count = 0;
					do {
						count++;
						menu->selected++;
						if (menu->selected >= menu->numEntries) {
							menu->selected = 0;
						}
					}
					while(!menu->entries[menu->selected].name && count < menu->numEntries);
				} break;
				case KEY_LEFT: {
					ctx->currMenu--;
					if (ctx->currMenu < 0) { ctx->currMenu = ctx->numMenus - 1; }
				} break;
				case KEY_RIGHT: {
					ctx->currMenu++;
					if (ctx->currMenu >= ctx->numMenus) { ctx->currMenu = 0; }
				} break;
				case '\n':
				case '\r':
				case KEY_ENTER: {
					ctx->state = ES_OPEN;
					menuEntry* entry = &menu->entries[menu->selected];
					if (isalnum(entry->shortcut)) {
						// Convert shortcut character to keypress value
						int key = 0;
						if (isdigit(entry->shortcut)) {
							char digit[2] = "\0\0";
							digit[0] = entry->shortcut;
							key = KEY_F(atoi(digit));
						} else {
							key = CTRL_KEY(entry->shortcut);
						}
						editorHandleInput(ctx, key);
					} else {
						// Use specified callback
						entry->callback((void*)ctx, 1);
					}
				} break;
			}
		} break;

	}
}

bool editorOpenPage(editorContext* ctx, char* filename, int internal) {
	if (!ctx) { return false; }
	
	// Resize array if necessary
	if (ctx->numPages >= ctx->maxPages) { 
		editorGrowPages(ctx); 
	}
	
	if (!filename && internal < 0) {
		// Create empty page
		ctx->currPage = ctx->numPages;
		ctx->numPages++;
		editorPage* page = EDITOR_CURR_PAGE(ctx);
		pageInit(page);
		return true;
	} else {
		// Check if file is valid
		int pageFlags = 0;
		FILE* fp = NULL;
		if (!filename) {
			// Internal documents
			pageFlags |= EF_READONLY;
			if (internal == 0) {
				fp = fmemopen((void*)_help_docs_contents, strlen(_help_docs_contents), "r");
				filename = (char*)&_help_docs_filename[0];
			}
			if (!fp) {
				editorSetMessage(ctx, "Failed to open internal file (%d)!", internal);
				return false;
			}
		} else {
			// External files
			fp = fopen(filename, "r");
			if (!fp) { 
				editorSetMessage(ctx, "Failed to open file (%s)!", filename);
				return false;
			}
		}

		// Create page
		ctx->currPage = ctx->numPages;
		ctx->numPages++;
		editorPage* page = EDITOR_CURR_PAGE(ctx);
		pageInit(page);
		page->filename = strdup(basename(filename));
		page->fullFilename = strdup(filename);

		// Populate page with file contents
		char* line = NULL;
		size_t n = 0;
		ssize_t linelen;
		while((linelen = getline(&line, &n, fp)) != -1) {
			// Trim newlines
			while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
				linelen--;
			}
			pageInsertRow(page, -1, line, linelen);
		}
		free(line);
		fclose(fp);
		page->flags = pageFlags;
		return true;
	}
}

void editorSetPage(editorContext* ctx, int at) {
	if (!ctx || at >= ctx->numPages) { return; }
	
	if (at < 0) {
		at = ctx->numPages - 1;
	}
	ctx->currPage = at;
}

int editorSetMessage(editorContext* ctx, const char* fmt, ...) {
	if (!ctx) { return 0; }

	int msgLen = min(ctx->screenCols - 16, 79);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(&ctx->statusMsg[0], msgLen, fmt, ap);
	va_end(ap);
	ctx->statusMsgTime = time(NULL);
	return msgLen;
}

void editorClosePage(editorContext* ctx, int at, bool save) {
	if (!ctx || at >= ctx->numPages) { return; }

	// Adjust bondaries
	if (at < 0) { 
		at = ctx->numPages - 1; 
	}
	editorPage* page = &ctx->pages[at];
	if (page) {
		// Ask to save
		if (PAGE_FLAG_ISSET(page, EF_DIRTY) && PAGE_FLAG_ISCLEAR(page, EF_READONLY) && save) {
			pageSave(ctx, page);
		}

		// Close page
		pageClear(page);
		if (at < ctx->numPages - 1) {
			memmove(&ctx->pages[at], &ctx->pages[at + 1], (ctx->numPages - at) * sizeof(*ctx->pages));
		}
		ctx->numPages--;
		if (ctx->currPage >= ctx->numPages) {
			ctx->currPage = ctx->numPages - 1;
		}
	}
}

bool editorCloseAll(editorContext* ctx) {
	if (!ctx) { return false; }

	// Check for unsaved files
	bool save = false;
	for(int i=0; i<ctx->numPages; ++i) {
		editorPage* page = &ctx->pages[i];
		if (PAGE_FLAG_ISSET(page, EF_DIRTY) && PAGE_FLAG_ISCLEAR(page, EF_READONLY)) {
			save = true;
			break;
		}
	}

	// Ask user if they want to save
	if (save) {
		while (1) {
			strbuf input;
			editorPrompt(ctx, &input, "Save all files? (y/n/c) %s");
			if (input.data) {
				for(char* i = input.data; *i; ++i) { *i = tolower(*i); }
				if (strcmp(input.data, "y") == 0) {
					strbufClear(&input);
					break;
				} else if (strcmp(input.data, "n") == 0) {
					save = false;
					strbufClear(&input);
					break;
				} else if (strcmp(input.data, "c") == 0) {
					strbufClear(&input);
					return false;
				}
			}
			strbufClear(&input);
		}
	}

	// Close pages
	while(ctx->numPages > 0) {
		editorClosePage(ctx, 0, save);
	}
	return true;
}

void editorPrompt(editorContext* ctx, strbuf* buf, const char* prompt) {
	strbufInit(buf, 1);
	if (!ctx) { 
		strbufClear(buf);
		return; 
	}

	int lastState = ctx->state;
	ctx->state = ES_PROMPT;
	while(1) {
		// Update editor state
		editorSetMessage(ctx, prompt, buf->data);
		editorPrint(ctx);
		refresh();
		
		// Get input
		int c = getch();
		if (c == KEY_DC || c == KEY_BACKSPACE || c == CTRL_KEY('h')) {
			strbufDelChar(buf);
		} else if (c == CTRL_KEY('q') || c == CTRL_KEY('c')) {
			editorSetMessage(ctx, "");
			strbufClear(buf);
			break;
		} else if (c == '\r' || c == '\n' || c == KEY_ENTER) {
			if (buf->size != 0) {
				editorSetMessage(ctx, "");
				break;
			}
		} else if (!iscntrl(c) && c < 128) {
			strbufAddChar(buf, c);
		}		
	}
	ctx->state = lastState;
}

void cbMenuHelpAbout(void* data, int num) {
	if (!data) { return; }
	(void)(num);

	// Extract arguments
	editorContext* ctx = (editorContext*)(data);
	editorSetMessage(ctx, "VERSION %s", NEO_VERSION);
}

const char _help_docs_filename[] = "DOCS";
const char _help_docs_contents[] = "\
=====CONCEPTS======\n\
Status Bar:\n\
\tThe white bar along the bottom of the screen is for displaying\n\
\tstatus messages, as well as prompting user input. When opening\n\
\tor saving a file, for instance, you will be asked to input a\n\
\tfilename on that bar. Note that you can cancel out of input\n\
\tprompting at any time with the Quit shortcut, Ctrl-Q. It also\n\
\tdisplays where your cursor is in the file.\n\
\n\
Info Bar:\n\
\tThe bottom line of the screen is reserved for showing information\n\
\tabout the file itself, such as it's full path, how many lines\n\
\tare in it, and any special attributes, such as whether it is\n\
\tread-only.\n\
\n\
Menu Bar:\n\
\tIf you can't remember the key shortcut to perform an action, or\n\
\tjust don't want to use it, you can open the menu bar to select\n\
\tan action manually. To close the menu bar and return to editing,\n\
\teither select an action with Enter(navigating using the arrow keys),\n\
\tuse the Quit shortcut Ctrl-Q, or use the same shortcut you used\n\
\tto open the currently active menu.\n\
\n\
=====SHORTUCTS=====\n\
Open menu groups:\n\
\tFile: Ctrl-F\n\
\tEdit: Ctrl-E\n\
\tHelp: Ctrl-H\n";
