/*
 * rofi-qalculate
 * Copyright (C) 2024-2025 svenvvv
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
#pragma once

#include "log_message.h"

namespace rq
{

struct Options
{
    Options();

    /** libqalculate evaluation timeout, in milliseconds */
    int eval_timeout_ms = 1000;
    /** Disable history file access */
    bool no_history;
    /** TODO */
    bool no_persist_history;
    /** Enable automatically saving last result to history */
    bool auto_save_last_to_history;
    /** Maximum number of history entries cached */
    unsigned history_length = 100;
    /** Whether to automatically clear the filter text after adding to history  */
    bool no_auto_clear_filter;
    /** Whether to not load variables into qalculate from history */
    bool no_load_history_variables;
    /**
     * Whether to print out local variables during evaluation.
     * @note You'll need to set G_MESSAGES_DEBUG=rq to see the messages
     */
    bool dump_local_variables;
    /**
     * Severity of messages to display in the rofi window.
     * See ::MessageType for values.
     */
    unsigned message_severity = ERROR;
};

} /* namespace rq */
