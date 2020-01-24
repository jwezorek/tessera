#include "tessera/tesserascript.h"
#include "parser.h"
#include "util.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {
        struct keywords_t : x3::symbols<x3::unused_type> {
            keywords_t() {
                add(kw_if, x3::unused);
                add(kw_else, x3::unused);
                add(kw_lay, x3::unused);
                add(kw_tile, x3::unused);
                add(kw_vertex, x3::unused);
                add(kw_edge, x3::unused);
                add(kw_angle, x3::unused);
                add(kw_class, x3::unused);
                add(kw_patch, x3::unused);
                add(kw_such_that, x3::unused);
                add(kw_tableau, x3::unused);
                add(kw_where, x3::unused);
                add(kw_length, x3::unused);
                add(kw_pi, x3::unused);
                add(kw_sqrt, x3::unused);
            }
        } const keywords;

        auto const distinct_keyword = x3::lexeme[keywords >> !(x3::alnum | '_')];
        auto const unchecked_identifier = x3::lexeme[(x3::alpha | x3::char_('_')) >> *(x3::alnum | x3::char_('_'))];
        auto const indentifier_str = as<std::string>[unchecked_identifier - distinct_keyword];

        auto const non_brace = x3::char_ - (x3::lit('{') | x3::lit('}'));
        auto const basic_code_block = x3::char_('{') >> *non_brace >> x3::char_('}');
        x3::rule<class code_block_, std::string> const code_block = "identifier_expr";
        auto const code_block_def = x3::raw[x3::lexeme[basic_code_block | x3::char_('{') >> *(*non_brace >> code_block) >> *non_brace >> x3::char_('}')]];

        BOOST_SPIRIT_DEFINE(
            code_block
        );

        auto const non_paren = x3::char_ - (x3::lit('(') | x3::lit(')'));
        auto const parameters = x3::raw[x3::lexeme[x3::char_('(') >> *non_paren >> x3::char_(')')]];

        auto const toplevel_script_entity_kw = (x3::string(kw_tile) | x3::string(kw_patch));
        auto const toplevel_script_entity = toplevel_script_entity_kw >> indentifier_str >> parameters >> code_block;
        auto const tableau = x3::string(kw_tableau) >> code_block;

    }
}

std::variant<tess::tessera_script, tess::error> tess::parser::parse(const tess::text_range& input)
{
    tess::tessera_script script;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tess::parser::tableau, x3::space);
    }
    catch (...) {
    }

    if (success && iter == input.end())
        return script;
    else
        return input.left_range(iter).make_error("invalid expression");
}
