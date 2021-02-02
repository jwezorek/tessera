#include "lay_expr_parser.h"
#include "with_parser.h"
#include "keywords.h"
#include "expr_parser.h"
#include "../lay_expr.h"
#include "../expression.h"
#include "../where_expr.h"
#include "../with_expr.h"
#include "../object_expr.h"
#include "../cluster_expr.h"
#include "skipper.h"
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace x3 = boost::spirit::x3;

struct layee_param {
    tess::expr_ptr layee;
    std::string alias;
};

struct lay_params {
    std::string kw;
    std::vector<layee_param> tiles;
    std::vector<std::tuple<tess::expr_ptr, tess::expr_ptr>> edge_mappings;
};

struct full_lay_params {
    std::string kw;
    std::vector<layee_param> tiles;
    std::vector<std::tuple<tess::expr_ptr, tess::expr_ptr>> edge_mappings;
    tess::field_definitions with;
};

struct map_lay_params {
    std::string kw;
    tess::expr_ptr tiles;
    tess::expr_ptr mapping;
};

struct full_map_lay_params {
    std::string kw;
    tess::expr_ptr tiles;
    tess::expr_ptr mapping;
    tess::field_definitions with;
};

BOOST_FUSION_ADAPT_STRUCT(layee_param,
    layee, alias
)

BOOST_FUSION_ADAPT_STRUCT(lay_params,
    kw, tiles, edge_mappings
)

BOOST_FUSION_ADAPT_STRUCT(full_lay_params,
    kw, tiles, edge_mappings, with
)

BOOST_FUSION_ADAPT_STRUCT(map_lay_params, 
    kw, tiles, mapping
)

BOOST_FUSION_ADAPT_STRUCT(full_map_lay_params,
    kw, tiles, mapping, with
)

namespace tess {
    namespace parser {
        template<typename T>
        auto make_ = [](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

        auto make_lay_params = [](auto& ctx) { 
            lay_params p;
            p.kw = std::get<0>(_attr(ctx));
            p.tiles = std::get<1>(_attr(ctx));
            _val(ctx) = p; 
        };

        tess::assignment_block get_placeholder_assignments(const std::vector<layee_param> vals) {
            std::vector<tess::var_assignment> assignments;
            for (int i = 0; i < vals.size(); i++) {
                auto var = std::to_string(i + 1);
                const auto& layee = vals[i];
                auto val = std::make_shared<tess::clone_expr>(layee.layee);
                assignments.emplace_back( std::vector<std::string>{var}, val );

                if (!layee.alias.empty()) {
                    auto alias_val = std::make_shared<tess::var_expr>(var);
                    assignments.emplace_back(std::vector<std::string>{layee.alias}, alias_val);
                }
            }
            return tess::assignment_block(assignments);
        }

        tess::expr_ptr make_lay_or_join(bool is_lay, const std::vector<layee_param>& args, expr_ptr lay_body, const field_definitions& with)
        {
            if ( ! is_lay )
                lay_body = std::make_shared<special_function_expr>(parser::kw::join, lay_body);
            if ( ! with.empty() )
                lay_body = std::make_shared<with_expr>(with, lay_body);
            return std::make_shared<where_expr>(get_placeholder_assignments(args), lay_body);
        }

        auto make_lay_expr = [](auto& ctx) { 
            lay_params lp = _attr(ctx);
            _val(ctx) = make_lay_or_join(lp.kw == keyword(kw::lay), lp.tiles, std::make_shared<tess::lay_expr>(lp.edge_mappings), {});
        };

        auto make_full_lay_expr = [](auto& ctx) {
            full_lay_params lp = _attr(ctx);
            _val(ctx) = make_lay_or_join(lp.kw == keyword(kw::lay), lp.tiles, std::make_shared<tess::lay_expr>(lp.edge_mappings), lp.with);
        };

        auto make_map_lay = [](auto& ctx) {
            map_lay_params lp = _attr(ctx);

            auto partitioned_layees = std::make_shared<tess::partition_expr>(lp.tiles);
            auto apply_the_func = std::make_shared<tess::map_expr>(lp.mapping, partitioned_layees);

            //_val(ctx) = make_lay_or_join(lp.kw == keyword(kw::lay), lp.tiles, std::make_shared<tess::lay_expr>(lp.edge_mappings), {});
        };
            
        auto make_full_map_lay = [](auto& ctx) {
            full_map_lay_params lp = _attr(ctx);
            //_val(ctx) = make_lay_or_join(lp.kw == keyword(kw::lay), lp.tiles, std::make_shared<tess::lay_expr>(lp.edge_mappings), {});
        };

        x3::rule<class layee_param_, layee_param> layee = "layee_param";
        x3::rule<class obj_ref_pair_, std::tuple<expr_ptr, expr_ptr> > obj_ref_pair = "obj_ref_pair";
        x3::rule<class basic_lay_expr_aux_, lay_params> const basic_lay_expr_aux = "basic_lay_expr_aux";
        x3::rule<class full_lay_expr_aux_, full_lay_params> const full_lay_expr_aux = "full_lay_expr_aux";
        x3::rule<class partial_lay_expr_aux_, std::tuple<std::string, std::vector<layee_param>>> const partial_lay_expr_aux = "partial_lay_expr_aux";
        x3::rule<class partial_lay_expr_, lay_params> const partial_lay_expr = "partial_lay_expr";
        x3::rule<class lay_stmt_, expr_ptr> const lay_expr = "lay_expr";

        x3::rule<class map_lay_aux_, map_lay_params> const map_lay_aux = "map_lay_aux";
        x3::rule<class full_map_lay_aux_, full_map_lay_params> const full_map_lay_aux = "full_map_lay_aux";

        auto const expr = expression_();
        const auto identifier = indentifier_str_();
        auto const trailing_with = trailing_with_();
        auto const obj_ref_pair_def = expr >> "<->" >> expr;
        auto const lay_or_join = kw_<kw::lay>() | kw_<kw::join>();
        auto const layee_def = (expr >> x3::lit("as") > identifier) | (expr >> x3::string(""));
        auto const basic_lay_expr_aux_def = lay_or_join >> (layee_def % x3::lit(",")) >> kw_lit<kw::such_that>() >> (obj_ref_pair % x3::lit(','));
        auto const full_lay_expr_aux_def = lay_or_join >> (layee_def % x3::lit(",")) >> kw_lit<kw::such_that>() >> (obj_ref_pair % x3::lit(',')) >> trailing_with;
        auto const partial_lay_expr_aux_def = lay_or_join >> (layee_def % x3::lit(","));
        auto const partial_lay_expr_def = partial_lay_expr_aux [make_lay_params];

        auto const map_lay_aux_def = lay_or_join >> expr >> kw_lit<kw::mapping>() >> expr;
        auto const full_map_lay_aux_def = lay_or_join >> expr >> kw_lit<kw::mapping>() >> expr >> trailing_with;
        
        auto const lay_expr_def = (full_lay_expr_aux[make_full_lay_expr]) | (basic_lay_expr_aux[make_lay_expr]) | (partial_lay_expr[make_lay_expr]) |
            (map_lay_aux[make_map_lay]) | (full_map_lay_aux[make_full_map_lay]);

        BOOST_SPIRIT_DEFINE(
            obj_ref_pair,
            basic_lay_expr_aux,
            partial_lay_expr_aux,
            full_lay_expr_aux,
            partial_lay_expr,
            lay_expr,
            layee,
            map_lay_aux,
            full_map_lay_aux
        );
    }
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::lay_expr_::parse_aux(const text_range& input) const
{
    tess::expr_ptr output;
    auto iter = input.begin();
    bool success = x3::phrase_parse(iter, input.end(), tess::parser::lay_expr, skipper(), output);
    if (success)
        return { output, iter };
    else
        return { tess::expr_ptr(), iter };
}
