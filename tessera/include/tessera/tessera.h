#pragma once

#include <variant>
#include <string>
#include "tesserascript.h"
#include "error.h"

namespace tess {
    std::variant<tess::tessera_script, tess::error> parse(const std::string& script);
};