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
#include "gc_heap.h"

namespace tess {

    namespace detail {
        class vertex_impl : public tessera_impl {
            private:
                const_tile_graph_ptr parent_;
                int index_;
                std::variant<int, point> location_;

            public:
                vertex_impl(gc_heap& a) {};
                vertex_impl(gc_heap& a, int index, point loc);
                void initialize( vertex_root_ptr p) {}

                void set_parent(const_tile_root_ptr parent);
                std::tuple<double, double> to_floats() const;
                point pos() const;
                value_ get_field(gc_heap& allocator, const std::string& field) const;
                void apply(const matrix& mat);
                const_tile_root_ptr parent() const;
                const_edge_root_ptr in_edge() const;
                const_edge_root_ptr out_edge() const;
                void insert_field(const std::string& var, const value_& val) {}
                //void get_references(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, vertex_root_ptr clone) const;
                const_patch_root_ptr grandparent() const;
                void set_location(int vert_index);
                void set_location(point pt);
                int location_index() const;
                std::string debug() const;
        };

        class edge_impl : public tessera_impl {
            private:
                tile_root_ptr parent_;
                int index_;
                int u_, v_;
                std::map<std::string, field_value> fields_;
            public:
                edge_impl(gc_heap& a) {};
                edge_impl(gc_heap& a, int index, int u, int v);
                void initialize( edge_root_ptr p) {}

                void set_parent(const_tile_root_ptr parent);
                const_vertex_root_ptr u() const;
                const_vertex_root_ptr v() const;
                vertex_root_ptr u();
                vertex_root_ptr v();
                const_edge_root_ptr next_edge() const;
                const_edge_root_ptr prev_edge() const;
                value_ get_field(gc_heap& allocator, const std::string& field) const;
                value_ get_field(const std::string& field) const;
                bool has_property(const std::string& prop) const;
                const_tile_root_ptr parent() const;
                tile_root_ptr parent();
                void insert_field(const std::string& var, const value_& val);
                const std::map<std::string, field_value>& fields() const;
                //void get_references(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, edge_root_ptr clone) const;
                void flip();
                edge_indices get_edge_location_indices() const;
                std::string debug() const;
        };

        class tile_impl : public tessera_impl {
            private:
                std::map<std::string, field_value> fields_;
                std::vector<tess::vertex_root_ptr> vertices_;
                std::vector<tess::edge_root_ptr> edges_;
                patch_root_ptr parent_;
                tile_root_ptr self_;
                int index_;

            public:
                tile_impl(gc_heap& a) {};
                tile_impl(tess::gc_heap& allocator, const std::vector<std::tuple<tess::number, tess::number>>& vertex_locations);
                void initialize( tile_root_ptr self);

                std::vector<tess::const_vertex_root_ptr> vertices() const;
                std::vector<tess::const_edge_root_ptr> edges() const;
                const std::vector<tess::vertex_root_ptr>& vertices();
                const std::vector<tess::edge_root_ptr>& edges();

                const_vertex_root_ptr vertex(int index) const;
                vertex_root_ptr vertex(int index);

                const_edge_root_ptr edge(int index) const;
                edge_root_ptr edge(int index);

                value_ get_field(const std::string& field) const;
                value_ get_field(gc_heap& allocator, const std::string& field) const;
                const std::map<std::string, field_value>& fields() const;
                void apply(const matrix& mat);
                tess::const_tile_root_ptr flip(gc_heap& a) const;
                void flip();
                bool has_parent() const;
                const_patch_root_ptr parent() const;
                patch_root_ptr parent();
                void set_parent(patch_root_ptr parent, int index);
                void detach();
                void insert_field(const std::string& var, const value_& val);
                //void get_references(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, tile_root_ptr clone) const;
                bool is_detached() const;
                tess::tile_root_ptr clone_detached(tess::gc_heap& a) const;
                std::string debug() const;
                const_tile_root_ptr get_adjacent_tile(int edge_index) const;
                const_edge_root_ptr get_edge_on(gc_heap& a, const_edge_root_ptr e) const;
                value_ get_on(gc_heap& a, std::variant<tess::const_edge_root_ptr, tess::const_cluster_root_ptr>& e) const;
        };

    }
}