#pragma once 

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

    class allocator;

    using tile_visitor = std::function<void(const_tile_ptr)>;

    namespace detail {
        class patch_impl : public tessera_impl
        {
        private:
            std::vector<tess::tile_ptr> tiles_;
            std::map<std::string, value_> fields_;
            vertex_location_table vert_tbl_;
            mutable edge_table<const_edge_ptr> edge_tbl_;

            void build_edge_table() const;

        public:
            patch_impl() {};
            patch_impl(const std::vector<tess::tile_ptr>& tiles);
            void insert_tile(tess::tile_ptr t);
            int count() const;
            std::vector<tess::const_tile_ptr> tiles() const;
            const std::vector< tess::tile_ptr>& tiles();
            value_ get_field(allocator& allocator, const std::string& field) const;
            value_ get_ary_item(int i) const;
            int get_ary_count() const;
            void apply(const matrix& mat);
            patch_ptr flip(allocator& allocator) const;
            void flip();
            tess::const_edge_ptr get_edge_on(int u, int v) const;
            tess::const_edge_ptr get_edge_on(tess::point u, tess::point v) const;
            value_ get_on(allocator& a, const std::variant<tess::const_edge_ptr, tess::const_cluster_ptr>& e) const;
            void insert_field(const std::string& var, const value_& val);
            void get_references(std::unordered_set<obj_id>& alloc_set) const;
            void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, patch_ptr clone) const;
            point get_vertex_location(int index) const;
            tile_ptr join(allocator& allocator) const;
            void dfs(tile_visitor visit) const;

            std::string debug() const;
        };

        class cluster_impl : public tessera_impl
        {
        private:
            std::vector<value_> values_;
        public:
            cluster_impl()  {};
            cluster_impl( const std::vector<value_>& tiles);
            value_ get_field(allocator& allocator, const std::string& field) const;
            value_ get_ary_item(int i) const;
            void push_value(value_ val);
            int get_ary_count() const;
            const std::vector<value_>& values() const;
            void insert_field(const std::string& var, const value_& val);
            void get_references(std::unordered_set<obj_id>& alloc_set) const;
            void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, cluster_ptr clone) const;
            std::vector<value_>::const_iterator begin() const;
            std::vector<value_>::const_iterator end() const;
            const std::vector<value_>& items() const;
        };

    }

    patch_ptr flatten(allocator& a, const std::vector<value_>& tiles_and_patches, bool should_join_broken_tiles);
    tile_ptr join(allocator& a, const std::vector<value_>& tiles_and_patches, bool should_join_broken_tiles);
    tile_ptr join(allocator& a, const std::vector<tile_ptr>& tiles);

}