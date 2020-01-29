#include "tile.h"
#include "parser/tile_parser.h"

tess::tile::tile(const std::string& name, std::vector<std::string> params, const text_range& source_code) :
    name_(name), params_(std::move(params))
{
    auto results = tess::parser::parse_tile(source_code);
    if (std::holds_alternative<tess::parser::tile_verts_and_edges>(results)) {
        auto& verts_and_edges = std::get<tess::parser::tile_verts_and_edges>(results);
        vertex_definitions_ = std::move(std::get<0>(verts_and_edges));
        edge_definitions_ = std::move(std::get<1>(verts_and_edges));
    } else {
        auto e = std::get<tess::parser::exception>(results);
        e.push_stack_item("tile " + name);
        if (!e.has_where())
            e.set_where(source_code.end());
        throw e;
    }
}

