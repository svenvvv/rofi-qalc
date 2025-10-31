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
#include "qalc.h"

#include <gmodule.h>
#include <libqalculate/qalculate.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "rq"

using namespace rq;

ThreadData::ThreadData(Options const & options)
    : calc(std::make_unique<Calculator>())
    , last_result(std::make_unique<MathStructure>())
    , options(options)
    , has_new_data(false)
    , queued_query({})
{
}

static constexpr bool starts_with(char const * const haystack, std::string const & needle)
{
    bool gobble_whitespace = true;
    char const * ptr = haystack;
    std::string::size_type pos = 0;
    while (*ptr != 0) {
        if (std::isspace(*ptr) && gobble_whitespace) {
            ptr += 1;
            continue;
        }
        gobble_whitespace = false;

        if (std::tolower(*ptr) != std::tolower(needle.c_str()[pos])) {
            return false;
        }

        pos += 1;
        ptr += 1;

        if (pos == needle.length()) {
            return true;
        }
    }

    return false;
}

/**
 * Calculator thread entrypoint.
 * A separate thread is used call libqalculate since some expressions can take a while to
 * evaluate (and block the main thread). As the main thread is shared with rofi, then that also
 * blocks the entire rofi window.
 * @param data Thread data
 */
void RofiQalc::_calculator_thread_entry(ThreadData & data)
{
    auto & calc = data.calc;
    EvaluationOptions const & eo = default_evaluation_options;
    PrintOptions po = default_print_options;
    std::vector<LogMessage> log_messages;

    po.use_unicode_signs = true;
    po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;

    bool btrue = true;
    bool bfalse = false;

    for (;;) {
        data.has_new_data.wait(false);
        data.has_new_data.compare_exchange_weak(btrue, bfalse);

        ExpressionQuery query;
        MathStructure ms;
        std::string result;
        std::string unlocalized_expr;
        bool is_plot_query;
        /* Divided by 2 due to the hack, read below */
        int eval_timeout_ms = data.options.eval_timeout_ms / 2;

        if (data.should_quit.load()) {
            break;
        }

        data.eval_in_progress = true;
        {
            std::lock_guard lock(data.mtx_queued_query);
            query = data.queued_query;
        }

        log_messages.clear();
        g_debug("Evaluating %s...", query.expression.c_str());

        if (query.callback == nullptr) {
            g_warning("Missing callback!");
            log_messages.emplace_back(ERROR, "Missing callback");
            goto exit;
        }

        unlocalized_expr = calc->unlocalizeExpression(query.expression);
        is_plot_query = starts_with(unlocalized_expr.c_str(), "plot(");

        /*
         * A bit hackish, but:
         *  - calculate() and print() don't support "to" expressions;
         *  - calculateAndPrint() doesn't support plotting;
         *  - calculateAndPrint() doesn't give out the resulting MathStructure (for ans vars).
         * Looking through the libqalculate code it looks like I'd have to lift a huge chunk of
         * code to make calculate() and print() support "to" expressions.
         *
         * So instead let's do a dirty hack and just run it through calculate() first to get the
         * MathStructure, then (if not a plot) then calculate it again in calculateAndPrint().
         * Should be fine (for my use case :)) since I only use the rofi calc for simple
         * calculations.
         */
        if (!calc->calculate(&ms, unlocalized_expr, eval_timeout_ms, eo)) {
            g_info("Timed out after %d ms!", eval_timeout_ms);
            log_messages.emplace_back(ERROR, "Evaluation timed out after {} ms", eval_timeout_ms);
            goto exit;
        }

        // calculateAndPrint doesn't show plots??
        if (is_plot_query) {
            result = calc->print(ms, eval_timeout_ms, po);
        } else {
            result = calc->calculateAndPrint(query.expression, eval_timeout_ms, eo, po);

            std::string warning;
            while (calc->message()) {
                auto const & msg = *calc->message();
                g_info("libqalculate message (%d): %s", msg.type(), msg.c_message());
                log_messages.emplace_back(msg);
                calc->nextMessage();
            }
        }
        g_debug("Finished evaluation");

        data.is_plot_open = calc->gnuplotOpen();
        if (data.is_plot_open && !is_plot_query){
            calc->closeGnuplot();
        }

        if (data.options.dump_local_variables) {
            for (auto * var : calc->variables) {
                if (var->isLocal()) {
                    auto * kv = dynamic_cast<KnownVariable *>(var);
                    g_debug("Local variable dump: \"%s\" has value \"%s\"",
                        kv->name(false).c_str(), kv->get().print().c_str());
                }
            }
        }

        data.last_result->set(ms);

exit:
        data.eval_in_progress = false;
        query.callback(result, log_messages, query.userdata);
    }
}

