#pragma once

#include "parser.h"
#include "../expression.h"
#include "expr.h"
#include "keywords.h"
#include "util.h"
#include "../tile.h"
#include "tile_parser.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>


namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser
    {
        using namespace x3;

        auto make_tile = [&](auto& ctx) { _val(ctx) = tess::tile("foobar"); };
        auto make_vertex_decl = [&](auto& ctx) { _val(ctx) = tess::tile_component_declaration("vertex"); };
        auto make_edge_decl = [&](auto& ctx) { _val(ctx) = tess::tile_component_declaration("edge"); };

        rule<class tile_, x3::unused_type> const tile = "tile";
        rule<class tile_component_decl_, tess::tile_component_declaration>  const tile_component_decl = "tile_component_declaration";
        rule<class vertex_decl_, tess::tile_component_declaration>  const vertex_decl = "vertex_decl";
        rule<class edge_decl_, tess::tile_component_declaration>  const edge_decl = "edge_decl";

        auto const vertex_decl_def = (lit(kw_vertex) > identifier_parser() > lit(';'))[make_vertex_decl];
        auto const edge_decl_def = (lit(kw_edge) > identifier_parser() > lit(';'))[make_edge_decl];
        auto const tile_component_decl_def = (vertex_decl | edge_decl);
        //auto const tile_def = (lit(kw_tile) > identifier_parser() > lit('{') > *tile_component_decl > lit('}'))[make_tile];
        auto const tile_def = (lit(kw_tile) > identifier_parser() > lit('{') > "stuff" > lit('}'));

        BOOST_SPIRIT_DEFINE(
           // vertex_decl,
           // edge_decl,
          //  tile_component_decl,
            tile
        );

        const tile_type& tile_parser()
        {
            return tile;
        }
    }
}
