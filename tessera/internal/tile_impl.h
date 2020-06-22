#pragma once

#include "tessera/tile.h"
#include "tessera_impl.h"
#include "tile_patch_impl.h"
#include "expr_value.h"
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <map>

namespace tess {

    class allocator;

    class vertex::impl_type : public tessera_impl {
    private:
        tile::impl_type* parent_;
        int index_;
        number x_;
        number y_;

    public:
        impl_type() {};
		impl_type(tile::impl_type* parent, int index, std::tuple<number, number> loc);
        std::tuple<double, double> to_floats() const;
        point pos() const;
		expr_value get_field(allocator& allocator, const std::string& field) const;
		void apply(const matrix& mat);
		tile::impl_type* parent() const;
        void insert_field(const std::string& var, const expr_value& val) {}
        void get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const;
        void clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, vertex::impl_type* clone) const; 
    };

    class edge::impl_type : public tessera_impl {
        private:
            tile::impl_type* parent_;
            int index_;
            int u_, v_;
        public:
            impl_type() {};
			impl_type(tile::impl_type* parent, int index, int u, int v);
			const tess::vertex& u() const;
			const tess::vertex& v() const;
			expr_value get_field(allocator& allocator, const std::string& field) const;
			tile::impl_type* parent() const;
            void insert_field(const std::string& var, const expr_value& val) {}
            void get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const;
            void clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, edge::impl_type* clone) const;
    };

    class tile::impl_type : public tessera_impl {
        private:
            std::map<std::string, expr_value> fields_;
            std::vector<tess::vertex> vertices_;
            std::vector<tess::edge> edges_;
            tile_patch::impl_type* parent_;
			bool untouched_;

        public:
            impl_type() {};
            impl_type(tess::allocator* allocator, const std::vector<std::tuple<tess::number, tess::number>>& vertex_locations, bool foo);

            const std::vector<tess::vertex>& vertices() const;
            const std::vector<tess::edge>& edges() const;
            void set(std::vector<tess::vertex>&& vertices, std::vector<tess::edge>&& edges );
			expr_value get_field(allocator& allocator, const std::string& field) const;
			bool is_untouched() const;
			void apply(const matrix& mat);
            bool has_parent() const;
            tile_patch::impl_type* parent() const;
            void  set_parent(tile_patch::impl_type* parent);
            void insert_field(const std::string& var, const expr_value& val);
            void get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const;
            void clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, tile::impl_type* clone) const;
    };
}