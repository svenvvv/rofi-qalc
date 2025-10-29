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
#include "options.h"

#include <rofi/helper.h>
#include <string>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN    ((gchar*) "rq")

using namespace rq;

static char const * const opt_eval_timeout_ms = "-eval-timeout-ms";
#define OPT_EVAL_TIMEOUT_MS_DEFAULT 1000
static char const * const opt_no_persist_history = "-no-persist-history";
static char const * const opt_no_history = "-no-history";
static char const * const opt_auto_save_history = "-automatic-save-to-history";
static char const * const opt_history_only_save_results = "-history-only-save-results";
static char const * const opt_history_length = "-history-length";
#define OPT_HISTORY_LENGTH_DEFAULT 100
static char const * const opt_no_auto_clear_filter = "-no-auto-clear-filter";

Options::Options()
{
    char * strarg = nullptr;

    this->no_persist_history = find_arg(opt_no_persist_history) != -1;
    this->no_history = find_arg(opt_no_history) != -1;
    this->auto_save_last_to_history = find_arg(opt_auto_save_history) != -1;
    this->history_only_save_results = find_arg(opt_history_only_save_results) != -1;
    this->no_auto_clear_filter = find_arg(opt_no_auto_clear_filter) != -1;

    if (find_arg_str(opt_history_length, &strarg) == TRUE) {
        this->history_length = std::stoul(strarg);
    } else {
        this->history_length = OPT_HISTORY_LENGTH_DEFAULT;
    }

    if (find_arg_str(opt_eval_timeout_ms, &strarg) == TRUE) {
        this->eval_timeout_ms = std::stoi(strarg);
    } else {
        this->eval_timeout_ms = OPT_EVAL_TIMEOUT_MS_DEFAULT;
    }

    g_debug("Parsed options:");
    g_debug("  no_persist_history = %d", this->no_persist_history);
    g_debug("  no_history = %d", this->no_history);
    g_debug("  auto_save_last_to_history = %d", this->auto_save_last_to_history);
    g_debug("  history_only_save_results = %d", this->history_only_save_results);
    g_debug("  history_length = %lu", this->history_length);
    g_debug("  eval_timeout_ms = %i", this->eval_timeout_ms);
}
