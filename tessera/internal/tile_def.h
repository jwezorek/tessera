#pragma once

#include "math_util.h"
#include "text_range.h"
#include "expression.h"
#include "parser/exception.h"
#include <symengine/expression.h>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace tess {
    struct vertex_def;

    struct edge_def {
        std::string name;
        std::string u;
        std::string v;
        std::string class_;
        expr_ptr length;
        vertex_def* prev;
        vertex_def* next;
        edge_def() : prev(nullptr), next(nullptr) {}
    };

    struct vertex_def {
        std::string name;
        expr_ptr angle;
        std::string class_;
        edge_def* prev;
        edge_def* next;
        std::optional<SymPoint> pos;
        vertex_def() : prev(nullptr), next(nullptr) {}
    };

    class tile_def
    {
    private: 
        std::string name_;
        std::vector<std::string> params_;
        std::unordered_map<std::string, edge_def> edges_;
        std::unordered_map<std::string, vertex_def> vertices_;
        std::string first_vert_label;

        std::optional<tess::parser::exception> link_vertices();
        std::optional<tess::parser::exception> build();
        tess::parser::exception get_exception(const std::string& msg);

    public:
        tile_def(const std::string& name , std::vector<std::string> params, const text_range& source_code);
        vertex_def& first_vertex();
        edge_def& first_edge();
    };
}