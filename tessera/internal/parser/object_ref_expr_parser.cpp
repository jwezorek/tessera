#include "object_ref_expr_parser.h"
#include "../expression.h"
#include "expr_parser.h"
#include "util.h"
#include "keywords.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

namespace x3 = boost::spirit::x3;

BOOST_FUSION_ADAPT_STRUCT(tess::ary_item,
    name, index
)

BOOST_FUSION_ADAPT_STRUCT(tess::place_holder_ary_item,
    place_holder, index
)

BOOST_FUSION_ADAPT_STRUCT(tess::func_call_item,
    name, args
)

namespace tess {
    namespace parser
    {
        template<typename T>
        auto make_ = [&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

        auto const indentifier_str = indentifier_str_();
        const auto expr = expression_();
        auto const ary_item = as<tess::ary_item>[indentifier_str >> '[' >> expr >> ']'];
        auto const place_holder = x3::lexeme[x3::lit('$') > x3::uint_];
        auto const place_holder_ary_item = as<tess::place_holder_ary_item>[place_holder >> '[' >> expr >> ']'];
        auto const func_call_item = as<tess::func_call_item>[indentifier_str >> '(' >> (expr % ',') >> ')'];

        auto const object_ref_item = as<tess::object_ref_item>[func_call_item | ary_item | place_holder_ary_item | indentifier_str | place_holder];

        auto const object_ref_expr = as<tess::expr_ptr>[(object_ref_item % '.')[make_<tess::object_ref_expr>]];
    }
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::parse_object_ref(const text_range& input)
{
    tess::expr_ptr output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tess::parser::object_ref_expr, x3::space, output);
    } catch (...) {
    }

    if (success)
        return { output, iter };
    else
        return { tess::expr_ptr(), iter };
}
