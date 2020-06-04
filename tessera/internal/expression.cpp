#include "math_util.h"
#include "expression.h"
#include "expr_value.h"
#include "execution_state.h"
#include "tile_def.h"
#include "tile_impl.h"
#include "tessera_impl.h"
#include "parser/keywords.h"
#include "parser/expr_parser.h"
#include "parser/exception.h"
#include "execution_state.h"
#include "ops.h"
#include "allocator.h"
#include <sstream>

namespace {
	tess::expr_ptr get_interior_angle_expr(int n) {
		std::stringstream ss;
		ss << "( pi / " << n << ") * " << n - 2;
		return tess::parser::parse_expression(ss.str())->simplify();
	}

	tess::number central_angle(int n) {
		std::string expr_str = "(2 * pi) / " + std::to_string(n);
		return tess::number( expr_str );
	}

	std::vector<std::tuple<tess::number, tess::number>> regular_polygon_vertices(tess::number num_sides)
	{
		int n = tess::to_int(num_sides);
		std::vector<std::tuple<tess::number, tess::number>> points(n);

		auto angle = central_angle( n );
		for (int i = 0; i < n; i++) {
			auto theta = tess::number(i) * angle;
			points[i] = { tess::cos(theta), tess::sin(theta) };
		}

		return points;
	}

	tess::expr_value generate_regular_polygon_tile(tess::execution_state& state, tess::number num_sides) {
		int n = tess::to_int(num_sides);
		std::vector<tess::vertex_def> vertices(n);
		int i = 0;
		tess::expr_ptr theta = get_interior_angle_expr(n);
		std::generate(vertices.begin(), vertices.end(),
			[&i, &theta]() {
				return tess::vertex_def(
					std::string(),
					theta,
					std::string(),
					i++
				);
			}
		);

		std::vector<tess::edge_def> edges(n);
		for (i = 0; i < n; i++) {
			edges[i] = tess::edge_def(
				"",
				i, (i < n - 1) ? i + 1 : 0,
				"",
				std::make_shared<tess::number_expr>(1),
				i
			);
		}

		auto tile_def_expr = std::make_shared<tess::tile_def_expr>(vertices, edges);
		auto ctxt = state.create_eval_context();
		return tile_def_expr->eval(ctxt);
	}
}
std::optional<tess::number> eval_number_expr(const tess::expr_ptr& expr, tess::evaluation_context& ctxt)
{
	auto val = expr->eval(ctxt);
	if (!std::holds_alternative<tess::number>(val))
		return std::nullopt;
	return std::get<tess::number>(val);
}

std::optional<bool> eval_bool_expr(const tess::expr_ptr& expr, tess::evaluation_context& ctxt)
{
	auto val = expr->eval(ctxt);
	if (!std::holds_alternative<bool>(val))
		return std::nullopt;
	return std::get<bool>(val);
}

/*----------------------------------------------------------------------*/

void tess::expression::compile(stack_machine::stack& stack) const
{
	stack.push({ expr_value{ std::string("TODO")} });
}


std::string tess::expression::to_string() const
{
	return "<TODO>";
}

tess::number_expr::number_expr(double v) 
{
    throw std::exception("TODO");
}

tess::number_expr::number_expr(int v) : val_(v)
{
}

tess::expr_value tess::number_expr::eval( tess::evaluation_context&) const {
	return tess::expr_value{ tess::number(val_) };
}

void tess::number_expr::compile(stack_machine::stack& stack) const
{
	stack.push(tess::expr_value{ tess::number(val_) });
}

std::string tess::number_expr::to_string() const
{
	std::stringstream ss;
	ss << val_;
	return ss.str();
}

tess::expr_ptr tess::number_expr::simplify() const
{
	return std::make_shared<number_expr>(val_);
}

void tess::number_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
}

/*----------------------------------------------------------------------*/

tess::addition_expr::addition_expr(const expression_params& terms) {
    terms_.emplace_back(std::make_tuple(true, std::get<0>(terms)));
    for (const auto& [op, expr] : std::get<1>(terms))
        terms_.emplace_back(std::make_tuple(op == '+', expr));
}

tess::addition_expr::addition_expr(const std::vector<std::tuple<bool, expr_ptr>>& terms) :
	terms_(terms)
{
}

tess::expr_value tess::addition_expr::eval( tess::evaluation_context& ctxt) const {

	tess::number sum(0);

	for (auto [sign, term_expr] : terms_) {
		auto term = eval_number_expr(term_expr, ctxt);
		if (!term.has_value())
			return tess::expr_value{ tess::error("non-number in numeric expression (+)") };
		sum += (sign) ? term.value() : -term.value();
	}
	return tess::expr_value{ sum };
}

void tess::addition_expr::compile(stack_machine::stack& stack) const
{
	stack.push(std::make_shared<add_op>( static_cast<int>(terms_.size()) ));
	for (auto [sign, term_expr] : terms_) {
		term_expr->compile(stack);
		if (!sign)
			stack.push(std::make_shared<neg_op>());
	}
}

std::string tess::addition_expr::to_string() const
{
	std::stringstream ss;
	ss << "( + ";
	for (const auto& [sign, term_expr] : terms_) {
		if (!sign) {
			ss << "( - " << term_expr->to_string() << ")";
		} else {
			ss << term_expr->to_string();
		}
		ss << " ";
	}
	ss << ")";

	return ss.str();
}

void tess::addition_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const {
	for (auto [dummy, term_expr] : terms_)
		term_expr->get_dependencies(dependencies);
}

tess::expr_ptr tess::addition_expr::simplify() const
{
	// if there is only one term and it is positive we dont need
	// addition node. 
	if (terms_.size() == 1 && std::get<0>(terms_[0])) {
		return std::get<1>(terms_[0])->simplify();
	}
	std::vector<std::tuple<bool, expr_ptr>> simplified(terms_.size());
	std::transform(terms_.begin(), terms_.end(), simplified.begin(),
		[](const auto& term)->std::tuple<bool, expr_ptr> {
			const auto& [sgn, e] = term;
			return { sgn, e->simplify() };
		}
	);
	return std::make_shared<addition_expr>(simplified);
}

/*----------------------------------------------------------------------*/

tess::multiplication_expr::multiplication_expr(const expression_params& terms) {
    factors_.emplace_back(std::make_tuple(true, std::get<0>(terms)));
    for (const auto& [op, expr] : std::get<1>(terms))
        factors_.emplace_back(std::make_tuple(op == '*', expr));
}

tess::multiplication_expr::multiplication_expr(const std::vector<std::tuple<bool, expr_ptr>>& factors) :
	factors_(factors)
{
}

tess::expr_value tess::multiplication_expr::eval( tess::evaluation_context& ctxt) const {

	tess::number product(1);

	for (auto [op, factor_expr] : factors_) {
		auto factor = eval_number_expr(factor_expr, ctxt);
		if (!factor.has_value())
			return tess::expr_value{ tess::error("non-number in numeric expression (*)") };
		product *= (op) ? factor.value() : number(1) / factor.value();
	}
	return tess::expr_value{ product };
}

void tess::multiplication_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	for (auto [op, factor_expr] : factors_)
		factor_expr->get_dependencies(dependencies);
}

tess::expr_ptr tess::multiplication_expr::simplify() const
{
	if (factors_.size() == 1 && std::get<0>(factors_[0])) {
		return  std::get<1>(factors_[0])->simplify();
	}
	std::vector<std::tuple<bool, expr_ptr>> simplified(factors_.size());
	std::transform(factors_.begin(), factors_.end(), simplified.begin(),
		[](const auto& term)->std::tuple<bool, expr_ptr> {
			const auto& [op, e] = term;
			return { op, e->simplify() };
		}
	);
	return std::make_shared< multiplication_expr>(simplified);
}

/*----------------------------------------------------------------------*/

tess::exponent_expr::exponent_expr(const expression_params& params) 
{
    base_ = std::get<0>(params);
    for (const auto& [dummy, expr] : std::get<1>(params))
        exponents_.emplace_back(expr);
}

tess::exponent_expr::exponent_expr(expr_ptr base, const std::vector<expr_ptr>& exponents) :
	base_(base), exponents_(exponents)
{
}

tess::expr_value tess::exponent_expr::eval( tess::evaluation_context& ctxt) const
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


void  tess::exponent_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const {

	base_->get_dependencies(dependencies);
	for (const auto& exponent_expr : exponents_)
		exponent_expr->get_dependencies(dependencies);
}
tess::expr_ptr tess::exponent_expr::simplify() const
{
	if (exponents_.empty())
		return base_->simplify();
	std::vector<expr_ptr> simplified(exponents_.size());
	std::transform(exponents_.begin(), exponents_.end(), simplified.begin(),
		[](const auto& e) {
			return e->simplify();
		}
	);
	return std::make_shared< exponent_expr>(base_->simplify(), simplified);
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

tess::special_number_expr::special_number_expr(special_num which) :
	num_(which)
{
}

tess::expr_value tess::special_number_expr::eval( tess::evaluation_context& ctxt) const
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

tess::expr_ptr tess::special_number_expr::simplify() const
{
	return std::make_shared<special_number_expr>(num_);
}

void tess::special_number_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
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
	else if (func_keyword == parser::keyword(parser::kw::regular_polygon))
		func_ = special_func::regular_polygon;
    else
        throw parser::exception("expr", "attempted to parse invalid special function");

    arg_ = arg;
}

tess::special_function_expr::special_function_expr(special_func func, expr_ptr arg) :
	func_(func), arg_(arg)
{
}

std::function<tess::expr_value(tess::allocator&, tess::expr_value)> get_special_function(tess::special_func code) {
	switch (code)
	{
	case tess::special_func::arccos:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value { return { tess::acos(std::get<tess::number>(arg)) }; };
	case tess::special_func::arcsin:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value { return { tess::asin(std::get<tess::number>(arg)) }; };
	case tess::special_func::arctan:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value { return { tess::atan(std::get<tess::number>(arg)) }; };
	case tess::special_func::cos:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value { return { tess::cos(std::get<tess::number>(arg)) }; };
	case tess::special_func::sin:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value { return { tess::sin(std::get<tess::number>(arg)) }; };
	case tess::special_func::sqrt:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value { return { tess::sqrt(std::get<tess::number>(arg)) }; };
	case tess::special_func::tan:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value { return { tess::tan(std::get<tess::number>(arg)) }; };
	case tess::special_func::regular_polygon:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value {
			auto locs = regular_polygon_vertices(std::get<tess::number>(arg));
			return { a.create<tess::tile>(&a, locs, true) };
		};
	default:
		throw tess::error("Unknown special function");
	}
}

std::string get_special_function_name(tess::special_func code) {
	switch (code)
	{
	case tess::special_func::arccos:
		return "arcos";
	case tess::special_func::arcsin:
		return "arcsin";
	case tess::special_func::arctan:
		return "arctan";
	case tess::special_func::cos:
		return "cosine";
	case tess::special_func::sin:
		return "sine";
	case tess::special_func::sqrt:
		return "sqrt";
	case tess::special_func::tan:
		return "tan";
	case tess::special_func::regular_polygon:
		return "regular_poly";
	default:
		throw tess::error("Unknown special function");
	}
}

tess::expr_value tess::special_function_expr::eval( tess::evaluation_context& ctxt) const
{
	auto possible_arg = eval_number_expr(arg_, ctxt);
	if (!possible_arg.has_value())
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
		case special_func::regular_polygon:
			return generate_regular_polygon_tile(ctxt.execution_state(), arg);
		default:
			return tess::expr_value{ tess::error("Unknown special function") };
	}
	return tess::expr_value{ e };
}

void tess::special_function_expr::compile(stack_machine::stack& stack) const
{
	stack.push(
		std::make_shared<one_param_op>(
			get_special_function(func_),
			get_special_function_name(func_)
		)
	);
	arg_->compile(stack);
}

void tess::special_function_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	arg_->get_dependencies(dependencies);
}

tess::expr_ptr tess::special_function_expr::simplify() const
{
	return std::make_shared< special_function_expr>(func_, arg_->simplify());
}

std::string tess::special_function_expr::to_string() const
{
	std::stringstream ss;
	ss << "( " << get_special_function_name(func_) << " " << arg_->to_string() << " )";
	return ss.str();
}

/*----------------------------------------------------------------------*/

tess::and_expr::and_expr(const std::vector<expr_ptr>& conjuncts) :
	conjuncts_(conjuncts)
{
}

tess::expr_value tess::and_expr::eval( tess::evaluation_context& ctx) const
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

void tess::and_expr::compile(stack_machine::stack& stack) const
{
	stack.push(
		std::make_shared<val_func_op>(conjuncts_.size(),
			[](const std::vector<expr_value>& args)->expr_value {
				for (const auto& conjunct : args) {
					auto val = std::get<bool>(conjunct);
					if (!val)
						return { false };
				}
				return { true };
			},
			"<and>"
		)
	);
	for (const auto& conjuct : conjuncts_)
		conjuct->compile(stack);
}

void tess::and_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	for (const auto& conjunct : conjuncts_)
		conjunct->get_dependencies(dependencies);
}

tess::expr_ptr tess::and_expr::simplify() const
{
	if (conjuncts_.size() == 1)
		return conjuncts_[0]->simplify();
	std::vector<expr_ptr> simplified(conjuncts_.size());
	std::transform(conjuncts_.begin(), conjuncts_.end(), simplified.begin(),
		[](const auto& e) {return e->simplify(); }
	);
	return std::make_shared<and_expr>(simplified);
}

/*----------------------------------------------------------------------*/

tess::equality_expr::equality_expr(const std::vector<expr_ptr> operands) :
	operands_(operands)
{
}

tess::expr_value tess::equality_expr::eval( tess::evaluation_context& ctx) const
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

void tess::equality_expr::compile(stack_machine::stack& stack) const
{
	stack.push(
		std::make_shared<val_func_op>(operands_.size(),
			[](const std::vector<expr_value>& args) -> expr_value {

				std::vector<number> expressions;
				expressions.reserve(args.size());
				for (const auto& op : args) {
					expressions.push_back(std::get<number>(op));
				}

				auto first = expressions.front();
				for (int i = 1; i < expressions.size(); ++i)
					if (!equals(first, expressions[i]))
						return { false };

				return { true };
			},
			"<eq>"
		)
	);
	for (const auto& o : operands_)
		o->compile(stack);
}

void tess::equality_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	for (const auto& op : operands_)
		op->get_dependencies(dependencies);
}

tess::expr_ptr tess::equality_expr::simplify() const
{
	if (operands_.size() == 1)
		return operands_[0]->simplify();
	std::vector<expr_ptr> simplified(operands_.size());
	std::transform(operands_.begin(), operands_.end(), simplified.begin(),
		[](const auto& e) {return e->simplify(); }
	);
	return std::make_shared<equality_expr>(simplified);
}

/*----------------------------------------------------------------------*/

tess::or_expr::or_expr(const std::vector<expr_ptr> disjuncts) :
	disjuncts_(disjuncts)
{
}

tess::expr_value tess::or_expr::eval( tess::evaluation_context& ctx) const
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

void tess::or_expr::compile(stack_machine::stack& stack) const
{
	stack.push(
		std::make_shared<val_func_op>(disjuncts_.size(),
			[](const std::vector<expr_value>& args) -> expr_value {
				for (const auto& disjunct : args) {
					auto val = std::get<bool>(disjunct);
					if (val)
						return { true };
				}
				return { false };
			},
			"<or>"
		)
	);
	for (const auto& d : disjuncts_)
		d->compile(stack);
}

void tess::or_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	for (const auto& disjunct : disjuncts_)
		disjunct->get_dependencies(dependencies);
}

tess::expr_ptr tess::or_expr::simplify() const
{
	if (disjuncts_.size() == 1)
		return disjuncts_[0]->simplify();
	std::vector<expr_ptr> simplified(disjuncts_.size());
	std::transform(disjuncts_.begin(), disjuncts_.end(), simplified.begin(),
		[](const auto& e) {return e->simplify(); }
	);
	return std::make_shared<or_expr>(simplified);
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

tess::relation_expr::relation_expr(expr_ptr lhs, relation_op op, expr_ptr rhs) :
	lhs_(lhs), op_(op), rhs_(rhs)
{
}

tess::expr_value tess::relation_expr::eval( tess::evaluation_context& ctx) const
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

void tess::relation_expr::compile(stack_machine::stack& stack) const
{
	auto relation = op_;
	stack.push(
		std::make_shared<val_func_op>(
			2,
			[relation](const std::vector<expr_value>& args) -> expr_value {
				for (const auto& disjunct : args) {
					auto lhs = std::get<number>(args[0]);
					auto rhs = std::get<number>(args[1]);

					bool result;
					switch (relation) {
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
					return tess::expr_value{ result };
				}
				return { false };
			},
			"<relation>"
		)
	);
	lhs_->compile(stack);
	rhs_->compile(stack);
}

void tess::relation_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	lhs_->get_dependencies(dependencies);
	rhs_->get_dependencies(dependencies);
}

tess::expr_ptr tess::relation_expr::simplify() const
{
	return std::make_shared<relation_expr>(
		lhs_->simplify(),
		op_,
		rhs_->simplify()
	);
}

tess::nil_expr::nil_expr()
{
}

tess::expr_value tess::nil_expr::eval( tess::evaluation_context& ctx) const
{
    return tess::expr_value{ nil_val() };
}

tess::expr_ptr tess::nil_expr::simplify() const
{
	return std::make_shared<nil_expr>();
}

void tess::nil_expr::compile(stack_machine::stack& stack) const
{
}

void tess::nil_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
}

tess::if_expr::if_expr(std::tuple<expr_ptr, expr_ptr, expr_ptr> exprs) :
	condition_(std::get<0>(exprs)),
	then_clause_(std::get<1>(exprs)),
	else_clause_(std::get<2>(exprs))
{
}

tess::if_expr::if_expr(expr_ptr condition, expr_ptr then_clause, expr_ptr else_clause) :
	condition_(condition), 
	then_clause_(then_clause), 
	else_clause_(else_clause)
{
}

tess::expr_value tess::if_expr::eval(evaluation_context& ctxt) const
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

void tess::if_expr::compile(stack_machine::stack& stack) const
{
	stack_machine::stack then;
	stack_machine::stack else_;

	then_clause_->compile(then);
	else_clause_->compile(else_);

	stack.push(std::make_shared<if_op>(then.pop_all(), else_.pop_all()));
	condition_->compile(stack);
}

void tess::if_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	condition_->get_dependencies(dependencies);
	then_clause_->get_dependencies(dependencies);
	else_clause_->get_dependencies(dependencies);
}

tess::expr_ptr tess::if_expr::simplify() const
{
	return std::make_shared< if_expr>(
		condition_->simplify(),
		then_clause_->simplify(),
		else_clause_->simplify()
	);
}
