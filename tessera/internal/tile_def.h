#pragma once

#include "math_util.h"
#include "text_range.h"
#include "expression.h"
#include "tessera_impl.h"
#include "tessera/error.h"
#include "parser/exception.h"
#include <symengine/expression.h>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <unordered_map>

namespace tess {

    struct edge_def_helper {
        std::string name;
        std::string u;
        std::string v;
        std::string class_;
        expr_ptr length;
        int index;
    };

    struct edge_def {
        std::string name;
        int u;
        int v;
        std::string class_;
        expr_ptr length;
        int index;

        edge_def() : index(-1) {}

        edge_def(const edge_def_helper& helper, int u, int v) :
            name(helper.name), u(u), v(v), class_(helper.class_), length(helper.length), index(helper.index)
        {}

        edge_def( std::string n, int uu, int vv, std::string c, expr_ptr l, int i) :
            name(n), u(uu), v(vv), class_(c), length(l), index(i)
        {}

        edge_def simplify() {
            return edge_def(
                name,
                u,
                v,
                class_,
                length->simplify(),
                index
            );
        }
    };

    struct vertex_def {
        std::string name;
        expr_ptr angle;
        std::string class_;
        int index;
        vertex_def() : index(-1) {}

        vertex_def(std::string n, expr_ptr ang, std::string c, int i) :
            name(n), angle(ang), class_(c), index(i)
        { }

        vertex_def simplify() {
            return vertex_def(
                name,
                angle->simplify(),
                class_,
                index
            );
        }
    };

    using tile_verts_and_edges = std::tuple< std::unordered_map<std::string, vertex_def>, std::unordered_map<std::string, edge_def_helper>>;

    class tile_def : public tessera_impl, public std::enable_shared_from_this<tile_def>
    {
    private: 
        std::vector<std::string> params_;
        std::unordered_map<std::string, edge_def&> name_to_edge_;
        std::unordered_map<std::string, vertex_def&> name_to_vertex_;
        std::vector<std::shared_ptr<edge_def>> edges_;
        std::vector<std::shared_ptr<vertex_def>> vertices_;
        tess::parser::exception get_exception(const std::string& msg);
        std::optional<parser::exception> initialize( std::unordered_map<std::string, vertex_def>&, std::unordered_map<std::string, edge_def_helper>&);
        std::vector<std::tuple<number, number>> evaluate_vertices(eval_context&) const;

    public:
        tile_def(const std::vector<std::string>& params, const tile_verts_and_edges& v_e);
        tile_def(const std::vector<std::string>& params, const std::vector<std::shared_ptr<vertex_def>>& vertices, const std::vector<std::shared_ptr<edge_def>>& edges);
        const std::vector<std::string>& parameters() const;
        std::vector<std::string> get_variables() const;
        tile_def simplify() const;
		expr_value call( eval_context& ) const;
        const vertex_def& vertex(int i) const;
        const edge_def& edge(int i) const;
        const vertex_def& vertex(const std::string& v) const;
        const edge_def& edge(const std::string& e) const;
		int num_vertices() const;
		int get_edge_index(const std::string& e) const;
		int get_vertex_index(const std::string& v) const;
    };

    class patch_def : public tessera_impl
    {
    private:
        std::vector<std::string> params_;
        expr_ptr body_;
    public:
        patch_def(const std::vector<std::string>& params, expr_ptr body);
        std::vector<std::string> get_variables() const;
        patch_def simplify() const;
        expr_value call(eval_context&) const;
        const std::vector<std::string>& parameters() const;
        expr_ptr body() const;
    };
}