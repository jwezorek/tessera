#include "cond_expr_parser.h"
#include "expr_parser.h"
#include "util.h"
#include "keywords.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {
		template<typename T>
		auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

		const auto expr = expression_();

		x3::rule<class eq_expr_, tess::expr_ptr> const eq_expr = "eq_expr";
		x3::rule<class rel_expr_aux_, std::tuple<expr_ptr, std::string, expr_ptr>> const rel_expr_aux = "rel_expr_aux";
		x3::rule<class rel_expr_, tess::expr_ptr> const  rel_expr = "rel_expr";
		x3::rule<class basic_cond_expr_, tess::expr_ptr> const basic_cond_expr = "basic_cond_expr";
		x3::rule<class and_expr_, tess::expr_ptr> const and_expr = "and_expr";
		x3::rule<class cond_expr_parser_, tess::expr_ptr> const  cond_expr = "cond_expr";

		const auto eq_expr_def = (expr % x3::lit(equ))[make_<equality_expr>];
		const auto rel_op = x3::string(not_equ) | x3::string(lt) | x3::string(gt) |
			x3::string(le) | x3::string(ge);
		
		const auto rel_expr_aux_def = expr >> rel_op >> expr;
		const auto rel_expr_def = rel_expr_aux[make_<tess::relation_expr>];

		const auto basic_cond_expr_def = rel_expr | eq_expr | (x3::lit('(') >> cond_expr >> x3::lit(')'));
		const auto and_expr_def = (basic_cond_expr % kw_lit<kw::and_>())[make_<tess::and_expr>];
		const auto cond_expr_def = (and_expr % kw_lit<kw::or_>())[make_<tess::or_expr>];

		BOOST_SPIRIT_DEFINE(
			eq_expr,
			rel_expr_aux,
			rel_expr,
			basic_cond_expr,
			and_expr,
			cond_expr
		);
	}
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::parse_cond_expr(const text_range& input)
{
	tess::expr_ptr output;
	auto iter = input.begin();
	bool success = false;

	try {
		success = x3::phrase_parse(iter, input.end(), tess::parser::cond_expr, x3::space, output);
	}
	catch (...) {
	}

	if (success)
		return { output, iter };
	else
		return { tess::expr_ptr(), iter };
}
