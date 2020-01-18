#pragma once

#include "parser.h"
#include "../expression.h"
#include "expr.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser
    {
        using namespace x3;

        template <typename T>
        struct as_type {
            template <typename E>
            constexpr auto operator[](E e) const { return x3::rule<struct _, T> {} = e; }
        };

        template <typename T>
        static inline as_type<T> as;

        auto const skip_whitespace =
            ascii::space |
            lexeme["//" >> *(char_ - eol) >> eol] |
            lexeme["/*"] >> *(char_ - "*/") >> "*/";

        template<typename T>
        auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };
        using op_expr = std::tuple<char, std::shared_ptr<expression>>;

        rule<class indentifier_, std::shared_ptr<expression>> const identifier = "identifier";
        rule<class number_, std::shared_ptr<expression>> const number = "number";
        rule<class basic_expr_, std::shared_ptr<expression>> const basic_expr = "basic_expr";
        rule<class factor_, std::shared_ptr<expression>> const factor = "factor";
        rule<class term_, std::shared_ptr<expression>> const term = "term";
        rule<class expr_, std::shared_ptr<expression>> const expr = "expr";

        auto const identifier_def = as<std::string>[lexeme[(alpha | char_('_')) >> *(alnum | char_('_'))]][make_<variable_expr>];
        auto const number_def = double_[make_<number_expr>];
        auto const basic_expr_def = identifier | number | ('(' >> expr >> ')');
        auto const exp_pair = as<op_expr>[char_("^") >> basic_expr];
        auto const factor_def = as<tess::expression_params>[basic_expr >> *exp_pair][make_<exponent_expr>];
        auto const factor_pair = as<op_expr>[(char_('*') | char_('/')) >> factor];
        auto const term_def = as<tess::expression_params>[factor >> *factor_pair][make_<multiplication_expr>];
        auto const term_pair = as<op_expr>[(char_('+') | char_('-')) >> term];
        auto const expr_def = as<tess::expression_params>[term >> *term_pair][make_<addition_expr>];

        BOOST_SPIRIT_DEFINE(
            identifier,
            number,
            basic_expr,
            factor,
            term,
            expr
        );
    }

    parser::expr_type parser::expr_parser()
    {
        return parser::expr;
    }
}