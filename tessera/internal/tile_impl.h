#pragma once

#include "tessera/tile.h"
#include "tessera_impl.h"
#include "tile_patch_impl.h"
#include "tile_def.h"
#include "expr_value.h"
#include <string>
#include <vector>
#include <tuple>
#include <memory>

namespace tess {

    class vertex::impl_type : public tessera_impl {
    private:
        tile::impl_type* parent_;
        int index_;
        number x_;
        number y_;

    public:
		impl_type(tile::impl_type* parent, int index, std::tuple<number, number> loc);
        std::string name() const;
        std::string vertex_class() const;
        std::tuple<double, double> to_floats() const;
        point pos() const;
		expr_value get_field(const std::string& field) const;
		void apply(const matrix& mat);
		tile::impl_type* parent() const;
        void insert_field(const std::string& var, const expr_value& val) {}
    };

    class edge::impl_type : public tessera_impl {
        private:
            tile::impl_type* parent_;
            int index_;
        public:
			impl_type(tile::impl_type* parent, int index);
            std::string name() const;
            std::string edge_class() const;
			const tess::vertex& u() const;
			const tess::vertex& v() const;
			expr_value get_field(const std::string& field) const;
			tile::impl_type* parent() const;
            void insert_field(const std::string& var, const expr_value& val) {}
    };

    class tile::impl_type : public tessera_impl {
        private:
            struct fields {
                std::vector<tess::vert_fields> vertices;
                std::vector<tess::edge_fields> edges;
                std::unordered_map<std::string, int> vert_name_to_index;
                std::unordered_map<std::string, int> edge_name_to_index;

                int get_edge_index(const std::string& e);
                int get_vert_index(const std::string& v);

                fields(const std::vector<tess::vert_fields>& v, const std::vector<tess::edge_fields>& e);
            };
            std::vector<tess::vertex> vertices_;
            std::vector<tess::edge> edges_;
            std::shared_ptr<fields> fields_;
            tile_patch::impl_type* parent_;
			bool untouched_;
        public:
			impl_type(const std::vector<vert_fields>& v, const std::vector<edge_fields>& e);

            const tess::vertex& vertex(const std::string& v) const;
            const std::vector<tess::vertex>& vertices() const;
            const std::vector<tess::edge>& edges() const;
            void set(std::vector<tess::vertex>&& vertices, std::vector<tess::edge>&& edges );
			expr_value get_field(const std::string& field) const;
			bool is_untouched() const;
			void apply(const matrix& mat);
            bool has_parent() const;
            tile_patch::impl_type* parent() const;
            void  set_parent(tile_patch::impl_type* parent);

            const vert_fields& vert_fields(int i) const;
            const edge_fields& edge_fields(int i) const;
            void insert_field(const std::string& var, const expr_value& val);
    };
}