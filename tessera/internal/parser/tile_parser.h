#pragma once

#include <boost/spirit/home/x3.hpp>
#include "../tile.h"
#include <memory>

namespace tess {
    namespace parser {
        namespace x3 = boost::spirit::x3;
        using tile_type = x3::rule<class tile_, x3::unused_type>;
        BOOST_SPIRIT_DECLARE(tile_type);

        const tile_type& tile_parser();
    }
}