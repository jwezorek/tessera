#pragma once 

#include "tessera_impl.h"
#include "tessera/tile.h"
#include "tessera/tile_patch.h"
#include "geometry.h"
#include "expr_value.h"
#include "geometry.h"
#include <vector>
#include <map>
#include <optional>
#include <functional>

namespace tess {

    class allocator;

    using tile_visitor = std::function<void(const tess::tile&)>;

    class tile_patch::impl_type : public tessera_impl
    {
    private:
        std::vector<tess::tile> tiles_;
        std::map<std::string, expr_value> fields_;
        vertex_location_table vert_tbl_;
        mutable edge_table<edge> edge_tbl_;

        void build_edge_table() const;

    public:
        impl_type(obj_id id) : tessera_impl(id) {};
        impl_type(obj_id id, std::vector<tess::tile>& tiles);
        void insert_tile( tess::tile& t);
        const std::vector<tess::tile>& tiles() const;
		expr_value get_field(allocator& allocator, const std::string& field) const;
        expr_value get_ary_item(int i) const;
        int get_ary_count() const;
		void apply(const matrix& mat);
        tile_patch flip(allocator& allocator) const;
        void flip();
        std::optional<edge> get_edge_on(int u, int v) const;
        expr_value get_on(allocator& a, const std::variant<tess::edge, tess::cluster>& e) const;
        void insert_field(const std::string& var, const expr_value& val);
        void get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const;
        void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, tile_patch::impl_type* clone) const;
        point get_vertex_location(int index) const;
        tile join(allocator& allocator) const;
        void dfs(tile_visitor visit) const;
        
        std::string debug() const;
    };

    class cluster::impl_type : public tessera_impl
    {
    private:
        std::vector<expr_value> values_;
    public:
        impl_type(obj_id id) : tessera_impl(id) {};
        impl_type(obj_id id, const std::vector<expr_value>& tiles);
        expr_value get_field(allocator& allocator, const std::string& field) const;
        expr_value get_ary_item(int i) const;
        void push_value(expr_value val);
        int get_ary_count() const;
        const std::vector<expr_value>& values();
        void insert_field(const std::string& var, const expr_value& val);
        void get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const;
        void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, cluster::impl_type* clone) const;
        std::vector<expr_value>::const_iterator begin() const;
        std::vector<expr_value>::const_iterator end() const;
    };

    tile_patch flatten(allocator& a, const std::vector<expr_value>& tiles_and_patches);
}