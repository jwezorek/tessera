#pragma once

#include "tessera_impl.h"
#include "expression.h"
#include "number.h"
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

    class lay_expr : public expression, public tessera_impl {
		private:
			std::vector<expr_ptr> tiles_;
			std::vector<std::tuple<expr_ptr, expr_ptr>> edge_mappings_;

			std::optional<error> apply_mapping(const std::tuple<edge, edge>& mapping, evaluation_context& ctxt) const;

		public:
			lay_expr(const lay_params& params);
			lay_expr(const std::vector<expr_ptr>& tiles);
			lay_expr(const std::vector<expr_ptr>& tiles, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings);
			void compile(stack_machine::stack& stack) const override;
			void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
			expr_ptr simplify() const override;
    };

}