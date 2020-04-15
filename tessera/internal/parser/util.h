#pragma once

#include "../expression.h"
#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser
    {
        using namespace x3;

        template <typename T>
        struct as_type {
            template <typename E>
            constexpr auto operator[](E e) const { return x3::rule<struct _, T> {} = e; }
        };

        template <typename T>
        static inline as_type<T> as;

		template<typename T, typename A>
		struct tess_parser : x3::parser<T> {
		public:

			using attribute_type = A;

			template<typename Iterator, typename Context, typename RContext, typename Attribute>
			bool parse(Iterator& first, Iterator const& last, Context const& context,
				RContext const& rcontext, Attribute& attr) const
			{
				auto [output, iter] = static_cast<const T*>(this)->parse_aux(text_range(first, last));
				first = iter;
				attr = output;
				return (output != A());
			};
		};

		template<typename T>
		using tess_expr = tess_parser<T, tess::expr_ptr>;

    }
}