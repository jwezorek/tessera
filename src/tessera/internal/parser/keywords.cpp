#include "keywords.h"
#include <unordered_map>
#include <tuple>
#include <cassert>

namespace {

    const std::vector<std::tuple<tess::parser::kw, std::string>>& keyword_list() {
        static std::vector<std::tuple<tess::parser::kw, std::string>> keywords = {
            {tess::parser::kw::if_, "if"},
            { tess::parser::kw::then,               "then" },
            { tess::parser::kw::else_,              "else" },
            { tess::parser::kw::lay,                "lay" },
            { tess::parser::kw::func,               "func" },
            { tess::parser::kw::edges,              "edges" },
            { tess::parser::kw::class_,             "class" },
            { tess::parser::kw::such_that,          "such_that" },
            { tess::parser::kw::tableau,            "tableau" },
            { tess::parser::kw::where,              "where" },
            { tess::parser::kw::with,               "with" },
            { tess::parser::kw::length,             "length" },
            { tess::parser::kw::sqrt,               "sqrt" },
            { tess::parser::kw::sin,                "sin" },
            { tess::parser::kw::cos,                "cos" },
            { tess::parser::kw::tan,                "tan" },
            { tess::parser::kw::arcsin,             "arcsin" },
            { tess::parser::kw::arccos,             "arccos" },
            { tess::parser::kw::arctan,             "arctan" },
            { tess::parser::kw::pi,                 "pi" },
            { tess::parser::kw::is,                 "is" },
            { tess::parser::kw::this_,              "this" },
            { tess::parser::kw::phi,                "phi" },
            { tess::parser::kw::root_2,             "root_2" },
            { tess::parser::kw::and_,		       "and" },
            { tess::parser::kw::or_,		           "or" },
            { tess::parser::kw::not_,		       "not" },
            { tess::parser::kw::nil,                "nil" },
            { tess::parser::kw::let,                "let" },
            { tess::parser::kw::regular_polygon,    "regular_polygon" },
            { tess::parser::kw::isosceles_triangle, "isosceles_triangle" },
            { tess::parser::kw::isosceles_trapezoid,"isosceles_trapezoid" },
            { tess::parser::kw::rhombus,            "rhombus" },
            { tess::parser::kw::polygon,            "polygon" },
            { tess::parser::kw::flip,               "flip" },
            { tess::parser::kw::for_,               "for" },
            { tess::parser::kw::in,                 "in" },
            { tess::parser::kw::on,                 "on" },
            { tess::parser::kw::join,               "join" },
            { tess::parser::kw::true_,              "true" },
            { tess::parser::kw::false_,             "false" },
            { tess::parser::kw::triangle_by_sides,  "triangle_by_sides" },
            { tess::parser::kw::map,                "map" },
            { tess::parser::kw::mapping,            "mapping" }
        };
        return keywords;
    }
}

#define assertm(exp, msg) assert(((void)msg, exp))

tess::parser::kw& tess::parser::operator++(tess::parser::kw& val)
{
    val = static_cast<tess::parser::kw>(static_cast<int>(val) + 1);
    return val;
}

const std::string& tess::parser::keyword(tess::parser::kw tok)
{
    static std::unordered_map<tess::parser::kw, std::string> keyword_tbl;
    if (keyword_tbl.empty()) {
        const auto keywords = keyword_list();
        std::transform(keywords.begin(), keywords.end(), std::inserter(keyword_tbl, keyword_tbl.end()),
            [](const auto& tup) -> std::unordered_map<tess::parser::kw, std::string>::value_type {
                return { std::get<0>(tup), std::get<1>(tup) };
            }
        );
    }
    return keyword_tbl.at(tok);
}

tess::parser::kw tess::parser::token(std::string keyword)
{
    static std::unordered_map<std::string, tess::parser::kw> token_tbl;
    if (token_tbl.empty()) {
        const auto keywords = keyword_list();
        std::transform(keywords.begin(), keywords.end(), std::inserter(token_tbl, token_tbl.end()),
            [](const auto& tup) -> std::unordered_map<std::string, tess::parser::kw>::value_type {
                return { std::get<1>(tup), std::get<0>(tup) };
            }
        );
    }
    return token_tbl.at(keyword);
}
