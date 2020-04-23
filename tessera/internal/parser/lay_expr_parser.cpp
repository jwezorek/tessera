#include "lay_expr_parser.h"
#include "keywords.h"
#include "expr_parser.h"
#include "../lay_expr.h"
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace x3 = boost::spirit::x3;

BOOST_FUSION_ADAPT_STRUCT(tess::lay_params,
    tiles, edge_mappings
)

namespace tess {
    namespace parser {
        template<typename T>
        auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

        x3::rule<class obj_ref_pair_, std::tuple<expr_ptr, expr_ptr> > obj_ref_pair = "obj_ref_pair";
        x3::rule<class full_lay_expr_aux_, lay_params> const full_lay_expr_aux = "full_lay_expr_aux";
        x3::rule<class full_lay_expr_, expr_ptr> const full_lay_expr = "full_lay_expr";
        x3::rule<class partial_lay_expr_aux_, std::vector<expr_ptr>> const partial_lay_expr_aux = "partial_lay_expr_aux";
        x3::rule<class partial_lay_expr_, expr_ptr> const partial_lay_expr = "partial_lay_expr";
        x3::rule<class lay_stmt_, expr_ptr> const lay_expr = "lay_expr";

        auto const expr = expression_();
        auto const obj_ref_pair_def = expr >> "<->" >> expr;
        auto const full_lay_expr_aux_def = kw_lit<kw::lay>() >> (expr % x3::lit(",")) >> kw_lit<kw::such_that>() >> (obj_ref_pair % x3::lit(','));
        auto const full_lay_expr_def = full_lay_expr_aux[make_<tess::lay_expr>];
        auto const partial_lay_expr_aux_def = kw_lit<kw::lay>() >> (expr % x3::lit(","));
        auto const partial_lay_expr_def = partial_lay_expr_aux[make_<tess::lay_expr>];
        auto const lay_expr_def = full_lay_expr | partial_lay_expr;

        BOOST_SPIRIT_DEFINE(
            obj_ref_pair,
            full_lay_expr_aux,
            full_lay_expr,
            partial_lay_expr_aux,
            partial_lay_expr,
            lay_expr
        );
    }
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::lay_expr_::parse_aux(const text_range& input) const
{
    tess::expr_ptr output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tess::parser::lay_expr, x3::space, output);
    }
    catch (...) {
    }

    if (success)
        return { output, iter };
    else
        return { tess::expr_ptr(), iter };
}
