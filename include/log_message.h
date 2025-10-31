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

#include <format>
#include <string>
#include <utility>

class CalculatorMessage;

namespace rq
{

enum MessageType
{
    INFORMATION = 0,
    WARNING = 1,
    ERROR = 2
};

struct LogMessage
{
    MessageType type;
    std::string message;

    explicit LogMessage(CalculatorMessage const & message);

    constexpr LogMessage(MessageType type, std::string message)
        : type(type), message(std::move(message))
    {}

    template <typename... Args>
    constexpr LogMessage(MessageType type, std::string const & format, Args&&... args)
        : type(type), message(std::vformat(format, std::make_format_args(args...)))
    {}
};

}
