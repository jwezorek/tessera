#include "tile_parser.h"
#include "../tile.h"


std::variant<tess::parser::tile_verts_and_edges, tess::parser::exception> tess::parser::parse_tile(const tess::text_range& input)
{
    return std::variant<tile_verts_and_edges, exception>();
}
