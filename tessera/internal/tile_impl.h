#pragma once

#include "tessera/tile.h"
#include "tessera_impl.h"
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
        std::shared_ptr<const vertex_def> def_;
        number x_;
        number y_;
    public:
		impl_type(tile::impl_type* parent, std::shared_ptr<const vertex_def> prototype, std::tuple<number, number> loc);
        std::string name() const;
        std::string vertex_class() const;
        std::tuple<double, double> to_floats() const;
        point pos() const;
		expr_value get_field(const std::string& field) const;
		void apply(const matrix& mat);
		tile::impl_type& parent() const;
    };

    class edge::impl_type : public tessera_impl {
        private:
            tile::impl_type* parent_;
            std::shared_ptr<const edge_def> def_;
        public:
			impl_type(tile::impl_type* parent, std::shared_ptr<const edge_def> prototype);
            std::string name() const;
            std::string edge_class() const;
			const tess::vertex& u() const;
			const tess::vertex& v() const;
			expr_value get_field(const std::string& field) const;
			tile::impl_type& parent() const;
    };

    class tile::impl_type : public tessera_impl {
        private:
            std::vector<tess::edge> edges_;
            std::vector<tess::vertex> vertices_;
            std::shared_ptr<const tile_def> def_;
			bool untouched_;
        public:
			impl_type(std::shared_ptr<const tile_def> def);

            const tess::vertex& vertex(const std::string& v) const;
            const std::vector<tess::vertex>& vertices() const;
            const std::vector<tess::edge>& edges() const;
            std::string name() const;
            void set(std::vector<tess::vertex>&& vertices, std::vector<tess::edge>&& edges );
			expr_value get_field(const std::string& field) const;
			bool is_untouched() const;
			void apply(const matrix& mat);
    };
}