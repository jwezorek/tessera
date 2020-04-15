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

    using tile_verts_and_edges = std::tuple< std::unordered_map<std::string, vertex_def>, std::unordered_map<std::string, edge_def> >;

    class tile_def : public tessera_impl
    {
        friend tessera_script;

    private: 
        std::string name_;
        std::vector<std::string> params_;
        std::unordered_map<std::string, edge_def> name_to_edge_;
        std::unordered_map<std::string, vertex_def> name_to_vertex_;
        std::vector<std::shared_ptr<const edge_def>> edges_;
        std::vector<std::shared_ptr<const vertex_def>> vertices_;
        //std::optional<tile> prototype_;
        tess::parser::exception get_exception(const std::string& msg);

        //void set_value(const tile& prototype);
        void set_indices();
        std::optional<parser::exception> initialize();

        std::vector<std::tuple<number, number>> evaluate_vertices(execution_ctxt&) const;

    public:
        tile_def(const std::vector<std::string>& params, const tile_verts_and_edges& v_e);
        tile_def(const std::string& name , std::vector<std::string> params, const text_range& source_code); //deprecated...
		std::string name() const;
		std::vector<std::string> params() const;
		expr_value eval( execution_ctxt& ) const;
        const vertex_def& vertex(const std::string& v) const;
        const edge_def& edge(const std::string& e) const;
		int num_vertices() const;
		int get_edge_index(const std::string& e) const;
		int get_vertex_index(const std::string& v) const;
    };
}