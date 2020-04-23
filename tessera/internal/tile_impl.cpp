#include "tile_impl.h"
#include "cluster.h"
#include "parser/keywords.h"
#include <symengine/expression.h>
#include <algorithm>

namespace se = SymEngine;

namespace {

	tess::cluster edges_as_cluster(const std::vector<tess::edge>& edges) {
		std::vector<tess::expr_value> cluster_contents(edges.size());
		std::transform(edges.begin(), edges.end(), cluster_contents.begin(),
			[](const tess::edge& e)->tess::expr_value { return { e }; }
		);
		return tess::cluster(cluster_contents);
	}
}

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

void tess::tile::impl_type::set( std::vector<tess::vertex>&& vertices, std::vector<tess::edge>&& edges )
{
	vertices_ = std::move(vertices);
    edges_ = std::move(edges);
}

tess::expr_value tess::tile::impl_type::get_field(const std::string& field) const
{
	if (field == parser::keyword(parser::kw::edge)) {
		return { edges_as_cluster(edges_) };
	}

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

const tess::vertex_def& tess::tile::impl_type::vert_fields(int i) const
{
	return def_->vertex(i);
}

const tess::edge_def& tess::tile::impl_type::edge_fields(int i) const
{
	return def_->edge(i);
}

bool tess::tile::impl_type::has_parent() const {
	return parent_ != nullptr;
}

tess::tile_patch::impl_type* tess::tile::impl_type::parent() const {
	return parent_;
}


/*--------------------------------------------------------------------------------*/

tess::edge::impl_type::impl_type( tile::impl_type* parent, int index) :
	parent_(parent),
	index_(index)
{}

std::string tess::edge::impl_type::name() const
{
    return parent_->edge_fields(index_).name;
}

std::string tess::edge::impl_type::edge_class() const
{
	return parent_->edge_fields(index_).class_;
}

const tess::vertex& tess::edge::impl_type::u() const
{
	int index = parent_->edge_fields(index_).u;
    return parent_->vertices().at(index);
}

const tess::vertex& tess::edge::impl_type::v() const
{
	int index = parent_->edge_fields(index_).v;
    return parent_->vertices().at(index);
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

tess::vertex::impl_type::impl_type( tile::impl_type* parent, int n, std::tuple<number, number> loc) :
    parent_(parent), index_(n), x_(std::get<0>(loc)), y_(std::get<1>(loc))
{
}

std::string tess::vertex::impl_type::name() const
{
    return parent_->vert_fields(index_).name;
}

std::string tess::vertex::impl_type::vertex_class() const
{
    return parent_->vert_fields(index_).class_;
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
