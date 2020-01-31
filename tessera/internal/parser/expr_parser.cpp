#include "../expression.h"
#include "expr_parser.h"
#include "object_ref_expr_parser.h"
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
        auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };
        using op_expr = std::tuple<char, expr_ptr>;
   
        x3::rule<class number_, expr_ptr> const number = "number";
        x3::rule<class basic_expr_, expr_ptr> const basic_expr = "basic_expr";
        x3::rule<class factor_, expr_ptr> const factor = "factor";
        x3::rule<class term_, expr_ptr> const term = "term";
        x3::rule<class expr_, expr_ptr> const expr = "expr";
        x3::rule<class special_func_, std::string> const special_func = "special_func";
        x3::rule<class special_func_expr_, expr_ptr> const special_func_expr = "special_func_expr";
        x3::rule<class special_func_expr_aux_, std::tuple<std::string, expr_ptr>> const special_func_expr_aux = "special_func_expr_aux";

        auto const indentifier_str = indentifier_str_();
        auto const object_ref_expr = object_ref_expr_();
        auto const number_def = x3::double_[make_<number_expr>];
        auto const special_func_def = kw_<kw::sqrt>() | kw_<kw::sin>() | kw_<kw::cos>() | kw_<kw::tan>() | 
                kw_<kw::arcsin>() | kw_<kw::arccos>() | kw_<kw::arctan>();
        auto const special_func_expr_aux_def = special_func > '(' > expr > ')';
        auto const special_func_expr_def = special_func_expr_aux[make_<special_function_expr>];
        auto const special_num_expr = as<std::string>[(kw_<kw::pi>() | kw_<kw::phi>() | kw_<kw::root_2>())][make_<special_number_expr>];
        auto const basic_expr_def = object_ref_expr | number | special_num_expr | special_func_expr | ('(' >> expr >> ')');
        auto const exp_pair = as<op_expr>[x3::char_("^") >> basic_expr];
        auto const factor_def = as<tess::expression_params>[basic_expr >> *exp_pair][make_<exponent_expr>];
        auto const factor_pair = as<op_expr>[(x3::char_('*') | x3::char_('/')) >> factor];
        auto const term_def = as<tess::expression_params>[factor >> *factor_pair][make_<multiplication_expr>];
        auto const term_pair = as<op_expr>[(x3::char_('+') | x3::char_('-')) >> term];
        auto const expr_def = as<tess::expression_params>[term >> *term_pair][make_<addition_expr>];

        BOOST_SPIRIT_DEFINE(
            number,
            special_func,
            special_func_expr_aux,
            special_func_expr,
            basic_expr,
            factor,
            term,
            expr
        );
    }
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::parse_expression(const tess::text_range& input)
{
    tess::expr_ptr output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tess::parser::expr, x3::space, output);
    } catch (...) {
    }

    if (success)
        return { output, iter };
    else
        return { tess::expr_ptr(), iter };
}