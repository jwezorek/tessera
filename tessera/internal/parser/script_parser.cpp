#include "script_parser.h"

std::variant<tess::script, tess::parser::exception> tess::parser::parse(const text_range& input)
{
    return std::variant<tess::script, tess::parser::exception>();
}
