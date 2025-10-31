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
#include "parsing.h"

#include <algorithm>

using namespace rq::parsing;

static constexpr std::string_view VARIABLE_ASSIGNMENT_SEPARATORS[] = { ":=", "=" };

static constexpr std::string strip_all_whitespace(std::string_view const & str)
{
    std::string line{str};

    auto const start = std::ranges::remove_if(line, isspace).begin();
    line.erase(start, line.end());

    return line;
}

std::optional<ParsedVariable> rq::parsing::parse_variable_parts(std::string_view const & expression_input)
{
    std::string const expression = strip_all_whitespace(expression_input);

    for (auto const & separator : VARIABLE_ASSIGNMENT_SEPARATORS) {
        auto const separator_pos = expression.find(separator);
        if (separator_pos == std::string_view::npos) {
            continue;
        }

        return ParsedVariable{
            expression.substr(0, separator_pos),
            expression.substr(separator_pos + separator.length())
        };
    }

    return std::nullopt;
}
