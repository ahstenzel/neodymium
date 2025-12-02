#include "neo_editor.h"
#include "neo_actions.h"

static bool _neo_edit_row_valid(neo_edit_row_t* row) {
	return (row && row->page);
}

static bool _neo_edit_page_valid(neo_edit_page_t* page) {
	return (page && page->context && page->rows && page->max_rows >= page->num_rows);
}

static bool _neo_edit_ctx_valid(neo_edit_ctx_t* context) {
	return (
		context && context->pages &&
		context->max_pages >= context->num_pages &&
		context->state != NSTATE_INVALID
	);
}

static bool _neo_edit_page_check_row_resize(neo_edit_page_t* page, size_t len) {
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
	row->dirty = true;
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
		NEO_CHAR_T c = string_at(&row->content, j);
		if (c == '\t') {
			if (!string_push_back(&row->rcontent, ' ', 1)) { 
				ret = false;
				break;
			}
			while(row->rcontent.length % tab_length != 0) {
				if (!string_push_back(&row->rcontent, ' ', 1)) {
					ret = false;
					break;
				}
			}
		}
		else {
			if (!string_push_back(&row->rcontent, c, 1)) {
				ret = false;
				break;
			}
		}
	}
	row->dirty = false;
	return ret;
}

bool neo_edit_row_insert_text(neo_edit_row_t* row, int position, const NEO_CHAR_T* insert, size_t len) {
	// Validate row
	NEO_CLEAR_ERROR;
	if (!_neo_edit_row_valid(row)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid row");
		return false; 
	}

	// Insert text
	size_t idx = 0;
	if (position == -1) { idx = row->content.length; }
	else if (position >= 0 && position <= (int)row->content.length) { idx = (size_t)position; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return false;
	}
	if (!string_insert(&row->content, idx, insert, len)) {
		return false;
	}
	PAGE_FLAG_SET(row->page, NPAGE_FLAG_DIRTY);
	row->dirty = true;
	return true;
}

bool neo_edit_row_set_text(neo_edit_row_t* row, const NEO_CHAR_T* insert, size_t len) {
	// Validate row
	NEO_CLEAR_ERROR;
	if (!_neo_edit_row_valid(row)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid row");
		return false; 
	}

	// Erase existing text
	if (!string_erase_all(&row->content)) {
		return false;
	}
	if (!string_set(&row->content, 0, insert, len)) {
		return false;
	}
	PAGE_FLAG_SET(row->page, NPAGE_FLAG_DIRTY);
	row->dirty = true;
	return true;
}

bool neo_edit_row_erase_text(neo_edit_row_t* row, size_t position, size_t len) {
	// Validate row
	NEO_CLEAR_ERROR;
	if (!_neo_edit_row_valid(row)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid row");
		return false; 
	}

	// Remove text
	if (!string_erase(&row->content, position, len)) {
		return false;
	}
	PAGE_FLAG_SET(row->page, NPAGE_FLAG_DIRTY);
	row->dirty = true;
	return true;
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
	if (!string_init(&page->filename) || !string_init(&page->filename_base)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate filename string");
		return false;
	}

	// Initialize memory
	size_t len = sizeof(*page->rows) * NEO_EDIT_PAGE_DEFAULT_CAPACITY;
	page->rows = NEO_MALLOC(len);
	if (!page->rows) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate row buffer");
		string_clear(&page->filename);
		string_clear(&page->filename_base);
		return false;
	}
	memset(page->rows, 0, len);
	int screen_rows, screen_cols;
	getmaxyx(stdscr, screen_rows, screen_cols);
	page->window_cols = MAX(screen_cols, 4);
	page->window_rows = MAX(screen_rows - (NEO_SIZE_HEADER + NEO_SIZE_FOOTER), 1);
	page->nc_window = newwin(page->window_rows, page->window_cols, NEO_SIZE_HEADER, 0);
	if (!page->nc_window) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses window");
		string_clear(&page->filename);
		string_clear(&page->filename_base);
		return false;
	}
	page->nc_panel = new_panel(page->nc_window);
	if (!page->nc_panel) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses panel");
		string_clear(&page->filename);
		string_clear(&page->filename_base);
		return false;
	}
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
	string_clear(&page->filename_base);
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

	// Update window size
	if (_neo_flag_resized) {
		int screen_rows, screen_cols;
		getmaxyx(stdscr, screen_rows, screen_cols);
		page->window_cols = MAX(screen_cols, 4);
		page->window_rows = MAX(screen_rows - (NEO_SIZE_HEADER + NEO_SIZE_FOOTER), 1);
		wresize(page->nc_window, page->window_rows, page->window_cols);
	}

	// Iterate through rows
	for(size_t i = 0; i < page->num_rows; ++i) {
		neo_edit_row_t* row = &page->rows[i];
		row->page = page;
		if (!neo_edit_row_update(row)) { 
			return false; 
		}
		page->num_cols = MAX(page->num_cols, row->rcontent.length);
	}

	// Calculate rendered cursor position
	if (page->cursor_y < page->num_rows) {
		page->rcursor_x = neo_edit_row_cursor_update(PAGE_GET_CURR_ROW(page), page->cursor_x);
		page->rcursor_y = page->cursor_y;
	}
	else {
		page->rcursor_x = 0;
		page->rcursor_y = page->cursor_y;
	}

	// Calculate scroll offsets
	if ((int)page->cursor_y - NEO_SCROLL_MARGIN < (int)page->row_off) {
		page->row_off = MAX(0, (int)(page->cursor_y) - NEO_SCROLL_MARGIN);
	}
	if (page->cursor_y + NEO_SCROLL_MARGIN >= page->row_off + page->window_rows) {
		page->row_off = MIN(
			(page->num_rows + NEO_SCROLL_MARGIN + 1) - page->window_rows,
			(page->cursor_y - page->window_rows) + NEO_SCROLL_MARGIN + 1
		);
	}
	if ((int)page->rcursor_x - NEO_SCROLL_MARGIN < (int)page->col_off) {
		page->col_off = MAX(0, (int)(page->rcursor_x) - NEO_SCROLL_MARGIN);
	}
	if (page->rcursor_x + NEO_SCROLL_MARGIN >= page->col_off + page->window_cols) {
		neo_edit_row_t* row = PAGE_GET_CURR_ROW(page);
		if (!row) { page->col_off = 0; }
		else {
			page->col_off = MIN(
				((int)(row->rcontent.length) + NEO_SCROLL_MARGIN + 1) - page->window_cols,
				(page->rcursor_x - page->window_cols) + NEO_SCROLL_MARGIN + 1
			);
		}
	}

	return true;
}

bool neo_edit_page_draw(neo_edit_page_t* page) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return false; 
	}

	// Draw page contents
	for(size_t i = 0; i < page->window_rows; ++i) {
		wmove(page->nc_window, i, 0);
		neo_hvline(page->nc_window, ' ', page->window_cols);
		size_t row_idx = page->row_off + i;
		if (row_idx >= page->num_rows) { neo_waddch(page->nc_window, '~'); }
		else {
			neo_edit_row_t* row = &page->rows[row_idx];
			int len = CLAMP((int)row->rcontent.length - (int)page->col_off, 0, (int)page->window_cols);
			neo_waddnstr(page->nc_window, &row->rcontent.data[page->col_off], len);
		}
	}

	// Draw vertical scroll bar
	if (page->num_rows + NEO_SCROLL_MARGIN >= page->window_rows) {
		// Calculate bar size & offset
		float size_ratio = page->window_rows / (float)(page->num_rows + 1 + NEO_SCROLL_MARGIN);
		int bar_size = MAX(1, (int)(size_ratio * (page->window_rows - 2)));
		float offset_ratio = page->row_off / (float)((page->num_rows + 1 + NEO_SCROLL_MARGIN) - page->window_rows);
		int bar_offset = (int)(offset_ratio * (page->window_rows - 2 - bar_size));

		// Draw bar
		wmove(page->nc_window, 0, page->window_cols - 1);
		neo_waddch(page->nc_window, ACS_UARROW | A_REVERSE);
		wmove(page->nc_window, 1, page->window_cols - 1);
		neo_wvline(page->nc_window, ACS_VLINE, page->window_rows - 2);
		wmove(page->nc_window, 1 + bar_offset, page->window_cols - 1);
		neo_wvline(page->nc_window, ' ' | A_REVERSE, bar_size);
		wmove(page->nc_window, page->window_rows - 1, page->window_cols - 1);
		neo_waddch(page->nc_window, ACS_DARROW | A_REVERSE);
	}

	// Draw horizontal scroll bar
	if (page->num_cols + NEO_SCROLL_MARGIN >= page->window_cols) {
		// Calculate bar size & offset
		float size_ratio = page->window_cols / (float)(page->num_cols + 1 + NEO_SCROLL_MARGIN);
		int bar_size = MAX(1, (int)(size_ratio * (page->window_cols - 2)));
		float offset_ratio = page->col_off / (float)((page->num_cols + 1 + NEO_SCROLL_MARGIN) - page->window_cols);
		int bar_offset = (int)(offset_ratio * (page->window_cols - 2 - bar_size));

		// Draw bar
		wmove(page->nc_window, page->window_rows - 1, 0);
		neo_waddch(page->nc_window, ACS_LARROW | A_REVERSE);
		neo_hvline(page->nc_window, ACS_HLINE, page->window_cols - 2);
		wmove(page->nc_window, page->window_rows - 1, 1 + bar_offset);
		neo_hvline(page->nc_window, ' ' | A_REVERSE, bar_size);
		wmove(page->nc_window, page->window_rows - 1, page->window_cols - 1);
		neo_waddch(page->nc_window,
			((page->num_rows + NEO_SCROLL_MARGIN >= page->window_rows) ? 'x' : ACS_RARROW) | A_REVERSE
		);
	}
	return true;
}

neo_edit_row_t* neo_edit_page_get_row(neo_edit_page_t* page, int position) {
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
	if (position < 0) { idx = page->num_rows - 1; }
	else if (position < (int)page->num_rows) { idx = (size_t)position; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return NULL;
	}
	return &page->rows[idx];
}

neo_edit_row_t* neo_edit_page_insert_row(neo_edit_page_t* page, int position) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return NULL; 
	}
	if (!_neo_edit_page_check_row_resize(page, 1)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize row buffer");
		return NULL;
	}

	// Calculate index
	size_t idx = 0;
	if (position < 0) { idx = page->num_rows; }
	else if (position <= (int)page->num_rows) { idx = (size_t)position; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return NULL;
	}

	// Initialize row
	neo_edit_row_t new_row = { 0 };
	if (!neo_edit_row_init(&new_row, page)) {
		return NULL;
	}

	// Shift end of buffer forward
	size_t n = sizeof(*(page->rows));
	memmove(&page->rows[idx + 1], &page->rows[idx], n * (page->num_rows - idx));
	memcpy(&page->rows[idx], &new_row, n);
	PAGE_FLAG_SET(page, NPAGE_FLAG_DIRTY);
	page->num_rows++;
	return &page->rows[idx];
}

void neo_edit_page_remove_row(neo_edit_page_t* page, int position) {
	// Validate page
	NEO_CLEAR_ERROR;
	if (!_neo_edit_page_valid(page)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid page");
		return; 
	}

	// Calculate index
	size_t idx = 0;
	if (position < 0) { idx = page->num_rows - 1; }
	else if (position < (int)page->num_rows) { idx = (size_t)position; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return;
	}

	// Clear row
	neo_edit_row_clear(&page->rows[idx]);

	// Shift end of group backward
	size_t n = sizeof(*(page->rows));
	memmove(&page->rows[idx], &page->rows[idx + 1], n * (page->num_rows - idx - 1));
	PAGE_FLAG_SET(page, NPAGE_FLAG_DIRTY);
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
	else if (row <= (int)page->num_rows) { idx = (size_t)row; }
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
	else if (col <= (int)row->content.length) { idx = (size_t)col; }
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
					if ((curr_row->content.length > 0 && page->cursor_x == curr_row->content.length) ||
						page->cursor_x >= next_row->content.length) {
						page->cursor_x = next_row->content.length;
					}
				}
			break;
			case NDIR_DOWN:
				if (page->cursor_y < page->num_rows) {
					next_row = &page->rows[++(page->cursor_y)];
					if ((curr_row->content.length > 0 && page->cursor_x == curr_row->content.length) ||
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

	// Strip file to basename
	string_t filename_base = { 0 };
	if (!string_init(&filename_base)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initialize filename basename string");
		return;
	}
	size_t idx = 0;
	int pos = -1;
	do {
		pos = string_find_next_of(&filename, "/", 1, idx);
		if (pos >= 0) { idx = pos + 1; }
	} while(pos != -1);
	if (!string_substr(&filename, idx, filename.length - idx, &filename_base)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to trim filename to basename");
		string_clear(&filename_base);
		return;
	}

	// Replace current filename
	string_clear(&page->filename);
	page->filename = filename;
	string_clear(&page->filename_base);
	page->filename_base = filename_base;
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

	// Initialize members
	if (!neo_settings_init(&context->settings) || !neo_settings_load_defaults(&context->settings)) {
		return false;
	}
	if (!neo_menu_bar_init(&context->menu_bar)) {
		return false;
	}
	if (!string_init(&context->status_message)) {
		return false;
	}
	int screen_rows, screen_cols;
	getmaxyx(stdscr, screen_rows, screen_cols);
	context->window_cols = MAX(screen_cols, 4);
	context->window_rows = MAX(screen_rows - NEO_SIZE_MENU_BAR, NEO_SIZE_MENU_BAR + NEO_SIZE_FOOTER + 1);
	context->nc_window = newwin(context->window_rows, context->window_cols, NEO_SIZE_MENU_BAR, 0);
	if (!context->nc_window) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses window");
		return false;
	}
	context->nc_panel = new_panel(context->nc_window);
	if (!context->nc_panel) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses panel");
		return false;
	}
	context->status_timer = time(NULL);
	context->num_pages = 0;
	context->max_pages = NEO_EDIT_CTX_DEFAULT_PAGE_CAPACITY;
	context->curr_page = 0;
	context->state = NSTATE_OPEN;

	// Add menu bar entries
	neo_menu_group_t* menu_group_file = neo_menu_bar_insert_group(&context->menu_bar, -1, NEO_STR_CAST("File"), 0);
	neo_menu_group_insert_entry(menu_group_file, -1, NEO_STR_CAST("New File"), 'n', neo_cb_new_file);
	neo_menu_group_insert_entry(menu_group_file, -1, NEO_STR_CAST("Open File"), 'o', neo_cb_open_file);
	neo_menu_group_insert_seperator(menu_group_file, -1);
	neo_menu_group_insert_entry(menu_group_file, -1, NEO_STR_CAST("Save File"), 's', neo_cb_save_file);
	neo_menu_group_insert_entry(menu_group_file, -1, NEO_STR_CAST("Save File As"), 'b', neo_cb_save_file_as);
	neo_menu_group_insert_entry(menu_group_file, -1, NEO_STR_CAST("Save All Files"), 'e', neo_cb_save_all_file);
	neo_menu_group_insert_seperator(menu_group_file, -1);
	neo_menu_group_insert_entry(menu_group_file, -1, NEO_STR_CAST("Next Tab"), 't', neo_cb_next_page);
	neo_menu_group_insert_entry(menu_group_file, -1, NEO_STR_CAST("Prev Tab"), 'r', neo_cb_prev_page);
	neo_menu_group_insert_entry(menu_group_file, -1, NEO_STR_CAST("Close Tab"), 'w', neo_cb_close_page);
	neo_menu_group_insert_entry(menu_group_file, -1, NEO_STR_CAST("Quit"), 'q', neo_cb_quit);
	neo_menu_group_t* menu_group_edit = neo_menu_bar_insert_group(&context->menu_bar, -1, NEO_STR_CAST("Edit"), 0);
	neo_menu_group_insert_entry(menu_group_edit, -1, NEO_STR_CAST("Cut"), 'x', neo_cb_cut);
	neo_menu_group_insert_entry(menu_group_edit, -1, NEO_STR_CAST("Copy"), 'c', neo_cb_copy);
	neo_menu_group_insert_entry(menu_group_edit, -1, NEO_STR_CAST("Paste"), 'v', neo_cb_paste);
	neo_menu_group_insert_entry(menu_group_edit, -1, NEO_STR_CAST("Duplicate Line"), 'd', neo_cb_duplicate);
	neo_menu_group_insert_seperator(menu_group_edit, -1);
	neo_menu_group_insert_entry(menu_group_edit, -1, NEO_STR_CAST("Select All"), 'a', neo_cb_select_all);
	neo_menu_group_insert_seperator(menu_group_edit, -1);
	neo_menu_group_insert_entry(menu_group_edit, -1, NEO_STR_CAST("Undo"), 'z', neo_cb_undo);
	neo_menu_group_insert_entry(menu_group_edit, -1, NEO_STR_CAST("Redo"), 'y', neo_cb_redo);
	neo_menu_group_t* menu_group_help = neo_menu_bar_insert_group(&context->menu_bar, -1, NEO_STR_CAST("Help"), 0);
	neo_menu_group_insert_entry(menu_group_help, -1, NEO_STR_CAST("About"), 0, neo_cb_about);
	return true;
}

void neo_edit_ctx_clear(neo_edit_ctx_t* context) {
	if (!context) { return; }
	neo_settings_clear(&context->settings);
	neo_menu_bar_clear(&context->menu_bar);
	string_clear(&context->status_message);
	if (context->pages) {
		for(size_t i = 0; i < context->num_pages; ++i) {
			neo_edit_page_clear(&context->pages[i]);
		}
	}
	NEO_FREE(context->pages);
	context->pages = NULL;
	del_panel(context->nc_panel);
	delwin(context->nc_window);
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

	// Update window size
	if (_neo_flag_resized) {
		int screen_rows, screen_cols;
		getmaxyx(stdscr, screen_rows, screen_cols);
		context->window_cols = MAX(screen_cols, 4);
		context->window_rows = MAX(screen_rows - NEO_SIZE_MENU_BAR, NEO_SIZE_MENU_BAR + NEO_SIZE_FOOTER + 1);
		wresize(context->nc_window, context->window_rows, context->window_cols);
	}

	// Update pages
	for(size_t i = 0; i < context->num_pages; ++i) {
		neo_edit_page_t* page = &context->pages[i];
		page->context = context;
		if (!neo_edit_page_update(page)) {
			return false;
		}
	}

	// Update components
	neo_menu_bar_update(&context->menu_bar);

	_neo_flag_resized = false;
	return true;
}

bool neo_edit_ctx_draw(neo_edit_ctx_t *context) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!_neo_edit_ctx_valid(context)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return false; 
	}

	// Get all tab names
	bool exit = false;
	size_t curr_tab_pos = 0;
	string_t all_names = { 0 };
	if (!string_init(&all_names)) { return false; }
	for(size_t i = 0; i < context->num_pages; ++i) {
		neo_edit_page_t* page = &context->pages[i];

		// Mark start of tab name with special character
		if (i == context->curr_page) { 
			curr_tab_pos = all_names.length; 
			string_push_back(&all_names, '?', 1);
		}
		else {
			string_push_back(&all_names, '/', 1);
		}

		// Page flags
		if (PAGE_FLAG_ISSET(page, NPAGE_FLAG_DIRTY)) {
			if (!string_push_back(&all_names, '*', 1)) {
				exit = true;
				break;
			}
		}

		// Page name
		if (string_empty(&page->filename)) {
			if (!string_append(&all_names, NEO_STR_CAST("<New File>"), -1)) { 
				exit = true;
				break;
			}
		}
		else {
			string_t truncated_filename = { 0 };
			if (!string_init(&truncated_filename) ||
				!string_duplicate(&page->filename_base, &truncated_filename) ||
				string_truncate(&truncated_filename, (context->window_cols / 4), -1) < 0 ||
				!string_append(&all_names, truncated_filename.data, truncated_filename.length)) {
				string_clear(&truncated_filename);
				exit = true;
				break;
			}
			string_clear(&truncated_filename);
		}
		string_push_back(&all_names, '\\', 1);
		string_push_back(&all_names, ' ', 2);
	}
	if (exit) { 
		string_clear(&all_names);
		return false; 
	}

	// Check if page tabs need to be truncated
	size_t max_width = (context->window_cols - 2);
	size_t margin = max_width / 2;
	int tab_offset = 0;
	if (all_names.length > max_width) {
		// Try to put the currently selected tab in the middle of the bar
		tab_offset = (int)(curr_tab_pos) - margin;
		if (tab_offset < 0) { tab_offset = 0; }
		else { 
			tab_offset = string_find_next_of(&all_names, NEO_STR_CAST("?/"), 2, tab_offset); 
			if (tab_offset + max_width > all_names.length) { 
				tab_offset = all_names.length - max_width;
			}
		}
	}

	// Truncate page tabs
	string_t tab_string = { 0 };
	if (!string_init(&tab_string) ||
		!string_substr(&all_names, tab_offset, max_width, &tab_string)) {
		string_clear(&tab_string);
		string_clear(&all_names);
		return false;
	}

	// If a name got cut off, clear its closing bracket
	int open_offset = string_find_next_of(&tab_string, NEO_STR_CAST("?/"), 2, 0);
	int close_offset = string_find_next_of(&tab_string, NEO_STR_CAST("\\"), 1, 0);
	if (close_offset < open_offset && close_offset > -1) {
		string_set(&tab_string, close_offset, NEO_STR_CAST(" "), 1);
	}

	// Draw page tabs
	wmove(context->nc_window, 2, 0);
	neo_hvline(context->nc_window, ACS_HLINE, context->window_cols);
	wmove(context->nc_window, 0, 0);
	neo_hvline(context->nc_window, ' ', context->window_cols);
	wmove(context->nc_window, 1, 0);
	neo_hvline(context->nc_window, ' ', context->window_cols);
	wmove(context->nc_window, 1, 1);
	neo_waddnstr(context->nc_window, tab_string.data, tab_string.length);
	int offset = -1;
	do {
		offset = string_find_next_of(&tab_string, NEO_STR_CAST("?/"), 2, offset + 1);
		if (offset > -1) {
			if (string_at(&tab_string, offset) == '/') {
				wmove(context->nc_window, 1, offset + 1);
				neo_waddch(context->nc_window, ' ');
				offset = string_find_next_of(&tab_string, NEO_STR_CAST("\\"), 1, offset + 1);
				wmove(context->nc_window, 1, offset + 1);
				neo_waddch(context->nc_window, ' ');
			}
			else if (string_at(&tab_string, offset) == '?') {
				wmove(context->nc_window, 1, offset + 1);
				neo_waddch(context->nc_window, ACS_VLINE);
				int next_offset = string_find_next_of(&tab_string, NEO_STR_CAST("\\"), 1, offset + 1);
				int dist = next_offset - offset;
				wmove(context->nc_window, 1, next_offset + 1);
				neo_waddch(context->nc_window, ACS_VLINE);
				wmove(context->nc_window, 0, offset + 1);
				neo_hvline(context->nc_window, ACS_HLINE, dist);
				neo_waddch(context->nc_window, ACS_ULCORNER);
				wmove(context->nc_window, 0, next_offset + 1);
				neo_waddch(context->nc_window, ACS_URCORNER);
				wmove(context->nc_window, 2, offset + 1);
				neo_hvline(context->nc_window, ' ', dist);
				neo_waddch(context->nc_window, ACS_LRCORNER);
				wmove(context->nc_window, 2, next_offset + 1);
				neo_waddch(context->nc_window, ACS_LLCORNER);
				offset = next_offset;
			}
		}
	} while(offset > -1);

	// Draw arrows
	if (tab_offset > 0) {
		wmove(context->nc_window, 1, 0);
		neo_waddch(context->nc_window, ACS_LARROW | A_REVERSE);
	}
	if ((tab_offset + max_width) < all_names.length) {
		wmove(context->nc_window, 1, context->window_cols - 1);
		neo_waddch(context->nc_window, ACS_RARROW | A_REVERSE);
	}
	string_clear(&all_names);
	string_clear(&tab_string);

	// Draw page contents
	neo_edit_page_t* curr_page = EDITOR_GET_CURR_PAGE(context);
	if (curr_page) {
		if (!neo_edit_page_draw(curr_page)) {
			return false;
		}
	}

	// Draw status message
	wmove(context->nc_window, context->window_rows - NEO_SIZE_FOOTER, 0);
	neo_hvline(context->nc_window, ACS_HLINE, context->window_cols);
	wmove(context->nc_window, context->window_rows - NEO_SIZE_FOOTER + 1, 0);
	neo_hvline(context->nc_window, ' ', context->window_cols);
	neo_waddnstr(context->nc_window, context->status_message.data, context->status_message.length);

	// Draw cursor position
	if (curr_page) {
		wmove(context->nc_window, context->window_rows - NEO_SIZE_FOOTER + 1, context->window_cols - 16);
		wprintw(context->nc_window, "L:%d C:%d", (int)curr_page->rcursor_y, (int)curr_page->rcursor_x);
	}

	// Draw file bar
	if (!neo_menu_bar_draw(&context->menu_bar)) {
		return false;
	}

	// Set final cursor position
	if (curr_page) {
		wmove(curr_page->nc_window, (int)(curr_page->rcursor_y - curr_page->row_off), (int)(curr_page->rcursor_x - curr_page->col_off));
	}
	else {
		wmove(context->nc_window, 0, 0);
	}
	return true;
}

size_t neo_edit_ctx_open_page(neo_edit_ctx_t *context, char* filename) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!_neo_edit_ctx_valid(context)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return SIZE_MAX; 
	}
	int ret = _neo_edit_ctx_check_page_resize(context, 1);
	if (ret == -1) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Maximum number of open pages reached");
		return SIZE_MAX;
	}
	else if (ret == -2) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize page buffer");
		return SIZE_MAX;
	}

	// Initialize page
	size_t idx = context->num_pages;
	string_t filename_str = { 0 };
	NEO_CHAR_T* filename_full = NULL;
	neo_edit_page_t* curr_page = &context->pages[idx];
	if (!string_init(&filename_str)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initialize page name string");
		return SIZE_MAX;
	}
	if ((filename_full = ascii_to_string(filename, strlen(filename))) == NULL) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to convert page name string");
		return SIZE_MAX;
	}
	if (!string_set(&filename_str, 0, filename_full, neo_strlen(filename_full))) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to set page name string");
		return SIZE_MAX;
	}
	if (!neo_edit_page_init(curr_page, context)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initialize new page");
		return SIZE_MAX;
	}
	neo_edit_page_set_filename(curr_page, filename_str);
	NEO_FREE(filename_full);

	// Get file contents
	FILE* fp = fopen(filename, "r");
	if (fp) {
		NEO_CHAR_T* line = NULL;
		size_t n = 0;
		ssize_t len = -1;
		while((len = neo_getline(&line, &n, fp)) >= 0) {
			// Trim newlines
			while(len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
				len--;
			}
			neo_edit_row_t* curr_row = neo_edit_page_insert_row(curr_page, -1);
			if (!curr_row) {
				NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to add rows to file");
				NEO_FREE(line);
				return SIZE_MAX;
			}
			if (!neo_edit_row_set_text(curr_row, line, (size_t)(len))) {
				NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to add text to file");
				NEO_FREE(line);
				return SIZE_MAX;
			}
		}
		NEO_FREE(line);
		fclose(fp);
		PAGE_FLAG_CLEAR(curr_page, NPAGE_FLAG_DIRTY);
	}
	context->num_pages++;
	return idx;
}

size_t neo_edit_ctx_new_page(neo_edit_ctx_t *context) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!_neo_edit_ctx_valid(context)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return SIZE_MAX; 
	}
	int ret = _neo_edit_ctx_check_page_resize(context, 1);
	if (ret == -1) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Maximum number of open pages reached");
		return SIZE_MAX;
	}
	else if (ret == -2) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize page buffer");
		return SIZE_MAX;
	}

	// Initialize page
	size_t idx = context->num_pages;
	if (!neo_edit_page_init(&context->pages[idx], context)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initialize new page");
		return SIZE_MAX;
	}
	context->num_pages++;
	return idx;
}

bool neo_edit_ctx_close_page(neo_edit_ctx_t* context, int position) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!_neo_edit_ctx_valid(context)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return false; 
	}

	// Calculate index
	size_t idx = 0;
	if (position < 0) { idx = context->num_pages - 1; }
	else if (position < (int)context->num_pages) { idx = (size_t)position; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return false;
	}
	neo_edit_page_t* page = &context->pages[idx];

	// Handle panel shuffling
	if (idx == context->curr_page) { hide_panel(page->nc_panel); }

	// Remove page
	neo_edit_page_clear(page);
	size_t n = sizeof(*(context->pages));
	memmove(&context->pages[idx], &context->pages[idx + 1], n * (context->num_pages - idx - 1));
	context->num_pages--;

	// Move to new page if current page was removed
	if (context->num_pages > 0) {
		if (context->curr_page >= context->num_pages) {
			neo_edit_ctx_set_page(context, -1);
		}
		else if (idx == context->curr_page) {
			neo_edit_ctx_set_page(context, idx);
		}
	}
	return true;
}

bool neo_edit_ctx_set_page(neo_edit_ctx_t *context, int position) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!_neo_edit_ctx_valid(context)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return false; 
	}

	// Calculate index
	size_t idx = 0;
	if (position < 0) { idx = context->num_pages - 1; }
	else if (position < (int)context->num_pages) { idx = (size_t)position; }
	else {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return false;
	}

	// Hide current page
	neo_edit_page_t* curr_page = EDITOR_GET_CURR_PAGE(context);
	if (curr_page) { hide_panel(curr_page->nc_panel); }

	// Switch to new page
	context->curr_page = idx;
	curr_page = EDITOR_GET_CURR_PAGE(context);
	if (curr_page) { show_panel(curr_page->nc_panel); }
	else { 
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to switch to new page");
		return false; 
	}
	return true;
}

bool neo_edit_ctx_handle_input(neo_edit_ctx_t* context, int key) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!_neo_edit_ctx_valid(context)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return false; 
	}
	neo_edit_page_t* curr_page = EDITOR_GET_CURR_PAGE(context);
	neo_edit_row_t* curr_row = PAGE_GET_CURR_ROW(curr_page);

	// Parse input
	for(size_t g = 0; g < context->menu_bar.num_groups; ++g) {
		neo_menu_group_t* group = neo_menu_bar_get_group(&context->menu_bar, g);
		if (!group) { continue; }
		for(size_t e = 0; e < group->num_entries; ++e) {
			neo_menu_entry_t* entry = neo_menu_group_get_entry(group, e);
			if (!entry) { continue; }
			if (entry->callback && entry->shortcut != 0 && CTRL_KEY(entry->shortcut) == key) {
				neo_menu_callback_fptr cb = entry->callback;
				int res = cb(context);
				if (res != 0) { return false; }
			}
		}
	}
	switch(key) {
		case KEY_LEFT:  if (curr_page) { neo_edit_page_move_cursor(curr_page, NDIR_LEFT, 1); } break;
		case KEY_RIGHT: if (curr_page) { neo_edit_page_move_cursor(curr_page, NDIR_RIGHT, 1); } break;
		case KEY_UP:    if (curr_page) { neo_edit_page_move_cursor(curr_page, NDIR_UP, 1); } break;
		case KEY_DOWN:  if (curr_page) { neo_edit_page_move_cursor(curr_page, NDIR_DOWN, 1); } break;
		case KEY_PPAGE: if (curr_page) { neo_edit_page_move_cursor(curr_page, NDIR_UP, curr_page->window_rows); } break;
		case KEY_NPAGE: if (curr_page) { neo_edit_page_move_cursor(curr_page, NDIR_DOWN, curr_page->window_rows); } break;
		case KEY_HOME:  if (curr_page) { neo_edit_page_set_cursor_col(curr_page, 0); } break;
		case KEY_END:   if (curr_page) { neo_edit_page_set_cursor_col(curr_page, -1); } break;
		case KEY_SLEFT: /* Shift-left */ break;
		case KEY_SRIGHT: /* Shift-right */ break;
		case KEY_SR: /* Shift-up */ break;
		case KEY_SF: /* Shift-down */ break;
		case KEY_SHOME: /* Shift-home */ break;
		case KEY_SEND: /* Shift-end */ break;
		case KEY_BACKSPACE: {
			if (!curr_page) { break; }
			if (PAGE_FLAG_ISSET(curr_page, NPAGE_FLAG_READONLY)) { 
				neo_edit_ctx_status(context, NEO_STR_CAST("File is in read-only mode!"));
				break; 
			}
			else if (curr_row) {
				if (curr_page->cursor_x == 0) {
					if (curr_page->cursor_y > 0 && curr_page->cursor_y < curr_page->num_rows) {
						// Merge text with previous line
						neo_edit_row_t* prev_row = &curr_page->rows[curr_page->cursor_y - 1];
						string_t tmp;
						string_init(&tmp);
						string_append(&tmp, curr_row->content.data, curr_row->content.length);
						neo_edit_page_remove_row(curr_page, curr_page->cursor_y);
						neo_edit_row_insert_text(prev_row, -1, tmp.data, tmp.length);
						neo_edit_page_move_cursor(curr_page, NDIR_UP, 1);
						neo_edit_page_set_cursor_col(curr_page, prev_row->content.length - tmp.length);
						string_clear(&tmp);
					}
				}
				else {
					neo_edit_page_move_cursor(curr_page, NDIR_LEFT, 1);
					neo_edit_row_erase_text(curr_row, curr_page->cursor_x, 1);
				}
			}
		} break;
		case KEY_DC: {
			if (!curr_page) { break; }
			if (PAGE_FLAG_ISSET(curr_page, NPAGE_FLAG_READONLY)) { 
				neo_edit_ctx_status(context, NEO_STR_CAST("File is in read-only mode!"));
				break; 
			}
			else if (curr_row) {
				if (curr_page->cursor_x == curr_row->content.length) {
					if (curr_page->cursor_y < curr_page->num_rows - 1) {
						// Bring next line onto current line
						neo_edit_row_t* next_row = &curr_page->rows[curr_page->cursor_y + 1];
						string_t tmp;
						string_init(&tmp);
						string_append(&tmp, next_row->content.data, next_row->content.length);
						neo_edit_page_remove_row(curr_page, curr_page->cursor_y + 1);
						neo_edit_row_insert_text(curr_row, -1, tmp.data, tmp.length);
						string_clear(&tmp);
					}
				}
				else if (curr_row->content.length > 0) {
					neo_edit_row_erase_text(curr_row, curr_page->cursor_x, 1);
				}
			}
		} break;
		case '\n':
		case '\r':
		case KEY_ENTER: {
			if (!curr_page) { break; }
			if (PAGE_FLAG_ISSET(curr_page, NPAGE_FLAG_READONLY)) { 
				neo_edit_ctx_status(context, NEO_STR_CAST("File is in read-only mode!"));
				break; 
			}
			else if (curr_row) {
				if (curr_page->cursor_x >= curr_row->content.length) {
					// Create empty new line
					neo_edit_page_insert_row(curr_page, curr_page->cursor_y + 1);
					neo_edit_page_move_cursor(curr_page, NDIR_DOWN, 1);
					neo_edit_page_set_cursor_col(curr_page, 0);
				}
				else {
					// Split text onto a new line
					string_t tmp;
					string_init(&tmp);
					string_append(&tmp, &curr_row->content.data[curr_page->cursor_x], curr_row->content.length - curr_page->cursor_x);
					neo_edit_row_erase_text(curr_row, curr_page->cursor_x, curr_row->content.length - curr_page->cursor_x);
					neo_edit_row_t* new_row = neo_edit_page_insert_row(curr_page, curr_page->cursor_y + 1);
					neo_edit_row_set_text(new_row, tmp.data, tmp.length);
					neo_edit_page_move_cursor(curr_page, NDIR_DOWN, 1);
					neo_edit_page_set_cursor_col(curr_page, 0);
					string_clear(&tmp);
				}
			}
			else {
				// Create new line in an empty file
				neo_edit_page_insert_row(curr_page, -1);
				neo_edit_page_move_cursor(curr_page, NDIR_DOWN, 1);
			}
		} break;
		default: {
			if (!curr_page) { break; }
			if (PAGE_FLAG_ISSET(curr_page, NPAGE_FLAG_READONLY)) { 
				neo_edit_ctx_status(context, NEO_STR_CAST("File is in read-only mode!"));
				break; 
			}
			if ((!iscntrl(key) && key < 128 && key >= 0) || key == '\t') {
				if (!curr_row) {
					curr_row = neo_edit_page_insert_row(curr_page, -1);
				}
				NEO_CHAR_T text = (NEO_CHAR_T)(key);
				neo_edit_row_insert_text(curr_row, curr_page->cursor_x, &text, 1);
				neo_edit_page_move_cursor(curr_page, NDIR_RIGHT, 1);
			}
		} break;
	}
	return true;
}

int neo_edit_ctx_status(neo_edit_ctx_t *context, const NEO_CHAR_T* fmt, ...) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!_neo_edit_ctx_valid(context)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return 0;
	}

	// Prepare string
	int len = MIN(context->window_cols - 16, NEO_EDIT_MAX_STATUS_MESSAGE_LEN);
	if (!string_erase_all(&context->status_message)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to clear status message");
		return 0;
	}
	if (!string_reserve(&context->status_message, len + 1)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to reserve space in status message");
		return 0;
	}

	// Set string
	va_list ap;
	va_start(ap, fmt);
	neo_vsnprintf(context->status_message.data, len, fmt, ap);
	context->status_message.data[len] = '\0';
	context->status_message.length = len;
	va_end(ap);
	context->status_timer = time(NULL);
	return len;
}
