#include "statement_parser.h"
#include "ctrl_struct_stmts_parser.h"
#include "cond_expr_parser.h"
#include "util.h"
#include "keywords.h"
#include "../statement.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

namespace x3 = boost::spirit::x3;

BOOST_FUSION_ADAPT_STRUCT(tess::if_params,
	condition, then_clause, else_clause
)

namespace tess {
	namespace parser
	{
		template<typename T>
		auto make_ =
			[&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

		x3::rule<class if_stmt_aux_, if_params> const if_stmt_aux = "if_stmt_aux";
		x3::rule<class if_statement_, stmt_ptr> const if_stmt = "if_stmt";

		auto const stmt = statement_();
		auto const cond_expr = cond_expr_();
		auto const if_stmt_aux_def = kw_lit<kw::if_>() > cond_expr > stmt > kw_lit<kw::else_>() > stmt;
		auto const if_stmt_def = if_stmt_aux[make_<tess::if_statement>];

		BOOST_SPIRIT_DEFINE(
			if_stmt_aux,
			if_stmt
		);
	}
}

std::tuple<tess::stmt_ptr, std::string::const_iterator> tess::parser::parse_if_stmt(const text_range& input)
{
	tess::stmt_ptr output;
	auto iter = input.begin();
	bool success = false;

	try {
		success = x3::phrase_parse(iter, input.end(), tess::parser::if_stmt, x3::space, output);
	}
	catch (...) {
	}

	if (success)
		return { output, iter };
	else
		return { tess::stmt_ptr(), iter };
}
