#include "tile_parser.h"
#include "expr_parser.h"
#include "assignment_parser.h"
#include "keywords.h"
#include "util.h"
#include <tuple>
#include <optional>
#include <unordered_map>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <variant>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {
        struct tile_def_fields {
            std::vector<std::string> parameters;
            expr_ptr body;
            std::optional<assignment_block> maybe_where;
        };
    }
}

BOOST_FUSION_ADAPT_STRUCT(tess::parser::tile_def_fields,
    parameters, body, maybe_where
)

namespace tess {
    namespace parser {
        x3::rule<class tile_definition__, tile_def_fields > tile_definition = "tile_definition";

        const auto expr = expression_();
        const auto identifier_str = indentifier_str_();
        const auto trailing_where = trailing_where_();
        auto const parameters = -(x3::lit('(') >> (identifier_str % x3::lit(',')) >> x3::lit(')'));
        auto const tile_definition_def = kw_lit<kw::tile>() >> parameters >> x3::lit('{') > expr > trailing_where;

        BOOST_SPIRIT_DEFINE(
            tile_definition
        );
    }
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::tile_def_::parse_aux(const text_range& input) const
{
    tess::parser::tile_def_fields output;
    auto iter = input.begin();
    bool success = x3::phrase_parse(iter, input.end(), tess::parser::tile_definition, x3::space, output);
    if (success) {
        auto [params, body, maybe_where_clause] = output;

        expr_ptr tile_def = (maybe_where_clause.has_value() && !maybe_where_clause.value().empty()) ?
            std::make_shared<function_def>(
                params,
                std::make_shared<where_expr>(
                    maybe_where_clause.value(),
                    body
                    )
                ) :
            std::make_shared<function_def>(params, body);
        return  { tile_def, iter };
    }
    return { tess::expr_ptr(), iter };
}