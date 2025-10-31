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
#include "parsing.h"

#include <algorithm>
#include <sstream>
#include <gmodule.h>
#include <libqalculate/qalculate.h>
#include <cstring>
#include <functional>
#include <numeric>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "rq"

using namespace rq;

static gchar * get_config_basedir()
{
    return g_build_filename(g_get_user_data_dir(), "rofi", NULL);
}

static gchar * get_config_history_filename(gchar const * basedir)
{
    return g_build_filename(basedir, "rofi_calc_history", NULL);
}

// static inline gchar * get_config_variables_filename(gchar const * basedir)
// {
//     return g_build_filename(basedir, "rofi_qalc_variables", NULL);
// }

void RofiQalc::append_result_to_history(bool persistent)
{
    if (this->_last_expr.empty() || this->previous_result.empty()) {
        g_debug("Not appending result to history, no data");
        return;
    }
    if (this->history.size() == this->options.history_length) {
        this->history.erase(this->history.begin());
    }

    // Check whether the expression is a save (e.g. "a = 20"), those don't have a result
    // as such. libqalculate does return the stored value as the answer, but we don't
    // want to save "a = 20 = 20" to history.
    bool is_save =
        expression_contains_save_function(this->_last_expr, default_parse_options, false);
    if (is_save) {
        g_debug("Appending variable \"%s\" to history", this->_last_expr.c_str());
        this->history.emplace_back(this->_last_expr, "", persistent, true);
    } else {
        g_debug("Appending \"%s\" = \"%s\" to history",
            this->_last_expr.c_str(), this->previous_result.c_str());
        this->history.emplace_back(this->_last_expr, this->previous_result, persistent, false);
    }
}

void RofiQalc::_load_history_variable_into_qalculate(std::string const & history_line)
{
    auto & calc = this->_thread_data.calc;

    auto const parsed_variable_opt = parsing::parse_variable_parts(history_line);
    if (!parsed_variable_opt.has_value()) {
        return;
    }
    auto const &[name, value] = parsed_variable_opt.value();

    g_info("Found save \"%s\" = \"%s\", adding to qalculate variables", name.c_str(), value.c_str());

    auto * history_variable = new KnownVariable(calc->temporaryCategory(), name, value);
    calc->addVariable(history_variable);
}

void RofiQalc::load_history()
{
    GError * error = nullptr;
    gchar * history_dir = get_config_basedir();
    gchar * history_file = get_config_history_filename(history_dir);
    gchar * history_data = nullptr;
    gsize history_size;

    g_debug("Loading history from %s", history_file);

    this->history.clear();

    g_mkdir_with_parents(history_dir, 0755);
    if (g_file_test(history_file, static_cast<GFileTest>(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
        g_file_get_contents(history_file, &history_data, &history_size, &error);
        if (error != nullptr) {
            g_error("Failed to read history file: %s", error->message);
        }

        g_debug("History file is %lu b", history_size);

        gchar * head = history_data;
        while (head < history_data + history_size) {
            auto * newline = static_cast<gchar*>(memchr(head, '\n', history_size - (head - history_data)));
            if (newline == nullptr) {
                newline = history_data + history_size;
            }
            // g_debug("history line %lu (%lu) %.*s",
            //         state->history_used, newline - head, (int)(newline - head), head);

            std::string line{head, static_cast<size_t>(newline - head)};

            bool is_save =
                expression_contains_save_function(std::string{line}, default_parse_options, false);
            if (is_save) {
                g_debug("Loading history variable \"%s\"", line.c_str());
                this->history.emplace_back(line, "", true, true);

                if (!options.no_load_history_variables) {
                    _load_history_variable_into_qalculate(line);
                }
            } else {
                std::string expression;
                std::string result;

                g_debug("Loading history expression \"%s\"", line.c_str());

                auto separator_pos = line.rfind(HistoryEntry::separator);
                if (separator_pos != std::string::npos) {
                    expression = line.substr(0, separator_pos);
                    result = line.substr(separator_pos + HistoryEntry::separator.length());
                } else {
                    expression = "";
                    result = line;
                }
                this->history.emplace_back(expression, result, true, false);
            }

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

void RofiQalc::save_history() const
{
    GError * error = nullptr;
    gchar * history_dir = get_config_basedir();
    gchar * history_file = get_config_history_filename(history_dir);

    auto accumulator = [](size_t const acc, HistoryEntry const & e) {
        if (!e.persistent) {
            return acc;
        }
        return acc + e.print().length() + 1;
    };
    ssize_t const filesize = std::accumulate(this->history.begin(), this->history.end(), 0, accumulator);
    g_debug("Saving history to %s, %lu b", history_file, filesize);

    auto * history_data = static_cast<gchar*>(g_malloc(filesize));
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
    if (error != nullptr) {
        g_error("Failed to write history file: %s", error->message);
    }

    g_free(history_data);
    g_free(history_file);
    g_free(history_dir);
}

void RofiQalc::erase_history_line(int index)
{
    auto const & entry = this->history[index];

    g_debug("Removing history line %d, expression \"%s\", assignment %d",
        index, entry.expression.c_str(), entry.is_assignment);

    if (entry.is_assignment) {
        auto & calc = this->_thread_data.calc;

        auto const variable_parts_opt = parsing::parse_variable_parts(entry.expression);
        if (!variable_parts_opt.has_value()) {
            throw std::runtime_error("Failed to parse variable parts");
        }
        auto const &[var_name, _] = variable_parts_opt.value();

        auto comparator = [&var_name](Variable const * var) {
            return var->name() == var_name;
        };
        auto const it = std::ranges::find_if(calc->variables, comparator);
        if (it != std::end(calc->variables)) {
            g_debug("Found created variable from expr \"%s\", removing it as well", (*it)->title().c_str());
            calc->expressionItemDeleted(*it);
        }
    }

    this->history.erase(this->history.begin() + index);
}

RofiQalc::RofiQalc()
    : _var_ans{}
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
    for (size_t i = 0; i < std::size(this->_var_ans); ++i) {
        auto index_str = std::to_string(i + 1);
        auto * kv = new KnownVariable(
            calc->temporaryCategory(), ans_str + index_str, m_undefined,
            "Answer " + index_str, false, true);
        this->_var_ans[i] = dynamic_cast<KnownVariable*>(calc->addVariable(kv));
    }
    // Add aliases for answer variable
	this->_var_ans[0]->addName("answer");
	this->_var_ans[0]->addName(ans_str);

    this->_thread = std::thread{_calculator_thread_entry, std::ref(this->_thread_data)};
}

RofiQalc::~RofiQalc()
{
    this->_thread_data.should_quit = true;
    this->_thread_data.has_new_data = true;
    this->_thread_data.has_new_data.notify_one();
    this->_thread.join();
}

void RofiQalc::evaluate(std::string_view const & expr, EvalCallback callback, void * userdata)
{
    constexpr std::hash<std::string_view> hasher;
    size_t hash = hasher(expr);

    if (hash == this->_last_expr_hash) {
        return;
    }
    this->_last_expr_hash = hash;
    this->_last_expr = expr;

    {
        std::lock_guard lock(this->_thread_data.mtx_queued_query);
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
