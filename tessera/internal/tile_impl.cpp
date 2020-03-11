#include "tile_impl.h"
#include <symengine/expression.h>

namespace se = SymEngine;

const std::vector<tess::vertex>& tess::tile::tile_impl::vertices()
{
    return vertices_;
}

const std::vector<tess::edge>& tess::tile::tile_impl::edges()
{
    return edges_;
}

std::string tess::tile::tile_impl::name() const
{
    return name_;
}

/*--------------------------------------------------------------------------------*/

std::string tess::edge::edge_impl::name() const
{
    return name_;
}

std::string tess::edge::edge_impl::edge_class() const
{
    return edge_class_;
}

tess::vertex tess::edge::edge_impl::u() const
{
    return u_;
}

tess::vertex tess::edge::edge_impl::v() const
{
    return v_;
}

/*--------------------------------------------------------------------------------*/

std::string tess::vertex::vertex_impl::name() const
{
    return name_;
}

std::string tess::vertex::vertex_impl::vertex_class() const
{
    return vertex_class_;
}

std::tuple<double, double> tess::vertex::vertex_impl::to_floats() const
{
    return {
        se::eval_double(x_),
        se::eval_double(y_),
    };
}

std::tuple<tess::number, tess::number> tess::vertex::vertex_impl::pos() const
{
    return {
        x_,
        y_
    };
}