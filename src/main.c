/**********************************************************
* neodymium
*
* Created by Alex Stenzel (2024)
**********************************************************/
#include "nd.h"

int main(int argc, char* argv[]) {
	enableRawMode();
	initEditor();
	if (argc >= 2) {
		for(int i=1; i<argc; ++i) {
			editorOpen(argv[i]);
		}
		editorSetTab(0);
	} else {
		editorOpen(NULL);
	}

	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}
