#include "../expression.h"
#include "expr_parser.h"
#include "util.h"
#include "keywords.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser
    {
        template<typename T>
        auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };
        using op_expr = std::tuple<char, expr_ptr>;

        x3::rule<class indentifier_expr_, expr_ptr> const identifier_expr = "identifier_expr";
        x3::rule<class number_, expr_ptr> const number = "number";
        x3::rule<class basic_expr_, expr_ptr> const basic_expr = "basic_expr";
        x3::rule<class factor_, expr_ptr> const factor = "factor";
        x3::rule<class term_, expr_ptr> const term = "term";
        x3::rule<class expr_, expr_ptr> const expr = "expr";
        
        auto const indentifier_str = indentifier_str_();
        auto const identifier_expr_def = indentifier_str[make_<variable_expr>];
        auto const number_def = x3::double_[make_<number_expr>];
        auto const basic_expr_def = identifier_expr | number | ('(' >> expr >> ')');
        auto const exp_pair = as<op_expr>[x3::char_("^") >> basic_expr];
        auto const factor_def = as<tess::expression_params>[basic_expr >> *exp_pair][make_<exponent_expr>];
        auto const factor_pair = as<op_expr>[(x3::char_('*') | x3::char_('/')) >> factor];
        auto const term_def = as<tess::expression_params>[factor >> *factor_pair][make_<multiplication_expr>];
        auto const term_pair = as<op_expr>[(x3::char_('+') | x3::char_('-')) >> term];
        auto const expr_def = as<tess::expression_params>[term >> *term_pair][make_<addition_expr>];

        BOOST_SPIRIT_DEFINE(
            identifier_expr,
            number,
            basic_expr,
            factor,
            term,
            expr
        );
    }
}

std::variant<tess::expr_ptr, tess::error> tess::parser::parse_expression(const tess::text_range& input)
{
    tess::expr_ptr output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tess::parser::expr, x3::space, output);
    } catch (...) { 
    }

    if (success && iter == input.end())
        return output;
    else
        return tess::error("expr error", -1); //TODO
}