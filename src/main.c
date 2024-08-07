/**
 * main.c
 * 
 * Created by Alex Stenzel (2024)
 */
#include "neo.h"

const char* argp_program_version = "0.1.0";

const char* argp_program_bug_address = "<alexhstenzel@gmail.com>";

static char doc[] = "neodymium -- terminal text editor for linux";

static char args_doc[] = "[FILES...]";

struct arguments {
	char** files;
	int num;
};

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
	(void)(arg);

	// Get the argument
	struct arguments* arguments = state->input;
	switch(key) {
		case ARGP_KEY_ARG: {
			if (!arguments->files) {
				arguments->num = 1;
				arguments->files = malloc(sizeof(*arguments->files));
			} else {
				arguments->num++;
				char** newFiles = realloc(arguments->files, arguments->num * sizeof(*arguments->files));
				if (!newFiles) { return ARGP_ERR_UNKNOWN; }
				arguments->files = newFiles;
			}
			arguments->files[arguments->num - 1] = strdup(arg);
		} break;
		default: {
			return ARGP_ERR_UNKNOWN;
		} break;
	};
	return 0;
}

static struct argp argp = { 0, parse_opt, args_doc, doc };

void eventLoop(editorContext* ctx) {
	editorUpdate(ctx);
	editorPrint(ctx);
	refresh();
}

int main(int argc, char* argv[]) {
	// Parse arguments
	struct arguments arguments = { 0 };
	if (argp_parse(&argp, argc, argv, 0, 0, &arguments) != 0) {
		return 1;
	}

	// Register signal handlers
	struct sigaction sa;
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGWINCH, &sa, NULL) == -1) { 
		return 1; 
	}

	// Initialize ncurses
	cursesInit();

	// Create editor context
	editorContext ctx;
	editorInit(&ctx);

	// Load files from command line
	if (arguments.num == 0) {
		// Open a blank untitled page
		editorOpenPage(&ctx, NULL, -1);
	} else {
		for(int i = 0; i < arguments.num; ++i) {
			if (!editorOpenPage(&ctx, arguments.files[i], -1)) {
				// If opening the file failed, create a blank page with the files name
				editorSetMessage(&ctx, "");
				editorOpenPage(&ctx, NULL, -1);
				pageSetFullFilename(EDITOR_CURR_PAGE(&ctx), arguments.files[i]);
			}
			free(arguments.files[i]);
		}
	}
	free(arguments.files);

	// Event loop
	while(editorGetState(&ctx) != ES_SHOULD_CLOSE) {
		editorUpdate(&ctx);
		editorPrint(&ctx);
		refresh();
		if (editorGetState(&ctx) != ES_SHOULD_CLOSE) {
			editorHandleInput(&ctx, getch());
		}
	}
	endwin();

	editorClear(&ctx);
	return status_code;
}