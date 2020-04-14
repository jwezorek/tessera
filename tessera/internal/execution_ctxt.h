#pragma once

#include "expr_value.h"
#include "tessera/old_tessera_script.h"
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
	};

    class execution_ctxt {
		private:
			const tessera_script& script_;
			std::vector<scope_frame> scope_stack_;

		public:
			execution_ctxt(const tessera_script& script);
			bool is_functional(const std::string& func) const;
			expr_value call(const std::string& func, const std::vector<expr_value>& args) const;
			expr_value eval(const std::string& var) const;
			expr_value get_placeholder(int i) const;
			void push_scope(scope_frame&& scope);
			void pop_scope();
			execution_ctxt get_global_scope() const;
			const script_impl& script() const;
    };

	class scope {
	private:
		execution_ctxt& ctxt_;
	public:
		scope(execution_ctxt& ctxt, scope_frame&& ls);
		~scope();
	};

}