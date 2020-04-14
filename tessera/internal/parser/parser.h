#pragma once

#include "tessera/tessera_script.h"
#include "tessera/error.h"
#include "../text_range.h"
#include "exception.h"
#include <variant>
#include <string>

namespace tess {
    namespace parser {
        std::variant<tess::tessera_script, tess::parser::exception> parse_old(const text_range& input);
    }
}