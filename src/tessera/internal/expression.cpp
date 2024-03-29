#include "number.h"
#include "expression.h"
#include "value.h"
#include "lambda_impl.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "execution_state.h"
#include "tile_impl.h"
#include "tessera_impl.h"
#include "parser/keywords.h"
#include "ops.h"
#include "gc_heap.h"
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

	std::vector<std::tuple<tess::number, tess::number>> quadrilateral(tess::number d1, tess::number theta1, tess::number theta2, tess::number d2) {
		using num = tess::number;
		return std::vector<std::tuple<tess::number, tess::number>> {
			{ num(0), num(0) },
			{ num(1), num(0) },
			{ num(1) + d2 * tess::cos(tess::pi() - theta2), d2 * tess::sin(tess::pi() - theta2)},
			{ d1 * tess::cos(theta1), d1 * tess::sin(theta1)}
		};
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

	std::vector<std::tuple<tess::number, tess::number>> isosceles_trapezoid(tess::number theta, tess::number len)
	{
		using num = tess::number;
		return quadrilateral(
			len, theta, theta, len
		);
	}

	std::vector<std::tuple<tess::number, tess::number>> rhombus(tess::number theta)
	{
		using num = tess::number;
		return quadrilateral(
			num(1), theta, tess::pi() - theta, num(1)
		);
	}

	std::vector<std::tuple<tess::number, tess::number>> triangle_by_sides(tess::number s1, tess::number s2, tess::number s3)
	{
		using num = tess::number;

		auto c = s1;
		auto a = s2;
		auto b = s3;

		auto cos_BAC = (b * b + c * c - a * a) / (2 * b * c);
		auto C_x = b * cos_BAC;
		auto C_y = tess::sqrt(b * b - C_x * C_x);

		return { {0,0}, {c,0}, {C_x,C_y} };
	}

	tess::value_ flip(tess::gc_heap& a, const tess::value_& arg)
	{
		if (!std::holds_alternative<tess::const_tile_root_ptr>(arg) && !std::holds_alternative<tess::const_patch_root_ptr>(arg))
			throw tess::error("attempted to flip a value that is not a tile or patch");

		std::variant<tess::const_tile_root_ptr, tess::const_patch_root_ptr> flippable_val = variant_cast(arg);
		auto flipped = std::visit(
			[&a](auto&& flippee)->tess::value_ {
				return tess::make_value( flippee->flip(a) );
			},
			flippable_val
		);

		return flipped;
	}

	std::vector<std::tuple<tess::number, tess::number>> polygon(const tess::value_& arg)
	{
		if (! std::holds_alternative<tess::const_cluster_root_ptr>(arg))
			return {};

		auto vertices = std::get<tess::const_cluster_root_ptr>(arg);
		std::vector<std::tuple<tess::number, tess::number>> tuples(vertices->get_ary_count());
		try {
			std::transform(vertices->begin(), vertices->end(), tuples.begin(),
				[&](const tess::field_value& ev)->std::tuple<tess::number, tess::number> {
					const auto& pt = std::get<tess::cluster_graph_ptr>(ev);
					return { std::get<tess::number>(pt->get_ary_item(0)),  std::get<tess::number>(pt->get_ary_item(1)) };
				}
			);
		} catch (...) {
			throw tess::error("invalid polygon definition.");
		}
		return tuples;
	}

	struct special_func_def {
		tess::parser::kw tok;
		int num_parameters;
		std::function<tess::value_(tess::gc_heap&, const std::vector<tess::value_>&)> func;
	};

	std::vector<special_func_def> g_special_function_definitions = {
		{
			tess::parser::kw::arccos, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ { return tess::value_( tess::acos(std::get<tess::number>(args[0])) ); }
		} , {
			tess::parser::kw::arcsin, 1,
			[](tess::gc_heap& a,  const std::vector<tess::value_>& args)->tess::value_ { return tess::value_(tess::asin(std::get<tess::number>(args[0])) ); }
		} , {
			tess::parser::kw::arctan, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ { return tess::value_(tess::atan(std::get<tess::number>(args[0])) ); }
		} , {
			tess::parser::kw::cos, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ { return tess::value_(tess::cos(std::get<tess::number>(args[0])) ); }
		} , {
			tess::parser::kw::sin, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ { return tess::value_(tess::sin(std::get<tess::number>(args[0])) ); }
		} , {
			tess::parser::kw::sqrt, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ { return tess::value_(tess::sqrt(std::get<tess::number>(args[0])) ); }
		} , {
			tess::parser::kw::tan, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ { return tess::value_(tess::tan(std::get<tess::number>(args[0])) ); }
		} , {
			tess::parser::kw::regular_polygon, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ {
				auto locs = regular_polygon_vertices(std::get<tess::number>(args[0]));
				return tess::value_( a.make_const<tess::const_tile_root_ptr>( locs) );
			}
		} , {
			tess::parser::kw::flip, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ {
				return flip(a, args[0]);
			}
		} , {
			tess::parser::kw::isosceles_triangle, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ {
				auto locs = isosceles_triangle(std::get<tess::number>(args[0]));
				return tess::value_( a.make_const<tess::const_tile_root_ptr>(locs) );
			}
		} , {
			tess::parser::kw::isosceles_trapezoid, 2,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ {
				auto locs = isosceles_trapezoid(std::get<tess::number>(args[0]), std::get<tess::number>(args[1]));
				return tess::value_( a.make_const<tess::const_tile_root_ptr>(locs));
			}
		} , {
			tess::parser::kw::rhombus, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ {
				auto locs = rhombus(std::get<tess::number>(args[0]));
				return tess::value_( a.make_const<tess::const_tile_root_ptr>(locs) );
			}
		} , {
			tess::parser::kw::polygon, 1,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ {
				auto locs = polygon(args[0]);
				return tess::value_( a.make_const<tess::const_tile_root_ptr>(locs) );
			}
		} , {
			tess::parser::kw::join, 1,
			[](tess::gc_heap& a,const std::vector<tess::value_>& args)->tess::value_ {
				auto patch = std::get<tess::const_patch_root_ptr>(args[0]);
				return tess::make_value(patch->join(a) );
			}
		} , {
			tess::parser::kw::triangle_by_sides, 3,
			[](tess::gc_heap& a, const std::vector<tess::value_>& args)->tess::value_ {
				auto locs = triangle_by_sides(std::get<tess::number>(args[0]), std::get<tess::number>(args[1]), std::get<tess::number>(args[2]));
				return tess::value_(a.make_const<tess::const_tile_root_ptr>(locs) );
			}
		} 
	};

	std::optional<special_func_def> get_special_func_def(tess::parser::kw tok) {
		static std::unordered_map<tess::parser::kw, special_func_def> tbl;
		if (tbl.empty()) {
			std::transform(g_special_function_definitions.begin(), g_special_function_definitions.end(), std::inserter(tbl, tbl.end()),
				[](const special_func_def& def)->std::unordered_map<tess::parser::kw, special_func_def>::value_type {
					return { def.tok, def };
				}
			);
		}
		auto iter = tbl.find(tok);
		if (iter == tbl.end())
			return std::nullopt;
		return iter->second;
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
	stack.push(tess::value_{ tess::number(val_) });
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
		[](gc_heap& a, const std::vector<value_>& args)->value_ {
			number sum = 0;
			for (const auto& term : args) {
				sum += std::get<number>(term);
			}
			return tess::value_(sum );
		},
		"<add " + std::to_string(terms_.size()) + ">"
		)
	);
	for (auto [sign, term_expr] : terms_) {
		if (!sign) {
			stack.push(
				std::make_shared<val_func_op>(
					1,
					[](gc_heap& a, const std::vector<value_>& args)->value_ {
						return tess::value_(-std::get<number>(args[0]) );
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
			[](gc_heap& a, const std::vector<value_>& args)->value_ {
				number product = 1;
				for (const auto& term : args) {
					product *= std::get<number>(term);
				}
				return tess::value_(product );
			},
			"<multiply " + std::to_string(factors_.size()) + ">"
		)
	);
	for (auto [sign, factor_expr] : factors_) {
		if (!sign) {
			stack.push(
				std::make_shared<val_func_op>(
					1,
					[](gc_heap& a, const std::vector<value_>& args)->value_ {
						return tess::value_(number(1) / std::get<number>(args[0]) );
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
			[](gc_heap& a, const std::vector<value_>& args)->value_ {
				number power = std::get<number>(args[0]);
				if (args.size() > 1) {
					for (auto e = std::next(args.begin()); e != args.end(); e++) 
						power = tess::pow(power, std::get<number>(*e));
				}
				return tess::value_(power );
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
			stack.push({ tess::value_{ tess::pi() } });
			break;
		case special_num::phi:
			stack.push({ tess::value_{ tess::phi() } });
			break;
		case special_num::root_2:
			stack.push({ tess::value_{ tess::root_2() } });
			break;
	}
}

/*----------------------------------------------------------------------*/

tess::string_expr::string_expr(std::string str) : val_(str)
{}

void tess::string_expr::compile(stack_machine::stack& stack) const
{
	stack.push(tess::value_{ val_ });
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

/*----------------------------------------------------------------------*/

tess::special_function_expr::special_function_expr(std::tuple<parser::kw, std::vector<expr_ptr>> param)
{
	auto [func_keyword, args] = param;
	func_ = func_keyword;
	args_ = args;
}

tess::special_function_expr::special_function_expr(parser::kw func, expr_ptr arg) :
	func_(func), args_({arg})
{
}

tess::special_function_expr::special_function_expr(parser::kw func, const std::vector<expr_ptr>& args) :
	func_(func), args_(args)
{
}

void tess::special_function_expr::compile(stack_machine::stack& stack) const
{
	auto maybe_func_definition = get_special_func_def(func_);
	if (!maybe_func_definition.has_value())
		throw tess::error("Unknown special function.");
	auto func_def = maybe_func_definition.value();

	stack.push(
		std::make_shared<val_func_op>(
			func_def.num_parameters,
			func_def.func,
			parser::keyword(func_)
		)
	);

	for (auto arg : args_)
		arg->compile(stack);
}

void tess::special_function_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	for (auto arg: args_)
		arg->get_dependencies(dependencies);
}

tess::expr_ptr tess::special_function_expr::simplify() const
{
	std::vector<expr_ptr> simplified_args(args_.size());
	std::transform(args_.begin(), args_.end(), simplified_args.begin(),
		[](const expr_ptr& e)->expr_ptr {
			return e->simplify();
		}
	);
	return std::make_shared< special_function_expr>(func_, simplified_args);
}

std::string tess::special_function_expr::to_string() const
{
	std::stringstream ss;
	ss << "( " << parser::keyword(func_) << " ";
	for (auto e : args_)
		ss << e->to_string() << " ";
	ss << ")";
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
			[](gc_heap& a, const std::vector<value_>& args)->value_ {
				for (const auto& conjunct : args) {
					auto val = std::get<bool>(conjunct);
					if (!val)
						return tess::value_(false );
				}
				return tess::value_(true);
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
			[](gc_heap& a, const std::vector<value_>& args) -> value_ {

				std::vector<number> expressions;
				expressions.reserve(args.size());
				for (const auto& op : args) {
					expressions.push_back(std::get<number>(op));
				}

				auto first = expressions.front();
				for (int i = 1; i < expressions.size(); ++i)
					if (!equals(first, expressions[i]))
						return tess::value_(false );

				return tess::value_(true );
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
			[](gc_heap& a, const std::vector<value_>& args) -> value_ {
				for (const auto& disjunct : args) {
					auto val = std::get<bool>(disjunct);
					if (val)
						return tess::value_(true );
				}
				return tess::value_(false);
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
			[relation](gc_heap& a, const std::vector<value_>& args) -> value_ {
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
					return tess::value_{ result };
				}
				return tess::value_(false );
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
	stack.push(value_{ nil_val() });
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

tess::on_expr::on_expr(expr_ptr patch_expr, expr_ptr arg_expr) :
	patch_expr_(patch_expr),
	arg_expr_(arg_expr)
{
}

void tess::on_expr::compile(stack_machine::stack& stack) const
{
	stack.push(
		std::make_shared<val_func_op>(
			2,
			[](gc_heap& a, const std::vector<value_>& args) -> value_ {
				std::variant<tess::const_tile_root_ptr, tess::const_patch_root_ptr> tile_or_patch = variant_cast(args[0]);
				std::variant<tess::const_edge_root_ptr, tess::const_cluster_root_ptr> arg = variant_cast(args[1]);
				return std::visit(
					[&](const auto& t)->value_ {
						return t->get_on(a, arg);
					},
					tile_or_patch
				);
			},
			"<on>"
		)		
	);
	patch_expr_->compile(stack);
	arg_expr_->compile(stack);
}

void tess::on_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	patch_expr_->get_dependencies(dependencies);
	arg_expr_->get_dependencies(dependencies);
}

tess::expr_ptr tess::on_expr::simplify() const
{
	return std::make_shared< on_expr>(patch_expr_->simplify(), arg_expr_->simplify());
}

std::string tess::on_expr::to_string() const
{
	return "( on " + patch_expr_->to_string() + " " + arg_expr_->to_string() + " )";
}

tess::bool_lit_expr::bool_lit_expr(const std::string& val) : val_(val == parser::keyword(parser::kw::true_))
{
}

tess::bool_lit_expr::bool_lit_expr(bool val) : val_(val)
{
}

void tess::bool_lit_expr::compile(stack_machine::stack& stack) const
{
	stack.push(
		tess::value_{ val_ }
	);
}

void tess::bool_lit_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
}

tess::expr_ptr tess::bool_lit_expr::simplify() const
{
	return std::make_shared<tess::bool_lit_expr>(val_);
}

std::string tess::bool_lit_expr::to_string() const
{
	using namespace std::string_literals;
	return "#("s + ( (val_) ? "true"s : "false"s) + ")"s;
}

tess::clone_expr::clone_expr(expr_ptr clonee) : clonee_(clonee)
{
}

void tess::clone_expr::compile(stack_machine::stack& stack) const
{
	stack.push(
		std::make_shared<one_param_op>(
			[](gc_heap& a, const value_& v)->value_ {
				return tess::clone_value(a, v);
			},
			"clone"
		)
	);
	clonee_->compile(stack);
}

void tess::clone_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	clonee_->get_dependencies(dependencies);
}

tess::expr_ptr tess::clone_expr::simplify() const
{
	return std::make_shared<clone_expr>(clonee_->simplify());
}

std::string tess::clone_expr::to_string() const
{
	return "( clone " + clonee_->to_string() + " )";
}
