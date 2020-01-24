#include "../expression.h"
#include "expr_parser.h"
#include "util.h"
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

        struct keywords_t : x3::symbols<x3::unused_type> {
            keywords_t() {
                add(kw_if, x3::unused);
                add(kw_else, x3::unused);
                add(kw_lay, x3::unused);
                add(kw_tile, x3::unused);
                add(kw_vertex, x3::unused);
                add(kw_edge, x3::unused);
                add(kw_angle, x3::unused);
                add(kw_class, x3::unused);
                add(kw_patch, x3::unused);
                add(kw_such_that, x3::unused);
                add(kw_tableau, x3::unused);
                add(kw_where, x3::unused);
                add(kw_length, x3::unused);
                add(kw_pi, x3::unused);
                add(kw_sqrt, x3::unused);
            }
        } const keywords;

        auto const distinct_keyword = x3::lexeme[keywords >> !(x3::alnum | '_')];
        auto const unchecked_identifier = x3::lexeme[(x3::alpha | x3::char_('_')) >> *(x3::alnum | x3::char_('_'))];
        auto const indentifier_str = as<std::string>[unchecked_identifier - distinct_keyword];
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
        return input.left_range(iter).make_error("invalid expression");
}