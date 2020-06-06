#include "tile_def.h"
#include "math_util.h"
#include "tessera/tile.h"
#include "tile_impl.h"
#include "parser/tile_parser.h"
#include "allocator.h"
#include "execution_state.h"
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

tess::parser::exception tess::tile_def_expr::get_exception(const std::string& msg)
{
    return tess::parser::exception("tile ", msg);
}

std::optional<tess::parser::exception> tess::tile_def_expr::initialize(std::unordered_map<std::string, vertex_def>& verts, std::unordered_map<std::string, edge_def_helper>& edges)
{
    try {
        vertices_.resize(verts.size());
        for (auto& [dummy, vert] : verts)
            vertices_[vert.index] = vert;

        edges_.resize(edges.size());
        for (auto& [dummy, edge_helper] : edges) {
            if (edge_helper.length == nullptr)
                edge_helper.length = std::make_shared<number_expr>(1);
            int u = verts[edge_helper.u].index;
            int v = verts[edge_helper.v].index;
            edges_[edge_helper.index] = edge_def(edge_helper, u, v);
        }

    }
    catch (tess::parser::exception e) {
        return e;
    }
    catch (...) {
        return tess::parser::exception("tile", "error resolving edges and vertices");
    }

    return std::nullopt;
}

std::vector<tess::edge_fields> tess::tile_def_expr::get_edge_fields() const
{
    std::vector<edge_fields> fields(edges_.size());
    std::transform(edges_.begin(), edges_.end(), fields.begin(),
        [](const auto& e) { return e.to_fields(); }
    );
    return fields;
}

std::vector<tess::vert_fields> tess::tile_def_expr::get_vert_fields() const
{
    std::vector<vert_fields> fields(vertices_.size());
    std::transform(vertices_.begin(), vertices_.end(), fields.begin(),
        [](const auto& v) { return v.to_fields(); }
    );
    return fields;
}

tess::tile_def_expr::tile_def_expr(const tile_verts_and_edges& v_e)
{
    auto name_to_vertex = std::get<0>(v_e);
    auto name_to_edge = std::get<1>(v_e);
    if (name_to_vertex.size() < 3)
        throw get_exception("too few vertices");
    if (name_to_edge.size() < name_to_vertex.size())
        throw get_exception("too few edges");
    if (name_to_edge.size() > name_to_vertex.size())
        throw get_exception("too many edges");

    set_indices(name_to_vertex, name_to_edge);
    auto maybe_err = initialize(name_to_vertex, name_to_edge);
    if (maybe_err.has_value())
        throw maybe_err.value();
}

tess::tile_def_expr::tile_def_expr(const std::vector<vertex_def>& v, const std::vector<edge_def>& e) :
    vertices_(v), edges_(e)
{
}

void tess::tile_def_expr::compile(stack_machine::stack& stack) const
{
    //DEPRECATED
}

tess::expr_ptr tess::tile_def_expr::simplify() const
{
    std::vector<vertex_def>  simplified_verts(vertices_.size());
    std::transform(vertices_.begin(), vertices_.end(), simplified_verts.begin(),
        [](const auto& v) {
            return v.simplify();
        }
    );
    std::vector<edge_def>  simplified_edges(edges_.size());
    std::transform(edges_.begin(), edges_.end(), simplified_edges.begin(),
        [](const auto& e) {
            return e.simplify();
        }
    );
    return std::make_shared<tile_def_expr>(simplified_verts, simplified_edges);
}

void tess::tile_def_expr::get_dependencies(std::unordered_set<std::string>& vars) const
{
    for (const auto& edge : edges_)
        edge.length->get_dependencies(vars);
    for (const auto& vertex : vertices_)
        vertex.angle->get_dependencies(vars);
}
