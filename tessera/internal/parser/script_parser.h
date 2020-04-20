#pragma once

#include "tessera/script.h"
#include "tessera/error.h"
#include "../text_range.h"
#include "assignment_parser.h"
#include "exception.h"
#include <variant>
#include <string>

namespace tess {


    namespace parser {
        std::variant<tess::script, tess::parser::exception> parse(const text_range& input);
    }
}