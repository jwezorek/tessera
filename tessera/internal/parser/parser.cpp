#include "tessera/tesserascript.h"
#include "../tile.h"
#include "../tableau.h"
#include "../patch.h"
#include "parser.h"
#include "exception.h"
#include "util.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <optional>
#include <variant>
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
        auto make_text_range = [](auto& ctx) {
            x3::_val(ctx) = inp_range{ x3::_attr(ctx).begin(), x3::_attr(ctx).end() };
        };
        x3::rule<class code_block_, inp_range> const code_block = "code_block";
        auto const code_block_def = x3::raw[x3::lexeme[basic_code_block | x3::char_('{') >> *(*non_brace >> code_block) >> *non_brace >> x3::char_('}')]][make_text_range];

        auto make_script = [](auto& ctx) {
            std::vector<std::variant<script_component_specifier, tab_spec>> sections = x3::_attr(ctx);
            std::optional<tab_spec> tab;
            std::vector<script_component_specifier> tiles_and_patches;
            for (const auto& section : sections) {
                if (std::holds_alternative<tab_spec>(section)) {
                    if (tab.has_value())
                        throw parser::exception("multiple tableau sections");
                    tab = std::get<tab_spec>(section);
                } else {
                    tiles_and_patches.emplace_back(std::get<script_component_specifier>(section));
                }
            }

            if (! tab.has_value())
                throw parser::exception("no tableau section");

            x3::_val(ctx) = tessera_script( tiles_and_patches, tab.value());
        };

        
        auto const parameters = as<std::vector<std::string>>[-(x3::lit('(') >> (indentifier_str% x3::lit(',')) >> x3::lit(')'))];
        auto const toplevel_script_entity = as<tess::script_component_specifier>[(x3::string(kw_tile) | x3::string(kw_patch)) >> indentifier_str >> parameters >> code_block];
        auto const tableau_entity = as<tab_spec>[x3::string(kw_tableau) >> code_block];
        auto const script_sections = as<std::vector<std::variant<script_component_specifier, tab_spec>>>[*(toplevel_script_entity | tableau_entity)];

        x3::rule<class tessera_script_parser_, tessera_script> const tessera_script_parser = "tessera_script_parser";
        auto const tessera_script_parser_def = script_sections[make_script];

        BOOST_SPIRIT_DEFINE(
            code_block,
            tessera_script_parser
        );
    }
}

tess::error make_error(tess::text_range script, tess::parser::exception e)
{
	int line_number = (e.has_where()) ?
		tess::text_range(script.begin(), e.where()).get_line_count() :
		-1;
	return tess::error(
		e.what(),
		line_number
	);
}

tess::error make_error(const std::string& msg, tess::text_range script, tess::text_range r)
{
	if (script.end() == r.end())
		return tess::error(msg, -1);
	return tess::error(msg, r.get_line_count());
}

std::variant<tess::tessera_script, tess::error> tess::parser::parse(const tess::text_range& input)
{
	auto whole_script = tess::text_range(input);
    tessera_script output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tessera_script_parser, x3::space, output);
    } catch (tess::parser::exception e) {
		return make_error(whole_script, e);
	} catch (...) {

	}

    if (success && iter == input.end())
        return output; 
    else
        return make_error("unknown syntax error", whole_script, tess::text_range(input.begin(), iter));
}
