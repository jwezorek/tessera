#pragma once

#include <variant>
#include <string>
#include "old_tessera_script.h"
#include "script.h"
#include "error.h"

namespace tess {
    std::variant<tess::tessera_script, tess::error> parse_old(const std::string& script);
    std::variant<tess::script, tess::error> parse(const std::string& script);
};