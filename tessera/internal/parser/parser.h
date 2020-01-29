#include "tessera/tesserascript.h"
#include "tessera/error.h"
#include "../text_range.h"
#include "exception.h"
#include <variant>
#include <string>

namespace tess {
    namespace parser {
        std::variant<tess::tessera_script, tess::parser::exception> parse(const text_range& input);
    }
}