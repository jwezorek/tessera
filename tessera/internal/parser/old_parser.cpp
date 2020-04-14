#include "tessera/old_tessera_script.h"
#include "../tile_def.h"
#include "../tile_patch_def.h"
#include "../statement.h"
#include "basic_stmt_parser.h"
#include "old_parser.h"
#include "exception.h"
#include "util.h"
#include "keywords.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <optional>
#include <variant>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {

        auto const non_brace = x3::char_ - (x3::lit('{') | x3::lit('}'));
        auto const basic_code_block = x3::char_('{') >> *non_brace >> x3::char_('}');
        auto make_text_range = [](auto& ctx) {
            x3::_val(ctx) = inp_range{ x3::_attr(ctx).begin(), x3::_attr(ctx).end() };
        };
        x3::rule<class code_block_, inp_range> const code_block = "code_block";
        auto const code_block_def = x3::raw[x3::lexeme[basic_code_block | x3::char_('{') >> *(*non_brace >> code_block) >> *non_brace >> x3::char_('}')]][make_text_range];

        auto make_script = [](auto& ctx) {
            std::vector<std::variant<script_component_specifier, tab_spec, stmt_ptr>> sections = x3::_attr(ctx);
            std::optional<tab_spec> tab;
            std::vector<script_component_specifier> tiles_and_patches;
			std::vector<std::tuple<std::string, expr_ptr>> globals_;

            for (const auto& section_or_assignment : sections) {
                if (std::holds_alternative<tab_spec>(section_or_assignment)) {
                    if (tab.has_value())
                        throw parser::exception("script", "multiple tableau sections");
                    tab = std::get<tab_spec>(section_or_assignment);
                } else if (std::holds_alternative< script_component_specifier>(section_or_assignment)){
                    tiles_and_patches.emplace_back(std::get<script_component_specifier>(section_or_assignment));
				} else if (std::holds_alternative<stmt_ptr>(section_or_assignment)) {
					auto asgn = std::static_pointer_cast<let_statement> (
						std::get<stmt_ptr>(section_or_assignment)
					);
					globals_.push_back({ asgn->lhs(), asgn->rhs() });
				}
            }

            if (! tab.has_value())
                throw parser::exception("script", "no tableau section");

            x3::_val(ctx) = tessera_script( tiles_and_patches, tab.value(), globals_);
        };

        auto const indentifier_str = indentifier_str_();
		auto const global_var = let_stmt_();
        auto const parameters = as<std::vector<std::string>>[-(x3::lit('(') >> (indentifier_str % x3::lit(',')) >> x3::lit(')'))];
        auto const toplevel_script_entity = as<tess::script_component_specifier>[(kw_<kw::tile>() | kw_<kw::patch>()) > indentifier_str >> parameters > code_block];
        auto const tableau_entity = as<tab_spec>[kw_<kw::tableau>() >> parameters >> code_block];
        auto const script_sections = as<std::vector<std::variant<script_component_specifier, tab_spec, stmt_ptr>>>[*(toplevel_script_entity | tableau_entity | global_var)];

        x3::rule<class tessera_script_parser_, tessera_script> const tessera_script_parser = "tessera_script_parser";
        auto const tessera_script_parser_def = script_sections[make_script];

        BOOST_SPIRIT_DEFINE(
            code_block,
            tessera_script_parser
        );
    }
}

std::variant<tess::tessera_script, tess::parser::exception> tess::parser::parse_old(const tess::text_range& input)
{
	auto whole_script = tess::text_range(input);
    tessera_script output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tessera_script_parser, x3::space, output);
    } catch (tess::parser::exception e) {
        e.push_stack_item("script");
        if (!e.has_where())
            e.set_where(iter);
        return e;
	} catch (x3::expectation_failure<std::string::const_iterator> const& e) {
        return tess::parser::exception("script", e);
    } catch (std::exception e) {
    }

    if (success && iter == input.end()) {
        return output;
    } else {
        return tess::parser::exception("script", "unknown syntax error", iter);
    }
}
