#pragma once

#include <variant>
#include <string>
#include "tesserascript.h"

namespace tess {
    std::variant<tess::tessera_script, tess::parse_error> parse(const std::string& script);
};