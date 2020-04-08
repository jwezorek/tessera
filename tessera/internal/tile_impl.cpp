#include "tile_impl.h"
#include <symengine/expression.h>

namespace se = SymEngine;

tess::tile::impl_type::impl_type( std::shared_ptr<const tile_def> def) :
    def_(def), untouched_(true), parent_(nullptr)
{
}

const tess::vertex& tess::tile::impl_type::vertex(const std::string& v) const
{
    return vertices_[def_->vertex(v).index];
}

const std::vector<tess::vertex>& tess::tile::impl_type::vertices() const
{
    return vertices_;
}

const std::vector<tess::edge>& tess::tile::impl_type::edges() const
{
    return edges_;
}

std::string tess::tile::impl_type::name() const
{
    return def_->name();
}

void tess::tile::impl_type::set( std::vector<tess::vertex>&& vertices, std::vector<tess::edge>&& edges )
{
	vertices_ = std::move(vertices);
    edges_ = std::move(edges);
}

tess::expr_value tess::tile::impl_type::get_field(const std::string& field) const
{
	int index = def_->get_edge_index(field);
	if (index >= 0) {
		return { edges_.at(index) };
	}
	index = def_->get_vertex_index(field);
	if (index >= 0) {
		return { vertices_.at(index) };
	}
	return { error(std::string("refrenced undefined tile edge or vertex: ") + field ) };
}

bool tess::tile::impl_type::is_untouched() const
{
	return untouched_;
}

void tess::tile::impl_type::apply(const matrix& mat)
{
	for (auto& vertex : vertices_) {
		get_impl(vertex)->apply(mat);
	}
	untouched_ = false;
}

void tess::tile::impl_type::set_parent(tess::tile_patch::impl_type* parent) {
	parent_ = parent;
	untouched_ = true;
}

bool tess::tile::impl_type::has_parent() const {
	return parent_ != nullptr;
}

tess::tile_patch::impl_type* tess::tile::impl_type::parent() const {
	return parent_;
}


/*--------------------------------------------------------------------------------*/

tess::edge::impl_type::impl_type( tile::impl_type* parent, std::shared_ptr<const edge_def> prototype) :
	parent_(parent),
	def_(prototype)
{}

std::string tess::edge::impl_type::name() const
{
    return def_->name;
}

std::string tess::edge::impl_type::edge_class() const
{
    return def_->class_;
}

const tess::vertex& tess::edge::impl_type::u() const
{
    return parent_->vertex(def_->u);
}

const tess::vertex& tess::edge::impl_type::v() const
{
    return parent_->vertex(def_->v);
}

tess::expr_value tess::edge::impl_type::get_field(const std::string& field) const
{
	return {};
}

tess::tile::impl_type* tess::edge::impl_type::parent() const
{
	return parent_;
}

/*--------------------------------------------------------------------------------*/

tess::vertex::impl_type::impl_type( tile::impl_type* parent, std::shared_ptr<const vertex_def> prototype, std::tuple<number, number> loc) :
    parent_(parent), def_(prototype), x_(std::get<0>(loc)), y_(std::get<1>(loc))
{
}

std::string tess::vertex::impl_type::name() const
{
    return def_->name;
}

std::string tess::vertex::impl_type::vertex_class() const
{
    return def_->class_;
}

std::tuple<double, double> tess::vertex::impl_type::to_floats() const
{
    return {
        se::eval_double(x_),
        se::eval_double(y_),
    };
}

tess::point tess::vertex::impl_type::pos() const
{
    return {
        x_,
        y_
    };
}

tess::expr_value tess::vertex::impl_type::get_field(const std::string& field) const
{
	return {};
}


void tess::vertex::impl_type::apply(const tess::matrix& mat) {

	auto [x,y] = apply_matrix( mat, pos() );
	x_ = x;
	y_ = y;
}

tess::tile::impl_type* tess::vertex::impl_type::parent() const
{
	return parent_;
}
