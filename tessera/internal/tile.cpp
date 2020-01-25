#include "tile.h"

tess::tile::tile(const std::string& name, std::vector<std::string> params, const text_range& source_code) :
    name_(name), params_(std::move(params)), code_(source_code)
{
}

tess::tile_component_declaration::tile_component_declaration(const std::string& test) : test_(test)
{
}
