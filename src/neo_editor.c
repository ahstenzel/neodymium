#include "neo_editor.h"
#include "neo_common.h"
#include "neo_menu.h"

static bool _neo_edit_row_valid(neo_edit_row_t* row) {
	return (row && row->page);
}

static bool _neo_edit_page_valid(neo_edit_page_t* page) {
	return (page && page->context && page->rows && page->max_rows >= page->num_rows);
}

static bool _neo_edit_ctx_valid(neo_edit_ctx_t* context) {
	return (
		context && context->pages &&
		context->max_pages && context->num_pages &&
		context->state != NSTATE_INVALID
	);
}

static bool _neo_edit_page_check_resize(neo_edit_page_t* page, size_t len) {
	assert(page && page->rows);
	if (len == 0) { return true; }
	while (page->max_rows == 0 || page->num_rows + len >= page->max_rows) {
		size_t new_capacity = page->max_rows * 2;
		if (new_capacity == 0) { new_capacity = NEO_EDIT_PAGE_DEFAULT_CAPACITY; }
		neo_edit_row_t* new_data = NEO_REALLOC(page->rows, sizeof(*(page->rows)) * new_capacity);
		if (!new_data) { return false; }
		page->rows = new_data;
		page->max_rows = new_capacity;
	}
	return true;
}

static int _neo_edit_ctx_check_page_resize(neo_edit_ctx_t* context, size_t len) {
	assert(context && context->pages);
	if (len == 0) { return 0; }
	while(context->max_pages == 0 || context->num_pages + len >= context->max_pages) {
		if (context->max_pages >= NEO_EDIT_MAX_OPEN_FILES) { return -1; }

		size_t new_capacity = context->max_pages * 2;
		if (new_capacity == 0) { new_capacity = NEO_EDIT_CTX_DEFAULT_PAGE_CAPACITY; }
		if (new_capacity > NEO_EDIT_MAX_OPEN_FILES) { new_capacity = NEO_EDIT_MAX_OPEN_FILES; }
		neo_edit_page_t* new_data = NEO_REALLOC(context->pages, sizeof(*(context->pages)) * new_capacity);
		if (!new_data) { return -2; }
		context->pages = new_data;
		context->max_pages = new_capacity;
	}
	return 0;
}

bool neo_edit_row_init(neo_edit_row_t* row, neo_edit_page_t* page) {
	// Validate row
	NEO_CLEAR_ERROR;
	if (!row) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid row");
		return false; 
	}

	// Initialize strings
	if (!string_init(&row->content) || !string_init(&row->rcontent)) {
		return false;
	}
	row->dirty = false;
	row->page = page;
	return true;
}

void neo_edit_row_clear(neo_edit_row_t* row) {
	if (!row) { return; }
	string_clear(&row->content);
	string_clear(&row->rcontent);
	row->dirty = false;
	row->page = NULL;
}

bool neo_edit_row_update(neo_edit_row_t* row) {
	// Validate row
	NEO_CLEAR_ERROR;
	if (!_neo_edit_row_valid(row)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid row");
		return false; 
	}
	if (!row->dirty) { return true; }

	// Get tab settings
	neo_edit_page_t* page = row->page;
	neo_edit_ctx_t* ctx = page->context;
	neo_settings_val_t tab_length_val;
	if (!neo_settings_get(&ctx->settings, "editor.tab_length", &tab_length_val)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_SETTING, "Failed to get editor.tab_length");
		return false;
	}
	size_t tab_length = tab_length_val.val_uint;
	if (!tab_length) { tab_length++; }

	// Iterate through string
	if (!string_erase_all(&row->rcontent)) {
		return false;
	}
	bool ret = true;
	for(size_t j = 0; j < row->content.length; ++j) {
		char c = string_at(&row->content, j);
		if (c == '\t') {
			if (!string_push_back(&row->rcontent, ' ')) { 
				ret = false;
				break;
			}
			while(row->rcontent.length % tab_length != 0) {
				if (!string_push_back(&row->rcontent, ' ')) {
					ret = false;
					break;
				}
			}
		}
		else {
			if (!string_push_back(&row->rcontent, c)) {
				ret = false;
				break;
			}
		}
	}
	row->dirty = false;
	return ret;
}

size_t neo_edit_row_cursor_update(neo_edit_row_t* row, size_t cx) {
	// Validate row
	NEO_CLEAR_ERROR;
	if (!_neo_edit_row_valid(row)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid row");
		return SIZE_MAX; 
	}

	// Get tab settings
	neo_edit_page_t* page = row->page;
	neo_edit_ctx_t* ctx = page->context;
	neo_settings_val_t tab_length_val;
	if (!neo_settings_get(&ctx->settings, "editor.tab_length", &tab_length_val)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_SETTING, "Failed to get editor.tab_length");
		return SIZE_MAX;
	}
	size_t tab_length = tab_length_val.val_uint;
	if (!tab_length) { tab_length++; }

	// Render cursor position
	size_t rx = 0;
	for(size_t i = 0; i < cx; ++i) {
		if (string_at(&row->content, i) == '\t') {
			rx += (tab_length - 1) - (rx % tab_length);
		}
		rx++;
	}
	return rx;
}

size_t neo_edit_row_length(neo_edit_row_t *row) {
	// Validate row
	NEO_CLEAR_ERROR;
	if (!_neo_edit_row_valid(row)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid row");
		return SIZE_MAX; 
	}
	return row->content.length;
}

bool neo_edit_page_init(neo_edit_page_t* page, neo_edit_ctx_t* context) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!page) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return false; 
	}

	// Initialize string
	if (!string_init(&page->filename)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate filename string");
		return false;
	}

	// Initialize memory
	size_t len = sizeof(*page->rows) * NEO_EDIT_PAGE_DEFAULT_CAPACITY;
	page->rows = NEO_MALLOC(len);
	if (!page->rows) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate row buffer");
		string_clear(&page->filename);
		return false;
	}
	memset(page->rows, 0, len);
	page->context = context;
	page->num_rows = 0;
	page->max_rows = NEO_EDIT_PAGE_DEFAULT_CAPACITY;
	page->num_cols = 0;
	page->row_off = 0;
	page->col_off = 0;
	page->cursor_x = 0;
	page->cursor_y = 0;
	page->rcursor_x = 0;
	page->rcursor_y = 0;
	page->flags = 0;
	return true;
}

void neo_edit_page_clear(neo_edit_page_t* page) {
	if (!page) { return; }
	string_clear(&page->filename);
	if (page->rows) {
		for(size_t i = 0; i < page->num_rows; ++i) {
			neo_edit_row_clear(&page->rows[i]);
		}
	}
	NEO_FREE(page->rows);
	page->rows = NULL;
	page->num_rows = 0;
	page->max_rows = 0;
	page->num_cols = 0;
	page->row_off = 0;
	page->col_off = 0;
	page->cursor_x = 0;
	page->cursor_y = 0;
	page->rcursor_x = 0;
	page->rcursor_y = 0;
	page->flags = 0;
}

bool neo_edit_page_update(neo_edit_page_t* page) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return false; 
	}

	// Iterate through rows
	for(size_t i = 0; i < page->num_rows; ++i) {
		if (!neo_edit_row_update(&page->rows[i])) { 
			return false; 
		}
	}
	return true;
}

neo_edit_row_t* neo_edit_page_get_row(neo_edit_page_t* page, int at) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return NULL; 
	}
	if (page->num_rows == 0) {
		return NULL;
	}

	// Calculate index
	size_t idx = 0;
	if (at < 0) { idx = page->num_rows - 1; }
	else if (at < page->num_rows) { idx = (size_t)at; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return NULL;
	}
	return &page->rows[idx];
}

neo_edit_row_t* neo_edit_page_insert_row(neo_edit_page_t* page, int at) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return NULL; 
	}
	if (!_neo_edit_page_check_resize(page, 1)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize row buffer");
		return NULL;
	}

	// Calculate index
	size_t idx = 0;
	if (at < 0) { idx = page->num_rows; }
	else if (at <= page->num_rows) { idx = (size_t)at; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return NULL;
	}

	// Initialize row
	size_t n = sizeof(*(page->rows));
	neo_edit_row_t new_row = {0};
	if (!neo_edit_row_init(&new_row, page)) {
		return NULL;
	}

	// Shift end of buffer forward
	memmove(page->rows + (n * (idx + 1)), page->rows + (n * idx), n * (page->num_rows - idx));
	memcpy(&page->rows[idx], &new_row, n);
	page->num_rows++;
	return &page->rows[idx];
}

void neo_edit_page_remove_row(neo_edit_page_t* page, int at) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return; 
	}

	// Calculate index
	size_t idx = 0;
	if (at < 0) { idx = page->num_rows; }
	else if (at <= page->num_rows) { idx = (size_t)at; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return;
	}

	// Shift end of group backward
	size_t n = sizeof(*(page->rows));
	memmove(page->rows + (n * idx), page->rows + (n * (idx + 1)), n * (page->num_rows - idx - 1));
	page->num_rows--;
}

void neo_edit_page_set_cursor_row(neo_edit_page_t *page, int row) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return; 
	}

	// Calculate index
	size_t idx = 0;
	if (row < 0) { idx = page->num_rows; }
	else if (row <= page->num_rows) { idx = (size_t)row; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return;
	}
	page->cursor_y = idx;
}

void neo_edit_page_set_cursor_col(neo_edit_page_t *page, int col) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return; 
	}

	// Get current row
	neo_edit_row_t* row = &page->rows[page->cursor_y];

	// Calculate index
	size_t idx = 0;
	if (col < 0) { idx = row->content.length; }
	else if (col <= row->content.length) { idx = (size_t)col; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return;
	}
	page->cursor_x = idx;
}

void neo_edit_page_move_cursor(neo_edit_page_t *page, neo_dir_t dir, size_t num) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return; 
	}

	// Move cursor one step at a time
	for(size_t i = 0; i < num; ++i) {
		// Get current row
		neo_edit_row_t* curr_row = &page->rows[page->cursor_y];
		neo_edit_row_t* next_row = curr_row;

		// Move cursor
		switch(dir) {
			case NDIR_UP: 
				if (page->cursor_y > 0) {
					next_row = &page->rows[--(page->cursor_y)];
					if (page->cursor_x == curr_row->content.length ||
						page->cursor_x >= next_row->content.length) {
						page->cursor_x = next_row->content.length;
					}
				}
			break;
			case NDIR_DOWN:
				if (page->cursor_y < page->num_rows) {
					next_row = &page->rows[++(page->cursor_y)];
					if (page->cursor_x == curr_row->content.length ||
						page->cursor_x >= next_row->content.length) {
						page->cursor_x = next_row->content.length;
					}
				} 
			break;
			case NDIR_LEFT: 
				if (page->cursor_x > 0) { page->cursor_x--; }
				else if (page->cursor_y > 0) {
					next_row = &page->rows[--(page->cursor_y)];
					page->cursor_x = next_row->content.length;
				}
			break;
			case NDIR_RIGHT: 
				if (page->cursor_x < curr_row->content.length) { page->cursor_x++; }
				else if (page->cursor_y < page->num_rows) {
					next_row = &page->rows[++(page->cursor_y)];
					page->cursor_x = 0;
				}
			break;
		}
		curr_row = next_row;
	}
}

void neo_edit_page_set_filename(neo_edit_page_t* page, string_t filename) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return; 
	}

	string_clear(&page->filename);
	page->filename = filename;
}

size_t neo_edit_page_get_index(neo_edit_page_t* page) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return SIZE_MAX; 
	}

	neo_edit_ctx_t* context = page->context;
	for(size_t i = 0; i < context->num_pages; ++i) {
		if (&context->pages[i] == page) { return i; }
	}
	return SIZE_MAX;
}

bool neo_edit_ctx_init(neo_edit_ctx_t* context) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!context) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return false;
	}

	// Initialize memory
	size_t page_len = sizeof(*(context->pages)) * NEO_EDIT_CTX_DEFAULT_PAGE_CAPACITY;
	context->pages = NEO_MALLOC(page_len);
	if (!context->pages) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate page buffer");
		return false;
	}
	memset(context->pages, 0, page_len);

	// Load settings
	if (!neo_settings_init(&context->settings)) {
		return false;
	}
	context->num_pages = 0;
	context->max_pages = NEO_EDIT_CTX_DEFAULT_PAGE_CAPACITY;
	context->curr_page = 0;
	context->state = NSTATE_OPEN;
	return true;
}

void neo_edit_ctx_clear(neo_edit_ctx_t* context) {
	if (!context) { return; }
	neo_settings_clear(&context->settings);
	if (context->pages) {
		for(size_t i = 0; i < context->num_pages; ++i) {
			neo_edit_page_clear(&context->pages[i]);
		}
	}
	NEO_FREE(context->pages);
	context->pages = NULL;
	context->num_pages = 0;
	context->max_pages = 0;
	context->curr_page = 0;
	context->state = NSTATE_INVALID;
}

bool neo_edit_ctx_update(neo_edit_ctx_t* context) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!_neo_edit_ctx_valid(context)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return false; 
	}

	// Iterate through pages
	for(size_t i = 0; i < context->num_pages; ++i) {
		if (!neo_edit_page_update(&context->pages[i])) {
			return false;
		}
	}
	return true;
}
