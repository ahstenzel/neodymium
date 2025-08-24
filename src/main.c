#include "neo.h"
#include "neo_string.h"
#define VEX_IMPLEMENTATION
#include <vex/vex.h>
#undef VEX_IMPLEMENTATION

#ifndef VERSION
#define VERSION "0.0.0"
#endif

int main(int argc, char** argv) {
	// Parse command line arguments
	vex_init_info parser_info = {
		.name = "neo",
		.description = "Terminal text editor with rich features.",
		.version = VERSION
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
	for(int i = 0; i < vex_token_count(&parser); ++i) {
		vex_arg_token* tok = vex_get_token(&parser, i);
		printf("File: %s\n", tok->arg->str_arg);
	}
	vex_free(&parser);

	// Initialize editor

	// Open files in editor

	// Run event loop
	
	return NEO_ERRORNO;
}