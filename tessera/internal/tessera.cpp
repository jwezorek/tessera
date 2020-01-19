#include "../include/tessera/tessera.h"
#include "parser/parser.h"
#include <memory>

std::variant<tess::tessera_script, tess::error> tess::parse(const std::string& script)
{
    return tess::parser::parse(script);
}
