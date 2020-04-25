#include "assignment_parser.h"
#include "keywords.h"
#include "expr_parser.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {
		const auto expr = expression_();
		const auto identifier = indentifier_str_();
		const auto assgnmnt_block = assignment_block_();
        auto const assignment_stmt = kw_lit<kw::let>() > identifier > x3::lit('=') > expr > x3::lit(';');
		auto const assignments = *assignment_stmt;
		auto const trailing_where_aux = x3::lit('}') >> kw_lit<kw::where>() > x3::lit('{') > assgnmnt_block > x3::lit('}');

		x3::rule<class trailing_where__, tess::assignment_block> trailing_where = "trailing_where";
		auto const trailing_where_def = trailing_where_aux | x3::lit('}');
		BOOST_SPIRIT_DEFINE(
			trailing_where
		);
    }
}

std::tuple<tess::assignment_block, std::string::const_iterator> tess::parser::assignment_block_::parse_aux(const text_range& input) const
{
	std::vector<var_assignment> output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::assignments, x3::space, output);
	if (success)
		return { assignment_block(output), iter };
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

