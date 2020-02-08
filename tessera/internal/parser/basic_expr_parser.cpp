#include "basic_expr_parser.h"
#include "../expression.h"
#include "object_ref_expr_parser.h"
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
        x3::rule<class special_num_aux_, std::string> special_num_aux = "special_num_aux";
        x3::rule<class special_num_, expr_ptr> const special_num;
        x3::rule<class special_func_, std::string> const special_func = "special_func";
        x3::rule<class special_func_expr_, expr_ptr> const special_func_expr = "special_func_expr";
        x3::rule<class special_func_expr_aux_, std::tuple<std::string, expr_ptr>> const special_func_expr_aux = "special_func_expr_aux";\
		x3::rule<class nil_, expr_ptr> const nil = "nil";

        auto const expr = expression_();
        auto const indentifier_str = indentifier_str_();
        auto const object_ref_expr = object_ref_expr_();
        auto const special_num_aux_def = kw_<kw::pi>() | kw_<kw::phi>() | kw_<kw::root_2>();
        auto const special_num_def = special_num_aux [make_<special_number_expr>];
        auto const number_def = x3::double_[make_<number_expr>];
        auto const special_func_def = kw_<kw::sqrt>() | kw_<kw::sin>() | kw_<kw::cos>() | kw_<kw::tan>() |
                                      kw_<kw::arcsin>() | kw_<kw::arccos>() | kw_<kw::arctan>(); 
        auto const special_func_expr_aux_def = special_func > '(' > expr > ')';
        auto const special_func_expr_def = special_func_expr_aux[make_<special_function_expr>]; 
		auto const nil_def = kw_lit<kw::nil>()[make_nil];

		auto const basic_expr_def = nil | object_ref_expr | number | special_num | special_func_expr | ('(' >> expr >> ')');

        BOOST_SPIRIT_DEFINE(
            number,
            special_num,
            special_num_aux,
            special_func,
            special_func_expr,
            special_func_expr_aux,
			nil,
            basic_expr
        )
    }
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::parse_basic_expr(const text_range& input)
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