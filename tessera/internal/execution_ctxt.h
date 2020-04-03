#pragma once

#include "expr_value.h"
#include "tessera/tessera_script.h"
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

namespace tess {

	class lexical_scope {
		private:
			std::unordered_map<int, expr_value> placeholders_;
			std::unordered_map<std::string, expr_value> variables_;
		public:
			lexical_scope(const std::vector<std::string>& param, const std::vector<expr_value>& arg);
			lexical_scope(const std::vector<expr_value>& arg);
			std::optional<expr_value> get(int ph) const;
			std::optional<expr_value> get(std::string str) const;
	};

    class execution_ctxt {
		private:
			const tessera_script& script_;
			std::vector<lexical_scope> scope_stack_;

		public:
			execution_ctxt(const tessera_script& script);
			bool is_functional(const std::string& func) const;
			expr_value call(const std::string& func, const std::vector<expr_value>& args) const;
			expr_value eval(const std::string& var) const;
			expr_value get_placeholder(int i) const;
			void push_scope(lexical_scope&& scope);
			void pop_scope();
			execution_ctxt get_global_scope() const;
			const script_impl& script() const;
    };

}