#include "tile_def.h"
#include "math_util.h"
#include "tessera/tile.h"
#include "tile_impl.h"
#include "parser/tile_parser.h"
#include <symengine/expression.h>
#include <symengine/logic.h>

namespace se = SymEngine;

void set_indices(std::unordered_map<std::string, tess::vertex_def>& verts, std::unordered_map<std::string, tess::edge_def_helper>& edges)
{
    std::unordered_map<std::string, std::string> edge_from_tbl;
    std::transform(edges.begin(), edges.end(), std::inserter(edge_from_tbl, edge_from_tbl.end()),
        [](const auto& pair) {
            const auto& [name, val] = pair;
            return std::pair<std::string, std::string>(val.u, name);
        }
    );
    std::string edge_lbl = edges.begin()->second.name;
    auto start = edges[edge_lbl].u;
    int e_index = 0;
    int v_index = 0;
    do {
        if (edges.find(edge_lbl) == edges.end())
            throw tess::parser::exception("tile", "unknown edge: " + edge_lbl);

        auto& edge = edges[edge_lbl];
        if (verts.find(edge.u) == verts.end())
            throw tess::parser::exception("tile", "unknown vertex: " + edge.v);
        if (verts.find(edge.v) == verts.end())
            throw tess::parser::exception("tile", "unknown vertex: " + edge.v);

        edge.index = e_index++;
        auto& u = verts[edge.u];
        auto& v = verts[edge.v];
        if (u.index < 0)
            u.index = v_index++;
        if (v.index < 0)
            v.index = v_index++;
        edge_lbl = edge_from_tbl[edge.v];
    } while (edges[edge_lbl].u != start);
}

tess::parser::exception tess::tile_def::get_exception(const std::string& msg)
{
    return tess::parser::exception("tile ", msg);
}

std::optional<tess::parser::exception> tess::tile_def::initialize(std::unordered_map<std::string, vertex_def>& verts, std::unordered_map<std::string, edge_def_helper>& edges)
{
    try {
        vertices_.resize(verts.size());
        for (auto& [dummy, vert] : verts)
            vertices_[vert.index] = std::make_shared<vertex_def>(vert);

        edges_.resize(edges.size());
		for (auto& [dummy, edge_helper] : edges) {
			if (edge_helper.length == nullptr)
                edge_helper.length = std::make_shared<number_expr>(1);
            int u = verts[edge_helper.u].index;
            int v = verts[edge_helper.v].index;
			edges_[edge_helper.index] = std::make_shared<edge_def>(edge_helper, u, v);
		}

    } catch (tess::parser::exception e) {
        return e;
    } catch (...) {
        return tess::parser::exception("tile", "error resolving edges and vertices");
    }

    std::transform(vertices_.begin(), vertices_.end(), std::inserter(name_to_vertex_, name_to_vertex_.end()),
        [](std::shared_ptr<vertex_def> v)->std::pair<std::string,vertex_def&> {
            return { v->name, *v };
        }
    );

    std::transform(edges_.begin(), edges_.end(), std::inserter(name_to_edge_, name_to_edge_.end()),
        [](std::shared_ptr<edge_def> e)->std::pair<std::string, edge_def&> {
            return { e->name, *e };
        }
    );

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
    params_(params)
{
    auto name_to_vertex = std::get<0>(v_e);
    auto name_to_edge = std::get<1>(v_e);
    if (name_to_vertex.size() < 3)
        throw get_exception("too few vertices");
    if (name_to_edge.size() < name_to_vertex_.size())
        throw get_exception("too few edges");
    if (name_to_edge.size() > name_to_vertex.size())
        throw get_exception("too many edges");

    set_indices(name_to_vertex, name_to_edge);
    auto maybe_err = initialize(name_to_vertex, name_to_edge);
    if (maybe_err.has_value())
        throw maybe_err.value();
}

tess::tile_def::tile_def(const std::vector<std::string>& params, 
            const std::vector<std::shared_ptr<vertex_def>>& vertices, 
            const std::vector<std::shared_ptr<edge_def>>& edges) :
    params_(params),
    vertices_(vertices),
    edges_(edges)
{
    std::transform(vertices_.begin(), vertices_.end(), std::inserter(name_to_vertex_, name_to_vertex_.end()),
        [](const std::shared_ptr<vertex_def>& vert)->std::pair<std::string, vertex_def&> {
            return { vert->name, *vert };
        }
    );

    std::transform(edges_.begin(), edges_.end(), std::inserter(name_to_edge_, name_to_edge_.end()),
        [](const std::shared_ptr<edge_def>& edge)->std::pair<std::string, edge_def&> {
            return { edge->name, *edge };
        }
    );
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
    std::vector<std::shared_ptr<vertex_def>>  simplified_verts(vertices_.size());
    std::transform(vertices_.begin(), vertices_.end(), simplified_verts.begin(),
        [](const auto& v) {
            return std::make_shared<vertex_def>(v->simplify());
        }
    );
    std::vector<std::shared_ptr<edge_def>>  simplified_edges(edges_.size());
    std::transform(edges_.begin(), edges_.end(), simplified_edges.begin(),
        [](const auto& e) {
            return std::make_shared<edge_def>(e->simplify());
        }
    );
    return tile_def(parameters(), simplified_verts, simplified_edges);
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
			return make_tess_obj<tess::vertex>(
				new_tile_impl.get(), v->index, vert_locations[v->index]
			);
        }
    );

	std::vector<tess::edge> edges(n);
	std::transform(edges_.cbegin(), edges_.cend(), edges.begin(),
		[&](auto e) {
			return make_tess_obj<tess::edge>(new_tile_impl.get(), e->index);
		}
	);
   
    new_tile_impl->set( std::move(verts), std::move(edges) );
    return { 
        make_tess_obj_from_impl<tess::tile>(
            new_tile_impl
        )
	};
}

const tess::vertex_def& tess::tile_def::vertex(int  i) const
{
    return *vertices_.at(i);
}

const tess::edge_def& tess::tile_def::edge(int i) const
{
    return *edges_.at(i);
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
    return tess::patch_def(
        params_,
        body_->simplify()
    );
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

