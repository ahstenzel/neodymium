#include "neo_string.h"
#include "neo_common.h"

static bool _string_check_resize(string_t *str, size_t len) {
	assert(str && str->capacity >= (str->length + 1));
	if (len == 0) { return true; }
	while (str->capacity == 0 || str->length + len >= (str->capacity - 1)) {
		size_t new_capacity = str->capacity * 2;
		if (new_capacity == 0) { new_capacity = NEO_STRING_DEFAULT_CAPACITY; }
		char* new_data = NEO_REALLOC(str->data, new_capacity);
		if (!new_data) { return false; }
		str->data = new_data;
		str->capacity = new_capacity;
	}
	return true;
}

static bool _string_valid(string_t *str) {
	return (str && str->data && str->capacity >= (str->length + 1));
}

bool string_init(string_t *str) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!str) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}

	// Allocate memory
	str->data = NEO_MALLOC(sizeof(*(str->data)) * NEO_STRING_DEFAULT_CAPACITY);
	if (!str->data) { 
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate string");
		return false; 
	}
	str->length = 0;
	str->capacity = NEO_STRING_DEFAULT_CAPACITY;
	return true;
}

void string_clear(string_t *str) {
	if (!str) { return; }
	NEO_FREE(str->data);
	str->data = NULL;
	str->length = 0;
	str->capacity = 0;
}

bool string_insert(string_t *str, size_t pos, const char *insert, size_t len) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (pos > str->length) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return false;
	}
	if (!_string_check_resize(str, len)) {
		NEO_THROW_ERROR(NERROR_BAD_ALLOC);
		return false;
	}

	// Shift end of string forward
	memmove(&str->data[pos + len], &str->data[pos], str->length - pos);
	memcpy(&str->data[pos], insert, len);
	str->length += len;
	str->data[str->length] = '\0';
	return true;
}

bool string_erase(string_t *str, size_t pos, size_t len) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (pos >= str->length) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return false;
	}
	if (len == 0 || str->length == 0) { return true; }
	if (pos + len > str->length) { len = (str->length - pos); }

	// Move end of string backwards
	memmove(&str->data[pos], &str->data[pos + len], str->length - len);
	str->length -= len;
	str->data[str->length] = '\0';
	return true;
}

bool string_erase_all(string_t *str) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}

	// Make previous contents inaccessible by setting length to 0
	str->length = 0;
	str->data[0] = '\0';
	return true;
}

bool string_append(string_t *str, const char *insert, size_t len) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (!_string_check_resize(str, len)) {
		NEO_THROW_ERROR(NERROR_BAD_ALLOC);
		return false;
	}

	// Add to end of string
	memcpy(&str->data[str->length], insert, len);
	str->length += len;
	str->data[str->length] = '\0';
	return true;
}

bool string_set(string_t *str, size_t pos, const char *insert, size_t len) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (pos > str->length) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return false;
	}
	int nlen = (pos + len) - str->length;
	if (nlen < 0) { nlen = 0; }
	if (!_string_check_resize(str, nlen)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize string");
		return false;
	}

	// Overwrite part of string
	memcpy(&str->data[pos], insert, len);
	str->length += nlen;
	str->data[str->length] = '\0';
	return true;
}

bool string_push_back(string_t *str, char insert) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (!_string_check_resize(str, 1)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize string");
		return false;
	}

	str->data[str->length] = insert;
	str->data[++str->length] = '\0';
	return true;
}

bool string_pop_back(string_t *str) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false;
	}
	if (str->length == 0) { return true; }

	str->data[--str->length] = '\0';
	return true;
}

bool string_push_front(string_t *str, char insert) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (!_string_check_resize(str, 1)) {
		NEO_THROW_ERROR(NERROR_BAD_ALLOC);
		return false;
	}

	memmove(&str->data[1], &str->data[0], str->length);
	str->data[str->length++] = '\0';
	str->data[0] = insert;
	return true;
}

bool string_pop_front(string_t *str) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (str->length == 0) { return true; }

	memmove(&str->data[0], &str->data[1], str->length - 1);
	str->data[--str->length] = '\0';
	return true;
}

char string_at(string_t *str, size_t pos) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return '\0'; 
	}
	if (pos == str->length) {
		return '\0';
	}
	else if (pos > str->length) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return '\0';
	}
	else {
		return str->data[pos];
	}
}

bool string_empty(string_t *str) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return true; 
	}
	return (str->length == 0);
}

char string_front(string_t *str) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return '\0'; 
	}
	if (str->length == 0) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "String is empty");
		return '\0'; 
	}
	return str->data[0];
}

char string_back(string_t *str) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return '\0'; 
	}
	if (str->length == 0) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "String is empty");
		return '\0'; 
	}
	return str->data[str->length - 1];
}

bool string_duplicate(string_t *src, string_t* dst) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(src)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Source string invalid");
		return false;
	}
	if (!_string_valid(dst)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Destination string invalid");
		return false;
	}
	if (!_string_check_resize(dst, (dst->length > src->length) ? 0 : (src->length - dst->length))) {
		NEO_THROW_ERROR(NERROR_BAD_ALLOC);
		return false;
	}

	// Overwrite destination string contents
	memcpy(&dst->data[0], &src->data[0], src->length + 1);
	dst->length = src->length;
	return true;
}

bool string_substr(string_t *src, size_t pos, size_t len, string_t* dst) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(src)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Source string invalid");
		return false;
	}
	if (!_string_valid(dst)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Destination string invalid");
		return false;
	}
	if (!_string_check_resize(dst, (dst->length > len) ? 0 : (len - dst->length))) {
		NEO_THROW_ERROR(NERROR_BAD_ALLOC);
		return false;
	}

	// Overwrite destination string contents
	memcpy(&dst->data[0], &src->data[pos], len);
	dst->length = len;
	dst->data[dst->length] = '\0';
	return true;
}

bool string_equal(string_t *str1, string_t *str2) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str1)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "First string invalid");
		return false;
	}
	if (!_string_valid(str1)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Second string invalid");
		return false;
	}

	// Compare strings
	if (str1->length != str2->length) { return false; }
	return (strncmp(str1->data, str2->data, str1->length) == 0);
}

int string_reserve(string_t *str, size_t new_capacity) {
	// Validate string
	NEO_CLEAR_ERROR;
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return -1; 
	}

	// Attempt resize
	if (new_capacity <= NEO_STRING_DEFAULT_CAPACITY) { new_capacity = NEO_STRING_DEFAULT_CAPACITY; }
	if (new_capacity <= str->capacity) { return str->capacity; }
	char* new_data = NEO_REALLOC(str->data, new_capacity);
	if (!new_data) { 
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize string");
		return -1; 
	}
	str->data = new_data;
	str->capacity = new_capacity;
	return new_capacity;
}
