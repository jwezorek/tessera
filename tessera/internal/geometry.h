#pragma once    

#include "tessera/tile.h"
#include <unordered_map>
#include <tuple>

namespace tess {

    using edge_indices = std::tuple<int, int>;
    struct edge_hash {
        std::size_t operator()(const edge_indices& key) const;
    };

    template<typename T>
    using edge_table = std::unordered_map<edge_indices, T, edge_hash>;
}