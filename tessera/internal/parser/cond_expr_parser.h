#pragma once

#include "../expression.h"
#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {
		std::tuple<tess::expr_ptr, std::string::const_iterator> parse_cond_expr(const text_range& input);

		struct cond_expr_ : x3::parser<cond_expr_> {

			using attribute_type = tess::expr_ptr;

			template<typename Iterator, typename Context, typename RContext, typename Attribute>
			bool parse(Iterator& first, Iterator const& last, Context const& context,
				RContext const& rcontext, Attribute& attr) const
			{
				auto [output, iter] = parse_cond_expr(text_range(first, last));
				first = iter;
				attr = output;
				return (output != nullptr);
			};
		};
	}
}