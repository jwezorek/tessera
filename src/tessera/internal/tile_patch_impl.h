#pragma once 

#include "gc_heap.h"
#include "tessera_impl.h"
#include "tessera/tile.h"
#include "cluster.h"
#include "tessera/tile_patch.h"
#include "geometry.h"
#include "value.h"
#include "geometry.h"
#include <vector>
#include <map>
#include <optional>
#include <functional>

namespace tess {

    using tile_visitor = std::function<void(const_tile_root_ptr)>;

    namespace detail {
        class patch_impl : public tessera_impl
        {
        private:
            std::vector<tess::tile_root_ptr> tiles_;
            std::map<std::string, field_value> fields_;
            vertex_location_table vert_tbl_;
            mutable edge_table<const_edge_root_ptr> edge_tbl_;
            patch_root_ptr self_;
            void build_edge_table() const;

        public:
            patch_impl(gc_heap& a) {};
            patch_impl(gc_heap& a, const std::vector<tess::tile_root_ptr>& tiles);
            void initialize(patch_root_ptr p) { self_ = p; }

            void insert_tile(tess::tile_root_ptr t);
            int count() const;
            std::vector<tess::const_tile_root_ptr> tiles() const;
            const std::vector<tess::tile_root_ptr>& tiles();
            value_ get_field(gc_heap& allocator, const std::string& field) const;
            value_ get_ary_item(int i) const;
            int get_ary_count() const;
            void apply(const matrix& mat);
            patch_root_ptr flip(gc_heap& allocator) const;
            void flip();
            tess::const_edge_root_ptr get_edge_on(int u, int v) const;
            tess::const_edge_root_ptr get_edge_on(tess::point u, tess::point v) const;
            value_ get_on(gc_heap& a, const std::variant<tess::const_edge_root_ptr, tess::const_cluster_root_ptr>& e) const;
            void insert_field(const std::string& var, const value_& val);
            //void get_references(std::unordered_set<obj_id>& alloc_set) const;
            void clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, patch_raw_ptr clone) const;
            point get_vertex_location(int index) const;
            tile_root_ptr join(gc_heap& allocator) const;
            void dfs(tile_visitor visit) const;

            std::string debug() const;
        };

        class cluster_impl : public tessera_impl
        {
        private:
            std::vector<value_> values_;
        public:
            cluster_impl(gc_heap& a) {};
            cluster_impl( gc_heap& a, const std::vector<value_>& tiles);
            void initialize( cluster_root_ptr p) {}

            value_ get_field(gc_heap& allocator, const std::string& field) const;
            value_ get_ary_item(int i) const;
            void push_value(value_ val);
            int get_ary_count() const;
            const std::vector<tess::value_>& values() const;
            void insert_field(const std::string& var, const value_& val);
            void clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, cluster_raw_ptr clone) const;
            std::vector<tess::value_>::const_iterator begin() const;
            std::vector<tess::value_>::const_iterator end() const;
            const std::vector<tess::value_>& items() const;
        };

    }

    patch_root_ptr flatten(gc_heap& a, const std::vector<value_>& tiles_and_patches, bool should_join_broken_tiles);
    tile_root_ptr join(gc_heap& a, const std::vector<value_>& tiles_and_patches, bool should_join_broken_tiles);
    tile_root_ptr join(gc_heap& a, const std::vector<tile_root_ptr>& tiles);

}