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

        constexpr auto kw_if = "if";
        constexpr auto kw_else = "else";
        constexpr auto kw_lay = "lay";
        constexpr auto kw_tile = "tile";
        constexpr auto kw_vertex = "vertex";
        constexpr auto kw_edge = "edge";
        constexpr auto kw_angle = "angle";
        constexpr auto kw_class = "class";
        constexpr auto kw_patch = "patch";
        constexpr auto kw_such_that = "such_that";
        constexpr auto kw_tableau = "tableau";
        constexpr auto kw_where = "where";
        constexpr auto kw_length = "length";
        constexpr auto kw_pi = "pi";
        constexpr auto kw_sqrt = "sqrt";

    }
}