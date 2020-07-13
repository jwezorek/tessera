#pragma once 

#include "tessera_impl.h"
#include "tessera/tile.h"
#include "tessera/tile_patch.h"
#include "vertex_table.h"
#include "expr_value.h"
#include <vector>
#include <map>

namespace tess {

    class allocator;

    class tile_patch::impl_type : public tessera_impl
    {
    private:
        std::vector<tess::tile> tiles_;
        std::map<std::string, expr_value> fields_;
        vertex_location_table vert_tbl_;
    public:
        impl_type() {};
        void insert_tile(tess::tile& t);
        const std::vector<tess::tile>& tiles() const;
		expr_value get_field(allocator& allocator, const std::string& field) const;
        expr_value get_method(allocator& allocator, const std::string& field) const;
        expr_value get_ary_item(int i) const;
        int get_ary_count() const;
		void apply(const matrix& mat);
        void flip();
        bool is_untouched() const;
        void insert_field(const std::string& var, const expr_value& val);
        void get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const;
        void clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, tile_patch::impl_type* clone) const;
        point get_vertex_location(int index) const;
    };

    class cluster::impl_type : public tessera_impl
    {
    private:
        std::vector<expr_value> values_;
    public:
        impl_type() {};
        impl_type(const std::vector<expr_value>& tiles);
        expr_value get_field(allocator& allocator, const std::string& field) const;
        expr_value get_ary_item(int i) const;
        void push_value(expr_value val);
        int get_ary_count() const;
        const std::vector<expr_value>& values();
        void insert_field(const std::string& var, const expr_value& val);
        void get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const;
        void clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, cluster::impl_type* clone) const;
        std::vector<expr_value>::const_iterator begin() const;
        std::vector<expr_value>::const_iterator end() const;
    };

    tile_patch flatten(allocator& a, const std::vector<expr_value>& tiles_and_patches);
}