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
#pragma once

#include "options.h"
#include "qalc_thread.h"

#include <future>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifndef ROFIQALC_MODE_NAME
#define ROFIQALC_MODE_NAME "qalc"
#endif

class KnownVariable;

namespace rq
{

struct HistoryEntry
{
    std::string expression;
    std::string result;
    /** Indicates whether the history entry should be persistent (stored in the history file) */
    bool persistent;

    [[nodiscard]]
    std::string print() const
    {
        std::stringstream ss;
        ss << expression << separator << result;
        return ss.str();
    }

    static constexpr std::string_view separator = " = ";
};

class RofiQalc
{
public:
    RofiQalc();
    ~RofiQalc();

    void append_result_to_history(bool persistent=true);
    void load_history();
    void save_history() const;

    void update_ans();
    void evaluate(std::string_view const & expr, EvalCallback callback, void * userdata);

    [[nodiscard]]
    bool is_plot_open() const
    {
        return _thread_data.is_plot_open;
    }

    [[nodiscard]]
    bool is_eval_in_progress() const
    {
        return _thread_data.eval_in_progress.load();
    }

public:
    /** Command-line options for the mode */
    Options options;
    /** History contents */
    std::vector<HistoryEntry> history;

    /** Result of the last successful evaluate() call */
    std::string previous_result;
    /** Whether the result contains an error string */
    bool result_is_error = false;

    /** Ugly hack, see usage rofi_shim.cpp */
    std::future<void> textbox_clear_fut;

protected:
    static void calculator_thread_entry(ThreadData & data);

protected:
    /** Calculator thread */
    std::thread _thread;
    /** Data written to by the calculator thread */
    ThreadData _thread_data { options };

    /** Last expression to be passed into evaluate() */
    std::string _last_expr;
    /** Hash of the previous expression, used for skipping double-calculation */
    size_t _last_expr_hash = 0;

    /** ansn variables used in libqalc Calculator */
    KnownVariable * _var_ans[5];
};

} /* namespace rq */
