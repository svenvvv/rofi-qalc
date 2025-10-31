/*
 * rofi-qalculate
 * Copyright (C) 2024 svenvvv
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "rofi_hacks.h"
#include "qalc.h"

#include <rofi/mode.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "rq"

using namespace rq;

typedef void (*MenuEntryCallback)(RofiQalc & state, MenuReturn action);

struct rq_menu_entry
{
    char const * title;
    MenuEntryCallback callback;
};

static RofiQalc * get_state_ptr(Mode const * sw)
{
    auto * ptr = static_cast<RofiQalc*>(mode_get_private_data(sw));
    if (ptr == nullptr) {
        g_error("State is nullptr");
    }
    return ptr;
}

static RofiQalc & get_state(Mode const * sw)
{
    return *get_state_ptr(sw);
}

static void menu_entry_save_history(RofiQalc & state, MenuReturn action)
{
    if (state.result_is_error) {
        g_debug("Result is error, not saving to history");
        return;
    }
    if (state.is_plot_open()) {
        g_debug("Result is plot, not saving to history");
        return;
    }

    if (!state.options.no_history) {
        state.append_result_to_history(action == MENU_OK);
    }

    state.update_ans();

    if (!state.options.no_auto_clear_filter) {
        state.clear_last_expression();
        // We can't clear it in the same thread, so lets leave it up to async
        state.textbox_clear_fut = std::async(std::launch::async, []() {
            rofi_view_trigger_action(rofi_view_get_active(), GLOBAL, CLEAR_LINE);
            rofi_view_reload();
        });
    }
}

static constexpr rq_menu_entry menu_entries[] = {
    { "Add to history",     menu_entry_save_history },
    // { "Clear variables",    menu_entry_save_history },
};

static int selected_line_to_history_index(RofiQalc const & state, unsigned selected_line)
{
    return state.history.size() - (selected_line - std::size(menu_entries)) - 1;
}

static int rq_mode_init(Mode * sw)
{
    g_debug("Initializing calc state...");

    if (mode_get_private_data(sw) == nullptr) {
        auto * state = new RofiQalc();

        if (!state->options.no_history) {
            state->load_history();
        }

        mode_set_private_data(sw, state);
    }

    g_debug("Successfully initialized calc state");
    return TRUE;
}

static unsigned int rq_mode_get_num_entries(Mode const * sw)
{
    return std::size(menu_entries) + get_state(sw).history.size();
}

static ModeMode rq_mode_result(Mode * sw, int menu_entry,
                               G_GNUC_UNUSED char ** input, unsigned selected_line)
{
    auto & state = get_state(sw);

    if (menu_entry & MENU_NEXT) {
        return NEXT_DIALOG;
    }
    if (menu_entry & MENU_PREVIOUS) {
        return PREVIOUS_DIALOG;
    }
    if (menu_entry & MENU_QUICK_SWITCH) {
        return static_cast<ModeMode>(menu_entry & MENU_LOWER_MASK);
    }
    if (menu_entry & MENU_OK) {
        if (selected_line < std::size(menu_entries)) {
            menu_entries[selected_line].callback(state, MENU_OK);
        } else {
            // struct rq_history_entry const * entry = &state->history[0];
            // memcpy(state->append_postfix, entry->result, entry->result_len);
            // state->append_postfix_length = entry->result_len;
        }
        return RELOAD_DIALOG;
    }
    if (menu_entry & MENU_ENTRY_DELETE) {
        if (selected_line >= std::size(menu_entries)) {
            int entry_index = selected_line_to_history_index(state, selected_line);
            if (entry_index >= 0) {
                state.erase_history_line(entry_index);
            }
        }
        return RELOAD_DIALOG;
    }
    if (menu_entry & MENU_CUSTOM_INPUT) {
        menu_entries[0].callback(state, MENU_CUSTOM_INPUT);
        return RELOAD_DIALOG;
    }

    return MODE_EXIT;
}

static void rq_mode_destroy(Mode * sw)
{
    auto & state = get_state(sw);

    if (!state.options.no_history) {
        if (state.options.auto_save_last_to_history) {
            state.append_result_to_history();
        }

        state.save_history();
    }

    delete get_state_ptr(sw);
    mode_set_private_data(sw, nullptr);
}

static int rq_mode_token_match(G_GNUC_UNUSED Mode const * sw,
                               G_GNUC_UNUSED rofi_int_matcher ** tokens,
                               G_GNUC_UNUSED unsigned index)
{
    return TRUE;
}

static char* rq_mode_get_display_value(Mode const * sw, unsigned selected_line,
                                       G_GNUC_UNUSED int * mode_state,
                                       G_GNUC_UNUSED GList ** attr_list, int get_entry)
{
    if (!get_entry) {
        return nullptr;
    }

    auto const & state = get_state(sw);

    if (selected_line < std::size(menu_entries)) {
        return g_strdup(menu_entries[selected_line].title);
    }

    int entry_index = selected_line_to_history_index(state, selected_line);
    if (entry_index < 0) {
        return nullptr;
    }
    auto const & entry = state.history[entry_index];

    auto entry_str = entry.print();
    if (!entry.persistent) {
        entry_str = "(tmp) " + entry_str;
    }

    auto * history_line = static_cast<gchar*>(g_malloc(entry_str.length() + 1));
    if (history_line == nullptr) {
        g_error("Failed to allocate history line");
    }
    memcpy(history_line, entry_str.c_str(), entry_str.length());
    history_line[entry_str.length()] = 0;

    return history_line;
}

char * rq_mode_get_completion(Mode const * sw, unsigned selected_line) {
    auto const & state = get_state(sw);

    if (selected_line < std::size(menu_entries)) {
        // A bit pointless to return this, but I'm really not sure what else to do here :-)
        return g_strdup(menu_entries[selected_line].title);
    }

    int entry_index = selected_line_to_history_index(state, selected_line);
    if (entry_index < 0) {
        return nullptr;
    }

    auto const & selected_entry = state.history[entry_index];
    auto const last_expression = state.get_last_expression();
    auto * history_line = static_cast<gchar*>(
        g_malloc(last_expression.length() + selected_entry.result.length() + 1));
    if (history_line == nullptr) {
        g_error("Failed to allocate history line");
    }

    memcpy(history_line, last_expression.begin(), last_expression.length());
    memcpy(history_line + last_expression.length(), selected_entry.result.c_str(),
        selected_entry.result.length());
    history_line[last_expression.length() + selected_entry.result.length()] = 0;

    return history_line;
}

static char *rq_mode_get_message(Mode const * sw)
{
    auto const & state = get_state(sw);

    if (state.is_eval_in_progress()) {
        return g_strdup("Evaluating...");
    }
    if (state.result_is_error) {
        return g_strdup_printf("Error: <b>%s</b>", state.previous_result.c_str());
    }
    if (state.is_plot_open()) {
        return g_strdup("Plot mode active");
    }
    if (!state.previous_result.empty()) {
        return g_strdup_printf("Result: <b>%s</b>", state.previous_result.c_str());
    }

    return g_strdup("Enter expression");
}

static void eval_callback(std::string const & result,
                          std::optional<std::string> const & error, void * userdata)
{
    auto * state = static_cast<RofiQalc*>(userdata);
    g_info("Reloading view %s", state->previous_result.c_str());

    state->previous_result = error.value_or(result);
    state->result_is_error = error.has_value();

    rofi_view_reload();
}

static char * rq_mode_preprocess_input(Mode * sw, char const * input)
{
    auto & state = get_state(sw);

    g_info("Preprocess input %s", input);

    state.evaluate(input, eval_callback, &state);

    rofi_view_reload();

    return g_strdup(input);
}

G_MODULE_EXPORT Mode mode =
{
    .abi_version                = ABI_VERSION,
    .name                       = const_cast<char*>(ROFIQALC_MODE_NAME),
    .cfg_name_key               = "display-qalculate",
    .display_name               = nullptr,

    ._init                      = rq_mode_init,
    ._destroy                   = rq_mode_destroy,
    ._get_num_entries           = rq_mode_get_num_entries,
    ._result                    = rq_mode_result,
    ._token_match               = rq_mode_token_match,
    ._get_display_value         = rq_mode_get_display_value,
    ._get_icon                  = nullptr,
    ._get_completion            = rq_mode_get_completion,
    ._preprocess_input          = rq_mode_preprocess_input,
    ._get_message               = rq_mode_get_message,

    .private_data               = nullptr,
    .free                       = nullptr,
#ifdef RQ_ROFI_NEXT
    ._create                    = nullptr,
    ._completer_result          = nullptr,
#endif
    .ed                         = nullptr,
    .module                     = nullptr,
    .fallback_icon_fetch_uid    = 0,
    .fallback_icon_not_found    = 0,
#ifdef RQ_ROFI_NEXT
    .type                       = MODE_TYPE_DMENU,
#endif
};
