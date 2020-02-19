#pragma once

#include "expression.h"
#include <memory>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <string>
#include <variant>

namespace tess {
    struct exec_ctxt {
    };

    class statement;
    using stmt_ptr = std::shared_ptr<statement>;
    using stmts = std::vector<std::shared_ptr<statement>>;

    class statement
    {
    public:
        virtual void execute(exec_ctxt&) const = 0;
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
        void execute(exec_ctxt&) const;
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
        void execute(exec_ctxt&) const;
    };

}