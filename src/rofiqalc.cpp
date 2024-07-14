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
#include <sstream>
#include <gmodule.h>
#include <libqalculate/qalculate.h>
#include <cstring>
#include <numeric>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "rq"

#include "qalc.h"

using namespace rq;

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

static inline gchar * get_config_basedir(void)
{
    return g_build_filename(g_get_user_data_dir(), "rofi", NULL);
}

static inline gchar * get_config_history_filename(gchar const * basedir)
{
    return g_build_filename(basedir, "rofi_calc_history", NULL);
}

// static inline gchar * get_config_variables_filename(gchar const * basedir)
// {
//     return g_build_filename(basedir, "rofi_qalc_variables", NULL);
// }

void RofiQalc::append_result_to_history(bool persistent)
{
    if (this->_last_expr.length() == 0 || this->result.length() == 0) {
        g_debug("Not appending result to history, no data");
        return;
    }
    if (this->history.size() == this->options.history_length) {
        this->history.erase(this->history.begin());
    }

    g_debug("Appending %s = %s to history",
            this->_last_expr.c_str(), this->result.c_str());

    this->history.emplace_back(this->_last_expr, this->result, persistent);
}


void RofiQalc::load_history(void)
{
    GError * error = NULL;
    gchar * history_dir = get_config_basedir();
    gchar * history_file = get_config_history_filename(history_dir);
    gchar * history_data = NULL;
    gsize history_size;

    g_debug("Loading history from %s", history_file);

    this->history.clear();

    g_mkdir_with_parents(history_dir, 0755);
    if (g_file_test(history_file, (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
        g_file_get_contents(history_file, &history_data, &history_size, &error);
        if (error != NULL) {
            g_error("Failed to read history file: %s", error->message);
            g_error_free(error);
        }

        g_debug("History file is %lu b", history_size);

        gchar * head = history_data;
        while (head < history_data + history_size) {
            gchar * newline =
                reinterpret_cast<gchar*>(memchr(head, '\n', history_size - (head - history_data)));
            if (newline == NULL) {
                newline = history_data + history_size;
            }
            // g_debug("history line %lu (%lu) %.*s",
            //         state->history_used, newline - head, (int)(newline - head), head);

            std::string_view line{head, size_t(newline - head)};
            std::string_view expression;
            std::string_view result;

            auto separator_pos = line.rfind(HistoryEntry::separator);
            if (separator_pos != std::string::npos) {
                expression = line.substr(0, separator_pos);
                result = line.substr(separator_pos + HistoryEntry::separator.length());
            } else {
                expression = "";
                result = line;
            }

            this->history.emplace_back(std::string{expression}, std::string{result}, true);
            if (this->history.size() == this->options.history_length) {
                g_warning("History file reading stopped, file longer than history_length");
                break;
            }

            head = newline + 1;
        }
    }

    g_free(history_data);
    g_free(history_file);
    g_free(history_dir);
}

void RofiQalc::save_history(void) const
{
    GError * error = NULL;
    gchar * history_dir = get_config_basedir();
    gchar * history_file = get_config_history_filename(history_dir);


    auto accumulator = [](size_t acc, HistoryEntry const & e) {
        if (!e.persistent) {
            return acc;
        }
        return acc + e.print().length() + 1;
    };
    size_t filesize = std::accumulate(this->history.begin(), this->history.end(), 0, accumulator);
    g_debug("Saving history to %s, %lu b", history_file, filesize);

    gchar * history_data = reinterpret_cast<gchar*>(g_malloc(filesize));
    size_t pos = 0;
    for (auto const & entry : this->history) {
        if (!entry.persistent) {
            continue;
        }
        auto line = entry.print();
        std::memcpy(history_data + pos, line.c_str(), line.length());
        history_data[pos + line.length()] = '\n';
        pos += line.length() + 1;
    }

    g_file_set_contents(history_file, history_data, filesize, &error);
    if (error != NULL) {
        g_error("Failed to write history file: %s", error->message);
        g_error_free(error);
    }

    g_free(history_data);
    g_free(history_file);
    g_free(history_dir);
}

ThreadData::ThreadData()
    : calc(new Calculator())
    , last_result(new MathStructure())
    , has_new_data(false)
    , queued_query({})
{
}

void RofiQalc::calculate_and_print(RofiQalc const & state, ThreadData & data)
{
    EvaluationOptions eo = default_evaluation_options;
    PrintOptions po = default_print_options;
    po.use_unicode_signs = true;

    bool btrue = true;
    bool bfalse = false;

    for (;;) {
        data.has_new_data.wait(false);
        data.has_new_data.compare_exchange_weak(btrue, bfalse);

        ExpressionQuery query;
        MathStructure ms;
        std::string result = "";
        std::string unlocalized_expr = "";
        std::optional<std::string> error = std::nullopt;
        bool is_plot_query;
        /* Divided by 2 due to the hack, read below */
        size_t eval_timeout_ms = state.options.eval_timeout_ms / 2;

        if (state._should_quit.load()) {
            break;
        }

        data.eval_in_progress = true;
        {
            std::lock_guard<std::mutex> lock(data.mtx_queued_query);
            query = data.queued_query;
        }

        g_debug("Evaluating %s...", query.expression.c_str());

        if (query.callback == nullptr) {
            g_debug("Missing callback!");
            error = std::make_optional("Missing callback");
            goto exit;
        }

        unlocalized_expr = data.calc->unlocalizeExpression(query.expression);
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
        if (!data.calc->calculate(&ms, unlocalized_expr, eval_timeout_ms, eo)) {
            g_debug("Timed out!");
            error = std::make_optional("Evaluation timed out");
            goto exit;
        }

        // calculateAndPrint doesn't show plots??
        if (is_plot_query) {
            result = data.calc->print(ms, eval_timeout_ms, po);
        } else {
            result = data.calc->calculateAndPrint(query.expression, eval_timeout_ms, eo, po);
            // No way to tell whether it was successful??
            if (result.length() == 0 && query.expression.length() != 0) {
                g_debug("Failed to calculate");
                error = std::make_optional("Failed to calculate");
                goto exit;
            }
        }
        g_debug("Finished evaluation");

        data.is_plot_open = data.calc->gnuplotOpen();
        if (data.is_plot_open && !is_plot_query){
            data.calc->closeGnuplot();
        }

        // for (auto * var : data.calc->variables) {
        //     if (var->isKnown() && !var->isBuiltin() && var->isActive()) {
        //         KnownVariable * kv = (KnownVariable*)var;
        //         std::cout << "var: " << kv->name(0) << " = " << kv->get() << "\n";
        //     }
        // }
        // std::this_thread::sleep_for(std::chrono::seconds(1));

        data.last_result->set(ms);

exit:
        data.eval_in_progress = false;
        query.callback(result, error, query.userdata);
    }
}

RofiQalc::RofiQalc()
{
    auto & calc = this->_thread_data.calc;

    if (!calc->loadExchangeRates()) {
        g_warning("Failed to load exchange rates");
    }
    if (!calc->loadGlobalDefinitions()) {
        g_warning("Failed to load global definitions");
    }
    if (!calc->loadLocalDefinitions()) {
        g_warning("Failed to load local definitions");
    }

    // Set up ans variables
    std::string const ans_str = "ans";
    for (int i = 0; i < 5; ++i) {
        auto index_str = std::to_string(i + 1);
        auto * kv = new KnownVariable(calc->temporaryCategory(),
                                      ans_str + index_str, m_undefined,
                                      "Answer " + index_str, false, true);
        this->_var_ans[i] = static_cast<KnownVariable*>(calc->addVariable(kv));
    }
	this->_var_ans[0]->addName("answer");
	this->_var_ans[0]->addName(ans_str);

    _thread = std::thread{calculate_and_print, std::ref(*this), std::ref(this->_thread_data)};
}

RofiQalc::~RofiQalc()
{
    this->_should_quit = true;
    this->_thread_data.has_new_data = true;
    this->_thread_data.has_new_data.notify_one();
    this->_thread.join();
}

void RofiQalc::evaluate(std::string_view const & expr, EvalCallback callback, void * userdata)
{
    std::hash<std::string_view> const hasher;
    size_t hash = hasher(expr);

    if (hash == this->_last_expr_hash) {
        return;
    }
    this->_last_expr_hash = hash;
    this->_last_expr = expr;

    {
        std::lock_guard<std::mutex> lock(this->_thread_data.mtx_queued_query);
        this->_thread_data.queued_query.expression = expr;
        this->_thread_data.queued_query.callback = callback;
        this->_thread_data.queued_query.userdata = userdata;
    }
    this->_thread_data.has_new_data = true;
    this->_thread_data.has_new_data.notify_one();
}

void RofiQalc::update_ans()
{
    MathStructure m4(this->_var_ans[3]->get());
    m4.replace(this->_var_ans[4], this->_var_ans[4]->get());
    this->_var_ans[4]->set(m4);

    MathStructure m3(this->_var_ans[2]->get());
    m3.replace(this->_var_ans[3], this->_var_ans[4]);
    this->_var_ans[3]->set(m3);

    MathStructure m2(this->_var_ans[1]->get());
    m2.replace(this->_var_ans[2], this->_var_ans[3]);
    this->_var_ans[2]->set(m2);

    MathStructure m1(this->_var_ans[0]->get());
    m1.replace(this->_var_ans[1], this->_var_ans[2]);
    this->_var_ans[1]->set(m1);

    this->_thread_data.last_result->replace(this->_var_ans[0], this->_var_ans[1]);
    this->_var_ans[0]->set(*this->_thread_data.last_result);
}
