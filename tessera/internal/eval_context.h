#pragma once

#include "expr_value.h"
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

namespace tess {

	class scope_frame {
		private:
			std::unordered_map<int, expr_value> placeholders_;
			std::unordered_map<std::string, expr_value> variables_;
		public:
			scope_frame(const std::vector<std::string>& param, const std::vector<expr_value>& arg);
			scope_frame(const std::vector<expr_value>& arg);
			std::optional<expr_value> get(int ph) const;
			std::optional<expr_value> get(std::string str) const;
			void set(const std::string& var, expr_value val);
			void set(int i, expr_value val);
			void set(const::std::vector<std::string>& vars, const::std::vector<expr_value>& vals);
	};

    class eval_context {
		private:
			std::vector<scope_frame> scope_stack_;

		public:
			eval_context();
			eval_context(const scope_frame& frame);
			expr_value get(const std::string& var) const;
			expr_value get(int i) const;
			scope_frame& peek();
			void push_scope();
			void push_scope(scope_frame&& scope);
			scope_frame pop_scope();
    };

	class scope {
		private:
			eval_context& ctxt_;
		public:
			scope(eval_context& ctxt, scope_frame&& ls);
			~scope();
	};

}

