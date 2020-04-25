#include "../variant_util.h"
#include "../expression.h"
#include "../object_expr.h"
#include "assignment_parser.h"
#include "keywords.h"
#include "expr_parser.h"
#include "util.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {

		using multi_asgn_tup = std::tuple<std::vector<std::string>, expr_ptr>;
		using asgn_var = std::variant< var_assignment, multi_asgn_tup>;

		x3::rule<class trailing_where__, tess::assignment_block> trailing_where = "trailing_where";
		x3::rule<class single_assignment_, var_assignment> single_assignment = "single_assignment";
		x3::rule<class multi_assignment_, multi_asgn_tup> multi_assignment = "assignment_stmt";
		x3::rule<class asgn_stmt_, asgn_var> assignment_stmt = "assignment_stmt";

		const auto expr = expression_();
		const auto identifier = indentifier_str_();
		const auto assgnmnt_block = assignment_block_();
        auto const single_assignment_def = kw_lit<kw::let>() >> identifier >> x3::lit('=') > expr > x3::lit(';');
		auto const multi_assignment_def = kw_lit<kw::let>() >> (identifier % x3::lit(',')) > x3::lit('=') > expr > x3::lit(';');
		auto const assignment_stmt_def = single_assignment | multi_assignment;
		auto const assignments = *(assignment_stmt);
		auto const trailing_where_aux = x3::lit('}') >> kw_lit<kw::where>() > x3::lit('{') > assgnmnt_block > x3::lit('}');
		auto const trailing_where_def = trailing_where_aux | x3::lit('}');

		BOOST_SPIRIT_DEFINE(
			trailing_where,
			single_assignment,
			multi_assignment,
			assignment_stmt
		);
    }
}

void unpack_multiple_assignment(std::vector<tess::var_assignment>& assignments, const tess::parser::multi_asgn_tup& multi)
{
	auto [variables, ary_expr] = multi;
	int i = 0;
	for (const auto& var : variables) {
		auto arry_item_reference = std::make_shared<tess::array_item_expr>(
			ary_expr,
			std::make_shared<tess::number_expr>(i++)
		);
		assignments.push_back( {var, arry_item_reference} );
	}
}

std::tuple<tess::assignment_block, std::string::const_iterator> tess::parser::assignment_block_::parse_aux(const text_range& input) const
{
	std::vector<tess::parser::asgn_var> output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::assignments, x3::space, output);

	std::vector<var_assignment> assign_stmts;
	for (const auto& single_or_multi : output) {
		std::visit(
			overloaded{
				[&assign_stmts](const var_assignment& single) {
					assign_stmts.push_back(single);
				},
				[&assign_stmts](const multi_asgn_tup& multi) {
					unpack_multiple_assignment(assign_stmts, multi);
				}
			},
			single_or_multi
		);
	}

	if (success)
		return { assignment_block(assign_stmts), iter };
	else
		return { assignment_block(), iter };
}

std::tuple<std::optional<tess::assignment_block>, std::string::const_iterator> tess::parser::trailing_where_::parse_aux(const text_range& input) const
{
	assignment_block output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::trailing_where, x3::space, output);

	if (success)
		return { assignment_block(output), iter };
	else
		return { std::nullopt, iter };
}

