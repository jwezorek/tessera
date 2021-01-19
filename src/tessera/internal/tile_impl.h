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
#include "allocator.h"

namespace tess {

    namespace detail {
        class vertex_impl : public tessera_impl {
            private:
                const_tile_ptr parent_;
                int index_;
                std::variant<int, point> location_;

            public:
                vertex_impl(allocator& a) {};
                vertex_impl(allocator& a, int index, point loc);
                void initialize( vertex_ptr p) {}

                void set_parent(const_tile_ptr parent);
                std::tuple<double, double> to_floats() const;
                point pos() const;
                value_ get_field(allocator& allocator, const std::string& field) const;
                void apply(const matrix& mat);
                const_tile_ptr parent() const;
                const_edge_ptr in_edge() const;
                const_edge_ptr out_edge() const;
                void insert_field(const std::string& var, const value_& val) {}
                //void get_references(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, mutable_object_value>& orginal_to_clone, vertex_ptr clone) const;
                const_patch_ptr grandparent() const;
                void set_location(int vert_index);
                void set_location(point pt);
                int location_index() const;
                std::string debug() const;
        };

        class edge_impl : public tessera_impl {
            private:
                tile_ptr parent_;
                int index_;
                int u_, v_;
                std::map<std::string, value_> fields_;
            public:
                edge_impl(allocator& a) {};
                edge_impl(allocator& a, int index, int u, int v);
                void initialize( edge_ptr p) {}

                void set_parent(const_tile_ptr parent);
                const_vertex_ptr u() const;
                const_vertex_ptr v() const;
                vertex_ptr u();
                vertex_ptr v();
                const_edge_ptr next_edge() const;
                const_edge_ptr prev_edge() const;
                value_ get_field(allocator& allocator, const std::string& field) const;
                value_ get_field(const std::string& field) const;
                bool has_property(const std::string& prop) const;
                const_tile_ptr parent() const;
                tile_ptr parent();
                void insert_field(const std::string& var, const value_& val);
                const std::map<std::string, value_>& fields() const;
                //void get_references(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::allocator& allocator, std::unordered_map<obj_id,mutable_object_value>& orginal_to_clone, edge_ptr clone) const;
                void flip();
                edge_indices get_edge_location_indices() const;
                std::string debug() const;
        };

        class tile_impl : public tessera_impl {
            private:
                std::map<std::string, value_> fields_;
                std::vector<tess::vertex_ptr> vertices_;
                std::vector<tess::edge_ptr> edges_;
                patch_ptr parent_;
                tile_ptr self_;
                int index_;

            public:
                tile_impl(allocator& a) {};
                tile_impl(tess::allocator& allocator, const std::vector<std::tuple<tess::number, tess::number>>& vertex_locations);
                void initialize( tile_ptr self);

                std::vector<tess::const_vertex_ptr> vertices() const;
                std::vector<tess::const_edge_ptr> edges() const;
                const std::vector<tess::vertex_ptr>& vertices();
                const std::vector<tess::edge_ptr>& edges();

                const_vertex_ptr vertex(int index) const;
                vertex_ptr vertex(int index);

                const_edge_ptr edge(int index) const;
                edge_ptr edge(int index);

                value_ get_field(const std::string& field) const;
                value_ get_field(allocator& allocator, const std::string& field) const;
                const std::map<std::string, value_>& fields() const;
                void apply(const matrix& mat);
                tess::const_tile_ptr flip(allocator& a) const;
                void flip();
                bool has_parent() const;
                const_patch_ptr parent() const;
                patch_ptr parent();
                void set_parent(patch_ptr parent, int index);
                void detach();
                void insert_field(const std::string& var, const value_& val);
                //void get_references(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, mutable_object_value>& orginal_to_clone, tile_ptr clone) const;
                bool is_detached() const;
                tess::tile_ptr clone_detached(tess::allocator& a) const;
                std::string debug() const;
                const_tile_ptr get_adjacent_tile(int edge_index) const;
                const_edge_ptr get_edge_on(allocator& a, const_edge_ptr e) const;
                value_ get_on(allocator& a, std::variant<tess::const_edge_ptr, tess::const_cluster_ptr>& e) const;
        };

    }
}