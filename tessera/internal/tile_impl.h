#pragma once

#include "tessera/tile.h"
#include "tessera_impl.h"
#include "tile_patch_impl.h"
#include "value.h"
#include "number.h"
#include "geometry.h"
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <map>
#include <variant>

namespace tess {

    class allocator;

    namespace detail {
        class vertex_impl : public tessera_impl {
            private:
                const_tile_ptr parent_;
                int index_;
                std::variant<int, point> location_;

            public:
                vertex_impl(obj_id id) : tessera_impl(id) {};
                vertex_impl(obj_id id, const_tile_ptr parent, int index, point loc);
                std::tuple<double, double> to_floats() const;
                point pos() const;
                expr_value get_field(allocator& allocator, const std::string& field) const;
                void apply(const matrix& mat);
                const_tile_ptr parent() const;
                const_edge_ptr in_edge() const;
                const_edge_ptr out_edge() const;
                void insert_field(const std::string& var, const expr_value& val) {}
                void get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, const_vertex_ptr clone) const;
                const_patch_ptr grandparent() const;
                void set_location(int vert_index);
                void set_location(point pt);
                int location_index() const;
                std::string debug() const;
        };

        class edge_impl : public tessera_impl {
            private:
                const_tile_ptr parent_;
                int index_;
                int u_, v_;
                std::map<std::string, expr_value> fields_;
            public:
                edge_impl(obj_id id) : tessera_impl(id), parent_(nullptr), index_(-1), u_(-1), v_(-1) {};
                edge_impl(obj_id id, const_tile_ptr parent, int index, int u, int v);
                const_vertex_ptr u() const;
                const_vertex_ptr v() const;
                tess::const_vertex_ptr u();
                tess::const_vertex_ptr v();
                tess::const_edge_ptr next_edge() const;
                tess::const_edge_ptr prev_edge() const;
                expr_value get_field(allocator& allocator, const std::string& field) const;
                expr_value get_field(const std::string& field) const;
                bool has_property(const std::string& prop) const;
                const_tile_ptr parent() const;
                void insert_field(const std::string& var, const expr_value& val);
                const std::map<std::string, expr_value>& fields() const;
                void get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, const_edge_ptr clone) const;
                void flip();
                edge_indices get_edge_location_indices() const;
                std::string debug() const;
        };

        class tile_impl : public tessera_impl {
            private:
                std::map<std::string, expr_value> fields_;
                std::vector<tess::const_vertex_ptr> vertices_;
                std::vector<tess::const_edge_ptr> edges_;
                const_patch_ptr parent_;
                int index_;

            public:
                tile_impl(obj_id id) : tessera_impl(id), parent_(nullptr), index_(-1) {};
                tile_impl(obj_id id, tess::allocator* allocator, const std::vector<std::tuple<tess::number, tess::number>>& vertex_locations);

                const std::vector<tess::const_vertex_ptr>& vertices() const;
                std::vector<tess::const_vertex_ptr>& vertices();
                const std::vector<tess::const_edge_ptr>& edges() const;
                std::vector<tess::const_edge_ptr>& edges();
                void set(std::vector<tess::const_vertex_ptr>&& vertices, std::vector<tess::const_edge_ptr>&& edges);
                expr_value get_field(const std::string& field) const;
                expr_value get_field(allocator& allocator, const std::string& field) const;
                const std::map<std::string, expr_value>& fields() const;
                void apply(const matrix& mat);
                tess::const_tile_ptr flip(allocator& a) const;
                void flip();
                bool has_parent() const;
                const_patch_ptr parent() const;
                void set_parent(const_patch_ptr parent, int index);
                void detach();
                void insert_field(const std::string& var, const expr_value& val);
                void get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, const_tile_ptr clone) const;
                bool is_detached() const;
                tess::const_tile_ptr clone_detached(tess::allocator& a) const;
                std::string debug() const;
                const const_tile_ptr get_adjacent_tile(int edge_index) const;
                const_edge_ptr get_edge_on(allocator& a, const_edge_ptr e) const;
                expr_value get_on(allocator& a, std::variant<tess::const_edge_ptr, tess::const_cluster_ptr>& e) const;
        };

    }
}