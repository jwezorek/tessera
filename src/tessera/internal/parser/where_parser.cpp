#include "../variant_util.h"
#include "../expression.h"
#include "../where_expr.h"
#include "../object_expr.h"
#include "where_parser.h"
#include "keywords.h"
#include "expr_parser.h"
#include "util.h"
#include "skipper.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {

		template<typename T>
		auto make_ = [](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

		x3::rule<class trailing_where__, tess::assignment_block> trailing_where = "trailing_where";
		x3::rule<class single_assignment_, var_assignment> single_assignment = "single_assignment";
		x3::rule<class single_ident_as_vec_, std::vector<std::string>> single_ident_as_vec = "single_ident_as_vec";
		x3::rule<class multi_assignment_, var_assignment> multi_assignment = "assignment_stmt";
		x3::rule<class asgn_stmt_, var_assignment> assignment_stmt = "assignment_stmt";
		x3::rule<class where_expression_, expr_ptr> where_expr = "where_expr";

		auto make_vector = [](auto& ctx) { _val(ctx) = std::vector<std::string>{ _attr(ctx) }; };

		const auto expr = expression_();
		const auto identifier = indentifier_str_();
		const auto assgnmnt_block = assignment_block_();
		const auto single_ident_as_vec_def = identifier[make_vector];
        auto const single_assignment_def = kw_lit<kw::let>() >> single_ident_as_vec > x3::lit('=') > expr > x3::lit(';');
		auto const multi_assignment_def = kw_lit<kw::let>() >> (identifier % x3::lit(',')) > x3::lit('=') >> expr > x3::lit(';');
		auto const assignment_stmt_def = multi_assignment | single_assignment;
		auto const assignments = *(assignment_stmt);
		auto const trailing_where_def = kw_lit<kw::where>() > x3::lit('{') > assgnmnt_block > x3::lit('}');
		auto const where_expr_def = as<where_expr_params>[x3::lit('{') >> expr >> x3::lit('}') >> trailing_where][make_<tess::where_expr>];

		BOOST_SPIRIT_DEFINE(
			trailing_where,
			single_assignment,
			multi_assignment,
			assignment_stmt,
			single_ident_as_vec,
			where_expr
		);
    }
}

std::tuple<tess::assignment_block, std::string::const_iterator> tess::parser::assignment_block_::parse_aux(const text_range& input) const
{
	std::vector<tess::var_assignment> output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::assignments, skipper(), output);
	if (success)
		return { assignment_block(output), iter };
	else
		return { assignment_block(), iter };
}

std::tuple<tess::assignment_block, std::string::const_iterator> tess::parser::trailing_where_::parse_aux(const text_range& input) const
{
	assignment_block output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::trailing_where, skipper(), output);

	if (success)
		return { assignment_block(output), iter };
	else
		return { assignment_block(), iter };
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::where_expr_::parse_aux(const text_range& input) const
{
	tess::expr_ptr output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::where_expr, skipper(), output);
	if (success)
		return { output, iter };
	else
		return { tess::expr_ptr(), iter };
}

