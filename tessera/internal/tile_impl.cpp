#include "tile_impl.h"
#include <symengine/expression.h>

namespace se = SymEngine;

const tess::vertex& tess::tile::tile_impl::vertex(const std::string& v) const
{
    return vertices_[def_->vertex(v).index];
}

const std::vector<tess::vertex>& tess::tile::tile_impl::vertices() const
{
    return vertices_;
}

const std::vector<tess::edge>& tess::tile::tile_impl::edges() const
{
    return edges_;
}

std::string tess::tile::tile_impl::name() const
{
    return def_->name();
}

/*--------------------------------------------------------------------------------*/

std::string tess::edge::edge_impl::name() const
{
    return def_->name;
}

std::string tess::edge::edge_impl::edge_class() const
{
    return def_->class_;
}

tess::vertex tess::edge::edge_impl::u() const
{
    return parent_->vertex(def_->u);
}

tess::vertex tess::edge::edge_impl::v() const
{
    return parent_->vertex(def_->v);
}

/*--------------------------------------------------------------------------------*/

std::string tess::vertex::vertex_impl::name() const
{
    return def_->name;
}

std::string tess::vertex::vertex_impl::vertex_class() const
{
    return def_->class_;
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