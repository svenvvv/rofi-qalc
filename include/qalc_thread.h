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

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

class Calculator;
class MathStructure;

namespace rq
{

typedef void (*EvalCallback)(std::string const & result,
                             std::optional<std::string> const & error, void * userdata);

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
    explicit ThreadData(Options const & options);

    /** The libqalculate calculator stucture */
    std::unique_ptr<Calculator> calc;
    /** Last calculated result */
    std::unique_ptr<MathStructure> last_result;
    /** Indicates whether GNUplot is currently open */
    bool is_plot_open = false;
    /** Indicates whether evaluation is currently in progress */
    std::atomic<bool> eval_in_progress;
    /** Indicates whether the calculator thread should quit */
    std::atomic<bool> should_quit = false;
    /** Rofi mode options, so we don't have to pass a reference to RofiQalc to calc. thread */
    Options const & options;

    /** Tracks whether queued_query contains new data */
    std::atomic<bool> has_new_data;
    /** Mutex used to guard queued_query */
    std::mutex mtx_queued_query;
    /** Queued query, lock mtx_queued_query when accessing */
    ExpressionQuery queued_query;
};

} // namespace rc