#pragma once

#include "tessera_impl.h"
#include "expression.h"
#include "math_util.h"
#include "tessera/tile.h"
#include "tessera/error.h"
#include "tile_impl.h"
#include <optional>

namespace tess {

	class evaluation_context;
	class tile;

	struct lay_params {
		std::vector<expr_ptr> tiles;
		std::vector<std::tuple<expr_ptr, expr_ptr>> edge_mappings;
	};

	std::optional<error> apply_mapping(const std::vector<std::tuple<edge::impl_type*, edge::impl_type*>>& mapping_data);
	std::vector<tile> flatten_tiles_and_patches(const std::vector<expr_value>& tiles_and_patches);

    class lay_expr : public expression, public tessera_impl {
		private:
			using expr_vals = std::vector<expr_value>;
			using piece_result = std::variant<expr_vals, error>;
			using edge_mapping_value = std::tuple<edge, edge>;
			using edge_mapping_values = std::vector< edge_mapping_value>;
			using edge_mapping_result = std::variant<edge_mapping_values, error>;

			std::vector<expr_ptr> tiles_;
			std::vector<std::tuple<expr_ptr, expr_ptr>> edge_mappings_;

			piece_result eval_pieces(evaluation_context&) const;
			edge_mapping_result eval_edge_mappings(evaluation_context&) const;
			std::optional<error> apply_mapping(const edge_mapping_value& mappings, evaluation_context& ctxt) const;

		public:
			lay_expr(const lay_params& params);
			lay_expr(const std::vector<expr_ptr>& tiles);
			lay_expr(const std::vector<expr_ptr>& tiles, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings);
			expr_value eval(evaluation_context&) const override;
			void compile(stack_machine::stack& stack) const override;
			void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
			expr_ptr simplify() const override;
    };

}