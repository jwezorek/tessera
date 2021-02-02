#include "cluster_expr_parser.h"
#include "expr_parser.h"
#include "../value.h"
#include "../cluster_expr.h"
#include "util.h"
#include "keywords.h"
#include "skipper.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {
		template<typename T>
		auto make_ = [](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };
		
		x3::rule<class cluster_expr_aux__, std::vector<expr_ptr>> cluster_expr_aux = "cluster_expr_aux";
		x3::rule<class cluster_expr__, expr_ptr> cluster_expr = "cluster_expr";
		x3::rule<class range_expr_aux__, std::tuple<expr_ptr, expr_ptr>> range_expr_aux = "range_expr";
		x3::rule<class range_expr__, expr_ptr> range_expr = "range_expr";
		x3::rule<class comprehension_expr_aux__, std::tuple<expr_ptr, std::string, expr_ptr>> comprehension_expr_aux = "comprehension_expr_aux";
		x3::rule<class comprehension_expr__, expr_ptr> comprehension_expr = "comprehension_expr";

		const auto expr = expression_();
		const auto identifier = indentifier_str_();

		const auto cluster_expr_aux_def = x3::lit('[') >> (expr % x3::lit(',')) >> x3::lit(']');
		const auto cluster_expr_def = cluster_expr_aux[make_<tess::cluster_expr>];

		const auto range_expr_aux_def = x3::lit('[') >> expr >> x3::lit("...") > expr > x3::lit(']');
		const auto range_expr_def = range_expr_aux[make_<tess::num_range_expr>];

		const auto comprehension_expr_aux_def = x3::lit('[') >> expr >> kw_lit<kw::for_>() > identifier > kw_lit<kw::in>() > expr > x3::lit(']');
		const auto comprehension_expr_def = comprehension_expr_aux [make_<tess::cluster_comprehension_expr>];

		const auto various_cluster_exprs = range_expr | cluster_expr | comprehension_expr;

		BOOST_SPIRIT_DEFINE(
			cluster_expr_aux,
			cluster_expr,
			range_expr_aux,
			range_expr,
			comprehension_expr_aux,
			comprehension_expr
		)
	}
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::cluster_expr_::parse_aux(const text_range& input) const
{
	expr_ptr output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::various_cluster_exprs, skipper(), output);
	if (success)
		return {  output, iter };
	else
		return { tess::expr_ptr(), iter };
}