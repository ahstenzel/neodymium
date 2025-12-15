/**
 * @file neo_actions.h
 * @brief Definitions for editor callbacks and modal dialogs.
 */

#ifndef NEO_ACTIONS_H
#define NEO_ACTIONS_H

#include "neo_editor.h"
#include "neo_menu.h"
#include "neo_string.h"

/**
 * @brief Callback function for a menu entry.
 */
typedef int (*neo_menu_callback_fptr)(neo_edit_ctx_t*);

int neo_cb_new_file(neo_edit_ctx_t* context);

int neo_cb_open_file(neo_edit_ctx_t* context);

int neo_cb_save_file(neo_edit_ctx_t* context);

int neo_cb_save_file_as(neo_edit_ctx_t* context);

int neo_cb_save_all_file(neo_edit_ctx_t* context);

int neo_cb_next_page(neo_edit_ctx_t* context);

int neo_cb_prev_page(neo_edit_ctx_t* context);

int neo_cb_close_page(neo_edit_ctx_t* context);

int neo_cb_quit(neo_edit_ctx_t* context);

int neo_cb_cut(neo_edit_ctx_t* context);

int neo_cb_copy(neo_edit_ctx_t* context);

int neo_cb_paste(neo_edit_ctx_t* context);

int neo_cb_duplicate(neo_edit_ctx_t* context);

int neo_cb_select_all(neo_edit_ctx_t* context);

int neo_cb_undo(neo_edit_ctx_t* context);

int neo_cb_redo(neo_edit_ctx_t* context);

int neo_cb_about(neo_edit_ctx_t* context);

#endif