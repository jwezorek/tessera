#include "tile_def.h"
#include "math_util.h"
#include "tessera/tile.h"
#include "tile_impl.h"
#include "parser/tile_parser.h"
#include <symengine/expression.h>
#include <symengine/logic.h>

namespace se = SymEngine;

tess::parser::exception tess::tile_def::get_exception(const std::string& msg)
{
    return tess::parser::exception("tile " + name_, msg);
}

void tess::tile_def::set_indices()
{
    std::unordered_map<std::string, std::string> edge_from_tbl;
    std::transform(name_to_edge_.begin(), name_to_edge_.end(), std::inserter(edge_from_tbl, edge_from_tbl.end()),
        [](const auto& pair) {
            const auto& [name, val] = pair;
            return std::pair<std::string, std::string>(val.u, name);
        }
    );
    std::string edge_lbl = name_to_edge_.begin()->second.name;
    auto start = name_to_edge_[edge_lbl].u;
    int e_index = 0;
    int v_index = 0;
    do {
        if (name_to_edge_.find(edge_lbl) == name_to_edge_.end())
            throw tess::parser::exception("tile " + name_, "unknown edge: " + edge_lbl);

        auto& edge = name_to_edge_[edge_lbl];
        if (name_to_vertex_.find(edge.u) == name_to_vertex_.end())
            throw tess::parser::exception("tile " + name_, "unknown vertex: " + edge.v);
        if (name_to_vertex_.find(edge.v) == name_to_vertex_.end())
            throw tess::parser::exception("tile " + name_, "unknown vertex: " + edge.v);

        edge.index = e_index++;
        auto& u = name_to_vertex_[edge.u];
        auto& v = name_to_vertex_[edge.v];
        if (u.index < 0)
            u.index = v_index++;
        if (v.index < 0)
            v.index = v_index++;
        edge_lbl = edge_from_tbl[edge.v];
    } while (name_to_edge_[edge_lbl].u != start);
}

std::optional<tess::parser::exception> tess::tile_def::initialize()
{
    try {
        set_indices();

        vertices_.resize(name_to_vertex_.size());
        for (auto& [dummy, vert] : name_to_vertex_) 
            vertices_[vert.index] = std::make_shared<const vertex_def>(vert);

        edges_.resize(name_to_edge_.size());
		for (auto& [dummy, edge] : name_to_edge_) {
			if (edge.length == nullptr)
				edge.length = std::make_shared<number_expr>(1);
			edges_[edge.index] = std::make_shared<const edge_def>(edge);
		}

    } catch (tess::parser::exception e) {
        return e;
    }
    catch (...) {
        return tess::parser::exception("tile " + name_, "error resolving edges and vertices");
    }
    return std::nullopt;
}

std::vector<std::tuple<tess::number, tess::number>> tess::tile_def::evaluate_vertices(eval_context& ctxt) const
{
	auto n = num_vertices();
	tess::number x = 0.0;
	tess::number y = 0.0;
	tess::number theta = 0.0;

    std::vector<std::tuple<number, number>> locs(vertices_.size());

	for (int i = 0; i < n; ++i) {
		locs[i] = { x,y };
		auto length = std::get<tess::number>(edges_[i]->length->eval(ctxt));
		x = x + length * se::cos(theta);
		y = y + length * se::sin(theta);
		auto delta_theta = se::Expression(se::pi) - std::get<tess::number>(vertices_[(i + 1) % n]->angle->eval(ctxt));
		theta = theta + delta_theta;
	}

    return locs;
}

tess::tile_def::tile_def(const std::vector<std::string>& params, const tile_verts_and_edges& v_e) :
    params_(params),
    name_to_vertex_(std::get<0>(v_e)),
    name_to_edge_(std::get<1>(v_e))
{
    if (name_to_vertex_.size() < 3)
        throw get_exception("too few vertices");
    if (name_to_edge_.size() < name_to_vertex_.size())
        throw get_exception("too few edges");
    if (name_to_edge_.size() > name_to_vertex_.size())
        throw get_exception("too many edges");

    auto maybe_err = initialize();
    if (maybe_err.has_value())
        throw maybe_err.value();
}

std::string tess::tile_def::name() const
{
	return name_;
}

const std::vector<std::string>& tess::tile_def::parameters() const
{
	return params_;
}

std::vector<std::string> tess::tile_def::get_variables() const
{
    std::vector<std::string> vars;
    for (const auto& edge : edges_)
        edge->length->get_dependencies(vars);
    for (const auto& vertex : vertices_)
        vertex->angle->get_dependencies(vars);
    return vars;
}

tess::tile_def tess::tile_def::simplify() const
{
    throw std::runtime_error("TODO");
}

tess::expr_value tess::tile_def::call( eval_context& ctxt) const
{
	auto n = num_vertices();
    auto new_tile_impl = std::make_shared<tile::impl_type>(
        shared_from_this()
    ); 
    auto vert_locations = evaluate_vertices(ctxt);
    
    std::vector<tess::vertex> verts(n);
    std::transform(vertices_.cbegin(), vertices_.cend(), verts.begin(),
        [&](auto v) {
            std::shared_ptr<const vertex_def> definition = v;
			return make_tess_obj<tess::vertex>(
				new_tile_impl.get(), definition, vert_locations[v->index]
			);
        }
    );

	std::vector<tess::edge> edges(n);
	std::transform(edges_.cbegin(), edges_.cend(), edges.begin(),
		[&](auto e) {
			std::shared_ptr<const edge_def> definition = e;
			return make_tess_obj<tess::edge>(new_tile_impl.get(), definition);
		}
	);
   
    new_tile_impl->set( std::move(verts), std::move(edges) );
    return { 
        make_tess_obj_from_impl<tess::tile>(
            new_tile_impl
        )
	};
}

const tess::vertex_def& tess::tile_def::vertex(const std::string& v) const
{
    return name_to_vertex_.at(v);
}

const tess::edge_def& tess::tile_def::edge(const std::string& e) const
{
    return name_to_edge_.at(e);
}

int tess::tile_def::num_vertices() const
{
	return static_cast<int>(vertices_.size());
}

int tess::tile_def::get_edge_index(const std::string& e) const
{
	auto i = name_to_edge_.find(e);
	if (i == name_to_edge_.end())
		return -1;
	return i->second.index;
}

int tess::tile_def::get_vertex_index(const std::string& v) const
{
	auto i = name_to_vertex_.find(v);
	if (i == name_to_vertex_.end())
		return -1;
	return i->second.index;
}

/*---------------------------------------------------------------------------------------------------*/

tess::patch_def::patch_def(const std::vector<std::string>& params, expr_ptr body):
    params_(params),
    body_(body)
{
}

std::vector<std::string> tess::patch_def::get_variables() const
{
    std::vector<std::string> vars;
    body_->get_dependencies(vars);
    return vars;
}

tess::patch_def tess::patch_def::simplify() const
{
    throw std::runtime_error("TODO");
}

tess::expr_value tess::patch_def::call(eval_context& ctxt) const
{
    return body_->eval(ctxt);
}

const std::vector<std::string>& tess::patch_def::parameters() const
{
    return params_;
}

tess::expr_ptr tess::patch_def::body() const
{
    return body_;
}
