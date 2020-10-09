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
            std::vector<tess::const_tile_ptr> tiles_;
            std::map<std::string, value_> fields_;
            vertex_location_table vert_tbl_;
            mutable edge_table<const_edge_ptr> edge_tbl_;

            void build_edge_table() const;

        public:
            patch_impl(obj_id id) : tessera_impl(id) {};
            patch_impl(obj_id id, const std::vector<tess::const_tile_ptr>& tiles);
            void insert_tile(tess::const_tile_ptr t);
            const std::vector<tess::const_tile_ptr>& tiles() const;
            value_ get_field(allocator& allocator, const std::string& field) const;
            value_ get_ary_item(int i) const;
            int get_ary_count() const;
            void apply(const matrix& mat);
            const_patch_ptr flip(allocator& allocator) const;
            void flip();
            tess::const_edge_ptr get_edge_on(int u, int v) const;
            tess::const_edge_ptr get_edge_on(tess::point u, tess::point v) const;
            value_ get_on(allocator& a, const std::variant<tess::const_edge_ptr, tess::const_cluster_ptr>& e) const;
            void insert_field(const std::string& var, const value_& val);
            void get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const;
            void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, const_patch_ptr clone) const;
            point get_vertex_location(int index) const;
            const_tile_ptr join(allocator& allocator) const;
            void dfs(tile_visitor visit) const;

            std::string debug() const;
        };

        class cluster_impl : public tessera_impl
        {
        private:
            std::vector<value_> values_;
        public:
            cluster_impl(obj_id id) : tessera_impl(id) {};
            cluster_impl(obj_id id, const std::vector<value_>& tiles);
            value_ get_field(allocator& allocator, const std::string& field) const;
            value_ get_ary_item(int i) const;
            void push_value(value_ val);
            int get_ary_count() const;
            const std::vector<value_>& values();
            void insert_field(const std::string& var, const value_& val);
            void get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const;
            void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, const_cluster_ptr clone) const;
            std::vector<value_>::const_iterator begin() const;
            std::vector<value_>::const_iterator end() const;
            const std::vector<value_>& items() const;
        };

    }

    const_patch_ptr flatten(allocator& a, const std::vector<value_>& tiles_and_patches, bool should_join_broken_tiles);
    const_tile_ptr join(allocator& a, const std::vector<value_>& tiles_and_patches, bool should_join_broken_tiles);
    const_tile_ptr join(allocator& a, const std::vector<const_tile_ptr>& tiles);

}