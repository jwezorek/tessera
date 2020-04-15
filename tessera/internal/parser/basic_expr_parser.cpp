#include "basic_expr_parser.h"
#include "cond_expr_parser.h"
#include "../expression.h"
#include "object_ref_expr_parser.h"
#include "special_parser.h"
#include "function_parser.h"
#include "expr_parser.h"
#include "keywords.h"
#include "util.h"
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace tess {
    namespace parser{

        template<typename T>
        auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };
		auto make_nil = [](auto& ctx) { _val(ctx) = std::make_shared<tess::nil_expr>(); };

        x3::rule<class basic_expr_tag, expr_ptr> const basic_expr = "basic_expr";
        x3::rule<class number_, expr_ptr> const number = "number"; 
		x3::rule<class nil_, expr_ptr> const nil = "nil";

        auto const expr = expression_();
        auto const if_expr = if_expr_();
        auto const indentifier_str = indentifier_str_();
        auto const object_ref_expr = object_ref_expr_();
		auto const special_expr = special_expr_();
        auto const function = function_def_();
		auto const number_def = x3::int32[make_<number_expr>];
		auto const nil_def = kw_lit<kw::nil>()[make_nil];

		auto const basic_expr_def =  nil | object_ref_expr | special_expr | number | function | if_expr | ('(' >> expr >> ')') ;

        BOOST_SPIRIT_DEFINE(
            number,
			nil,
            basic_expr
        )
    }
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::basic_expr_::parse_aux(const text_range& input) const
{
    tess::expr_ptr output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tess::parser::basic_expr, x3::space, output);
    }
    catch (...) {
    }

    if (success)
        return { output, iter };
    else
        return { tess::expr_ptr(), iter };
}