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
		context->max_pages >= context->num_pages &&
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
	int screen_rows, screen_cols;
	getmaxyx(stdscr, screen_rows, screen_cols);
	page->window_cols = MAX(screen_cols, 1);
	page->window_rows = MAX(screen_rows - (NEO_SIZE_HEADER + NEO_SIZE_FOOTER), 1);
	page->nc_window = newwin(page->window_rows, page->window_cols, NEO_SIZE_HEADER, 0);
	if (!page->nc_window) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses window");
		string_clear(&page->filename);
		return false;
	}
	page->nc_panel = new_panel(page->nc_window);
	if (!page->nc_panel) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses panel");
		string_clear(&page->filename);
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
		page->window_cols = MAX(screen_cols, 1);
		page->window_rows = MAX(screen_rows - (NEO_SIZE_HEADER + NEO_SIZE_FOOTER), 1);
		wresize(page->nc_window, page->window_rows, page->window_cols);
	}

	// Iterate through rows
	for(size_t i = 0; i < page->num_rows; ++i) {
		if (!neo_edit_row_update(&page->rows[i])) { 
			return false; 
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
		waddch(page->nc_window, '~');
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
	else if (at < (int)page->num_rows) { idx = (size_t)at; }
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
	else if (at <= (int)page->num_rows) { idx = (size_t)at; }
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
	else if (at <= (int)page->num_rows) { idx = (size_t)at; }
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

	// Initialize members
	if (!neo_settings_init(&context->settings)) {
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
	context->window_cols = MAX(screen_cols, 1);
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
	neo_menu_group_t* menu_group_file = neo_menu_bar_insert_group(&context->menu_bar, -1, "File", 'f');
	neo_menu_group_insert_entry(menu_group_file, -1, "New File", 'n', NULL);
	neo_menu_group_insert_entry(menu_group_file, -1, "Open File", 'o', NULL);
	neo_menu_group_insert_seperator(menu_group_file, -1);
	neo_menu_group_insert_entry(menu_group_file, -1, "Save File", 's', NULL);
	neo_menu_group_insert_entry(menu_group_file, -1, "Save File As", 'b', NULL);
	neo_menu_group_insert_entry(menu_group_file, -1, "Save All Files", 'd', NULL);
	neo_menu_group_insert_seperator(menu_group_file, -1);
	neo_menu_group_insert_entry(menu_group_file, -1, "Next Tab", 't', NULL);
	neo_menu_group_insert_entry(menu_group_file, -1, "Prev Tab", 'r', NULL);
	neo_menu_group_insert_entry(menu_group_file, -1, "Close Tab", 'w', NULL);
	neo_menu_group_insert_entry(menu_group_file, -1, "Quit", 'q', NULL);
	neo_menu_group_t* menu_group_edit = neo_menu_bar_insert_group(&context->menu_bar, -1, "Edit", 'e');
	neo_menu_group_insert_entry(menu_group_edit, -1, "Cut", 'x', NULL);
	neo_menu_group_insert_entry(menu_group_edit, -1, "Copy", 'c', NULL);
	neo_menu_group_insert_entry(menu_group_edit, -1, "Paste", 'v', NULL);
	neo_menu_group_insert_seperator(menu_group_edit, -1);
	neo_menu_group_insert_entry(menu_group_edit, -1, "Select All", 'a', NULL);
	neo_menu_group_insert_seperator(menu_group_edit, -1);
	neo_menu_group_insert_entry(menu_group_edit, -1, "Undo", 'z', NULL);
	neo_menu_group_insert_entry(menu_group_edit, -1, "Redo", 'y', NULL);
	neo_menu_group_t* menu_group_help = neo_menu_bar_insert_group(&context->menu_bar, -1, "Help", 'h');
	neo_menu_group_insert_entry(menu_group_help, -1, "About", 0, NULL);
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
		context->window_cols = MAX(screen_cols, 1);
		context->window_rows = MAX(screen_rows - NEO_SIZE_MENU_BAR, NEO_SIZE_MENU_BAR + NEO_SIZE_FOOTER + 1);
		wresize(context->nc_window, context->window_rows, context->window_cols);
	}

	// Update pages
	for(size_t i = 0; i < context->num_pages; ++i) {
		if (!neo_edit_page_update(&context->pages[i])) {
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

	// Draw file tabs
	wmove(context->nc_window, 0, 0);
	for(size_t i = 0; i < context->num_pages; ++i) {
		neo_edit_page_t* page = &context->pages[i];
		if (i == context->curr_page) { attron(A_BOLD); }
		waddch(context->nc_window, (i == context->curr_page) ? '/' : ' ');
		if (string_empty(&page->filename)) {
			wprintw(context->nc_window, " <New File> ");
		}
		else {
			wprintw(context->nc_window, " %s ", page->filename.data);
		}
		if (PAGE_FLAG_ISSET(page, NPAGE_FLAG_DIRTY)) {
			waddch(context->nc_window, '*');
		}
		waddch(context->nc_window, (i == context->curr_page) ? '\\' : ' ');
		if (i == context->curr_page) { attroff(A_BOLD); }
	}
	wmove(context->nc_window, 1, 0);
	whline(context->nc_window, '-', context->window_cols);

	// Draw page contents
	neo_edit_page_t* curr_page = EDITOR_GET_CURR_PAGE(context);
	if (curr_page) {
		if (!neo_edit_page_draw(curr_page)) {
			return false;
		}
		//show_panel(curr_page->nc_panel);
	}

	// Draw status message
	wmove(context->nc_window, context->window_rows - NEO_SIZE_FOOTER, 0);
	whline(context->nc_window, '-', context->window_cols);
	wmove(context->nc_window, context->window_rows - NEO_SIZE_FOOTER + 1, 0);
	whline(context->nc_window, ' ', context->window_cols);
	wprintw(context->nc_window, "%s", context->status_message.data);

	// Draw cursor position
	if (curr_page) {
		wmove(context->nc_window, context->window_rows - NEO_SIZE_FOOTER + 1, context->window_cols - 16);
		wprintw(context->nc_window, "L:%d C:%d", (int)curr_page->cursor_y, (int)curr_page->cursor_x);
	}

	// Draw file bar
	if (!neo_menu_bar_draw(&context->menu_bar)) {
		return false;
	}

	// Enforce the draw order of panels
	//bottom_panel(context->nc_panel);
	//top_panel(context->menu_bar.nc_panel);

	// Set final cursor position
	if (curr_page) {
		wmove(curr_page->nc_window, (int)curr_page->cursor_y, (int)curr_page->cursor_x);
		//waddch(curr_page->nc_window, '!');
	}
	else {
		wmove(context->nc_window, NEO_SIZE_FILE_BAR, 0);
	}
	return true;
}

size_t neo_edit_ctx_open_file(neo_edit_ctx_t *context, char *filename) {
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
	if (!string_init(&filename_str)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initialize page name string");
		return SIZE_MAX;
	}
	if (!string_set(&filename_str, 0, filename, strlen(filename))) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to set page name string");
		return SIZE_MAX;
	}
	if (!neo_edit_page_init(&context->pages[idx], context)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initialize new page");
		return SIZE_MAX;
	}
	neo_edit_page_set_filename(&context->pages[idx], filename_str);
	context->curr_page = idx;
	context->num_pages++;
	return idx;
}

size_t neo_edit_ctx_new_file(neo_edit_ctx_t *context) {
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
	context->curr_page = idx;
	context->num_pages++;
	return idx;
}

bool neo_edit_ctx_handle_input(neo_edit_ctx_t *context, int key) {
	// Validate context
	NEO_CLEAR_ERROR;
	if (!_neo_edit_ctx_valid(context)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid context");
		return false; 
	}

	// Parse input
	if (key == CTRL_KEY('q')) {
		context->state = NSTATE_SHOULD_CLOSE;
	}
	return true;
}

int neo_edit_ctx_status(neo_edit_ctx_t *context, const char *fmt, ...) {
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
	len = vsnprintf(context->status_message.data, len, fmt, ap);
	context->status_message.data[len] = '\0';
	context->status_message.length = len;
	va_end(ap);
	context->status_timer = time(NULL);
	return len;
}
