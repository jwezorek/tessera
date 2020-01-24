#pragma once

#include "tessera/error.h"
#include "../expression.h"
#include "../text_range.h"
#include <variant>

namespace tess {
    namespace parser {
        std::variant<tess::expr_ptr, tess::error> parse_expression(const text_range& input);
    }
}
