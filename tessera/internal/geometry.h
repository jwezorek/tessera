#pragma once    

#include "tessera/tile.h"
#include "tessera/tile_patch.h"
#include "number.h"
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <unordered_map>
#include <tuple>
#include <memory>

namespace tess {

    namespace geometry {
        namespace bg = boost::geometry;
        namespace bgi = boost::geometry::index;
        using rtree_point = bg::model::point<double, 2, bg::cs::cartesian>;
        using rtree_box = bg::model::box<rtree_point>;
        using rtree_value = std::pair< rtree_point, int>;
        using rtree = bgi::rtree<rtree_value, bgi::rstar<8>>; //bgi::rtree<rtree_value, bgi::quadratic<16>>;
        using point = bg::model::d2::point_xy<double>;
        using polygon = bg::model::polygon<point, false>;
    };

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

    std::vector<point> join(const tess::tile_patch::impl_type* tiles);
}