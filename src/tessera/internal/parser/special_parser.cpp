#include "special_parser.h"
#include "expr_parser.h"
#include "util.h"
#include "keywords.h"
#include "skipper.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {
		template<typename T>
		auto make_ = [](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };
		auto make_token = [](auto& ctx) { _val(ctx) = token(_attr(ctx)); };

		const auto expr = expression_();
		x3::rule<class special_num_aux_, std::string> special_num_aux = "special_num_aux";
		x3::rule<class special_num_, expr_ptr> const special_num;
		x3::rule<class special_func_, kw> const special_func = "special_func";
		x3::rule<class special_func_expr_, expr_ptr> const special_func_expr = "special_func_expr";
		x3::rule<class special_func_expr_aux_, std::tuple<kw, std::vector<expr_ptr>>> const special_func_expr_aux = "special_func_expr_aux";
		x3::rule<class special_, expr_ptr> const special_expr = "special_expr";

		auto const special_num_aux_def = kw_<kw::pi>() | kw_<kw::phi>() | kw_<kw::root_2>();
		auto const special_num_def = special_num_aux[make_<special_number_expr>];
		auto const special_func_def = as<std::string>[kw_<kw::sqrt>() | kw_<kw::sin>() | kw_<kw::cos>() | kw_<kw::tan>() |
			kw_<kw::arcsin>() | kw_<kw::arccos>() | kw_<kw::arctan>() | kw_<kw::regular_polygon>() | kw_<kw::triangle_by_sides>() |
			kw_<kw::isosceles_triangle>() | kw_<kw::isosceles_trapezoid>() | kw_<kw::rhombus>() | kw_<kw::flip>() | kw_<kw::polygon>() | kw_<kw::join>()] [make_token];
		auto const special_func_expr_aux_def = special_func > '(' > (expr % x3::lit(',')) > ')';
		auto const special_func_expr_def = special_func_expr_aux[make_<special_function_expr>];
		auto const special_expr_def = special_func_expr | special_num;

		BOOST_SPIRIT_DEFINE(
			special_num,
			special_num_aux,
			special_func,
			special_func_expr,
			special_func_expr_aux,
			special_expr
		)
	}
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::special_expr_::parse_aux(const text_range& input) const
{
	tess::expr_ptr output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::special_expr, skipper(), output);
	if (success)
		return { output, iter };
	else
		return { tess::expr_ptr(), iter };
}