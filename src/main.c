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
	editorContext ctx;
	editorInit(&ctx);

	// Load files from command line
	if (argc >= 2) {
		for(int i=1; i<argc; ++i) {
			if (!editorOpenPage(&ctx, argv[i], -1)) {
				// If opening the file failed, create a blank page with the files name
				editorSetMessage(&ctx, "");
				editorOpenPage(&ctx, NULL, -1);
				pageSetFullFilename(EDITOR_CURR_PAGE(&ctx), argv[i]);
			}
		}
		editorSetPage(&ctx, 0);
	} else {
		// Open a blank untitled page
		editorOpenPage(&ctx, NULL, -1);
	}

	// Event loop
	while(editorGetState(&ctx) != ES_SHOULD_CLOSE) {
		editorUpdate(&ctx);
		editorPrint(&ctx);
		refresh();
		int ch = getch();
		editorHandleInput(&ctx, ch);
	}
	endwin();

	editorClear(&ctx);
	return 0;
}