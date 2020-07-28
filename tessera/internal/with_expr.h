#pragma once

#include "expression.h"

namespace tess {

	using field_def = std::tuple< std::vector<tess::expr_ptr>, std::string, tess::expr_ptr >;

	class field_definitions
	{
	public:
		field_definitions() {}
		field_definitions(const std::vector<field_def>& assignments);
		void compile(stack_machine::stack& stack) const;
		std::string to_string() const;
		bool operator!=(const field_definitions& block) { return impl_.get() != block.impl_.get(); }
		field_definitions simplify() const;
		void get_dependencies(std::unordered_set<std::string>& dependencies);
		bool empty() const;
	private:
		std::shared_ptr<std::vector<field_def>> impl_;
	};

	class with_expr : public expression
	{
	private:
		field_definitions field_defs_;
		expr_ptr body_;

	public:
		with_expr(const field_definitions& field_defs, expr_ptr body);
		void compile(stack_machine::stack& stack) const override;
		std::string to_string() const override;
		expr_ptr simplify() const override;
		void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
	};

}