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

#include "log_message.h"

#include <libqalculate/Calculator.h>

static constexpr rq::MessageType convert_message_type(MessageType type)
{
    switch (type) {
        case MESSAGE_INFORMATION:
            return rq::INFORMATION;
        case MESSAGE_WARNING:
            return rq::WARNING;
        case MESSAGE_ERROR:
            return rq::ERROR;
    }
    return rq::ERROR;
}

rq::LogMessage::LogMessage(CalculatorMessage const & message)
    : type(convert_message_type(message.type()))
    , message(message.message())
{}
