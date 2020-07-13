#include "number.h"
#include "expression.h"
#include "expr_value.h"
#include "execution_state.h"
#include "tile_impl.h"
#include "tessera_impl.h"
#include "parser/keywords.h"
#include "parser/expr_parser.h"
#include "execution_state.h"
#include "ops.h"
#include "allocator.h"
#include "variant_util.h"
#include <sstream>

namespace {
	tess::number central_angle(int n) {
		return (tess::number(2) * tess::pi()) / tess::number(n);
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

	std::vector<std::tuple<tess::number, tess::number>> isosceles_triangle(tess::number theta)
	{
		using num = tess::number;
		num half_theta = theta / num(2);
		num half_base = tess::sin(half_theta);
		num height = tess::cos(half_theta);

		return std::vector<std::tuple<tess::number, tess::number>> {
			{ num(0), num(0) },
			{ num(2) * half_base, num(0) },
			{ half_base, height }
		};
	}

	tess::expr_value flip(tess::allocator& a, const tess::expr_value& arg)
	{
		if (!std::holds_alternative<tess::tile>(arg) && !std::holds_alternative<tess::tile_patch>(arg))
			return { tess::error("attempted to flip a value that is not a tile or patch") };

		std::variant<tess::tile, tess::tile_patch> flippable_val = variant_cast(arg.clone(a));
		auto flipped = std::visit(
			[](auto&& flippee)->tess::expr_value {
				tess::get_impl(flippee)->flip();
				return { flippee };
			},
			flippable_val
		);

		return flipped;
	}

	std::vector<std::tuple<tess::number, tess::number>> polygon(const tess::expr_value& arg)
	{
		if (! std::holds_alternative<tess::cluster>(arg))
			return {};

		auto* vertices = tess::get_impl(std::get<tess::cluster>(arg));
		std::vector<std::tuple<tess::number, tess::number>> tuples(vertices->get_ary_count());
		try {
			std::transform(vertices->begin(), vertices->end(), tuples.begin(),
				[&](const auto& ev)->std::tuple<tess::number, tess::number> {
					auto* pt = tess::get_impl(std::get<tess::cluster>(ev));
					return { std::get<tess::number>(pt->get_ary_item(0)),  std::get<tess::number>(pt->get_ary_item(1)) };
				}
			);
		} catch (...) {
			return {};
		}
		return tuples;
	}

}

/*----------------------------------------------------------------------*/

tess::number_expr::number_expr(double v) : val_(v)
{
}

tess::number_expr::number_expr(int v) : val_(v)
{
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

void tess::addition_expr::compile(stack_machine::stack& stack) const
{
	stack.push(std::make_shared<val_func_op>( 
		static_cast<int>(terms_.size()),
		[](allocator& a, const std::vector<expr_value>& args)->expr_value {
			number sum = 0;
			for (const auto& term : args) {
				sum += std::get<number>(term);
			}
			return { sum };
		},
		"<add " + std::to_string(terms_.size()) + ">"
		)
	);
	for (auto [sign, term_expr] : terms_) {
		if (!sign) {
			stack.push(
				std::make_shared<val_func_op>(
					1,
					[](allocator& a, const std::vector<expr_value>& args)->expr_value {
						return { -std::get<number>(args[0]) };
					},
					"<neg>"
				)
			);
		}
		term_expr->compile(stack);
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

void tess::multiplication_expr::compile(stack_machine::stack& stack) const
{
	stack.push(std::make_shared<val_func_op>(
			static_cast<int>(factors_.size()),
			[](allocator& a, const std::vector<expr_value>& args)->expr_value {
				number product = 1;
				for (const auto& term : args) {
					product *= std::get<number>(term);
				}
				return { product };
			},
			"<multiply " + std::to_string(factors_.size()) + ">"
		)
	);
	for (auto [sign, factor_expr] : factors_) {
		if (!sign) {
			stack.push(
				std::make_shared<val_func_op>(
					1,
					[](allocator& a, const std::vector<expr_value>& args)->expr_value {
						return { number(1) / std::get<number>(args[0]) };
					},
					"<reciprocal>"
				)
			);
		}
		factor_expr->compile(stack);
	}
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

std::string tess::multiplication_expr::to_string() const
{
	std::stringstream ss;
	ss << "( * ";
	for (const auto& [op, factor] : factors_) {
		if (!op) {
			ss << "( / 1 " << factor->to_string() << ")";
		}
		else {
			ss << factor->to_string();
		}
		ss << " ";
	}
	ss << ")";

	return ss.str();
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

void tess::exponent_expr::compile(stack_machine::stack& stack) const
{
	int n = static_cast<int>(exponents_.size() + 1);
	stack.push(std::make_shared<val_func_op>(
			n,
			[](allocator& a, const std::vector<expr_value>& args)->expr_value {
				number power = std::get<number>(args[0]);
				if (args.size() > 1) {
					for (auto e = std::next(args.begin()); e != args.end(); e++) 
						power = tess::pow(power, std::get<number>(*e));
				}
				return { power };
			},
			"<exp " + std::to_string(n) + ">"
		)
	);
	base_->compile(stack);
	for (auto exp : exponents_) {
		exp->compile(stack);
	}
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
        throw tess::error("attempted to parse invalid special number");
}

tess::special_number_expr::special_number_expr(special_num which) :
	num_(which)
{
}

void tess::special_number_expr::compile(stack_machine::stack& stack) const
{
	switch (num_) {
		case special_num::pi:
			stack.push({ tess::expr_value{ tess::pi() } });
			break;
		case special_num::phi:
			stack.push({ tess::expr_value{ tess::phi() } });
			break;
		case special_num::root_2:
			stack.push({ tess::expr_value{ tess::root_2() } });
			break;
	}
}

/*----------------------------------------------------------------------*/

tess::string_expr::string_expr(std::string str) : val_(str)
{}

void tess::string_expr::compile(stack_machine::stack& stack) const
{
	stack.push(tess::expr_value{ val_ });
}

std::string tess::string_expr::to_string() const {
	return std::string("#(\"") + val_ + "\")";
}

tess::expr_ptr tess::string_expr::simplify() const {
	return std::make_shared<string_expr>(val_);
}

void tess::string_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const {
}

/*----------------------------------------------------------------------*/

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
	else if (func_keyword == parser::keyword(parser::kw::isosceles_triangle))
		func_ = special_func::isosceles_triangle;
	else if (func_keyword == parser::keyword(parser::kw::flip))
		func_ = special_func::flip;
	else if (func_keyword == parser::keyword(parser::kw::polygon))
		func_ = special_func::polygon;
	else
        throw tess::error("attempted to parse invalid special function");

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
			return { a.create<tess::tile>(&a, locs) };
		};
	case tess::special_func::flip:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value {
			return flip(a, arg);
		};
	case tess::special_func::isosceles_triangle:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value {
			auto locs = isosceles_triangle(std::get<tess::number>(arg));
			return { a.create<tess::tile>(&a, locs) };
		};
	case tess::special_func::polygon:
		return [](tess::allocator& a, tess::expr_value arg)->tess::expr_value {
			auto locs = polygon( arg );
			return { a.create<tess::tile>(&a, locs) };
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
	case tess::special_func::polygon:
		return "polygon";
	case tess::special_func::isosceles_triangle:
		return "isosceles_triangle";
	case tess::special_func::flip:
		return "flip";
	default:
		throw tess::error("Unknown special function");
	}
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

void tess::and_expr::compile(stack_machine::stack& stack) const
{
	int n = static_cast<int>(conjuncts_.size());
	stack.push(
		std::make_shared<val_func_op>(
			n,
			[](allocator& a, const std::vector<expr_value>& args)->expr_value {
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

void tess::equality_expr::compile(stack_machine::stack& stack) const
{
	int n = static_cast<int>(operands_.size());
	stack.push(
		std::make_shared<val_func_op>(
			n,
			[](allocator& a, const std::vector<expr_value>& args) -> expr_value {

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

void tess::or_expr::compile(stack_machine::stack& stack) const
{
	stack.push(
		std::make_shared<val_func_op>(
			static_cast<int>(disjuncts_.size()),
			[](allocator& a, const std::vector<expr_value>& args) -> expr_value {
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
        throw tess::error("attempted to parse invalid relation_expr");
}

tess::relation_expr::relation_expr(expr_ptr lhs, relation_op op, expr_ptr rhs) :
	lhs_(lhs), op_(op), rhs_(rhs)
{
}

void tess::relation_expr::compile(stack_machine::stack& stack) const
{
	auto relation = op_;
	stack.push(
		std::make_shared<val_func_op>(
			2,
			[relation](allocator& a, const std::vector<expr_value>& args) -> expr_value {
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

tess::expr_ptr tess::nil_expr::simplify() const
{
	return std::make_shared<nil_expr>();
}

void tess::nil_expr::compile(stack_machine::stack& stack) const
{
	stack.push(expr_value{ nil_val() });
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

std::string tess::if_expr::to_string() const
{
	return "( if " + condition_->to_string() + " " + then_clause_->to_string() + " " + else_clause_->to_string() + " )";
}
