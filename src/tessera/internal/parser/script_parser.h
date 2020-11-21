#pragma once

#include "tessera/script.h"
#include "../text_range.h"
#include <variant>
#include <string>

namespace tess {


    namespace parser {
        std::variant<tess::script, tess::error> parse(const text_range& input);
    }
}