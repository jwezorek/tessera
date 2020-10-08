#pragma once    

#include "value.h"
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
        using segment = bg::model::segment<point>;
        using segment_rtree_value = std::pair<segment, tess::edge_ptr>;
        using segment_rtree = bgi::rtree<segment_rtree_value, bgi::quadratic<16>>;

        class rtree_tbl
        {
        public:
            rtree_tbl(tess::number eps);
            std::optional<int> get(const tess::point& pt) const;
            int insert(const tess::point& pt);
            void clear();

        private:
            tess::number eps_;
            rtree tree_;
        };

    };

    class vertex_location_table {
    public:
        vertex_location_table();
        int get_index(const tess::point& pt) const;
        tess::point get_location(int index) const;
        int insert(const tess::point& pt);
        void apply_transformation(const matrix& mat);
    private:
        geometry::rtree_tbl pt_to_index_;
        std::vector<tess::point> index_to_pt_;
    };

    class edge_location_table {
    public:
        edge_location_table(number eps = tess::eps);
        void insert(const tess::edge& edge);
        void insert(tess::edge_ptr edge);
        std::vector<tess::edge_ptr> get(tess::point a, tess::point b);
        std::vector<tess::edge_ptr> get(const tess::edge& edge);
        std::vector<tess::edge_ptr> get(tess::edge_ptr edge);
    private:
        number eps_;
        geometry::segment_rtree impl_;
    };

    using edge_indices = std::tuple<int, int>;
    struct edge_hash {
        std::size_t operator()(const edge_indices& key) const;
    };

    template<typename T>
    using edge_table = std::unordered_map<edge_indices, T, edge_hash>;

    std::vector<point> join(tess::patch_ptr tiles);

    bool are_collinear(const tess::point& a, const tess::point& b, const tess::point& c, tess::number eps = tess::eps);
    bool are_parallel(const tess::point& a1, const tess::point& a2, const tess::point& b1, const tess::point& b2, tess::number eps = tess::eps);
}