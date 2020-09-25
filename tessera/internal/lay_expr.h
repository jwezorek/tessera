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

	class lay_expr : public tess::expression {
		public:
			lay_expr(const std::vector<std::tuple<tess::expr_ptr, tess::expr_ptr>>& edge_mappings);
			void compile(tess::stack_machine::stack& stack) const override;
			std::string to_string() const override;
			tess::expr_ptr simplify() const override;
			void get_dependencies(std::unordered_set<std::string>& dependencies) const override;

		private:
			std::vector<std::tuple<tess::expr_ptr, tess::expr_ptr>> edge_mappings_;

	};

}