#include "neo.h"

void cursesInit() {
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, true);
}

editorRow* rowInit() {

}

void rowUpdate(editorRow* row) {

}

editorPage* pageInit() {

}

void pageUpdate(editorPage* page) {

}

void pageInsertRow(editorPage* page, int at, char* str, unsigned int len) {

}

void pageAppendRow(editorPage* page, int at, char* str, unsigned int len) {

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

editorContext* editorInit() {
	editorContext* ctx = calloc(1, sizeof *ctx);
	if (!ctx) { return NULL; }
	ctx->currPage = -1;
	ctx->statusMsg[0] = '\0';
	ctx->state = ES_OPEN;
	getmaxyx(stdscr, ctx->screenRows, ctx->screenCols);
	ctx->screenCols -= (NEO_HEADER + NEO_FOOTER);
	return ctx;
}

void editorUpdate(editorContext* ctx) {

}

int editorGetState(editorContext* ctx) {

}

void editorPrint(editorContext* ctx) {

}

void editorHandleInput(editorContext* ctx, int key) {

}

void editorOpenPage(editorContext* ctx, char* filename) {

}

void editorSetPage(editorContext* ctx, int at) {

}