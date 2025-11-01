#include "neo_menu.h"
#include "neo_common.h"

static bool _neo_menu_group_check_resize(neo_menu_group_t* group, size_t len) {
	assert(group && group->max_entries >= group->num_entries);
	if (len == 0) { return true; }
	while (group->max_entries == 0 || group->num_entries + len >= group->max_entries) {
		size_t new_capacity = group->max_entries * 2;
		if (new_capacity == 0) { new_capacity = NEO_MENU_GROUP_DEFAULT_CAPACITY; }
		neo_menu_entry_t* new_data = NEO_REALLOC(group->entries, sizeof(*(group->entries)) * new_capacity);
		if (!new_data) { return false; }
		group->entries = new_data;
		group->max_entries = new_capacity;
	}
	return true;
}

static bool _neo_menu_bar_check_resize(neo_menu_bar_t* bar, size_t len) {
	assert(bar && bar->groups);
	if (len == 0) { return true; }
	while(bar->max_groups == 0 || bar->num_groups + len >= bar->max_groups) {
		size_t new_capacity = bar->max_groups * 2;
		if (new_capacity == 0) { new_capacity = NEO_MENU_BAR_DEFAULT_CAPACITY; }
		neo_menu_group_t* new_data = NEO_REALLOC(bar->groups, sizeof(*(bar->groups)) * new_capacity);
		if (!new_data) { return false; }
		bar->groups = new_data;
		bar->max_groups = new_capacity;
	}
	return true;
}

static bool _neo_menu_group_valid(neo_menu_group_t* group) {
	return (group && group->entries && group->max_entries >= group->num_entries);
}

static bool _neo_menu_bar_valid(neo_menu_bar_t* bar) {
	return (bar && bar->groups && bar->max_groups >= bar->num_groups);
}

static size_t _neo_menu_group_idx_end(neo_menu_group_t* group, int position) {
	assert(group && group->max_entries >= (group->num_entries + 1));
	size_t idx = 0;
	if (position == -1) { idx = group->num_entries; }
	else if (position >= 0 && position <= group->num_entries) { idx = (size_t)position; }
	else { idx = SIZE_MAX; }
	return idx;
}

static size_t _neo_menu_group_idx_last(neo_menu_group_t* group, int position) {
	assert(group && group->max_entries >= (group->num_entries + 1));
	size_t idx = 0;
	if (position == -1) { idx = group->num_entries - 1; }
	else if (position >= 0 && position < group->num_entries) { idx = (size_t)position; }
	else { idx = SIZE_MAX; }
	return idx;
}

static size_t _neo_menu_bar_idx_end(neo_menu_bar_t* bar, int position) {
	assert(bar && bar->max_groups >= (bar->num_groups + 1));
	size_t idx = 0;
	if (position == -1) { idx = bar->num_groups; }
	else if (position >= 0 && position <= bar->num_groups) { idx = (size_t)position; }
	else { idx = SIZE_MAX; }
	return idx;
}

static size_t _neo_menu_bar_idx_last(neo_menu_bar_t* bar, int position) {
	assert(bar && bar->max_groups >= (bar->num_groups + 1));
	size_t idx = 0;
	if (position == -1) { idx = bar->num_groups - 1; }
	else if (position >= 0 && position < bar->num_groups) { idx = (size_t)position; }
	else { idx = SIZE_MAX; }
	return idx;
}

bool neo_menu_group_init(neo_menu_group_t* group) {
	// Validate group
	NEO_CLEAR_ERROR;
	if (!group) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu group");
		return false; 
	}

	// Initialize string
	if (!string_init(&group->name)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initialize menu group name");
		return false;
	}

	// Allocate memory
	size_t len = sizeof(*(group->entries)) * NEO_MENU_GROUP_DEFAULT_CAPACITY;
	group->entries = NEO_MALLOC(len);
	if (!group->entries) { 
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate menu entry buffer");
		return false; 
	}
	memset(group->entries, 0, len);
	group->num_entries = 0;
	group->max_entries = NEO_MENU_GROUP_DEFAULT_CAPACITY;
	group->selected = -1;
	return true;
}

void neo_menu_group_clear(neo_menu_group_t* group) {
	if (!group) { return; }
	string_clear(&group->name);
	if (group->entries) {
		for(size_t i = 0; i < group->num_entries; ++i) {
			neo_menu_entry_t* entry = &group->entries[i];
			if (!entry->seperator) { string_clear(&entry->name); }
		}
	}
	NEO_FREE(group->entries);
	group->entries = NULL;
	group->num_entries = 0;
	group->max_entries = 0;
	group->selected = -1;
}

neo_menu_entry_t* neo_menu_group_insert_entry(neo_menu_group_t* group, int position, const char *entry_name, char entry_shortcut, neo_menu_callback_fptr entry_callback) {
	// Validate group
	NEO_CLEAR_ERROR;
	size_t idx = 0;
	if (!_neo_menu_group_valid(group)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu group");
		return NULL; 
	}
	if ((idx = _neo_menu_group_idx_end(group, position)) == SIZE_MAX) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return NULL;
	}
	if (!_neo_menu_group_check_resize(group, 1)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize menu group");
		return NULL;
	}

	// Create string
	string_t name_str = {0};
	if (!string_init(&name_str)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initialize menu entry name");
		return NULL;
	}
	if (!string_set(&name_str, 0, entry_name, strlen(entry_name))) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to set menu entry name");
		return NULL;
	}

	// Shift end of group forward
	size_t n = sizeof(*(group->entries));
	memmove(group->entries + (n * (idx + 1)), group->entries + (n * idx), n * (group->num_entries - idx));

	// Create entry
	neo_menu_entry_t* entry = &group->entries[idx];
	entry->name = name_str;
	entry->shortcut = entry_shortcut;
	entry->callback = entry_callback;
	entry->seperator = false;
	group->num_entries++;
	return entry;
}

void neo_menu_group_insert_seperator(neo_menu_group_t* group, int position) {
	// Validate group
	NEO_CLEAR_ERROR;
	size_t idx = 0;
	if (!_neo_menu_group_valid(group)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu group");
		return;
	}
	if ((idx = _neo_menu_group_idx_end(group, position)) == SIZE_MAX) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return;
	}
	if (!_neo_menu_group_check_resize(group, 1)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize menu group");
		return;
	}

	// Shift end of group forward
	size_t n = sizeof(*(group->entries));
	memmove(group->entries + (n * (idx + 1)), group->entries + (n * idx), n * (group->num_entries - idx));

	// Create blank entry
	neo_menu_entry_t* entry = &group->entries[idx];
	memset(entry, 0, sizeof(*entry));
	entry->seperator = true;
	group->num_entries++;
}

neo_menu_entry_t* neo_menu_group_get_entry(neo_menu_group_t* group, int position) {
	// Validate group
	NEO_CLEAR_ERROR;
	if (!_neo_menu_group_valid(group)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu group");
		return NULL;
	}
	size_t idx = 0;
	if ((idx = _neo_menu_group_idx_last(group, position)) == SIZE_MAX) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return NULL;
	}

	// Get entry
	return &group->entries[idx];
}

int neo_menu_group_get_entry_position(neo_menu_group_t *group, const char *name) {
	// Validate group
	NEO_CLEAR_ERROR;
	if (!_neo_menu_group_valid(group)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu group");
		return -1;
	}

	// Iterate over entries
	for(size_t i = 0; i < group->num_entries; ++i) {
		if (strcmp(group->entries[i].name.data, name) == 0) {
			return (int)i;
		}
	}
	return -1;
}

void neo_menu_group_remove_entry(neo_menu_group_t *group, int position) {
	// Validate group
	NEO_CLEAR_ERROR;
	size_t idx = 0;
	if (!_neo_menu_group_valid(group)) { 
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu group");
		return;
	}
	if ((idx = _neo_menu_group_idx_last(group, position)) == SIZE_MAX) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return;
	}
	if (group->num_entries == 0) { return; }

	// Clear entry
	neo_menu_entry_t* entry = &group->entries[idx];
	if (!entry->seperator) { string_clear(&entry->name); }

	// Shift end of group backward
	size_t n = sizeof(*(group->entries));
	memmove(group->entries + (n * idx), group->entries + (n * (idx + 1)), n * (group->num_entries - idx - 1));
	group->num_entries--;
}

bool neo_menu_bar_init(neo_menu_bar_t *bar) {
	// Validate bar
	NEO_CLEAR_ERROR;
	if (!bar) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu bar");
		return false;
	}

	// Initialize memory
	size_t len = sizeof(*(bar->groups)) * NEO_MENU_BAR_DEFAULT_CAPACITY;
	bar->groups = NEO_MALLOC(len);
	if (!bar->groups) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to allocate menu group buffer");
		return false;
	}
	memset(bar->groups, 0, len);
	bar->num_groups = 0;
	bar->max_groups = NEO_MENU_BAR_DEFAULT_CAPACITY;
	bar->selected = -1;

	// Create default menu groups

	return true;
}

void neo_menu_bar_clear(neo_menu_bar_t *bar) {
	if (!bar) { return; }
	if (bar->groups) {
		for(size_t i = 0; i < bar->num_groups; ++i) {
			neo_menu_group_clear(&bar->groups[i]);
		}
	}
	NEO_FREE(bar->groups);
	bar->groups = NULL;
	bar->num_groups = 0;
	bar->max_groups = 0;
	bar->selected = -1;
}

neo_menu_group_t* neo_menu_bar_insert_group(neo_menu_bar_t *bar, int position, const char *group_name, char group_shortcut) {
	// Validate bar
	NEO_CLEAR_ERROR;
	size_t idx = 0;
	if (!_neo_menu_bar_valid(bar)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu bar");
		return NULL;
	}
	if ((idx = _neo_menu_bar_idx_end(bar, position)) != SIZE_MAX) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return NULL;
	}
	if (!_neo_menu_bar_check_resize(bar, 1)) {
		NEO_THROW_ERROR_MSG(NERROR_BAD_ALLOC, "Failed to resize menu bar");
		return NULL;
	}

	// Initialize group
	neo_menu_group_t* new_group = {0};
	if (!neo_menu_group_init(&new_group)) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to initialize menu group");
		return NULL;
	}
	if (!string_set(&(new_group->name), 0, group_name, strlen(group_name))) {
		NEO_THROW_ERROR_MSG(NERROR_GENERIC, "Failed to set menu group name");
		return NULL;
	}
	new_group->shortcut = group_shortcut;

	// Shift end of group forward
	size_t n = sizeof(*(bar->groups));
	memmove(bar->groups + (n * (idx + 1)), bar->groups + (n * idx), n * (bar->num_groups - idx));
	memcpy(&bar->groups[idx], &new_group, n);
	return true;
}

neo_menu_group_t* neo_menu_bar_get_group(neo_menu_bar_t* bar, int position) {
	// Validate bar
	NEO_CLEAR_ERROR;
	if (!_neo_menu_bar_valid(bar)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu bar");
		return NULL;
	}
	size_t idx = 0;
	if ((idx = _neo_menu_bar_idx_last(bar, position)) == SIZE_MAX) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return NULL;
	}

	// Get group
	return &bar->groups[idx];
}

int neo_menu_bar_get_group_position(neo_menu_bar_t* bar, const char* name) {
	// Validate bar
	NEO_CLEAR_ERROR;
	if (!_neo_menu_bar_valid(bar)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu bar");
		return -1;
	}

	// Iterate over entries
	for(size_t i = 0; i < bar->num_groups; ++i) {
		if (strcmp(bar->groups[i].name.data, name) == 0) {
			return (int)i;
		}
	}
	return -1;
}

void neo_menu_bar_remove_group(neo_menu_bar_t* bar, int position) {
	// Validate group
	NEO_CLEAR_ERROR;
	size_t idx = 0;
	if (!_neo_menu_bar_valid(bar)) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Invalid menu bar");
		return;
	}
	if ((idx = _neo_menu_bar_idx_last(bar, position)) == SIZE_MAX) {
		NEO_THROW_ERROR_MSG(NERROR_INVALID_PARAM, "Position out of bounds");
		return;
	}
	if (bar->num_groups == 0) { return; }

	// Clear entry
	neo_menu_group_t* group = &bar->groups[idx];
	neo_menu_group_clear(&group);

	// Shift end of group backward
	size_t n = sizeof(*(bar->groups));
	memmove(bar->groups + (n * idx), bar->groups + (n * (idx + 1)), n * (bar->num_groups - idx - 1));
	bar->num_groups--;
}
