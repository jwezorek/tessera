#include "function_parser.h"
#include "../function_def.h"
#include "where_parser.h"
#include "with_parser.h"
#include "expr_parser.h"
#include "util.h"
#include "keywords.h"
#include "skipper.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <optional>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {

		auto make_func = [&](auto& ctx) { 
			auto [params, body, where, with] = _attr(ctx);
			body = (with.has_value() ) ?
				std::make_shared<tess::with_expr>(with.value(), body) :
				body;
			body = (where.has_value()) ?
				std::make_shared<tess::where_expr>(where.value(), body) :
				body;
			_val(ctx) = std::make_shared<function_def>(params, body);
		};

		x3::rule<class func_aux_, std::tuple<std::vector<std::string>, expr_ptr, std::optional<assignment_block>, std::optional<field_definitions>>> const func_aux = "func_aux";
		x3::rule<class func_, tess::expr_ptr> const func = "func";

		const auto expr = expression_();
		const auto identifier = indentifier_str_();
		const auto where_clause = trailing_where_();
		const auto with_clause = trailing_with_();
		const auto parameters = -(x3::lit('(') >> (identifier % x3::lit(',')) > x3::lit(')'));
		const auto func_aux_def = kw_lit<kw::func>() > parameters > x3::lit('{') > expr > x3::lit('}') >> -(where_clause) >> -(with_clause);
		const auto func_def = func_aux[make_func];

		BOOST_SPIRIT_DEFINE(
			func_aux,
			func
		);

	}
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::function_def_::parse_aux(const text_range& input) const
{
	tess::expr_ptr output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::func, skipper(), output);

	if (success)
		return { output, iter };
	else
		return { tess::expr_ptr(), iter };
}
