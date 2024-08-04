/**
 * main.c
 * 
 * Created by Alex Stenzel (2024)
 */
#include "neo.h"

int main(int argc, char* argv[]) {
	// Register signal handlers
	struct sigaction sa;
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGWINCH, &sa, NULL) == -1) { return 1; }

	// Initialize ncurses
	cursesInit();

	// Create editor context
	editorContext* ctx = malloc(sizeof *ctx);
	editorInit(ctx);
	if (!ctx) { return 1; }

	// Load files from command line
	if (argc >= 2) {
		for(int i=1; i<argc; ++i) {
			editorOpenPage(ctx, argv[i], -1);
		}
		editorSetPage(ctx, 0);
	} else {
		editorOpenPage(ctx, NULL, -1);
	}

	// Event loop
	while(editorGetState(ctx) != ES_SHOULD_CLOSE) {
		editorUpdate(ctx);
		editorPrint(ctx);
		refresh();
		int ch = getch();
		editorHandleInput(ctx, ch);
	}
	endwin();

	editorClear(ctx);
	free(ctx);
	return 0;
}