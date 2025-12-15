#include "neo_common.h"
#include "neo_string.h"

static bool _string_check_resize(string_t *str, size_t len) {
	assert(str && str->capacity >= (str->length + 1));
	if (len == 0) { return true; }
	while (str->capacity == 0 || str->length + len >= (str->capacity - 1)) {
		size_t new_capacity = str->capacity * 2;
		if (new_capacity == 0) { new_capacity = NEO_STRING_DEFAULT_CAPACITY; }
		NEO_CHAR_T* new_data = NEO_REALLOC(str->data, NEO_CHAR_SIZE * new_capacity);
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
	if (!str) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}

	// Allocate memory
	str->data = NEO_MALLOC(NEO_CHAR_SIZE * NEO_STRING_DEFAULT_CAPACITY);
	if (!str->data) { 
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate string");
		return false; 
	}
	str->data[0] = '\0';
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

bool string_insert(string_t *str, size_t pos, const NEO_CHAR_T *insert, size_t len) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (pos > str->length) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return false;
	}
	if (!_string_check_resize(str, len)) {
		NEO_THROW_ERROR(NERROR_BAD_ALLOC, "Failed to resize string");
		return false;
	}

	// Shift end of string forward
	neo_memmove(&str->data[pos + len], &str->data[pos], str->length - pos);
	neo_memcpy(&str->data[pos], insert, len * NEO_CHAR_SIZE);
	str->length += len;
	str->data[str->length] = '\0';
	return true;
}

bool string_erase(string_t *str, size_t pos, size_t len) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (len == 0 || str->length == 0) { return true; }
	if (pos >= str->length) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return false;
	}
	if (pos + len > str->length) { len = (str->length - pos); }

	// Move end of string backwards
	neo_memmove(&str->data[pos], &str->data[pos + len], str->length - len);
	str->length -= len;
	str->data[str->length] = '\0';
	return true;
}

bool string_erase_all(string_t *str) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}

	// Make previous contents inaccessible by setting length to 0
	str->length = 0;
	str->data[0] = '\0';
	return true;
}

bool string_append(string_t *str, const NEO_CHAR_T *insert, int len) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (len < 0) { len = neo_strlen(insert); }
	if (!_string_check_resize(str, len)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize string");
		return false;
	}

	// Add to end of string
	neo_memcpy(&str->data[str->length], insert, len * NEO_CHAR_SIZE);
	str->length += len;
	str->data[str->length] = '\0';
	return true;
}

bool string_set(string_t *str, size_t pos, const NEO_CHAR_T* insert, int len) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (pos > str->length) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return false;
	}
	if (len < 0) { len = neo_strlen(insert); }
	int nlen = ((int)pos + len) - (int)str->length;
	if (nlen < 0) { nlen = 0; }
	if (!_string_check_resize(str, nlen)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize string");
		return false;
	}

	// Overwrite part of string
	neo_memcpy(&str->data[pos], insert, len * NEO_CHAR_SIZE);
	str->length += nlen;
	str->data[str->length] = '\0';
	return true;
}

bool string_push_back(string_t *str, NEO_CHAR_T insert, size_t count) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (count == 0) { return true; }
	if (!_string_check_resize(str, count)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize string");
		return false;
	}

	for(size_t i = 0; i < count; ++i) {
		str->data[str->length] = insert;
		str->data[++str->length] = '\0';
	}
	return true;
}

bool string_pop_back(string_t *str, size_t count) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false;
	}
	if (str->length == 0 || count == 0) { return true; }

	for(size_t i = 0; i < count; ++i) {
		str->data[--str->length] = '\0';
	}
	return true;
}

bool string_push_front(string_t *str, NEO_CHAR_T insert, size_t count) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (count == 0) { return true; }
	if (!_string_check_resize(str, count)) {
		NEO_THROW_ERROR(NERROR_BAD_ALLOC, "Failed to resize string");
		return false;
	}

	neo_memmove(&str->data[count], &str->data[0], str->length);
	for(size_t i = 0; i < count; ++i) {
		str->data[i] = insert;
	}
	str->length += count;
	str->data[str->length] = '\0';
	return true;
}

bool string_pop_front(string_t *str, size_t count) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return false; 
	}
	if (str->length == 0 || count == 0) { return true; }

	neo_memmove(&str->data[0], &str->data[count], str->length - count);
	str->length -= count;
	str->data[str->length] = '\0';
	return true;
}

NEO_CHAR_T string_at(string_t *str, size_t pos) {
	// Validate string
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
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return true; 
	}
	return (str->length == 0);
}

NEO_CHAR_T string_front(string_t *str) {
	// Validate string
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

NEO_CHAR_T string_back(string_t *str) {
	// Validate string
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
	if (!_string_valid(src)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Source string invalid");
		return false;
	}
	if (!_string_valid(dst)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Destination string invalid");
		return false;
	}
	if (!_string_check_resize(dst, (dst->length > src->length) ? 0 : (src->length - dst->length))) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize destination string");
		return false;
	}

	// Overwrite destination string contents
	neo_memcpy(&dst->data[0], &src->data[0], (src->length + 1) * NEO_CHAR_SIZE);
	dst->length = src->length;
	return true;
}

bool string_substr(string_t *src, size_t pos, size_t len, string_t* dst) {
	// Validate string
	if (!_string_valid(src)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Source string invalid");
		return false;
	}
	if (!_string_valid(dst)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Destination string invalid");
		return false;
	}
	if (pos + len > src->length) { len = src->length - pos; }
	if (!_string_check_resize(dst, (dst->length > len) ? 0 : (len - dst->length))) {
		NEO_THROW_ERROR(NERROR_BAD_ALLOC, "Failed to resize string");
		return false;
	}

	// Overwrite destination string contents
	neo_memcpy(&dst->data[0], &src->data[pos], len * NEO_CHAR_SIZE);
	dst->length = len;
	dst->data[dst->length] = '\0';
	return true;
}

bool string_equal(string_t *str1, string_t *str2) {
	// Validate string
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
	return (neo_strncmp(str1->data, str2->data, str1->length) == 0);
}

int string_reserve(string_t *str, size_t new_capacity) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return -1; 
	}

	// Attempt resize
	if (new_capacity <= NEO_STRING_DEFAULT_CAPACITY) { new_capacity = NEO_STRING_DEFAULT_CAPACITY; }
	if (new_capacity <= str->capacity) { return str->capacity; }
	NEO_CHAR_T* new_data = NEO_REALLOC(str->data, NEO_CHAR_SIZE * new_capacity);
	if (!new_data) { 
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize string");
		return -1; 
	}
	str->data = new_data;
	str->capacity = new_capacity;
	return new_capacity;
}

int string_wrap(string_t* str, size_t max_width) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return -1;
	}

	// Iterate through string
	int line_count = 1;
	size_t last_whitespace = 0;
	size_t last_newline = 0;
	for(size_t i = 0; i < str->length; ++i) {
		// Record special characters
		NEO_CHAR_T c = str->data[i];
		if (c == ' ' || c == '\t') { last_whitespace = i - last_newline; }
		else if (c == '\n') { 
			last_newline = i; 
			line_count++;
		}

		// Insert linebreak
		if ((i - last_newline) > max_width) {
			if (last_whitespace > 0) {
				// Overwrite previous space
				str->data[last_newline + last_whitespace] = '\n';
				last_newline += last_whitespace;
				last_whitespace = 0;
				line_count++;
			}
			else {
				// Insert newline into word
				if (!string_insert(str, i, NEO_STR_CAST("\n"), 1)) {
					NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to insert linebreak into string");
					return -1;
				}
				last_newline = i;
				last_whitespace = 0;
				line_count++;
			}
		}
	}
	return line_count;
}

int string_truncate(string_t *str, size_t max_width, int elipses) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return -1;
	}
	if (str->length <= max_width) { return (int)str->length; }

	// Trim string contents
	if (elipses < 0) {
		if (!string_erase(str, 0, str->length - max_width)) { return -1; }
	}
	else {
		if (!string_erase(str, max_width, str->length - max_width)) { return -1; }
	}

	// Overwrite with elipses
	if (elipses < 0) {
		if (!string_set(str, 0, NEO_STR_CAST("..."), 3)) { return -1; }
	}
	else if (elipses > 0) {
		if (!string_set(str, str->length - 3, NEO_STR_CAST("..."), 3)) { return -1; }
	}
	return str->length;
}

int string_find_next_of(string_t* str, const NEO_CHAR_T* find, int len, size_t pos) {
	// Validate string
	if (!_string_valid(str)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid string");
		return -1;
	}
	if (pos == str->length) { return -1; }
	else if (pos > str->length) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return -1;
	}
	if (len < 0) { len = neo_strlen(find); }

	// Search string for characters
	for(size_t i = pos; i < str->length; ++i) {
		for(int j = 0; j < len; ++j) {
			if (str->data[i] == find[j]) {
				return (int)i;
			}
		}
	}
	return -1;
}


NEO_CHAR_T* ascii_to_string(char* str, size_t len) {
	#ifdef NEO_USE_WCHAR
	NEO_CHAR_T* new_str = NEO_MALLOC(NEO_CHAR_SIZE * (len + 1));
	if (!new_str) { 
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate wide string");
		return NULL; 
	}
	if (mbstowcs(new_str, str, len) != len) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to convert string to wide string");
		return NULL;
	}
	new_str[len] = '\0';
	return new_str;
	#else
	NEO_CHAR_T* new_str = strndup(str, len);
	if (!new_str) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to duplicate string");
		return NULL; 
	}
	return new_str;
	#endif
}
