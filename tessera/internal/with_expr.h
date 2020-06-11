#pragma once

#include "expression.h"

namespace tess {

	class with_expr : public expression
	{
	private:
		std::vector<std::tuple<expr_ptr, expr_ptr>> field_defs_;
		expr_ptr body_;

	public:
		with_expr(const std::vector<std::tuple<expr_ptr, expr_ptr>>& field_defs, expr_ptr body);
		void compile(stack_machine::stack& stack) const override;
		std::string to_string() const override;
		expr_ptr simplify() const override;
		void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
	};

}