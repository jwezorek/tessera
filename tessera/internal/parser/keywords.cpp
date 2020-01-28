#include "keywords.h"
#include <unordered_map>

tess::parser::kw& tess::parser::operator++(tess::parser::kw& val)
{
    val = static_cast<tess::parser::kw>(static_cast<int>(val) + 1);
    return val;
}

const std::string& tess::parser::keyword(tess::parser::kw tok)
{
    using namespace tess::parser;
    static std::unordered_map<kw, std::string> keyword_tbl = {
            {kw::if_,       "if"},
            {kw::else_,     "else"},
            {kw::lay,       "lay"},
            {kw::tile,      "tile"},
            {kw::vertex,    "vertex"},
            {kw::edge,      "edge"},
            {kw::angle,     "angle"},
            {kw::class_,    "class"},
            {kw::patch,     "patch"},
            {kw::such_that, "such_that"},
            {kw::tableau,   "tableau"},
            {kw::where,     "where"},
            {kw::length,    "length"},
            {kw::pi,        "pi"},
            {kw::sqrt,      "sqrt"},
            {kw::sin,       "sin"},
            {kw::cos,       "cos"},
            {kw::tan,       "tan"},
            {kw::phi,       "phi"},
            {kw::nil,       "nil"}
    };
    return keyword_tbl.at(tok);
}
