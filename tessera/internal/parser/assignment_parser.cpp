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

        auto const assignment_stmt = kw_lit<kw::let>() > identifier > x3::lit('=') > expr > x3::lit(';');
    }
}

std::tuple<tess::var_assignment, std::string::const_iterator> tess::parser::assigment_stmt_::parse_aux(const text_range& input) const
{
	var_assignment output;
	auto iter = input.begin();
	bool success = false;

	try {
		success = x3::phrase_parse(iter, input.end(), tess::parser::assignment_stmt, x3::space, output);
	}
	catch (...) {
	}

	if (success)
		return { output, iter };
	else
		return { tess::var_assignment(), iter };
}
