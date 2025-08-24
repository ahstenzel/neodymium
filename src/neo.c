#include "neo.h"

int _neo_error_code = NERROR_SUCCESS;
char _neo_error_msg[NEO_MSG_BUFLEN] = { '\0' };
bool _neo_flag_resized = false;

bool ncurses_init() {
	initscr();
	cbreak();
	noecho();
	raw();
	keypad(stdscr, TRUE);
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
	}
}