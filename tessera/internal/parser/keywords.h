#pragma once

#include <boost/spirit/home/x3.hpp>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {

		static constexpr auto equ = "==";
		static constexpr auto not_equ = "!=";
		static constexpr auto lt = "<";
		static constexpr auto gt = ">";
		static constexpr auto le = "<=";
		static constexpr auto ge = "=>";

        enum class kw {
            if_,
            then,
            else_,
            lay,
			let,
            tile,
            vertex,
            edge,
            angle,
            class_,
            patch,
            such_that,
            tableau,
            where,
            with,
            length,
            pi,
            is,
            this_,
            sqrt,
            sin,
            cos,
            tan,
            arcsin,
            arccos,
            arctan,
            phi,
            nil,
            root_2,
			and_,
			or_,
			not_,
            regular_polygon,
            isosceles_triangle,
            polygon,
            flip,
            for_,
            in,
            none
        };

        kw& operator++(kw& val);
        const std::string& keyword(kw tok);

        template<kw e>
        auto kw_() {
            return x3::string(keyword(e));
        }

        template<kw e>
        auto kw_lit() {
            return x3::lit(keyword(e));
        }

        struct indentifier_str_ : x3::parser<indentifier_str_> {

            using attribute_type = std::string;

            template<typename Iterator, typename Context, typename RContext, typename Attribute>
            bool parse(Iterator& first, Iterator const& last, Context const& context,
                RContext const& rcontext, Attribute& attr) const
            {
                x3::skip_over(first, last, context);

                if (first == last || (*first != '_' && !isalpha(*first)))
                    return false;

                auto i = first;
                while (++i != last) 
                    if ((! isalnum(*i)) && *i != '_')
                        break;
                
                std::string ident_str(first, i);
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