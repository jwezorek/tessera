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

	class map_lay_expr : public tess::expression {
	public:
		map_lay_expr(expr_ptr mapping_cluster);
		void compile(tess::stack_machine::stack& stack) const override;
		std::string to_string() const override;
		tess::expr_ptr simplify() const override;
		void get_dependencies(std::unordered_set<std::string>& dependencies) const override;

	private:
		expr_ptr mapping_cluster_;

	};

	class partition_expr : public tess::expression {
	public:
		partition_expr(expr_ptr cluster_expr);
		void compile(tess::stack_machine::stack& stack) const override;
		std::string to_string() const override;
		tess::expr_ptr simplify() const override;
		void get_dependencies(std::unordered_set<std::string>& dependencies) const override;

	private:
		expr_ptr cluster_expr_;

	};

}