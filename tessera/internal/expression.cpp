#include "math_util.h"
#include "expression.h"
#include "expr_value.h"
#include "parser/keywords.h"
#include "parser/exception.h"

std::optional<tess::number> eval_number_expr(const tess::expr_ptr& expr, tess::eval_context& ctxt)
{
	auto val = expr->eval(ctxt);
	if (!std::holds_alternative<tess::number>(val))
		return std::nullopt;
	return std::get<tess::number>(val);
}

std::optional<int> eval_integer_expr(const tess::expr_ptr& expr, tess::eval_context& ctxt)
{
	auto int_val = expr->eval(ctxt);
	if (!std::holds_alternative<tess::number>(int_val))
		return std::nullopt;
	return tess::to_int( std::get<tess::number>(int_val) );
}

std::optional<bool> eval_bool_expr(const tess::expr_ptr& expr, tess::eval_context& ctxt)
{
	auto val = expr->eval(ctxt);
	if (!std::holds_alternative<bool>(val))
		return std::nullopt;
	return std::get<bool>(val);
}

/*----------------------------------------------------------------------*/

tess::number_expr::number_expr(double v) 
{
    throw std::exception("TODO");
}

tess::number_expr::number_expr(int v) : val_(v)
{
}

tess::expr_value tess::number_expr::eval( tess::eval_context&) const {
	return tess::expr_value{ tess::number(val_) };
}

/*----------------------------------------------------------------------*/

tess::addition_expr::addition_expr(const expression_params& terms) {
    terms_.emplace_back(std::make_tuple(true, std::get<0>(terms)));
    for (const auto& [op, expr] : std::get<1>(terms))
        terms_.emplace_back(std::make_tuple(op == '+', expr));
}

tess::expr_value tess::addition_expr::eval( tess::eval_context& ctxt) const {

	tess::number sum(0);

	for (auto [sign, term_expr] : terms_) {
		auto term = eval_number_expr(term_expr, ctxt);
		if (!term.has_value())
			return tess::expr_value{ tess::error("non-number in numeric expression (+)") };
		sum += (sign) ? term.value() : -term.value();
	}
	return tess::expr_value{ sum };
}

/*----------------------------------------------------------------------*/

tess::multiplication_expr::multiplication_expr(const expression_params& terms) {
    factors_.emplace_back(std::make_tuple(true, std::get<0>(terms)));
    for (const auto& [op, expr] : std::get<1>(terms))
        factors_.emplace_back(std::make_tuple(op == '*', expr));
}

tess::expr_value tess::multiplication_expr::eval( tess::eval_context& ctxt) const {

	tess::number product(1);

	for (auto [op, factor_expr] : factors_) {
		auto factor = eval_number_expr(factor_expr, ctxt);
		if (!factor.has_value())
			return tess::expr_value{ tess::error("non-number in numeric expression (*)") };
		product *= (op) ? factor.value() : number(1) / factor.value();
	}
	return tess::expr_value{ product };
}

/*----------------------------------------------------------------------*/

tess::exponent_expr::exponent_expr(const expression_params& params) 
{
    base_ = std::get<0>(params);
    for (const auto& [dummy, expr] : std::get<1>(params))
        exponents_.emplace_back(expr);
}

tess::expr_value tess::exponent_expr::eval( tess::eval_context& ctxt) const
{
    auto base_val = eval_number_expr(base_, ctxt);
	if (!base_val.has_value())
		return tess::expr_value{ tess::error("non-number in numeric expression (^)") };
	if (exponents_.empty())
		return tess::expr_value{ base_val.value() };

	auto power = base_val.value();
	for (const auto& exponent_expr : exponents_) {
		auto exp = eval_number_expr(exponent_expr, ctxt);
		if ( exp.has_value() )
			return tess::expr_value{ tess::error("non-number in numeric expression (^)") };
		power = tess::pow(power, exp.value() );
	}
    return tess::expr_value{ power };

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

tess::expr_value tess::special_number_expr::eval( tess::eval_context& ctxt) const
{
	switch (num_) {
		case special_num::pi:
			return tess::expr_value{ tess::pi() };
		case special_num::phi:
			return tess::expr_value{ tess::number("(1 + sqrt(5)) / 2") };
		case special_num::root_2:
			return tess::expr_value{ tess::number("sqrt(2)") };
	}
	return tess::expr_value{ tess::error("Unknown special number.") };
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

tess::expr_value tess::special_function_expr::eval( tess::eval_context& ctxt) const
{
	auto possible_arg = eval_number_expr(arg_, ctxt);
	if (possible_arg.has_value())
		return tess::expr_value{ tess::error("non-number in special function") };

	auto arg = possible_arg.value();
	tess::number e;
	switch (func_)
	{
		case special_func::arccos:
			e = acos(arg);
			break;
		case special_func::arcsin:
			e = asin(arg);
			break;
		case special_func::arctan:
			e = atan(arg);
			break;
		case special_func::cos:
			e = cos(arg);
			break;
		case special_func::sin:
			e = sin(arg);
			break;
		case special_func::sqrt:
			e = sqrt(arg);
			break;
		case special_func::tan:
			e = tan(arg);
			break;
		default:
			return tess::expr_value{ tess::error("Unknown special function") };
	}
	return tess::expr_value{ e };
}

/*----------------------------------------------------------------------*/

tess::and_expr::and_expr(const std::vector<expr_ptr> conjuncts) :
	conjuncts_(conjuncts)
{
}

tess::expr_value tess::and_expr::eval( tess::eval_context& ctx) const
{
	for (const auto& conjunct : conjuncts_) {
		auto val = eval_bool_expr(conjunct, ctx);
		if (! val.has_value())
			return tess::expr_value{ tess::error("non-boolean typed expression in logical-and expression") };
		if (!val.value())
			return tess::expr_value{ false };
	}
	return tess::expr_value{ true };
}

/*----------------------------------------------------------------------*/

tess::equality_expr::equality_expr(const std::vector<expr_ptr> operands) :
	operands_(operands)
{
}

tess::expr_value tess::equality_expr::eval( tess::eval_context& ctx) const
{
	std::vector<number> expressions;
	expressions.reserve(operands_.size());
	for (const auto& op : operands_) {
		auto val = eval_number_expr(op, ctx);
		if (!val.has_value())
			return tess::expr_value{ tess::error("non-number typed expression in an equality expr") };
		expressions.push_back(val.value());
	}
	auto first = expressions.front();
	for (int i = 1; i < expressions.size(); ++i)
		if (! equals( first, expressions[i]) )
			return tess::expr_value{ false };

	return tess::expr_value{ true };
}

/*----------------------------------------------------------------------*/

tess::or_expr::or_expr(const std::vector<expr_ptr> disjuncts) :
	disjuncts_(disjuncts)
{
}

tess::expr_value tess::or_expr::eval( tess::eval_context& ctx) const
{
	for (const auto& disjunct : disjuncts_) {
		auto val = eval_bool_expr(disjunct, ctx);
		if (!val.has_value())
			return tess::expr_value{ tess::error("non-boolean typed expression in logical-or expression") };
		if (val.value())
			return tess::expr_value{ true };
	}
	return tess::expr_value{ false };
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
        op_ = relation_op::gt;
    else if (op == parser::le)
        op_ = relation_op::le;
    else
        throw parser::exception("expr", "attempted to parse invalid relation_expr");
}

tess::expr_value tess::relation_expr::eval( tess::eval_context& ctx) const
{
	auto maybe_lhs = eval_number_expr(lhs_, ctx);
	if (! maybe_lhs.has_value())
		return tess::expr_value{ tess::error("non-number typed expression in relational expression") };
	auto maybe_rhs = eval_number_expr(rhs_, ctx);
	if (!maybe_rhs.has_value())
		return tess::expr_value{ tess::error("non-number typed expression in relational expression") };
	auto lhs = maybe_lhs.value();
	auto rhs = maybe_rhs.value();

	bool result;
	switch (op_) {
		case relation_op::ge:
			result = is_ge(lhs, rhs);
			break;
		case relation_op::gt:
			result = is_gt(lhs, rhs);
			break;
		case relation_op::le:
			result = is_le(lhs, rhs);
			break;
		case relation_op::lt:
			result = is_lt(lhs, rhs);
			break;
		case relation_op::ne:
			result = is_ne(lhs, rhs);
			break;
	}
	return tess::expr_value{result };
}

tess::nil_expr::nil_expr()
{
}

tess::expr_value tess::nil_expr::eval( tess::eval_context& ctx) const
{
    return tess::expr_value{ nil_val() };
}

tess::if_expr::if_expr(std::tuple<expr_ptr, expr_ptr, expr_ptr> exprs) :
	condition_(std::get<0>(exprs)),
	then_clause_(std::get<1>(exprs)),
	else_clause_(std::get<2>(exprs))
{
}

tess::expr_value tess::if_expr::eval(eval_context& ctxt) const
{
	auto condition_val = condition_->eval(ctxt);
	if (std::holds_alternative<error>(condition_val))
		return condition_val;

	if (!std::holds_alternative<bool>(condition_val))
		return tess::expr_value{ error("if condition must evaluate to a boolean") };

	if (std::get<bool>(condition_val))
		return then_clause_->eval(ctxt);
	else
		return else_clause_->eval(ctxt);
}
