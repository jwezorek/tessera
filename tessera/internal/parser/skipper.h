#pragma once

#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>
namespace x3 = boost::spirit::x3;

namespace tess {

    namespace parser {

        struct skipper : x3::parser<skipper> {

            template<typename Iterator, typename Context, typename RContext, typename Attribute>
            bool parse(Iterator& first, Iterator const& last, Context const& context,
                RContext const& rcontext, Attribute& attr) const
            {
                auto [success, iter] = parse_aux(text_range(first, last));
                first = iter;
                return success;
            };

            std::tuple<bool, std::string::const_iterator> parse_aux(const text_range& input) const;
        };
    }

}