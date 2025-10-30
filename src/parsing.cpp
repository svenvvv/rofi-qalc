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
