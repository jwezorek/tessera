#pragma once

#include "tessera_impl.h"
#include "expression.h"
#include "eval_context.h"

namespace tess {

	struct lay_params {
		std::vector<expr_ptr> tiles;
		std::vector<std::tuple<expr_ptr, expr_ptr>> edge_mappings;
	};

    class lay_expr : public expression, public tessera_impl {
		private:
			using expr_vals = std::vector<expr_value>;
			using piece_result = std::variant<expr_vals, error>;
			using edge_mapping_value = std::tuple<edge, edge>;
			using edge_mapping_values = std::vector< edge_mapping_value>;
			using edge_mapping_result = std::variant<edge_mapping_values, error>;

			std::vector<expr_ptr> tiles_;
			std::vector<std::tuple<expr_ptr, expr_ptr>> edge_mappings_;

			piece_result eval_pieces(eval_context&) const;
			edge_mapping_result eval_edge_mappings(eval_context&) const;
			std::optional<error> apply_mapping(const edge_mapping_value& mappings, eval_context& ctxt) const;
			matrix edge_to_edge_matrix(const edge::impl_type& e1, const edge::impl_type& e2) const;

		public:
			lay_expr(const lay_params& params);
			lay_expr(const std::vector<expr_ptr>& tiles);
			lay_expr(const std::vector<expr_ptr>& tiles, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings);
			expr_value eval(eval_context&) const override;
			void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
			expr_ptr simplify() const override;
    };

}