#include "tile_def.h"
#include "math_util.h"
#include "tessera/tile.h"
#include "tile_impl.h"
#include "script_impl.h"
#include "parser/tile_parser.h"
#include <symengine/expression.h>
#include <symengine/logic.h>

namespace se = SymEngine;
/*
namespace {

    tess::SymPoint walk_edge(const tess::execution_ctxt& ctxt, const tess::edge_def* e, const se::Expression& theta) {
        auto pt = e->prev->pos.value();
        auto len = std::get<tess::number>(e->length->eval(ctxt));
        return  tess::SymPoint{ pt.x + len * se::cos(theta), pt.y + len * se::sin(theta) };
    }

}

std::optional<tess::parser::exception> tess::tile_def::link_vertices()
{
    for (auto& [name, e] : edges_) {
        if (vertices_.find(e.u) == vertices_.end())
            return get_exception("Unknown vertex: " + e.u);
        if (vertices_.find(e.v) == vertices_.end())
            return get_exception("Unknown vertex: " + e.v);
        auto& vert1 = vertices_[e.u];
        auto& vert2 = vertices_[e.v];
        vert1.next = &e;
        e.next = &vert2;

        e.prev = &vert1;
        vert2.prev = &e;
    }
    int count = 1;
    const auto* start = &first_vertex();
    edge_def* e = start->next;
    while (e != nullptr && e->next != start) {
        if (e->length == nullptr)
            e->length = std::make_shared<number_expr>(1);
        if (vertices_[e->u].angle == nullptr)
            return get_exception("insufficiently specified, vertex " + e->u);
        if (vertices_[e->v].angle == nullptr)
            return get_exception("insufficiently specified, vertex " + e->v);

        if (e->next == nullptr)
            return get_exception("edge " + e->name + " has no successor");
        ++count;
        e = e->next->next;
    }

    if (e == nullptr || count != edges_.size())
        return get_exception("insufficiently specified");
    
    if (e->length == nullptr)
        e->length = std::make_shared<number_expr>(1);

    return std::nullopt;
}

std::optional<tess::parser::exception> tess::tile_def::build(const execution_ctxt& ctxt)
{
    if (auto error_during_linking = link_vertices(); error_during_linking.has_value())
        return error_during_linking;

    auto start = &first_edge();
    start->prev->pos = { se::Expression(0), se::Expression(0) };
    edge_def* e = start;
    auto theta = se::Expression(0);
    while (!e->next->pos.has_value()) {

        auto maybe_length = e->length->eval(ctxt);
        if (!std::holds_alternative<tess::number>(maybe_length))
            return std::nullopt;

        e->next->pos = walk_edge(ctxt, e, theta);
        e = e->next->next;

        auto maybe_angle_at_v = e->next->angle->eval(ctxt);
        if (!std::holds_alternative<tess::number>(maybe_angle_at_v))
            return std::nullopt;

        auto delta_theta = se::Expression(se::pi) - std::get<tess::number>(maybe_angle_at_v);
        theta = theta + delta_theta;
    } 

    // check that the point after walking the edges is the start point;
    if (!equals(walk_edge(ctxt, e, theta), first_vertex().pos.value()))
        return get_exception("inconsistent tile geometry ");

    return std::nullopt;
}
*/

tess::parser::exception tess::tile_def::get_exception(const std::string& msg)
{
    return tess::parser::exception("tile " + name_, msg);
}
/*
void tess::tile_def::set_value(const tess::tile& prototype)
{
    prototype_ = prototype;
}
*/
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

std::vector<std::tuple<tess::number, tess::number>> tess::tile_def::evaluate_vertices(execution_ctxt& ctxt) const
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

tess::tile_def::tile_def(const std::string& name, std::vector<std::string> params, const text_range& source_code) :
    name_(name), params_(std::move(params))
{
    auto results = tess::parser::parse_tile(source_code);
    if (std::holds_alternative<tess::parser::tile_verts_and_edges>(results)) {
        auto& verts_and_edges = std::get<tess::parser::tile_verts_and_edges>(results);
        name_to_vertex_ = std::move(std::get<0>(verts_and_edges));
        name_to_edge_ = std::move(std::get<1>(verts_and_edges));
    } else {
        auto e = std::get<tess::parser::exception>(results);
        e.push_stack_item("tile " + name);
        if (!e.has_where())
            e.set_where(source_code.end());
        throw e;
    }

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
/*
tess::vertex_def& tess::tile_def::first_vertex()
{
    return vertices_.begin()->second;
}

tess::edge_def& tess::tile_def::first_edge()
{
    return *first_vertex().next;
}
*/
std::string tess::tile_def::name() const
{
	return name_;
}

std::vector<std::string> tess::tile_def::params() const
{
	return params_;
}

tess::expr_value tess::tile_def::eval( execution_ctxt& ctxt) const
{
	auto n = num_vertices();
    //if (prototype_.has_value())
    //    return { prototype_.value() };

    const auto& script = ctxt.script();
    auto new_tile = tile( 
        std::make_shared<tile_impl>(
            script.get_tile_prototype(name_)
        ) 
    );

    auto vert_locations = evaluate_vertices(ctxt);
    
    std::vector<tess::vertex> verts(n);
    std::transform(vertices_.cbegin(), vertices_.cend(), verts.begin(),
        [&](auto v) {

            const tile_impl* parent = &(*new_tile.impl_);
            std::shared_ptr<const vertex_def> prototype = v;
            std::tuple<number, number> loc = vert_locations[v->index];

            return tess::vertex(
                std::make_shared<vertex_impl>(parent, prototype, loc)
            );
        }
    );

	std::vector<tess::edge> edges(n);
	std::transform(edges_.cbegin(), edges_.cend(), edges.begin(),
		[&](auto e) {
			const tile_impl* parent = &(*new_tile.impl_);
			std::shared_ptr<const edge_def> prototype = e;
			return tess::edge(
				std::make_shared<edge_impl>(parent, prototype)
			);
		}
	);
   
	new_tile.impl_->set( std::move(verts), std::move(edges));

    return { 
		new_tile
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
