#pragma once

#include "../expression.h"
#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {

        struct object_ref_expr_ : x3::parser<object_ref_expr_> {
			private:
				static std::tuple<tess::expr_ptr, std::string::const_iterator> parse_object_ref(const text_range& input);
			public:
				using attribute_type = tess::expr_ptr;

				template<typename Iterator, typename Context, typename RContext, typename Attribute>
				bool parse(Iterator& first, Iterator const& last, Context const& context,
					RContext const& rcontext, Attribute& attr) const
				{
					auto [output, iter] = parse_object_ref(text_range(first, last));
					first = iter;
					attr = output;
					return (output != nullptr);
				};
        };
    }
}
