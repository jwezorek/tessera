#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "text_range.h"
#include "expression.h"

namespace tess {

    struct edge_fields {
        std::string name;
        std::string u;
        std::string v;
        std::string class_;
        expr_ptr length;
    };

    struct vertex_fields {
        std::string name;
        expr_ptr angle;
        std::string class_;
    };

    class tile
    {
    private: 
        std::string name_;
        std::vector<std::string> params_;
        std::unordered_map<std::string, edge_fields> edge_definitions_;
        std::unordered_map<std::string, vertex_fields> vertex_definitions_;

    public:
        tile(const std::string& name , std::vector<std::string> params, const text_range& source_code);
    };
}