#pragma once

#include "tessera/script.h"
#include "tessera/error.h"
#include "../text_range.h"
#include "assignment_parser.h"
#include "exception.h"
#include <variant>
#include <string>

namespace tess {

    using script_specifier = std::tuple<std::vector<var_assignment>, std::vector<std::string>, expr_ptr>;

    namespace parser {
        std::variant<tess::script_specifier, tess::parser::exception> parse(const text_range& input);
    }
}