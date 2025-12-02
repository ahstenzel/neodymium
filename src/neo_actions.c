#include "neo_actions.h"

int neo_cb_new_file(neo_edit_ctx_t* context) {
	size_t n = neo_edit_ctx_new_page(context);
	if (n == SIZE_MAX) { return 1; }
	neo_edit_ctx_set_page(context, n);
	return 0;
}

int neo_cb_open_file(neo_edit_ctx_t* context) {
	string_t message;
	string_init(&message);
	string_set(&message, 0, NEO_STR_CAST("Open file!"), -1);
	neo_dialog_message(context, &message);
	string_clear(&message);
	return 0;
}

int neo_cb_save_file(neo_edit_ctx_t* context) {
	string_t message;
	string_init(&message);
	string_set(&message, 0, NEO_STR_CAST("Save file!"), -1);
	neo_dialog_message(context, &message);
	string_clear(&message);
	return 0;
}

int neo_cb_save_file_as(neo_edit_ctx_t* context) {
	string_t message;
	string_init(&message);
	string_set(&message, 0, NEO_STR_CAST("Save file as!"), -1);
	neo_dialog_message(context, &message);
	string_clear(&message);
	return 0;
}

int neo_cb_save_all_file(neo_edit_ctx_t* context) {
	string_t message;
	string_init(&message);
	string_set(&message, 0, NEO_STR_CAST("Save all files!"), -1);
	neo_dialog_message(context, &message);
	string_clear(&message);
	return 0;
}

int neo_cb_next_page(neo_edit_ctx_t* context) {
	// Calculate index
	size_t idx = context->curr_page;
	if (idx < (context->num_pages - 1)) { idx++; }
	else { idx = 0; }

	// Set page
	if (!neo_edit_ctx_set_page(context, idx)) { return 1; }
	return 0;
}

int neo_cb_prev_page(neo_edit_ctx_t* context) {
	// Calculate index
	size_t idx = context->curr_page;
	if (idx > 0) { idx--; }
	else { idx = context->num_pages - 1; }

	// Set page
	if (!neo_edit_ctx_set_page(context, idx)) { return 1; }
	return 0;
}

int neo_cb_close_page(neo_edit_ctx_t* context) {
	// Close current tab
	if (!neo_edit_ctx_close_page(context, context->curr_page)) { return 1; }
	if (context->num_pages == 0) { return neo_cb_new_file(context); }
	return 0;
}

int neo_cb_quit(neo_edit_ctx_t* context) {
	context->state = NSTATE_SHOULD_CLOSE;
	return 0;
}

int neo_cb_cut(neo_edit_ctx_t* context) {
	UNUSED(context);
	return 0;
}

int neo_cb_copy(neo_edit_ctx_t* context) {
	UNUSED(context);
	return 0;
}

int neo_cb_paste(neo_edit_ctx_t* context) {
	UNUSED(context);
	return 0;
}

int neo_cb_duplicate(neo_edit_ctx_t *context) {
	neo_edit_page_t* curr_page = EDITOR_GET_CURR_PAGE(context);
	neo_edit_row_t* curr_row = PAGE_GET_CURR_ROW(curr_page);
	if (!curr_page || !curr_row) { return 0; }
	neo_edit_row_t* new_row = neo_edit_page_insert_row(curr_page, curr_page->cursor_y + 1);
	if (!new_row) { return 1; }
	neo_edit_row_set_text(new_row, curr_row->content.data, curr_row->content.length);
	neo_edit_page_move_cursor(curr_page, NDIR_DOWN, 1);
	return 0;
}

int neo_cb_select_all(neo_edit_ctx_t* context) {
	UNUSED(context);
	return 0;
}

int neo_cb_undo(neo_edit_ctx_t* context) {
	UNUSED(context);
	return 0;
}

int neo_cb_redo(neo_edit_ctx_t* context) {
	UNUSED(context);
	return 0;
}

int neo_cb_about(neo_edit_ctx_t* context) {
	UNUSED(context);
	return 0;
}

bool neo_dialog_message(neo_edit_ctx_t* context, string_t* message) {
	if (!context) { return false; }

	// Create window
	int screen_rows, screen_cols;
	getmaxyx(stdscr, screen_rows, screen_cols);
	size_t window_cols = CLAMP(screen_cols, 8, 40);
	int line_count = string_wrap(message, window_cols - 4);
	if (line_count < 0) { return false; }
	size_t window_rows = MIN(screen_rows, 3 + line_count);
	WINDOW* nc_window = newwin(window_rows, window_cols, (screen_rows - window_rows) / 2, (screen_cols - window_cols) / 2);
	if (!nc_window) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses window");
		return false;
	}
	PANEL* nc_panel = new_panel(nc_window);
	if (!nc_panel) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses panel");
		delwin(nc_window);
		return false;
	}
	top_panel(nc_panel);

	// Draw window contents
	wborder(nc_window, 0, 0, 0, 0, 0, 0, 0, 0);
	wmove(nc_window, 1, 2);
	neo_waddnstr(nc_window, message->data, message->length);
	wmove(nc_window, 1 + line_count, (window_cols - 4) / 2);
	wattron(nc_window, A_REVERSE);
	neo_waddnstr(nc_window, NEO_STR_CAST("[OK]"), 4);
	wattroff(nc_window, A_REVERSE);
	curs_set(0);

	// Event loop
	while(true) {
		update_panels();
		doupdate();
		int c = getch();
		if (c == KEY_ENTER || c == '\n' || c == '\r' || c == ' ') {
			break;
		}
	}

	// Cleanup
	hide_panel(nc_panel);
	del_panel(nc_panel);
	delwin(nc_window);
	curs_set(1);
	return true;
}

bool neo_dialog_choice(neo_edit_ctx_t* context, string_t* message, neo_choice_t* result) {
	if (!context) { return false; }
	UNUSED(message);
	UNUSED(result);
	return true;
}

bool neo_dialog_file(neo_edit_ctx_t* context, string_t* message, string_t* filename) {
	if (!context) { return false; }
	UNUSED(message);
	UNUSED(filename);
	return true;
}
