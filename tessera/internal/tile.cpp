#include "tile.h"
#include "math_util.h"
#include "parser/tile_parser.h"
#include <symengine/expression.h>
#include <symengine/logic.h>

namespace se = SymEngine;

namespace {

    tess::SymPoint walk_edge(const tess::edge* e, const se::Expression& theta) {
        auto pt = e->prev->pos.value();
        auto len = std::get<tess::number>(e->length->eval({}));
        return  tess::SymPoint{ pt.x + len * se::cos(theta), pt.y + len * se::sin(theta) };
    }

}

std::optional<tess::parser::exception> tess::tile::link_vertices()
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
    edge* e = start->next;
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

std::optional<tess::parser::exception> tess::tile::build()
{
    if (auto error_during_linking = link_vertices(); error_during_linking.has_value())
        return error_during_linking;

    auto start = &first_edge();
    start->prev->pos = { se::Expression(0), se::Expression(0) };
    edge* e = start;
    auto theta = se::Expression(0);
    while (!e->next->pos.has_value()) {

        auto maybe_length = e->length->eval({});
        if (!std::holds_alternative<tess::number>(maybe_length))
            return std::nullopt;

        e->next->pos = walk_edge(e, theta);
        e = e->next->next;

        auto maybe_angle_at_v = e->next->angle->eval({});
        if (!std::holds_alternative<tess::number>(maybe_angle_at_v))
            return std::nullopt;

        auto delta_theta = se::Expression(se::pi) - std::get<tess::number>(maybe_angle_at_v);
        theta = theta + delta_theta;
    } 

    // check that the point after walking the edges is the start point;
    if (!equals(walk_edge(e, theta), first_vertex().pos.value()))
        return get_exception("inconsistent tile geometry ");

    return std::nullopt;
}

tess::parser::exception tess::tile::get_exception(const std::string& msg)
{
    return tess::parser::exception("tile " + name_, msg);
}

tess::tile::tile(const std::string& name, std::vector<std::string> params, const text_range& source_code) :
    name_(name), params_(std::move(params))
{
    auto results = tess::parser::parse_tile(source_code);
    if (std::holds_alternative<tess::parser::tile_verts_and_edges>(results)) {
        auto& verts_and_edges = std::get<tess::parser::tile_verts_and_edges>(results);
        vertices_ = std::move(std::get<0>(verts_and_edges));
        edges_ = std::move(std::get<1>(verts_and_edges));
    } else {
        auto e = std::get<tess::parser::exception>(results);
        e.push_stack_item("tile " + name);
        if (!e.has_where())
            e.set_where(source_code.end());
        throw e;
    }

    if (vertices_.size() < 3)
        throw get_exception("too few vertices");
    if (edges_.size() < vertices_.size())
        throw get_exception("too few edges");
    if (edges_.size() > vertices_.size())
        throw get_exception("too many edges");

    auto result = build();
    if (result.has_value())
        throw result.value();
}

tess::vertex& tess::tile::first_vertex()
{
    return vertices_.begin()->second;
}

tess::edge& tess::tile::first_edge()
{
    return *first_vertex().next;
}

