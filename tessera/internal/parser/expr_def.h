#pragma once

#include "parser.h"
#include "../expression.h"
#include "expr.h"
#include "keywords.h"
#include "util.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser
    {
        using namespace x3;

        template<typename T>
        auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };
        using op_expr = std::tuple<char, std::shared_ptr<expression>>;

        rule<class indentifier_expr_, std::shared_ptr<expression>> const identifier_expr = "identifier_expr";
        rule<class indentifier_parser_, std::string> const indentifier_str_parser = "indentifier_str_parser";
        rule<class number_, std::shared_ptr<expression>> const number = "number";
        rule<class basic_expr_, std::shared_ptr<expression>> const basic_expr = "basic_expr";
        rule<class factor_, std::shared_ptr<expression>> const factor = "factor";
        rule<class term_, std::shared_ptr<expression>> const term = "term";
        rule<class expr_, std::shared_ptr<expression>> const expr = "expr";

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
        auto const indentifier_str_parser_def = as<std::string>[unchecked_identifier - distinct_keyword];
        auto const identifier_expr_def = indentifier_str_parser[make_<variable_expr>];
        auto const number_def = double_[make_<number_expr>];
        auto const basic_expr_def = identifier_expr | number | ('(' >> expr >> ')');
        auto const exp_pair = as<op_expr>[char_("^") >> basic_expr];
        auto const factor_def = as<tess::expression_params>[basic_expr >> *exp_pair][make_<exponent_expr>];
        auto const factor_pair = as<op_expr>[(char_('*') | char_('/')) >> factor];
        auto const term_def = as<tess::expression_params>[factor >> *factor_pair][make_<multiplication_expr>];
        auto const term_pair = as<op_expr>[(char_('+') | char_('-')) >> term];
        auto const expr_def = as<tess::expression_params>[term >> *term_pair][make_<addition_expr>];

        BOOST_SPIRIT_DEFINE(
            identifier_expr,
            indentifier_str_parser,
            number,
            basic_expr,
            factor,
            term,
            expr
        );
    }

    const parser::expr_type& parser::expr_parser()
    {
        return parser::expr;
    }

    const parser::indentifier_parser_type& parser::identifier_parser()
    {
        return parser::indentifier_str_parser;
    }
}