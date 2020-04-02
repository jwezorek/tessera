#pragma once

#include "tessera_impl.h"
#include "expression.h"
#include "execution_ctxt.h"
#include "expr_value.h"
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <string>
#include <variant>

namespace tess {

	class statement;
	using stmt_ptr = std::shared_ptr<statement>;
	using stmts = std::vector<std::shared_ptr<statement>>;

	class statement : public tessera_impl
	{
	public:
		virtual expr_value execute(execution_ctxt&) const = 0;
	};

	struct lay_params {
		std::vector<obj_ref_ptr> tiles;
		std::vector<std::tuple<obj_ref_ptr, obj_ref_ptr>> such_that_clauses;
	};

	class lay_statement : public statement
	{
	private:
		std::vector<obj_ref_ptr> tiles_;
		std::vector<std::tuple<obj_ref_ptr, obj_ref_ptr>> such_that_clauses_;
	public:
		lay_statement(const lay_params& params);
		lay_statement(const std::vector<obj_ref_ptr>& tiles);
		expr_value execute(execution_ctxt&) const override;
	};

	class let_statement : public statement
	{
	private:
		std::string lhs_;
		expr_ptr rhs_;
	public:
		let_statement(const std::tuple<std::string, expr_ptr> params);
		expr_value execute(execution_ctxt&) const override;
		std::string lhs() const;
		expr_ptr rhs() const;
	};

    struct if_params {
        expr_ptr condition;
        stmt_ptr then_clause;
        stmt_ptr else_clause;
    };

    class if_statement : public statement
    {
    private:
        expr_ptr condition_;
        stmt_ptr then_clause_;
        stmt_ptr else_clause_;
    public:
        if_statement(const if_params& params);
        expr_value execute(execution_ctxt&) const override;
    };

}