#pragma once

#include "tessera/tile.h"
#include <string>
#include <vector>
#include <tuple>
#include "expr_value.h"

namespace tess {

    class vertex::vertex_impl {
    private:
        std::string name_;
        std::string vertex_class_;
        number x_;
        number y_;
    public:
        std::string name() const;
        std::string vertex_class() const;
        std::tuple<double, double> to_floats() const;
        std::tuple<number, number> pos() const;
    };

    class edge::edge_impl {
        private:
            std::string name_;
            std::string edge_class_;
            vertex u_;
            vertex v_;
        public:
            std::string name() const;
            std::string edge_class() const;
            vertex u() const;
            vertex v() const;
    };

    class tile::tile_impl {
        private:
            std::string name_;
            std::vector<tess::edge> edges_;
            std::vector<tess::vertex> vertices_;
        public:
            const std::vector<tess::vertex>& vertices();
            const std::vector<tess::edge>& edges();
            std::string name() const;
    };
}