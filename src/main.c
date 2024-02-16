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
	} else {
		editorOpen(NULL);
	}

	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}
