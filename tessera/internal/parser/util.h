#pragma once

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

    }
}