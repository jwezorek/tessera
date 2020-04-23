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

    struct vert_fields {
        int index;
        std::string name;
        std::string class_;
    };

    struct edge_fields {
        int index;
        std::string name;
        std::string class_;
        int u;
        int v;
    };

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

        edge_def simplify() const {
            return edge_def(
                name,
                u,
                v,
                class_,
                length->simplify(),
                index
            );
        }

        edge_fields to_fields() const {
            return {
                 index,
                 name,
                 class_,
                 u, v,
            };
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

        vertex_def simplify() const {
            return vertex_def(
                name,
                angle->simplify(),
                class_,
                index
            );
        }

        vert_fields to_fields() const {
            return {
                index,
                name,
                class_
            };
        }
    };

    using tile_verts_and_edges = std::tuple< std::unordered_map<std::string, vertex_def>, std::unordered_map<std::string, edge_def_helper>>;

    class tile_def_expr : public expression, public tessera_impl
    {
    private:
        std::vector<vertex_def> vertices_;
        std::vector<edge_def> edges_;

        tess::parser::exception get_exception(const std::string& msg);
        std::optional<parser::exception> initialize(std::unordered_map<std::string, vertex_def>&, std::unordered_map<std::string, edge_def_helper>&);
        std::vector<std::tuple<number, number>> evaluate_vertices(eval_context&) const;
        std::vector<edge_fields> get_edge_fields() const;
        std::vector<vert_fields> get_vert_fields() const;

    public:
        tile_def_expr(const tile_verts_and_edges& v_e);
        tile_def_expr(const std::vector<vertex_def>& v, const std::vector<edge_def>& e);
        expr_value eval(eval_context&) const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::vector<std::string>& dependencies) const override;
    };

}