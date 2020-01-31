#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <string>

namespace tess {
    struct eval_ctxt {
        std::unordered_map<std::string, double> variables;
    };

    class expression;
    using expr_ptr = std::shared_ptr<expression>;

    class expression
    {
    public:
        virtual double eval(const eval_ctxt&) const = 0;
    };

    class number_expr : public expression
    {
    private:
        double val_;
    public:
        number_expr(double v);
        double eval(const eval_ctxt&) const override;
    };

    enum class special_num {
        pi,
        phi,
        root_2
    };

    class special_number_expr : public expression
    {
    private:
        special_num num_;
    public:
        special_number_expr(const std::string& v);
        double eval(const eval_ctxt& ctxt) const override;
    };

    enum class special_func {
        sqrt,
        sin,
        cos,
        tan,
        arcsin,
        arccos,
        arctan
    };

    class special_function_expr : public expression
    {
    private:
        special_func func_;
        expr_ptr arg_;
    public:
        special_function_expr(std::tuple<std::string, expr_ptr> param);
        double eval(const eval_ctxt& ctxt) const override;
    };

    class variable_expr : public expression
    {
    private:
        std::string var_;
    public:
        variable_expr(const std::string& v);
        double eval(const eval_ctxt& ctxt) const override;
    };

    using expression_params = std::tuple<std::shared_ptr<tess::expression>, std::vector<std::tuple<char, std::shared_ptr<tess::expression>>>>;

    class exponent_expr : public expression
    {
    private:
        expr_ptr base_;
        std::vector<expr_ptr> exponents_;
    public:
        exponent_expr(const expression_params& params);
        double eval(const eval_ctxt& ctxt) const override;
    };

    class addition_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, expr_ptr>> terms_;
    public:
        addition_expr(const expression_params& terms);
        double eval(const eval_ctxt& ctx) const override;
    };

    class multiplication_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, expr_ptr>> factors_;
    public:
        multiplication_expr(const expression_params& terms);
        double eval(const eval_ctxt& ctx) const override;
    };
}