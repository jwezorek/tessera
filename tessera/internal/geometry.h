#pragma once    

#include "tessera/tile.h"
#include "number.h"
#include <unordered_map>
#include <tuple>
#include <memory>

namespace tess {

    class vertex_location_table {
    public:
        vertex_location_table();
        int get_index(const tess::point& pt) const;
        tess::point get_location(int index) const;
        int insert(const tess::point& pt);
        void apply_transformation(const matrix& mat);
    private:
        class impl_type;
        std::shared_ptr<impl_type> impl_;
    };

    using edge_indices = std::tuple<int, int>;
    struct edge_hash {
        std::size_t operator()(const edge_indices& key) const;
    };

    template<typename T>
    using edge_table = std::unordered_map<edge_indices, T, edge_hash>;
}