#include "neo_common.h"
#include "neo_string.h"

int _neo_error_code = NERROR_SUCCESS;
NEO_CHAR_T _neo_error_msg[NEO_MSG_BUFLEN + 1] = { '\0' };
bool _neo_error_enable = true;
bool _neo_flag_resized = false;
int _neo_ext_signal = 0;

bool ncurses_init() {
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	raw();
	ESCDELAY = 10;
	return true;
}

void ncurses_clear() {
	endwin();
}

void signal_handler(int sig) {
	switch(sig) {
		case SIGWINCH: {
			_neo_flag_resized = true; 
			endwin();
			refresh();
			clear();
		} break;
		case SIGINT: {
			_neo_ext_signal = sig;
		} break;
	}
}

bool _neo_dialog_init(string_t* message, WINDOW** window, PANEL** panel, int* line_count, size_t* rows, size_t* cols) {
	int screen_rows, screen_cols;
	getmaxyx(stdscr, screen_rows, screen_cols);
	*cols = CLAMP(screen_cols, 8, 60);
	*line_count = string_wrap(message, *cols - 4);
	if ((*line_count) < 0) { return false; }
	*rows = MIN(screen_rows, 3 + (*line_count));
	*window = newwin(*rows, *cols, (screen_rows - *rows) / 2, (screen_cols - *cols) / 2);
	if (!(*window)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses window");
		return false;
	}
	*panel = new_panel(*window);
	if (!(*panel)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate ncurses panel");
		delwin(*window);
		return false;
	}
	keypad(*window, true);
	top_panel(*panel);
	curs_set(0);
	return true;
}

void _neo_dialog_clear(WINDOW** window, PANEL** panel) {
	hide_panel(*panel);
	del_panel(*panel);
	delwin(*window);
	*window = NULL;
	*panel = NULL;
	curs_set(1);
}

bool neo_dialog_message(string_t* message) {
	// Create window
	WINDOW* nc_window = NULL;
	PANEL* nc_panel = NULL;
	int line_count = 0;
	size_t window_rows, window_cols = 0;
	if (!_neo_dialog_init(message, &nc_window, &nc_panel, &line_count, &window_rows, &window_cols)) {
		return false;
	}

	// Draw window contents
	wborder(nc_window, 0, 0, 0, 0, 0, 0, 0, 0);
	wmove(nc_window, 1, 2);
	size_t line_num = 0;
	size_t pos = 0;
	int newline = -1;
	do {
		newline = string_find_next_of(message, "\n", 1, pos);
		if (newline == -1) {
			neo_waddnstr(nc_window, &message->data[pos], -1);
		}
		else {
			neo_waddnstr(nc_window, &message->data[pos], newline - (int)pos);
			pos = (size_t)newline + 1;
			line_num++;
			wmove(nc_window, 1 + line_num, 2);
		}
	} while(newline != -1);
	wmove(nc_window, 1 + line_count, (window_cols - 4) / 2);
	wattron(nc_window, A_REVERSE);
	neo_waddnstr(nc_window, NEO_STR_CAST("[OK]"), 4);
	wattroff(nc_window, A_REVERSE);

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
	_neo_dialog_clear(&nc_window, &nc_panel);
	return true;
}

bool neo_dialog_choice(string_t* message, neo_choice_t* result) {
	UNUSED(message);
	UNUSED(result);
	return true;
}

bool neo_dialog_file(string_t* message, string_t* filename) {
	// Create window
	WINDOW* nc_window = NULL;
	PANEL* nc_panel = NULL;
	int line_count = 0;
	size_t window_rows, window_cols = 0;
	if (!_neo_dialog_init(message, &nc_window, &nc_panel, &line_count, &window_rows, &window_cols)) {
		return false;
	}
	curs_set(1);

	// Draw window contents
	wborder(nc_window, 0, 0, 0, 0, 0, 0, 0, 0);
	wmove(nc_window, 1, 2);
	neo_waddnstr(nc_window, message->data, message->length);

	// Event loop
	int pos = 0;
	int offset = 0;
	bool execute = true;
	size_t field_length = window_cols - 4;
	while(execute) {
		// Draw text
		wmove(nc_window, 1 + line_count, 2);
		wattron(nc_window, A_UNDERLINE | A_REVERSE);
		neo_waddnstr(nc_window, &filename->data[offset], MIN(filename->length - (size_t)(offset), field_length));
		for(size_t i = (filename->length - (size_t)(offset)); i < field_length; ++i) {
			neo_waddch(nc_window, ' ');
		}
		wattroff(nc_window, A_UNDERLINE | A_REVERSE);
		wmove(nc_window, 1 + line_count, 2 + (pos - offset));

		// Render panels
		update_panels();
		doupdate();

		// Handle input
		int c = wgetch(nc_window);
		switch(c) {
			case 27: {
				nodelay(nc_window, true);
				c = wgetch(nc_window);
				nodelay(nc_window, false);
				if (c == -1) {
					// Escape was pressed
					string_erase_all(filename);
					execute = false;
				}
			} break;
			case KEY_ENTER:
			case '\r':
			case '\n': execute = false; break;
			case KEY_SLEFT:
			case KEY_LEFT: if (pos > 0) { pos--; } break;
			case KEY_SRIGHT:
			case KEY_RIGHT: if (pos < filename->length) { pos++; } break;
			case KEY_SHOME:
			case KEY_HOME: pos = 0; break;
			case KEY_SEND:
			case KEY_END: pos = filename->length; break;
			case KEY_BACKSPACE: if (pos > 0) { pos--; }
			case KEY_DC: {
				if (pos < filename->length) { string_erase(filename, pos, 1); }
			} break;
			case KEY_UP:
			case KEY_DOWN:
			case KEY_SR:
			case KEY_SF:
			case KEY_PPAGE:
			case KEY_NPAGE: break;
			default: {
				string_push_back(filename, (NEO_CHAR_T)(c), 1);
				pos++;
			} break;
		}

		// Calculate scroll offset
		if ((int)pos - NEO_SCROLL_MARGIN < (int)offset) {
			offset = MAX(0, (int)(pos) - NEO_SCROLL_MARGIN);
		}
		if (pos + NEO_SCROLL_MARGIN >= offset + field_length) {
			offset = MIN(
				((int)(filename->length) + NEO_SCROLL_MARGIN + 1) - field_length,
				(pos - field_length) + NEO_SCROLL_MARGIN + 1
			);
		}
	}

	// Cleanup
	_neo_dialog_clear(&nc_window, &nc_panel);
	return true;
}