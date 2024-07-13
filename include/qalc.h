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

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "options.h"

class Calculator;
class MathStructure;
class KnownVariable;

namespace rq
{

class RofiQalc;

typedef void (*EvalCallback)(std::string const & result,
                             std::optional<std::string> error, void * userdata);

struct ExpressionQuery
{
    /** Expression to calculate */
    std::string expression;
    /** Callback to call after evaluation is finished successfully */
    EvalCallback callback = nullptr;
    /** User data passed to callback */
    void * userdata = nullptr;
};

struct ThreadData
{
    ThreadData();

    std::unique_ptr<Calculator> calc;
    // std::binary_semaphore eval_sem{0};
    std::unique_ptr<MathStructure> last_result;
    std::atomic<bool> has_new_data;
    /** Indicates whether GNUplot is currently open */
    bool is_plot_open = false;
    std::atomic<bool> eval_in_progress;

    std::mutex mtx_queued_query;
    ExpressionQuery queued_query;
};

class RofiQalc
{
public:
    RofiQalc();
    ~RofiQalc();

    void append_result_to_history(void);
    void load_history(void);
    void save_history(void) const;

    void update_ans(void);
    void evaluate(std::string_view const & expr, EvalCallback callback, void * userdata);

    bool is_plot_open(void) const
    {
        return _thread_data.is_plot_open;
    }

    bool is_eval_in_progress(void) const
    {
        return _thread_data.eval_in_progress.load();
    }

public:
    /** Command-line options for the mode */
    Options options;
    /** History contents */
    std::vector<std::string> history;

    /** Result of the last successful evaluate() call */
    std::string result;
    /** Whether the result contains an error string */
    bool result_is_error = false;

protected:
    static void calculate_and_print(RofiQalc const & state, ThreadData & data);

protected:
    /** Calculator thread */
    std::thread _thread;
    /** Data written to by the calculator thread */
    ThreadData _thread_data;
    /** Whether the calculator thread should quit */
    std::atomic<bool> _should_quit = false;

    /** Last expression to be passed into evaluate() */
    std::string _last_expr;
    /** Hash of the previous expression, used for skipping double-calculation */
    size_t _last_expr_hash = 0;

    /** ansn variables used in libqalc Calculator */
    KnownVariable * _var_ans[5];
};

} /* namespace rq */
