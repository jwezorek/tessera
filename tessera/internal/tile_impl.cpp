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

tess::tile::impl_type::impl_type(const std::vector<tess::vert_fields>& v, const std::vector<tess::edge_fields>& e) :
    fields_(std::make_shared<tess::tile::impl_type::fields>(v,e)), untouched_(true), parent_(nullptr)
{
}

const tess::vertex& tess::tile::impl_type::vertex(const std::string& v) const
{
	int index = fields_->vert_name_to_index.at(v);
    return vertices_[index];
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

void tess::tile::impl_type::insert_field(const std::string& var, const expr_value& val)
{
	// TODO: implement use_weak_reference_to
	if (std::holds_alternative<tess::tile>(val)) {
		if (this == get_impl(std::get<tess::tile>(val)).get())
			return;
	}
	fields_->custom_fields[var] = val;
}

tess::expr_value tess::tile::impl_type::get_field(const std::string& field) const
{
	const auto& custom_fields = fields_->custom_fields;
	if (custom_fields.find(field) != custom_fields.end())
		return custom_fields.at(field);

	if (field == parser::keyword(parser::kw::edge) || field == "edges") {
		return { edges_as_cluster(edges_) };
	}

	int index = fields_->get_edge_index(field);
	if (index >= 0) {
		return { edges_.at(index) };
	}
	index = fields_->get_vert_index(field);
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

const tess::vert_fields& tess::tile::impl_type::vert_fields(int i) const
{
	return fields_->vertices.at(i);
}

const tess::edge_fields& tess::tile::impl_type::edge_fields(int i) const
{
	return fields_->edges.at(i);
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

int tess::tile::impl_type::fields::get_edge_index(const std::string& e) {
	auto i = edge_name_to_index.find(e);
	return (i != edge_name_to_index.end()) ? i->second : -1;
}

int tess::tile::impl_type::fields::get_vert_index(const std::string& v) {
	auto i = vert_name_to_index.find(v);
	return (i != vert_name_to_index.end()) ? i->second : -1;
}

tess::tile::impl_type::fields::fields(const std::vector<tess::vert_fields>& v, const std::vector<tess::edge_fields>& e) :
	vertices(v), edges(e)
{
	std::transform(v.begin(), v.end(), std::inserter(vert_name_to_index, vert_name_to_index.end()),
		[](const tess::vert_fields& f)->std::pair<std::string, int> { return { f.name, f.index }; }
	);
	std::transform(e.begin(), e.end(), std::inserter(edge_name_to_index, edge_name_to_index.end()),
		[](const tess::edge_fields& f)->std::pair<std::string, int> { return { f.name, f.index }; }
	);
}

