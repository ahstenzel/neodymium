#include "neo.h"
#define VEX_IMPLEMENTATION
#include <vex/vex.h>
#undef VEX_IMPLEMENTATION

int main(int argc, char** argv) {
	// Parse command line arguments
	vex_init_info parser_info = {
		.name = "neo",
		.description = "Terminal text editor with rich features.",
		.version = NEO_VERSION_STR
	};
	vex_ctx parser = vex_init(parser_info);
	vex_parse(&parser, argc, argv);
	if (vex_arg_found(&parser, "v")) {
		const char* ver = vex_get_version(&parser);
		printf("%s\n", ver);
		return 0;
	}
	if (vex_arg_found(&parser, "h")) {
		const char* help = vex_get_help(&parser);
		printf("%s", help);
		return 0;
	}

	// Register signal handlers
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGWINCH, &sa, NULL) == -1) { return 1; }
	if (sigaction(SIGINT, &sa, NULL) == -1) { return 1; }

	// Startup sequence
	ncurses_init();
	neo_edit_ctx_t edit_ctx = { 0 };
	do {
		if (!neo_edit_ctx_init(&edit_ctx)) {
			NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initalize editor context");
			break;
		}
		else {
			// Open files in editor
			if (vex_token_count(&parser) > 0 && false) {
				for(int i = 0; i < vex_token_count(&parser); ++i) {
					vex_arg_token* tok = vex_get_token(&parser, i);
					char* filename = tok->arg->str_arg;
					size_t idx = SIZE_MAX;

					// Convert filenames to wide characters
					#ifdef NEO_USE_WCHAR
					size_t filename_len = strlen(filename);
					NEO_CHAR_T* filename_w = NEO_MALLOC(NEO_CHAR_SIZE * (filename_len + 1));
					if (!filename_w) {
						NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate wide filename buffer");
						break;
					}
					if (mbstowcs(filename_w, filename, filename_len) != filename_len) {
						NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to convert filename to wide string");
						break;
					}
					filename_w[filename_len] = '\0';
					idx = neo_edit_ctx_open_page(&edit_ctx, filename_w);
					#else
					idx = neo_edit_ctx_open_page(&edit_ctx, filename);
					#endif
					if (idx == SIZE_MAX) {
						NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to open file");
						break;
					}
				}
			}
			else {
				if (neo_edit_ctx_new_page(&edit_ctx) == SIZE_MAX) {
					NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to create new file");
					break;
				}
			}
		}
	} while(0);

	// Event loop
	while(edit_ctx.state != NSTATE_SHOULD_CLOSE) {
		bool ret = true;

		// Update & render context
		ret &= neo_edit_ctx_update(&edit_ctx);
		ret &= neo_edit_ctx_draw(&edit_ctx);

		// Draw to screen
		update_panels();
		doupdate();

		// Get input
		if (edit_ctx.state != NSTATE_SHOULD_CLOSE) {
			ret &= neo_edit_ctx_handle_input(&edit_ctx, getch());
		}

		// Check for close
		if (!ret || _neo_error_code == SIGINT) { edit_ctx.state = NSTATE_SHOULD_CLOSE; }
	}

	// Cleanup
	vex_free(&parser);
	neo_edit_ctx_clear(&edit_ctx);
	ncurses_clear();
	return NEO_ERRORNO;
}