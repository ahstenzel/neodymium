#include "neo_actions.h"

int neo_cb_new_file(neo_edit_ctx_t* context) {
	size_t n = neo_edit_ctx_new_page(context);
	if (n == SIZE_MAX) { 
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to make new file");
		return 1; 
	}
	neo_edit_ctx_set_page(context, n);
	return 0;
}

int neo_cb_open_file(neo_edit_ctx_t* context) {
	string_t message = { 0 };
	string_t filename = { 0 };
	string_init(&message);
	string_init(&filename);
	string_set(&message, 0, NEO_STR_CAST("Open file:"), -1);
	neo_dialog_file(&message, &filename);

	if (!string_empty(&filename)) {
		size_t page_idx = 0;
		if ((page_idx = neo_edit_ctx_open_page(context, filename.data)) == SIZE_MAX) {
			NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to open file");
			return 1;
		}
		else { neo_edit_ctx_set_page(context, page_idx); }
	}

	string_clear(&message);
	string_clear(&filename);
	return 0;
}

int neo_cb_save_file(neo_edit_ctx_t* context) {
	neo_edit_ctx_write_page(context, context->curr_page);
	return 0;
}

int neo_cb_save_file_as(neo_edit_ctx_t* context) {
	UNUSED(context);
	NEO_THROW_ERROR_MSG(NERROR_UNIMPLEMENTED, "Unimplemented feature: save file as");
	return 0;
}

int neo_cb_save_all_file(neo_edit_ctx_t* context) {
	UNUSED(context);
	NEO_THROW_ERROR_MSG(NERROR_UNIMPLEMENTED, "Unimplemented feature: save all files");
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
	NEO_THROW_ERROR_MSG(NERROR_UNIMPLEMENTED, "Unimplemented feature: cut");
	return 0;
}

int neo_cb_copy(neo_edit_ctx_t* context) {
	UNUSED(context);
	NEO_THROW_ERROR_MSG(NERROR_UNIMPLEMENTED, "Unimplemented feature: copy");
	return 0;
}

int neo_cb_paste(neo_edit_ctx_t* context) {
	UNUSED(context);
	NEO_THROW_ERROR_MSG(NERROR_UNIMPLEMENTED, "Unimplemented feature: paste");
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
	neo_edit_page_t* curr_page = EDITOR_GET_CURR_PAGE(context);
	neo_edit_row_t* last_row = neo_edit_page_get_row(curr_page, -1);
	if (!curr_page || !last_row) { return 0; }
	neo_edit_page_set_select_start(curr_page, 0, 0);
	neo_edit_page_set_cursor_row(curr_page, -1);
	neo_edit_page_set_cursor_col(curr_page, -1);
	neo_edit_page_set_select_end(curr_page, -1, -1);
	return 0;
}

int neo_cb_undo(neo_edit_ctx_t* context) {
	UNUSED(context);
	NEO_THROW_ERROR_MSG(NERROR_UNIMPLEMENTED, "Unimplemented feature: undo");
	return 0;
}

int neo_cb_redo(neo_edit_ctx_t* context) {
	UNUSED(context);
	NEO_THROW_ERROR_MSG(NERROR_UNIMPLEMENTED, "Unimplemented feature: redo");
	return 0;
}

int neo_cb_about(neo_edit_ctx_t* context) {
	UNUSED(context);
	NEO_THROW_ERROR_MSG(NERROR_UNIMPLEMENTED, "Unimplemented feature: about");
	return 0;
}