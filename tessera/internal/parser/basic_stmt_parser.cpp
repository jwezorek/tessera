#include "statement_parser.h"
#include "object_ref_expr_parser.h"
#include "expr_parser.h"
#include "cond_expr_parser.h"
#include "util.h"
#include "keywords.h"
#include "../statement.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include "basic_stmt_parser.h"

namespace x3 = boost::spirit::x3;

BOOST_FUSION_ADAPT_STRUCT(tess::lay_params,
	tiles, such_that_clauses
)

namespace tess {
	namespace parser
	{
		template<typename T>
		auto make_ =
			[&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

		auto make_obj_ref =
			[](auto& ctx) { _val(ctx) = std::static_pointer_cast<object_ref_expr>(_attr(ctx)); };

		const auto expr = expression_();
		const auto obj_ref_generic = object_ref_expr_();
		const auto identifier = indentifier_str_();

		x3::rule<class obj_ref_, obj_ref_ptr> const obj_ref = "obj_ref";
		x3::rule<class obj_ref_pair_, std::tuple< obj_ref_ptr, obj_ref_ptr> > obj_ref_pair = "obj_ref_pair";
		x3::rule<class full_lay_stmt_aux_, lay_params> const full_lay_stmt_aux = "full_lay_stmt_aux";
		x3::rule<class full_lay_stmt_, stmt_ptr> const full_lay_stmt = "full_lay_stmt";
		x3::rule<class partial_lay_stmt_aux_, std::vector<obj_ref_ptr>> const partial_lay_stmt_aux = "partial_lay_stmt_aux";
		x3::rule<class partial_lay_stmt_, stmt_ptr> const partial_lay_stmt = "partial_lay_stmt";
		x3::rule<class lay_statement_, stmt_ptr> const lay_stmt = "lay_stmt";
		x3::rule<class let_statement_aux_, std::tuple<std::string,expr_ptr>> const let_stmt_aux = "let_stmt_aux";
		x3::rule<class let_statement_, stmt_ptr> const let_stmt = "let_stmt";

		auto const obj_ref_def = obj_ref_generic[make_obj_ref];
		auto const obj_ref_pair_def = obj_ref >> "<->" >> obj_ref;
		auto const full_lay_stmt_aux_def = kw_lit<kw::lay>() >> (obj_ref% x3::lit("->")) >> kw_lit<kw::such_that>() >> (obj_ref_pair% x3::lit(',')) > x3::lit(';');
		auto const full_lay_stmt_def = full_lay_stmt_aux[make_<tess::lay_statement>];
		auto const partial_lay_stmt_aux_def = kw_lit<kw::lay>() >> (obj_ref% x3::lit("->")) > x3::lit(';');
		auto const partial_lay_stmt_def = partial_lay_stmt_aux[make_<tess::lay_statement>];
		auto const lay_stmt_def = full_lay_stmt | partial_lay_stmt;
		auto const let_stmt_aux_def = kw_lit<kw::let>() > identifier > x3::lit('=') > expr > x3::lit(';');
		auto const let_stmt_def = let_stmt_aux[make_<tess::let_statement>];

		BOOST_SPIRIT_DEFINE(
			obj_ref,
			obj_ref_pair,
			full_lay_stmt_aux,
			full_lay_stmt,
			partial_lay_stmt_aux,
			partial_lay_stmt,
			lay_stmt,
			let_stmt_aux,
			let_stmt
		);

	}
}

std::tuple<tess::stmt_ptr, std::string::const_iterator> tess::parser::lay_stmt_::parse_lay_stmt(const text_range& input)
{
	tess::stmt_ptr output;
	auto iter = input.begin();
	bool success = false;

	try {
		success = x3::phrase_parse(iter, input.end(), tess::parser::lay_stmt, x3::space, output);
	}
	catch (...) {
	}

	if (success)
		return { output, iter };
	else
		return { tess::stmt_ptr(), iter };
}

std::tuple<tess::stmt_ptr, std::string::const_iterator> tess::parser::let_stmt_::parse_let_stmt(const text_range& input)
{
	tess::stmt_ptr output;
	auto iter = input.begin();
	bool success = false;

	try {
		success = x3::phrase_parse(iter, input.end(), tess::parser::let_stmt, x3::space, output);
	} catch (...) {
	}

	if (success)
		return { output, iter };
	else
		return { tess::stmt_ptr(), iter };
}