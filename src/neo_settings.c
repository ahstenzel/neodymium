#include "neo_settings.h"
#include "neo_common.h"
#include <komihash/komihash.h>

static bool _neo_settings_valid(neo_settings_t* settings) {
	return (settings && settings->keys && settings->vals && settings->capacity >= settings->length);
}

static bool _neo_settings_meta_insert(char** keys, neo_settings_val_t* vals, size_t capacity, const char* key, size_t key_len, neo_settings_val_t val) {
	assert(keys && vals);
	size_t bucket = komihash(key, key_len, 0);
	size_t i = 0;
	for(; i < capacity; ++i) {
		size_t idx = (bucket + i) % capacity;
		if (!keys[idx]) {
			char* store_key = NEO_MALLOC(key_len + 1);
			if (!store_key) { return false; }
			memcpy(store_key, key, key_len);
			store_key[key_len] = '\0';
			keys[idx] = store_key;
			vals[idx] = val;
			break;
		}
		else if (strcmp(key, keys[i]) == 0) {
			vals[i] = val;
			break;
		}
	}
	return (i != capacity);
}

static bool _neo_settings_meta_get(char** keys, neo_settings_val_t* vals, size_t capacity, const char* key, size_t key_len, size_t* dest_idx) {
	assert(keys && vals && key && dest_idx);
	size_t bucket = komihash(key, key_len, 0);
	size_t i = 0;
	for(; i < capacity; ++i) {
		size_t idx = (bucket + i) % capacity;
		if (keys[idx] && strcmp(key, keys[idx]) == 0) {
			*dest_idx = idx;
			break;
		}
	}
	return (i != capacity);
}

static bool _neo_settings_check_resize(neo_settings_t* settings) {
	assert(settings && settings->capacity >= settings->length);

	// Determine new capacity
	size_t new_capacity = 0;
	if (settings->capacity < NEO_SETTINGS_DEFAULT_CAPACITY) { new_capacity = NEO_SETTINGS_DEFAULT_CAPACITY; }
	else {
		new_capacity = settings->capacity;
		while(settings->length + 1 >= ((new_capacity / 4) * 3)) {
			new_capacity *= 2;
		}
	}

	// Create new buffers
	char** new_keys = NEO_MALLOC(sizeof(*new_keys) * new_capacity);
	if (!new_keys) {
		return false;
	}
	neo_settings_val_t* new_vals = NEO_MALLOC(sizeof(*new_vals) * new_capacity);
	if (!new_vals) {
		NEO_FREE(new_keys);
		return false;
	}
	memset(new_keys, 0, sizeof(*new_keys) * new_capacity);
	memset(new_vals, 0, sizeof(*new_vals) * new_capacity);

	// Move over old data
	for(size_t i = 0; i < settings->capacity; ++i) {
		if (settings->keys[i]) {
			if (!_neo_settings_meta_insert(
				new_keys, new_vals, new_capacity, 
				settings->keys[i], strlen(settings->keys[i]), settings->vals[i]
			)) {
				NEO_FREE(new_keys);
				NEO_FREE(new_vals);
				return false;
			}
		}
	}

	// Replace old buffers
	NEO_FREE(settings->keys);
	NEO_FREE(settings->vals);
	settings->keys = new_keys;
	settings->vals = new_vals;
	settings->capacity = new_capacity;
	return true;
}

bool neo_settings_init(neo_settings_t* settings) {
	// Validate settings
	NEO_CLEAR_ERROR;
	if (!settings) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid settings");
		return false; 
	}

	// Allocate memory
	settings->keys = NEO_MALLOC(sizeof(*(settings->keys)) * NEO_SETTINGS_DEFAULT_CAPACITY);
	if (!settings->keys) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate keys buffer");
		return false;
	}
	settings->vals = NEO_MALLOC(sizeof(*(settings->vals)) * NEO_SETTINGS_DEFAULT_CAPACITY);
	if (!settings->vals) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate values buffer");
		NEO_FREE(settings->vals);
		settings->vals = NULL;
		return false;
	}
	memset(settings->keys, 0, sizeof(*(settings->keys)) * NEO_SETTINGS_DEFAULT_CAPACITY);
	memset(settings->vals, 0, sizeof(*(settings->vals)) * NEO_SETTINGS_DEFAULT_CAPACITY);
	settings->length = 0;
	settings->capacity = NEO_SETTINGS_DEFAULT_CAPACITY;
	return true;
}

void neo_settings_clear(neo_settings_t* settings) {
	if (!settings) { return; }
	NEO_FREE(settings->vals);
	for(size_t i = 0; i < settings->capacity; ++i) {
		NEO_FREE(settings->keys[i]);
	}
	NEO_FREE(settings->keys);
	settings->length = 0;
	settings->capacity = 0;
}

void neo_settings_insert(neo_settings_t* settings, const char* key, neo_settings_val_t val) {
	// Validate settings
	if (!_neo_settings_valid(settings)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid settings");
		return; 
	}
	if (!key || strlen(key) == 0) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid settings key");
		return;
	}
	if (!_neo_settings_check_resize(settings)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize settings map");
		return;
	}

	// Insert element
	if (!_neo_settings_meta_insert(
		settings->keys, settings->vals, settings->capacity,
		key, strlen(key), val
	)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to insert key");
		return;
	}
	settings->length++;
}

void neo_settings_remove(neo_settings_t* settings, const char* key) {
	// Validate settings
	if (!_neo_settings_valid(settings)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid settings");
		return; 
	}
	if (!key || strlen(key) == 0) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid settings key");
		return;
	}

	// Get element index
	size_t idx = 0;
	if (!_neo_settings_meta_get(
		settings->keys, settings->vals, settings->capacity,
		key, strlen(key), &idx
	)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Failed to find settings key");
		return; 
	}

	// Clear element
	NEO_FREE(settings->keys[idx]);
	settings->keys[idx] = NULL;
	settings->length--;
}

bool neo_settings_get(neo_settings_t* settings, const char* key, neo_settings_val_t* val) {
	// Validate settings
	if (!_neo_settings_valid(settings)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid settings");
		return false; 
	}
	if (!key || strlen(key) == 0) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid settings key");
		return false;
	}

	// Get element index
	size_t idx = 0;
	if (!_neo_settings_meta_get(
		settings->keys, settings->vals, settings->capacity,
		key, strlen(key), &idx
	)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Failed to find settings key");
		return false; 
	}

	// Get element
	if (val) { *val = settings->vals[idx]; }
	return true;
}
