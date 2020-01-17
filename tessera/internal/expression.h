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
        std::shared_ptr<tess::expression> base_;
        std::vector<std::shared_ptr<tess::expression>> exponents_;
    public:
        exponent_expr(const expression_params& params);
        double eval(const eval_ctxt& ctxt) const override;
    };

    class addition_expr : public expression
    {
    private:
        std::vector<std::tuple<bool,std::shared_ptr<expression>>> terms_;
    public:
        addition_expr(const expression_params& terms);
        double eval(const eval_ctxt& ctx) const override;
    };

    class multiplication_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, std::shared_ptr<expression>>> factors_;
    public:
        multiplication_expr(const expression_params& terms);
        double eval(const eval_ctxt& ctx) const override;
    };

}