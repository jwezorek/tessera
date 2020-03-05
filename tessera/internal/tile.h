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
    struct vertex;

    struct edge {
        std::string name;
        std::string u;
        std::string v;
        std::string class_;
        expr_ptr length;
        vertex* prev;
        vertex* next;
        edge() : prev(nullptr), next(nullptr) {}
    };

    struct vertex {
        std::string name;
        expr_ptr angle;
        std::string class_;
        edge* prev;
        edge* next;
        std::optional<SymPoint> pos;
        vertex() : prev(nullptr), next(nullptr) {}
    };

    class tile
    {
    private: 
        std::string name_;
        std::vector<std::string> params_;
        std::unordered_map<std::string, edge> edges_;
        std::unordered_map<std::string, vertex> vertices_;
        std::string first_vert_label;

        std::optional<tess::parser::exception> link_vertices();
        std::optional<tess::parser::exception> build();
        tess::parser::exception get_exception(const std::string& msg);

    public:
        tile(const std::string& name , std::vector<std::string> params, const text_range& source_code);
        vertex& first_vertex();
        edge& first_edge();
    };
}