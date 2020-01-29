#pragma once

#include "../tile.h"
#include "../text_range.h"
#include "exception.h"
#include <unordered_map>
#include <tuple>
#include <variant>

namespace tess {
    namespace parser {

        using tile_verts_and_edges = std::tuple< std::unordered_map<std::string, vertex_fields>, std::unordered_map<std::string, edge_fields> >;
        std::variant<tile_verts_and_edges, exception> parse_tile(const text_range& input);

    }
}

