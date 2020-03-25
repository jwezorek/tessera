#pragma once

#include "math_util.h"
#include "text_range.h"
#include "expression.h"
#include "tessera/error.h"
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
        int index;
        edge_def() : index(-1) {}
    };

    struct vertex_def {
        std::string name;
        expr_ptr angle;
        std::string class_;
        int index;
        vertex_def() : index(-1) {}
    };

    class tile_def
    {
        friend tessera_script;

    private: 
        std::string name_;
        std::vector<std::string> params_;
        std::unordered_map<std::string, edge_def> edges_;
        std::unordered_map<std::string, vertex_def> vertices_;
        std::optional<tile> prototype_;
        tess::parser::exception get_exception(const std::string& msg);

        void set_value(const tile& prototype);
        std::optional<parser::exception> initialize();

    public:
        tile_def(const std::string& name , std::vector<std::string> params, const text_range& source_code);
		std::string name() const;
		std::vector<std::string> params() const;
		expr_value eval( execution_ctxt& ) const;
        const vertex_def& vertex(const std::string& v) const;
        const edge_def& edge(const std::string& e) const;
    };
}