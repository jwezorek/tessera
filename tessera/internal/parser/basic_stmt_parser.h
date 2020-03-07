#pragma once

#include "../statement.h"
#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {
		
		struct lay_stmt_ : x3::parser<lay_stmt_> {
			private:
				static std::tuple<tess::stmt_ptr, std::string::const_iterator> parse_lay_stmt(const text_range& input);
			public:
				using attribute_type = tess::stmt_ptr;

				template<typename Iterator, typename Context, typename RContext, typename Attribute>
				bool parse(Iterator& first, Iterator const& last, Context const& context,
					RContext const& rcontext, Attribute& attr) const
				{
					auto [output, iter] = parse_lay_stmt(text_range(first, last));
					first = iter;
					attr = output;
					return (output != nullptr);
				};
		};

		struct let_stmt_ : x3::parser<let_stmt_> {
		private:
			static std::tuple<tess::stmt_ptr, std::string::const_iterator> parse_let_stmt(const text_range& input);
		public:
			using attribute_type = tess::stmt_ptr;

			template<typename Iterator, typename Context, typename RContext, typename Attribute>
			bool parse(Iterator& first, Iterator const& last, Context const& context,
				RContext const& rcontext, Attribute& attr) const
			{
				auto [output, iter] = parse_let_stmt(text_range(first, last));
				first = iter;
				attr = output;
				return (output != nullptr);
			};
		};
	}
}