#include "tile_impl.h"
#include <symengine/expression.h>

namespace se = SymEngine;

tess::tile_impl::tile_impl( std::shared_ptr<const tile_def> def) : 
    def_(def)
{
}

const tess::vertex& tess::tile_impl::vertex(const std::string& v) const
{
    return vertices_[def_->vertex(v).index];
}

const std::vector<tess::vertex>& tess::tile_impl::vertices() const
{
    return vertices_;
}

const std::vector<tess::edge>& tess::tile_impl::edges() const
{
    return edges_;
}

std::string tess::tile_impl::name() const
{
    return def_->name();
}

void tess::tile_impl::set( std::vector<tess::vertex>&& vertices, std::vector<tess::edge>&& edges )
{
	vertices_ = std::move(vertices);
    edges_ = std::move(edges);
}

/*--------------------------------------------------------------------------------*/

tess::edge_impl::edge_impl(const tile_impl* parent, std::shared_ptr<const edge_def> prototype) :
	parent_(parent),
	def_(prototype)
{}

std::string tess::edge_impl::name() const
{
    return def_->name;
}

std::string tess::edge_impl::edge_class() const
{
    return def_->class_;
}

const tess::vertex& tess::edge_impl::u() const
{
    return parent_->vertex(def_->u);
}

const tess::vertex& tess::edge_impl::v() const
{
    return parent_->vertex(def_->v);
}

/*--------------------------------------------------------------------------------*/

tess::vertex_impl::vertex_impl(const tile_impl* parent, std::shared_ptr<const vertex_def> prototype, std::tuple<number, number> loc) :
    parent_(parent), def_(prototype), x_(std::get<0>(loc)), y_(std::get<1>(loc))
{
}

std::string tess::vertex_impl::name() const
{
    return def_->name;
}

std::string tess::vertex_impl::vertex_class() const
{
    return def_->class_;
}

std::tuple<double, double> tess::vertex_impl::to_floats() const
{
    return {
        se::eval_double(x_),
        se::eval_double(y_),
    };
}

std::tuple<tess::number, tess::number> tess::vertex_impl::pos() const
{
    return {
        x_,
        y_
    };
}