#include "tessera/tesserascript.h"
#include "tessera/error.h"
#include <variant>
#include <string>

namespace tess {
    namespace parser {
        std::variant<tess::tessera_script, tess::error> parse(const std::string& script);
    }
}