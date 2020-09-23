#include "lay_expr_parser.h"
#include "with_parser.h"
#include "keywords.h"
#include "expr_parser.h"
#include "../lay_expr.h"
#include "../expression.h"
#include "../where_expr.h"
#include "../with_expr.h"
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace x3 = boost::spirit::x3;

BOOST_FUSION_ADAPT_STRUCT(tess::lay_params,
    kw, tiles, edge_mappings
)

BOOST_FUSION_ADAPT_STRUCT(tess::full_lay_params,
    kw, tiles, edge_mappings, with
)

namespace tess {
    namespace parser {
        template<typename T>
        auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

        auto make_lay_params = [&](auto& ctx) { 
            lay_params p;
            p.kw = std::get<0>(_attr(ctx));
            p.tiles = std::get<1>(_attr(ctx));
            _val(ctx) = p; 
        };

        tess::assignment_block get_placeholder_assignments(const std::vector<tess::expr_ptr> vals) {
            std::vector<tess::var_assignment> assignments(vals.size());
            for (int i = 0; i < vals.size(); i++) {
                auto var = std::to_string(i + 1);
                assignments[i] = { std::vector<std::string>{var}, vals[i] };
            }
            return tess::assignment_block(assignments);
        }

        tess::expr_ptr make_lay_or_join(bool is_lay, const std::vector<expr_ptr>& args, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings, const field_definitions& with)
        {
            expr_ptr body = std::make_shared<tess::lay_expr>(edge_mappings);
            if ( ! is_lay )
                body = std::make_shared<special_function_expr>(special_func::join, body);
            if ( ! with.empty() )
                body = std::make_shared<with_expr>(with, body);
            return std::make_shared<where_expr>(get_placeholder_assignments(args), body);
        }

        auto make_lay_expr = [&](auto& ctx) { 
            lay_params lp = _attr(ctx);
            _val(ctx) = make_lay_or_join(lp.kw == keyword(kw::lay), lp.tiles, lp.edge_mappings, {});
        };

        auto make_full_lay_expr = [&](auto& ctx) {
            full_lay_params lp = _attr(ctx);
            _val(ctx) = make_lay_or_join(lp.kw == keyword(kw::lay), lp.tiles, lp.edge_mappings, lp.with);
        };

        x3::rule<class obj_ref_pair_, std::tuple<expr_ptr, expr_ptr> > obj_ref_pair = "obj_ref_pair";
        x3::rule<class basic_lay_expr_aux_, lay_params> const basic_lay_expr_aux = "basic_lay_expr_aux";
        x3::rule<class full_lay_expr_aux_, full_lay_params> const full_lay_expr_aux = "full_lay_expr_aux";
        x3::rule<class partial_lay_expr_aux_, std::tuple<std::string, std::vector<expr_ptr>>> const partial_lay_expr_aux = "partial_lay_expr_aux";
        x3::rule<class partial_lay_expr_, lay_params> const partial_lay_expr = "partial_lay_expr";
        x3::rule<class lay_stmt_, expr_ptr> const lay_expr = "lay_expr";

        auto const expr = expression_();
        auto const trailing_with = trailing_with_();
        auto const obj_ref_pair_def = expr >> "<->" >> expr;
        auto const lay_or_join = kw_<kw::lay>() | kw_<kw::join>();
        auto const basic_lay_expr_aux_def = lay_or_join >> (expr % x3::lit(",")) >> kw_lit<kw::such_that>() >> (obj_ref_pair % x3::lit(','));
        auto const full_lay_expr_aux_def = lay_or_join >> (expr % x3::lit(",")) >> kw_lit<kw::such_that>() >> (obj_ref_pair % x3::lit(',')) >> trailing_with;
        auto const partial_lay_expr_aux_def = lay_or_join >> (expr % x3::lit(","));
        auto const partial_lay_expr_def = partial_lay_expr_aux [make_lay_params];
        auto const lay_expr_def = (full_lay_expr_aux[make_full_lay_expr]) | (basic_lay_expr_aux[make_lay_expr]) | (partial_lay_expr[make_lay_expr]);

        BOOST_SPIRIT_DEFINE(
            obj_ref_pair,
            basic_lay_expr_aux,
            partial_lay_expr_aux,
            full_lay_expr_aux,
            partial_lay_expr,
            lay_expr
        );
    }
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::lay_expr_::parse_aux(const text_range& input) const
{
    tess::expr_ptr output;
    auto iter = input.begin();
    bool success = x3::phrase_parse(iter, input.end(), tess::parser::lay_expr, x3::space, output);
    if (success)
        return { output, iter };
    else
        return { tess::expr_ptr(), iter };
}
