#include "expression.h"
#include <cmath>

/*----------------------------------------------------------------------*/

tess::number_expr::number_expr(double v) : val_(v)
{}

double tess::number_expr::eval(const eval_ctxt&) const {
    return val_;
}

/*----------------------------------------------------------------------*/

tess::variable_expr::variable_expr(const std::string& v) : var_(v)
{}

double tess::variable_expr::eval(const eval_ctxt& ctxt) const {
    return ctxt.variables.at(var_);
}

/*----------------------------------------------------------------------*/

tess::addition_expr::addition_expr(const expression_params& terms) {
    terms_.emplace_back(std::make_tuple(true, std::get<0>(terms)));
    for (const auto& [op, expr] : std::get<1>(terms))
        terms_.emplace_back(std::make_tuple(op == '+', expr));
}

double tess::addition_expr::eval(const tess::eval_ctxt& ctx) const {
    double sum = 0;
    for (auto [sign, term] : terms_)
        sum += (sign) ? term->eval(ctx) : -term->eval(ctx);
    return sum;
}

/*----------------------------------------------------------------------*/

tess::multiplication_expr::multiplication_expr(const expression_params& terms) {
    factors_.emplace_back(std::make_tuple(true, std::get<0>(terms)));
    for (const auto& [op, expr] : std::get<1>(terms))
        factors_.emplace_back(std::make_tuple(op == '*', expr));
}

double tess::multiplication_expr::eval(const tess::eval_ctxt& ctx) const {
    double prod = 1.0;
    for (auto [op, term] : factors_)
        prod *= (op) ? term->eval(ctx) : 1.0 / term->eval(ctx);
    return prod;
}

/*----------------------------------------------------------------------*/

tess::exponent_expr::exponent_expr(const expression_params& params) 
{
    base_ = std::get<0>(params);
    for (const auto& [dummy, expr] : std::get<1>(params))
        exponents_.emplace_back(expr);
}

double tess::exponent_expr::eval(const eval_ctxt& ctxt) const
{
    auto base_val = base_->eval(ctxt);
    if (exponents_.empty())
        return base_val;
    for (const auto& e : exponents_)
        base_val = std::pow(base_val, e->eval(ctxt));
    return base_val;
}
