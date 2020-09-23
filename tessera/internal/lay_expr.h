#pragma once

#include "tessera_impl.h"
#include "expression.h"
#include "with_expr.h"
#include "number.h"
#include "tessera/tile.h"
#include "tessera/error.h"
#include "tile_impl.h"
#include <optional>

namespace tess {

	class evaluation_context;
	class tile;

	struct lay_params {
		std::string kw;
		std::vector<expr_ptr> tiles;
		std::vector<std::tuple<expr_ptr, expr_ptr>> edge_mappings;
	};

	struct full_lay_params {
		std::string kw;
		std::vector<expr_ptr> tiles;
		std::vector<std::tuple<expr_ptr, expr_ptr>> edge_mappings;
		field_definitions with;
	};

    class lay_expr : public expression, public tessera_impl {
		private:
			std::vector<expr_ptr> tiles_;
			std::vector<std::tuple<expr_ptr, expr_ptr>> edge_mappings_;
			field_definitions with_;
		public:
			lay_expr(const lay_params& params);
			lay_expr(const full_lay_params& params);
			lay_expr(const std::vector<expr_ptr>& tiles, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings);
			lay_expr(const std::vector<expr_ptr>& tiles, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings, const field_definitions& with);
			void compile(stack_machine::stack& stack) const override;
			void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
			expr_ptr simplify() const override;
			std::string to_string() const override;
    };

}