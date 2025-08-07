/*
 vex.h

 Cross-platform single-header C99 argument parsing library.

 Written by Alex Stenzel (alexhstenzel@gmail.com) 2025.
 */
#ifndef VEX_H
#define VEX_H

#ifdef __cplusplus
extern "C" {
#define CPPCAST(s) (s)
#else
#define CPPCAST(s)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

// Pre-C11 function aliases
#if !defined(__STDC_LIB_EXT1__)
#define strcat_s(dest, destsz, src) strcat(dest, src)
#define strncat_s(dest, destsz, src, n) strncat(dest, src, n)
#endif

// Argument types
#define VEX_ARG_TYPE_UNKNOWN 0
#define VEX_ARG_TYPE_FLAG 1
#define VEX_ARG_TYPE_INT 2
#define VEX_ARG_TYPE_DUB 3
#define VEX_ARG_TYPE_STR 4

// Parser status
#define VEX_STATUS_OK 0
#define VEX_STATUS_BAD_ALLOC 1
#define VEX_STATUS_BAD_VALUE 2
#define VEX_STATUS_UNKNOWN_ARG 3

// Memory allocation
#ifndef VEX_MALLOC
#define VEX_MALLOC malloc
#endif
#ifndef VEX_REALLOC
#define VEX_REALLOC realloc
#endif
#ifndef VEX_FREE
#define VEX_FREE free
#endif

typedef struct {
	const char* name;
	const char* version;
	const char* description;
} vex_init_info;

typedef union {
	char* str_arg;
	double dub_arg;
	int int_arg;
} vex_value;

typedef struct {
	char* long_name;
	char short_name;
	vex_value* arg;
	int arg_count;
	int arg_type;
} vex_arg_token;

typedef struct {
	char* description;
	char* long_name;
	char short_name;
	int arg_type;
	int max_count;
} vex_arg_desc;

typedef struct {
	char* name;
	char* help_msg;
	char* status_msg;
	char* description;
	char* version;
	vex_arg_desc* arg_desc;
	int num_arg_desc;
	int capacity_arg_desc;
	vex_arg_token* arg_token;
	int num_arg_token;
	int capacity_arg_token;
	int status;
} vex_ctx;

vex_ctx vex_init(vex_init_info init_info);

bool vex_add_arg(vex_ctx* ctx, vex_arg_desc desc);

bool vex_parse(vex_ctx* ctx, int argc, char** argv);

int vex_token_count(vex_ctx* ctx);

vex_arg_token* vex_get_token(vex_ctx* ctx, int num);

bool vex_arg_found(vex_ctx* ctx, const char* name);

void vex_free(vex_ctx* ctx);

const char* vex_get_version(vex_ctx* ctx);

const char* vex_get_help(vex_ctx* ctx);

char* _vex_strdup(const char* str);

bool _vex_add_token(vex_ctx* ctx, vex_arg_token token);

void _vex_set_status(vex_ctx* ctx, int status, char* fmt, ...);

bool _vec_token_add_value(vex_ctx* ctx, vex_arg_token* token, vex_value value);

#ifdef VEX_IMPLEMENTATION

vex_ctx vex_init(vex_init_info init_info) {
	vex_ctx ctx = { 0 };
	ctx.name = _vex_strdup(init_info.name);
	ctx.help_msg = NULL;
	ctx.status_msg = NULL;
	ctx.description = _vex_strdup(init_info.description);
	ctx.version = _vex_strdup(init_info.version);
	ctx.arg_desc = NULL;
	ctx.num_arg_desc = 0;
	ctx.capacity_arg_desc = 0;
	ctx.arg_token = NULL;
	ctx.num_arg_token = 0;
	ctx.capacity_arg_token = 0;
	ctx.status = VEX_STATUS_OK;

	// Validate
	if (!ctx.name || !ctx.description || !ctx.version) {
		_vex_set_status(&ctx, VEX_STATUS_BAD_ALLOC, NULL);
		return ctx;
	}

	// Add default arguments
	vex_arg_desc arg_help_flag = { 0 };
	arg_help_flag.arg_type = VEX_ARG_TYPE_FLAG;
	arg_help_flag.long_name = "help";
	arg_help_flag.short_name = 'h';
	arg_help_flag.description = "Print this help message";
	arg_help_flag.max_count = 0;
	vex_add_arg(&ctx, arg_help_flag);

	vex_arg_desc arg_ver_flag = { 0 };
	arg_ver_flag.arg_type = VEX_ARG_TYPE_FLAG;
	arg_ver_flag.long_name = "version";
	arg_ver_flag.short_name = 'v';
	arg_ver_flag.description = "Print the version string";
	arg_ver_flag.max_count = 0;
	vex_add_arg(&ctx, arg_ver_flag);

	return ctx;
}

bool vex_add_arg(vex_ctx* ctx, vex_arg_desc desc) {
	// Validate arg
	if (!isalpha(desc.short_name)) {
		_vex_set_status(ctx, VEX_STATUS_BAD_VALUE, "Invalid short arg name: %c", desc.short_name);
		return false;
	}
	if (desc.short_name == '\0' && !desc.long_name) {
		_vex_set_status(ctx, VEX_STATUS_BAD_VALUE, "No arg name given");
		return false;
	}

	// Look for duplicates
	for (int i = 0; i < ctx->num_arg_desc; ++i) {
		if (ctx->arg_desc[i].short_name == desc.short_name ||
			strcmp(ctx->arg_desc[i].long_name, desc.long_name) == 0) {
			if (desc.short_name == '\0') {
				_vex_set_status(ctx, VEX_STATUS_BAD_VALUE, "Duplicate arguments: --%s", desc.long_name);
			}
			else {
				_vex_set_status(ctx, VEX_STATUS_BAD_VALUE, "Duplicate arguments: -%c", desc.short_name);
			}
			return false;
		}
	}

	// Resize arg descriptor buffer if needed
	while (ctx->num_arg_desc >= ctx->capacity_arg_desc) {
		int new_capacity = ctx->capacity_arg_desc * 2;
		new_capacity += (new_capacity == 0);
		vex_arg_desc* temp = CPPCAST(vex_arg_desc*)VEX_REALLOC(ctx->arg_desc, new_capacity * sizeof(*temp));
		if (!temp) {
			_vex_set_status(ctx, VEX_STATUS_BAD_ALLOC, NULL);
			return false;
		}
		memset(&temp[ctx->capacity_arg_desc], 0, (new_capacity - ctx->capacity_arg_desc) * sizeof(*temp));
		ctx->arg_desc = temp;
		ctx->capacity_arg_desc = new_capacity;
	}

	// Copy to description buffer
	ctx->arg_desc[ctx->num_arg_desc].arg_type = desc.arg_type;
	ctx->arg_desc[ctx->num_arg_desc].short_name = desc.short_name;
	ctx->arg_desc[ctx->num_arg_desc].long_name = _vex_strdup(desc.long_name);
	ctx->arg_desc[ctx->num_arg_desc].description = _vex_strdup(desc.description);
	ctx->arg_desc[ctx->num_arg_desc].max_count = desc.max_count;
	ctx->num_arg_desc++;
	if (ctx->help_msg) VEX_FREE(ctx->help_msg);
	ctx->help_msg = NULL;
	return true;
}

bool vex_parse(vex_ctx* ctx, int argc, char** argv) {
	// Clear any existing parsing results
	if (ctx->arg_token) VEX_FREE(ctx->arg_token);
	ctx->num_arg_token = 0;
	ctx->capacity_arg_token = 0;

	// Parse arguments
	int last_desc = -1;
	int last_token = -1;
	int token_count = 0;
	bool parse_options = true;
	for (int a = 1; a < argc; ++a) {
		char* arg = argv[a];
		if (!arg) continue;

		// Disable further option parsing
		if (strcmp(arg, "--") == 0) {
			parse_options = false;
			continue;
		}

		// Parse options
		if (parse_options && arg[0] == '-') {
			last_desc = -1;
			last_token = -1;
			token_count = 0;
			if (arg[1] == '-') {
				vex_arg_token token = { 0 };

				// Long option
				for (int d = 0; d < ctx->num_arg_desc; ++d) {
					size_t span = strcspn(&arg[2], "=");
					if (strncmp(&arg[2], ctx->arg_desc[d].long_name, span) == 0) {
						token.short_name = ctx->arg_desc[d].short_name;
						token.long_name = _vex_strdup(ctx->arg_desc[d].long_name);
						token.arg_type = ctx->arg_desc[d].arg_type;
						last_desc = d;
						break;
					}
				}

				// Check for unknown options
				if (!token.long_name) {
					_vex_set_status(ctx, VEX_STATUS_UNKNOWN_ARG, "Unknown option: %s", arg);
					return false;
				}

				// Check for value
				if (token.arg_type != VEX_ARG_TYPE_FLAG) {
					const char* pch = strchr(arg, '=');
					if (pch) {
						vex_value value = { 0 };
						switch (token.arg_type) {
						case VEX_ARG_TYPE_INT: value.int_arg = atoi(pch + 1); break;
						case VEX_ARG_TYPE_DUB: value.dub_arg = atof(pch + 1); break;
						case VEX_ARG_TYPE_STR: value.str_arg = _vex_strdup(pch + 1); break;
						}
						if (!_vec_token_add_value(ctx, &token, value)) return false;
					}
				}

				// Save to buffer
				if (!_vex_add_token(ctx, token)) return false;
				last_token = ctx->num_arg_token - 1;
				token_count++;
			}
			else {
				// Short option
				for (const char* c = &arg[1]; *c != '\0'; ++c) {
					vex_arg_token token = { 0 };

					// Check for flag name
					for (int d = 0; d < ctx->num_arg_desc; ++d) {
						assert(ctx->arg_desc);
						if (*c == ctx->arg_desc[d].short_name) {
							token.short_name = ctx->arg_desc[d].short_name;
							token.long_name = _vex_strdup(ctx->arg_desc[d].long_name);
							token.arg_type = ctx->arg_desc[d].arg_type;
							last_desc = d;
							break;
						}
					}

					// Check for unknown options
					if (token.short_name == '\0') {
						// An unknown character following a short option may not necessarily be an error; it could be the first
						// character of a value for that option (e.g. -ifile.txt)
						if (last_token >= 0 && ctx->arg_token[last_token].short_name != '\0' && ctx->arg_token[last_token].arg_type != VEX_ARG_TYPE_FLAG) {
							// Check for value
							vex_value value = { 0 };
							vex_arg_token* token = &ctx->arg_token[last_token];
							switch (token->arg_type) {
							case VEX_ARG_TYPE_INT: value.int_arg = atoi(c); break;
							case VEX_ARG_TYPE_DUB: value.dub_arg = atof(c); break;
							case VEX_ARG_TYPE_STR: value.str_arg = _vex_strdup(c); break;
							}
							if (!_vec_token_add_value(ctx, token, value)) return false;
							break;
						}
						else {
							_vex_set_status(ctx, VEX_STATUS_UNKNOWN_ARG, "Unknown option: -%c", *c);
							return false;
						}
					}
					else {
						// Save to buffer
						if (!_vex_add_token(ctx, token)) return false;
						last_token = ctx->num_arg_token - 1;
						token_count++;
					}
				}
			}
		}
		else {
			// Determine type
			int type = VEX_ARG_TYPE_UNKNOWN;
			for (const char* c = arg; *c != '\0'; ++c) {
				if (!isdigit(*c)) {
					if (*c == '.') {
						type = VEX_ARG_TYPE_DUB;
					}
					else {
						type = VEX_ARG_TYPE_STR;
						break;
					}
				}
				else if (type != VEX_ARG_TYPE_DUB) {
					type = VEX_ARG_TYPE_INT;
				}
			}

			bool group_with_last_token = false;
			if (last_token >= 0) {
				vex_arg_desc* desc = &ctx->arg_desc[last_desc];
				vex_arg_token* token = &ctx->arg_token[last_token];
				if (desc->max_count < 0 || token->arg_count < desc->max_count) group_with_last_token = true;
			}
			if (parse_options && group_with_last_token) {
				// Add to last parsed option
				vex_value value = { 0 };
				vex_arg_token* token = &ctx->arg_token[last_token];
				if (token->arg_type != type) {
					_vex_set_status(ctx, VEX_STATUS_BAD_VALUE, "Unexpected value");
					return false;
				}
				switch (type) {
				case VEX_ARG_TYPE_INT: value.int_arg = atoi(arg); break;
				case VEX_ARG_TYPE_DUB: value.dub_arg = atof(arg); break;
				case VEX_ARG_TYPE_STR: value.str_arg = _vex_strdup(arg); break;
				}
				if (!_vec_token_add_value(ctx, token, value)) return false;
			}
			else {
				// Add as a seperate token
				vex_value value = { 0 };
				vex_arg_token token = { 0 };
				token.arg_type = type;
				switch (type) {
				case VEX_ARG_TYPE_INT: value.int_arg = atoi(arg); break;
				case VEX_ARG_TYPE_DUB: value.dub_arg = atof(arg); break;
				case VEX_ARG_TYPE_STR: value.str_arg = _vex_strdup(arg); break;
				}
				if (!_vec_token_add_value(ctx, &token, value)) return false;
				if (!_vex_add_token(ctx, token)) return false;
				last_token = -1;
				last_desc = -1;
			}
		}
	}
	return true;
}

int vex_token_count(vex_ctx* ctx) {
	return ctx->num_arg_token;
}

vex_arg_token* vex_get_token(vex_ctx* ctx, int num) {
	if (num < 0 || num >= ctx->num_arg_token) return NULL;
	return &ctx->arg_token[num];
}

bool vex_arg_found(vex_ctx* ctx, const char* name) {
	if (!name) return false;
	for (int i = 0; i < vex_token_count(ctx); ++i) {
		vex_arg_token* token = vex_get_token(ctx, i);
		if (strlen(name) == 1 && name[0] == token->short_name) return true;
		else if (strlen(name) > 1 && strcmp(name, token->long_name) == 0) return true;
	}
	return false;
}

void vex_free(vex_ctx* ctx) {
	if (ctx->arg_desc) {
		for (int i = 0; i < ctx->num_arg_desc; ++i) {
			assert(ctx->arg_desc);
			vex_arg_desc desc = ctx->arg_desc[i];
			VEX_FREE(desc.long_name);
		}
		VEX_FREE(ctx->arg_desc);
	}
	if (ctx->arg_token) {
		for (int i = 0; i < ctx->num_arg_token; ++i) {
			assert(ctx->arg_token);
			vex_arg_token token = ctx->arg_token[i];
			VEX_FREE(token.long_name);
			if (token.arg_type == VEX_ARG_TYPE_STR) {
				for (int j = 0; j < token.arg_count; ++j) {
					VEX_FREE(token.arg[j].str_arg);
				}
			}
			VEX_FREE(token.arg);
		}
		VEX_FREE(ctx->arg_token);
	}
	if (ctx->status_msg) VEX_FREE(ctx->status_msg);
	if (ctx->help_msg) VEX_FREE(ctx->help_msg);
	ctx->status_msg = NULL;
	ctx->help_msg = NULL;
	VEX_FREE(ctx->description);
	VEX_FREE(ctx->version);
	VEX_FREE(ctx->name);
}

const char* vex_get_version(vex_ctx* ctx) {
	return ctx->version;
}

const char* vex_get_help(vex_ctx* ctx) {
	if (ctx->help_msg) return ctx->help_msg;

	// Initial allocation
	size_t buffer_len = strlen(ctx->name) + 32;
	if (ctx->description) buffer_len += strlen(ctx->description);
	size_t max_arg_len = 0;
	for (int i = 0; i < ctx->num_arg_desc; ++i) {
		size_t arg_len = 16;
		if (ctx->arg_desc[i].long_name) arg_len += strlen(ctx->arg_desc[i].long_name);
		max_arg_len = (arg_len > max_arg_len) ? arg_len : max_arg_len;
		if (ctx->arg_desc[i].description) buffer_len += strlen(ctx->arg_desc[i].description);
	}
	buffer_len += (2 * ctx->num_arg_desc * max_arg_len) + 1;
	char* buffer = CPPCAST(char*)VEX_MALLOC(buffer_len);
	if (!buffer) return NULL;
	snprintf(buffer, buffer_len, "Usage: %s", ctx->name);

	// Add args to usage
	for (int i = 0; i < ctx->num_arg_desc; ++i) {
		vex_arg_desc* desc = &ctx->arg_desc[i];
		strcat_s(buffer, buffer_len, " [");
		if (desc->short_name != '\0') {
			strcat_s(buffer, buffer_len, "-");
			strncat_s(buffer, buffer_len, &desc->short_name, 1);
			if (desc->long_name) strcat_s(buffer, buffer_len, "/");
		}
		if (desc->long_name) {
			strcat_s(buffer, buffer_len, "--");
			strcat_s(buffer, buffer_len, desc->long_name);
		}
		strcat_s(buffer, buffer_len, "] ");
		if (desc->arg_type != VEX_ARG_TYPE_FLAG && desc->max_count != 0) strcat_s(buffer, buffer_len, "... ");
	}
	strcat_s(buffer, buffer_len, "\n\n");

	// Add description
	if (ctx->description) {
		strcat_s(buffer, buffer_len, "Description:\n");
		strcat_s(buffer, buffer_len, ctx->description);
		strcat_s(buffer, buffer_len, "\n\n");
	}

	// Add arguments
	strcat_s(buffer, buffer_len, "Arguments:\n");
	for (int i = 0; i < ctx->num_arg_desc; ++i) {
		// Argument name
		size_t buffer_len_curr = strlen(buffer);
		size_t arg_len = 1;
		vex_arg_desc* desc = &ctx->arg_desc[i];
		strcat_s(buffer, buffer_len, " ");
		if (desc->short_name != '\0') {
			strcat_s(buffer, buffer_len, "-");
			strncat_s(buffer, buffer_len, &desc->short_name, 1);
			arg_len += 2;
			if (desc->long_name) {
				strcat_s(buffer, buffer_len, ", ");
				arg_len += 2;
			}
		}
		if (desc->long_name) {
			strcat_s(buffer, buffer_len, "--");
			strcat_s(buffer, buffer_len, desc->long_name);
			arg_len += 2 + strlen(desc->long_name);
		}

		// Padding
		while (arg_len < max_arg_len + 1) {
			buffer[buffer_len_curr + arg_len] = ' ';
			arg_len++;
			buffer[buffer_len_curr + arg_len] = '\0';
		}

		// Description
		if (desc->description) strcat_s(buffer, buffer_len, desc->description);
		strcat_s(buffer, buffer_len, "\n");
	}
	ctx->help_msg = buffer;
	return ctx->help_msg;
}

char* _vex_strdup(const char* str) {
	if (!str) return NULL;
	size_t len = strlen(str);
	char* dst = CPPCAST(char*)VEX_MALLOC(len + 1);
	if (!dst) return NULL;
	memcpy(dst, str, len + 1);
	return dst;
}

bool _vex_add_token(vex_ctx* ctx, vex_arg_token token) {
	// Resize arg token buffer if needed
	while (ctx->num_arg_token >= ctx->capacity_arg_token) {
		int new_capacity = ctx->capacity_arg_token * 2;
		new_capacity += (new_capacity == 0);
		vex_arg_token* temp = CPPCAST(vex_arg_token*)VEX_REALLOC(ctx->arg_token, new_capacity * sizeof(*temp));
		if (!temp) {
			_vex_set_status(ctx, VEX_STATUS_BAD_ALLOC, NULL);
			return false;
		}
		memset(&temp[ctx->capacity_arg_token], 0, (new_capacity - ctx->capacity_arg_token) * sizeof(*temp));
		ctx->arg_token = temp;
		ctx->capacity_arg_token = new_capacity;
	}

	// Save to buffer
	ctx->arg_token[ctx->num_arg_token++] = token;
	return true;
}

void _vex_set_status(vex_ctx* ctx, int status, char* fmt, ...) {
	ctx->status = status;
	if (status != VEX_STATUS_OK && status != VEX_STATUS_BAD_ALLOC && fmt) {
		if (ctx->status_msg) VEX_FREE(ctx->status_msg);
		ctx->status_msg = CPPCAST(char*)VEX_MALLOC(256);
		va_list args;
		va_start(args, fmt);
		vsnprintf(ctx->status_msg, 256, fmt, args);
		va_end(args);
	}
	else {
		if (ctx->status_msg) VEX_FREE(ctx->status_msg);
		ctx->status_msg = NULL;
	}
}

bool _vec_token_add_value(vex_ctx* ctx, vex_arg_token* token, vex_value value) {
	// Resize token value buffer
	vex_value* temp = CPPCAST(vex_value*)VEX_REALLOC(token->arg, (token->arg_count + 1) * sizeof(*token->arg));
	if (!temp) {
		_vex_set_status(ctx, VEX_STATUS_BAD_ALLOC, NULL);
		return false;
	}
	token->arg = temp;
	token->arg[token->arg_count++] = value;
	return true;
}

#endif

#ifdef __cplusplus
}
#endif

#endif // VEX_H
