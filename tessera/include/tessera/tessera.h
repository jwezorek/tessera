#pragma once

#include <variant>
#include <string>
#include "tessera_script.h"
#include "error.h"

namespace tess {
    std::variant<tess::tessera_script, tess::error> parse(const std::string& script);
};