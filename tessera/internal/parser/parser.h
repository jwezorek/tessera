#include "tessera/tesserascript.h"
#include <variant>
#include <string>

namespace tess {
    namespace parser {
        std::variant<tess::tessera_script, tess::parse_error> parse(const std::string& script);
    }
}