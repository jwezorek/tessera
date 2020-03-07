#pragma once

#include "../statement.h"
#include "../text_range.h"
#include "exception.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <vector>
#include <variant>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {
        std::variant<stmts, exception> parse_statements(const text_range& input);

        std::tuple<stmt_ptr, std::string::const_iterator> parse_statement_aux(const text_range& input);
        struct statement_ : x3::parser<statement_> {

            using attribute_type = tess::stmt_ptr;

            template<typename Iterator, typename Context, typename RContext, typename Attribute>
            bool parse(Iterator& first, Iterator const& last, Context const& context,
                RContext const& rcontext, Attribute& attr) const
            {
                auto [output, iter] = parse_statement_aux(text_range(first, last));
                first = iter;
                attr = output;
                return (output != nullptr);
            };
        };


    }
}