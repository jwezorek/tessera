#pragma once

#include "expression.h"

namespace tess {
    class cluster_expr : public expression
    {
    private:
        std::vector<expr_ptr> exprs_;
    public:
        cluster_expr(const std::vector<expr_ptr>& exprs);
        expr_value eval(evaluation_context&) const override;
        void compile(stack_machine::stack& stack) const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
    };

    class num_range_expr : public expression
    {
    private:
        expr_ptr from_, to_;
    public:
        num_range_expr(expr_ptr from, expr_ptr to);
        num_range_expr(const std::tuple<expr_ptr, expr_ptr>& tup);
        expr_value eval(evaluation_context&) const override;
        void compile(stack_machine::stack& stack) const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
    };

    class cluster_comprehension_expr : public expression
    {
    private:
        expr_ptr item_expr_;
        std::string var_;
        expr_ptr range_expr_;
    public:
        cluster_comprehension_expr(expr_ptr ex, const std::string& var, expr_ptr range_expr);
        cluster_comprehension_expr( std::tuple<expr_ptr, std::string, expr_ptr> tup);
        expr_value eval(evaluation_context&) const override;
        void compile(stack_machine::stack& stack) const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
    };
};