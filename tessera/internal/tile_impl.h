#pragma once

#include "tessera/tile.h"
#include "tile_def.h"
#include "expr_value.h"
#include <string>
#include <vector>
#include <tuple>
#include <memory>

namespace tess {

    class vertex::vertex_impl {
    private:
        const tile::tile_impl* parent_;
        std::shared_ptr<const vertex_def> def_;
        number x_;
        number y_;
    public:
        vertex_impl(const tile::tile_impl* parent, std::shared_ptr<const vertex_def> prototype, std::tuple<number, number> loc);
        std::string name() const;
        std::string vertex_class() const;
        std::tuple<double, double> to_floats() const;
        std::tuple<number, number> pos() const;
    };

    class edge::edge_impl {
        private:
            const tile::tile_impl* parent_;
            std::shared_ptr<const edge_def> def_;
        public:
            std::string name() const;
            std::string edge_class() const;
            vertex u() const;
            vertex v() const;
    };

    class tile::tile_impl {
        private:
            std::vector<tess::edge> edges_;
            std::vector<tess::vertex> vertices_;
            std::shared_ptr<const tile_def> def_;
        public:
            tile_impl(std::shared_ptr<const tile_def> def);

            const tess::vertex& vertex(const std::string& v) const;
            const std::vector<tess::vertex>& vertices() const;
            const std::vector<tess::edge>& edges() const;
            std::string name() const;
            void set(std::vector<tess::edge>&& edges, std::vector<tess::vertex>&& vertices);
    };
}