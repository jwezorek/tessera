#pragma once

#include "expression.h"

namespace tess {
    class cluster_expr : public expression
    {
    private:
        std::vector<expr_ptr> exprs_;
    public:
        cluster_expr(const std::vector<expr_ptr>& exprs);
        expr_value eval(eval_context&) const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::vector<std::string>& dependencies) const override;
    };

    class num_range_expr : public expression
    {
    private:
        expr_ptr from_, to_;
    public:
        num_range_expr(expr_ptr from, expr_ptr to);
        expr_value eval(eval_context&) const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::vector<std::string>& dependencies) const override;
    };

    class cluster_comprehension_expr : public expression
    {
    private:
        expr_ptr item_expr_;
        std::string var_;
        expr_ptr range_expr_;
    public:
        cluster_comprehension_expr(expr_ptr ex, const std::string& var, expr_ptr range_expr);
        expr_value eval(eval_context&) const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::vector<std::string>& dependencies) const override;
    };
};