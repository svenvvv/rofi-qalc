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
static char const * const opt_no_persist_history = "-no-persist-history";
static char const * const opt_no_history = "-no-history";
static char const * const opt_auto_save_history = "-automatic-save-to-history";
static char const * const opt_history_length = "-history-length";
static char const * const opt_no_auto_clear_filter = "-no-auto-clear-filter";
static char const * const opt_no_load_history_variables = "-no-load-history-variables";
static char const * const opt_dump_local_variables = "-dump-local-variables";
static char const * const opt_message_severity = "-message-severity";

Options::Options()
{
    this->no_persist_history = find_arg(opt_no_persist_history) != -1;
    this->no_history = find_arg(opt_no_history) != -1;
    this->auto_save_last_to_history = find_arg(opt_auto_save_history) != -1;
    this->no_auto_clear_filter = find_arg(opt_no_auto_clear_filter) != -1;
    this->no_load_history_variables = find_arg(opt_no_load_history_variables) != -1;
    this->dump_local_variables = find_arg(opt_dump_local_variables) != -1;

    find_arg_uint(opt_history_length, &this->history_length);
    find_arg_int(opt_eval_timeout_ms, &this->eval_timeout_ms);
    find_arg_uint(opt_message_severity, &this->message_severity);

    g_debug("Parsed options:");
    g_debug("  no_persist_history = %d", this->no_persist_history);
    g_debug("  no_history = %d", this->no_history);
    g_debug("  auto_save_last_to_history = %d", this->auto_save_last_to_history);
    g_debug("  history_length = %u", this->history_length);
    g_debug("  eval_timeout_ms = %i", this->eval_timeout_ms);
    g_debug("  no_load_history_variables = %i", this->no_load_history_variables);
    g_debug("  dump_local_variables = %i", this->dump_local_variables);
    g_debug("  message_severity = %i", this->message_severity);
}
