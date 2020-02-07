#include "expression.h"
#include "parser/keywords.h"
#include "parser/exception.h"
#include <cmath>

/*----------------------------------------------------------------------*/

tess::number_expr::number_expr(double v) : val_(v)
{}

double tess::number_expr::eval(const eval_ctxt&) const {
    return val_;
}

/*----------------------------------------------------------------------*/

tess::object_ref_expr::object_ref_expr(const std::vector<object_ref_item>& parts) : parts_(parts)
{
    int aaa;
    aaa = 5;
}

double tess::object_ref_expr::eval(const eval_ctxt& ctxt) const {
    return 0;
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

/*----------------------------------------------------------------------*/

tess::special_number_expr::special_number_expr(const std::string& v) 
{
    if (v == parser::keyword(parser::kw::pi))
        num_ = special_num::pi;
    else if (v == parser::keyword(parser::kw::phi))
        num_ = special_num::phi;
    else if (v == parser::keyword(parser::kw::root_2))
        num_ = special_num::root_2;
    else
        throw parser::exception("expr", "attempted to parse invalid special number");
}

double tess::special_number_expr::eval(const eval_ctxt& ctxt) const
{
    return 0.0;
}

tess::special_function_expr::special_function_expr(std::tuple<std::string, expr_ptr> param)
{
    auto [func_keyword, arg] = param;
    if (func_keyword == parser::keyword(parser::kw::sqrt))
        func_ = special_func::sqrt;
    else if (func_keyword == parser::keyword(parser::kw::sin))
        func_ = special_func::sin;
    else if (func_keyword == parser::keyword(parser::kw::cos))
        func_ = special_func::cos;
    else if (func_keyword == parser::keyword(parser::kw::tan))
        func_ = special_func::tan;
    else if (func_keyword == parser::keyword(parser::kw::arcsin))
        func_ = special_func::arcsin;
    else if (func_keyword == parser::keyword(parser::kw::arccos))
        func_ = special_func::arccos;
    else if (func_keyword == parser::keyword(parser::kw::arctan))
        func_ = special_func::arctan;
    else
        throw parser::exception("expr", "attempted to parse invalid special function");

    arg_ = arg;
}

double tess::special_function_expr::eval(const eval_ctxt& ctxt) const
{
    return 0.0;
}

/*----------------------------------------------------------------------*/

tess::function_call_expr::function_call_expr(std::tuple<std::string, std::vector<expr_ptr>> params) :
    func_(std::get<0>(params)),
    args_(std::get<1>(params))
{
}

double tess::function_call_expr::eval(const eval_ctxt& ctx) const
{
    return 0.0;
}

/*----------------------------------------------------------------------*/

tess::and_expr::and_expr(const std::vector<expr_ptr> conjuncts) :
	conjuncts_(conjuncts)
{
}

double tess::and_expr::eval(const eval_ctxt& ctx) const
{
	return 0.0;
}

/*----------------------------------------------------------------------*/

tess::equality_expr::equality_expr(const std::vector<expr_ptr> operands) :
	operands_(operands)
{
}

double tess::equality_expr::eval(const eval_ctxt& ctx) const
{
	return 0.0;
}

/*----------------------------------------------------------------------*/

tess::or_expr::or_expr(const std::vector<expr_ptr> disjuncts) :
	disjuncts_(disjuncts)
{
}

double tess::or_expr::eval(const eval_ctxt& ctx) const
{
	return 0.0;
}

/*----------------------------------------------------------------------*/

tess::relation_expr::relation_expr(std::tuple<expr_ptr, std::string, expr_ptr> param) :
    lhs_(std::get<0>(param)),
    op_(relation_op::ne),
    rhs_(std::get<2>(param))
{
    auto op = std::get<1>(param);
    if (op == parser::not_equ)
        op_ = relation_op::ne;
    else if (op == parser::ge)
        op_ = relation_op::ge;
    else if (op == parser::lt)
        op_ = relation_op::lt;
    else if (op == parser::gt)
        op_ == relation_op::gt;
    else if (op == parser::le)
        op_ = relation_op::le;
    else
        throw parser::exception("expr", "attempted to parse invalid relation_expr");
}

double tess::relation_expr::eval(const eval_ctxt& ctx) const
{
	return 0.0;
}
