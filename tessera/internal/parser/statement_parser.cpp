#include "statement_parser.h"
#include "ctrl_struct_stmts_parser.h"
#include "object_ref_expr_parser.h"
#include "basic_stmt_parser.h"
#include "../expression.h"
#include "../statement.h"
#include "expr_parser.h"
#include "util.h"
#include "keywords.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser
    {
        template<typename T>
        auto make_ = 
			[&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

		const auto if_stmt = if_stmt_();
		const auto lay_stmt = lay_stmt_();

        x3::rule<class stmt, stmt_ptr> const stmt = "statement";
		x3::rule<class statements_, std::vector<stmt_ptr>> const statements = "statements";

        auto const stmt_def = lay_stmt | if_stmt;
		auto const statements_def = +(stmt);

        BOOST_SPIRIT_DEFINE (
            stmt,
			statements
        );
    }
}

std::tuple<tess::stmt_ptr, std::string::const_iterator> tess::parser::statement_::parse_statement(const text_range& input)
{
	tess::stmt_ptr output;
    auto iter = input.begin();
    bool success = false;

    success = x3::phrase_parse(iter, input.end(), tess::parser::stmt, x3::space, output);

    if (success)
        return { output, iter };
    else
		return { stmt_ptr(), iter };
}

std::variant<tess::stmts, tess::parser::exception> tess::parser::parse_statements(const tess::text_range& input)
{
    tess::stmts output;
    bool success = false;
    auto iter = input.begin();

    try {
        success = x3::phrase_parse(iter, input.end(), x3::lit('{') >> tess::parser::statements >> x3::lit('}'), x3::space, output);
    }
    catch (x3::expectation_failure<std::string::const_iterator> ex_fail) {
        return exception("", ex_fail);
    }

    if (!success || iter != input.end())
        return exception("", "syntax error", iter);

    return output;
}

std::variant<tess::stmt_ptr, tess::parser::exception> tess::parser::parse_single_statement_block(const text_range& input)
{
    tess::stmt_ptr output;
    bool success = false;
    auto iter = input.begin();

    try {
        success = x3::phrase_parse(iter, input.end(), x3::lit('{') >> tess::parser::stmt >> x3::lit('}'), x3::space, output);
    } catch (x3::expectation_failure<std::string::const_iterator> ex_fail) {
        return exception("", ex_fail);
    }

    if (!success || iter != input.end())
        return exception("", "syntax error", iter);

    return output;
}