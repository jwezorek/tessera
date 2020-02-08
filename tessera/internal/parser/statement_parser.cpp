#include "statement_parser.h"
#include "cond_expr_parser.h"
#include "object_ref_expr_parser.h"
#include "../expression.h"
#include "../statement.h"
#include "expr_parser.h"
#include "util.h"
#include "keywords.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

namespace x3 = boost::spirit::x3;

BOOST_FUSION_ADAPT_STRUCT(tess::lay_params,
    layees, such_that_clauses
)

BOOST_FUSION_ADAPT_STRUCT(tess::if_params,
    condition, then_clause, else_clause
)

namespace tess {
    namespace parser
    {
        template<typename T>
        auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

        const auto expr = expression_();
        const auto obj_ref_generic = object_ref_expr_();
        const auto cond_expr = cond_expr_();

        x3::rule<class obj_ref_, obj_ref_ptr> const obj_ref = "obj_ref";
        x3::rule<class obj_ref_pair_, std::tuple< obj_ref_ptr, obj_ref_ptr> > obj_ref_pair = "obj_ref_pair";
        x3::rule<class lay_stmt_aux_, lay_params> const lay_stmt_aux = "lay_stmt_aux";
        x3::rule<class lay_stmt_, stmt_ptr> const lay_stmt = "lay_stmt";
        x3::rule<class if_stmt_aux_, if_params> const if_stmt_aux = "if_stmt_aux";
        x3::rule<class if_stmt_, stmt_ptr> const if_stmt = "if_stmt";
        x3::rule<class stmt, stmt_ptr> const stmt = "statement";

        auto const obj_ref_def = obj_ref_generic;
        auto const obj_ref_pair_def = obj_ref >> "<->" >> obj_ref;
        auto const lay_stmt_aux_def = kw_lit<kw::lay>() >> (obj_ref% x3::lit("->")) >> kw_lit<kw::such_that>() >> (obj_ref_pair% x3::lit(',')) >> x3::lit(';');
        auto const lay_stmt_def = lay_stmt_aux [make_<tess::lay_statement>];
        auto const if_stmt_aux_def = kw_lit<kw::if_>() >> cond_expr >> stmt >> kw_lit<kw::else_>() >> stmt;
        auto const if_stmt_def = if_stmt_aux [make_<tess::if_statement>];
        auto const stmt_def = lay_stmt | if_stmt;

        BOOST_SPIRIT_DEFINE(
            obj_ref,
            obj_ref_pair,
            lay_stmt_aux,
            lay_stmt,
            if_stmt_aux,
            if_stmt,
            stmt
        );
    }
}

std::tuple<std::vector<tess::stmt_ptr>, std::string::const_iterator> tess::parser::parse_statements(const text_range& input)
{
    /*
    tess::stmt_ptr output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tess::parser::object_ref_expr, x3::space, output);
    }
    catch (...) {
    }

    if (success)
        return { output, iter };
    else
        return { tess::expr_ptr(), iter };
    */
    return { std::vector<tess::stmt_ptr>(), input.begin() };
}


