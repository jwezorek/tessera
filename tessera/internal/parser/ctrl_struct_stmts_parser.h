#pragma once

#include "../statement.h"
#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {
		std::tuple<tess::stmt_ptr, std::string::const_iterator> parse_if_stmt(const text_range& input);

		struct if_stmt_ : x3::parser<if_stmt_> {

			using attribute_type = tess::expr_ptr;

			template<typename Iterator, typename Context, typename RContext, typename Attribute>
			bool parse(Iterator& first, Iterator const& last, Context const& context,
				RContext const& rcontext, Attribute& attr) const
			{
				auto [output, iter] = parse_if_stmt(text_range(first, last));
				first = iter;
				attr = output;
				return (output != nullptr);
			};
		};
	}
}
