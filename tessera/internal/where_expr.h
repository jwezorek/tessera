#pragma once

#include <vector>
#include <tuple>
#include <string>
#include <memory>
#include "expression.h"
#include "execution_state.h"

namespace tess {

	using var_assignment = std::tuple<std::vector<std::string>, tess::expr_ptr>;

	class assignment_block 
	{
		public:
			assignment_block() {}
			assignment_block(const std::vector<var_assignment>& assignments);
			void compile(stack_machine::stack& stack) const;
			std::string to_string() const;
			bool operator!=(const assignment_block& block) { return impl_.get() != block.impl_.get(); }
			assignment_block simplify() const;
			std::vector<std::string> get_variables() const;
			std::vector<expr_ptr> get_values() const;
			bool empty() const;
			int num_vars() const;
		private:
			std::shared_ptr<std::vector<var_assignment>> impl_;
	};

	class where_expr : public expression
	{
	private:
		assignment_block assignments_;
		expr_ptr body_;
	public:
		where_expr(const assignment_block& assignments, expr_ptr body);
		void compile(stack_machine::stack& stack) const override;
		std::string to_string() const override;
		expr_ptr simplify() const override;
		void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
	};

}