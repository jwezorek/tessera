#include "keywords.h"
#include <unordered_map>
#include <cassert>

#define assertm(exp, msg) assert(((void)msg, exp))

tess::parser::kw& tess::parser::operator++(tess::parser::kw& val)
{
    val = static_cast<tess::parser::kw>(static_cast<int>(val) + 1);
    return val;
}

const std::string& tess::parser::keyword(tess::parser::kw tok)
{
    using namespace tess::parser;
    static std::unordered_map<kw, std::string> keyword_tbl = {
            {kw::if_,              "if"},
            {kw::then,             "then"},
            {kw::else_,            "else"},
            {kw::lay,              "lay"},
            {kw::tile,             "tile"},
            {kw::vertex,           "vertex"},
            {kw::edge,             "edge"},
            {kw::angle,            "angle"},
            {kw::class_,           "class"},
            {kw::patch,            "patch"},
            {kw::such_that,        "such_that"},
            {kw::tableau,          "tableau"},
            {kw::where,            "where"},
            {kw::length,           "length"},
            {kw::sqrt,             "sqrt"},
            {kw::sin,              "sin"},
            {kw::cos,              "cos"},
            {kw::tan,              "tan"},
            {kw::arcsin,           "arcsin"},
            {kw::arccos,           "arccos"},
            {kw::arctan,           "arctan"},
            {kw::pi,               "pi"},
            {kw::phi,              "phi"},
            {kw::root_2,           "root_2"},
			{kw::and_,		       "and"},
			{kw::or_,		       "or"},
			{kw::not_,		       "not"},
            {kw::nil,              "nil"},
			{kw::let,              "let"},
            {kw::regular_polygon,  "regular_polygon"}
    };

    assertm( (keyword_tbl.size() == static_cast<int>(kw::none) - static_cast<int>(kw::if_)), "keyword table is messed up!");
    return keyword_tbl.at(tok);
}
