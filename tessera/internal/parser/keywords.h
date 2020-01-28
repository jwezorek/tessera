#pragma once

#include <boost/spirit/home/x3.hpp>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {
        enum class kw {
            if_,
            else_,
            lay,
            tile,
            vertex,
            edge,
            angle,
            class_,
            patch,
            such_that,
            tableau,
            where,
            length,
            pi,
            sqrt,
            sin,
            cos,
            tan,
            phi,
            nil,
            none
        };

        kw& operator++(kw& val);
        const std::string& keyword(kw tok);

        template<kw e>
        auto kw_() {
            return x3::string(keyword(e));
        }

        struct indentifier_str_ : x3::parser<indentifier_str_> {

            using attribute_type = std::string;

            template<typename Iterator, typename Context, typename RContext, typename Attribute>
            bool parse(Iterator& first, Iterator const& last, Context const& context,
                RContext const& rcontext, Attribute& attr) const
            {
                x3::skip_over(first, last, context);
                if (first == last)
                    return false;

                auto start_of_identifier = first;
                auto i = first+1;
                if (i == last || (*i != '_' && !isalpha(*i)))
                    return false;

                while (++i != last) 
                    if (! isalnum(*i))
                        break;
                
                std::string ident_str(start_of_identifier, i);
                for (auto tok = kw::if_; tok != kw::none; ++tok) {
                    if (ident_str == keyword(tok))
                        return false;
                }

                first = i;
                attr = ident_str;
                return true;
            };
        };

    }
}