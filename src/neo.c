#include "neo.h"

bool _neo_flag_resized = false;
int _neo_error_code = NERROR_SUCCESS;
char _neo_error_msg[1024] = { '\0' };

void ncurses_init() {
	initscr();
	cbreak();
	noecho();
	raw();
	keypad(stdscr, TRUE);
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

void string_init(string_t *str) {
	if (!_string_valid(str)) { 
		NEO_ERROR(NERROR_INVALID_PARAM);
		return; 
	}
	str->length = 0;
	str->capacity = 0;
	str->data = NEO_MALLOC(sizeof(*(str->data)) * NEO_STR_DEFAULT_CAPACITY);
	if (!str->data) { 
		NEO_ERROR(NERROR_BAD_ALLOC);
		return; 
	}
	str->capacity = NEO_STR_DEFAULT_CAPACITY;
}

void string_clear(string_t *str) {
	if (!str) { return; }
	NEO_FREE(str->data);
	str->data = NULL;
	str->length = 0;
	str->capacity = 0;
}

void string_insert(string_t *str, size_t pos, const char *insert, size_t len) {
	if (!_string_valid(str)) { 
		NEO_ERROR(NERROR_INVALID_PARAM);
		return; 
	}
	if (!_string_check_resize(str, len)) {
		NEO_ERROR(NERROR_BAD_ALLOC);
		return;
	}

}

void string_erase(string_t *str, size_t pos, size_t len) {

}

void string_append(string_t *str, const char *insert, size_t len) {

}

void string_set(string_t *str, size_t pos, const char *insert, size_t len) {

}

void string_push_back(string_t *str, char insert) {

}

void string_pop_back(string_t *str) {

}

void string_push_front(string_t *str, char insert) {

}

void string_pop_front(string_t *str) {

}

char string_at(string_t *str, size_t pos) {
	return 0;
}

bool string_empty(string_t *str) {
	return false;
}

char string_front(string_t *str) {
	return 0;
}

char string_back(string_t *str) {
	return 0;
}

string_t string_duplicate(string_t *str) {
	string_t new_string = {0};
	return new_string;
}

string_t string_substr(string_t *str, size_t pos, size_t len) {
	string_t new_string = {0};
	return new_string;
}

bool string_equal(string_t *str1, string_t *str2) {
	return false;
}

bool _string_check_resize(string_t *str, size_t len) {
	assert(str && str->capacity >= str->length);
	if (str->capacity == 0 || str->length + len >= (str->capacity - 1)) {
		size_t new_capacity = str->capacity * 2;
		if (new_capacity == 0) { new_capacity = NEO_STR_DEFAULT_CAPACITY; }
		char* new_data = NEO_REALLOC(str->data, new_capacity);
		if (!new_data) { return false; }
		str->data = new_data;
		str->capacity = new_capacity;
	}
	return true;
}

bool _string_valid(string_t *str) {
	return (str && str->capacity >= str->length);
}
