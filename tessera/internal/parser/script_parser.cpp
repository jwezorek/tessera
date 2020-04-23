#include "../tile_def.h"
#include "../script_impl.h"
#include "script_parser.h"
#include "assignment_parser.h"
#include "expr_parser.h"
#include "keywords.h"
#include "util.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {
        const auto assignments = assignment_block_();
        const auto expr = expression_();
        const auto identifier = indentifier_str_();

        using tableau_specifier = std::tuple<std::vector<std::string>, expr_ptr>;
        using script_spec = std::tuple<assignment_block, tableau_specifier>;

        x3::rule<class tableau_, tableau_specifier> const tableau = "tableau";
        x3::rule<class script_spec_, script_spec> const script_parser = "script";

        auto const parameters = -(x3::lit('(') >> (identifier % x3::lit(',')) >> x3::lit(')'));
        const auto tableau_def = kw_lit<kw::tableau>() >> parameters > x3::lit('{') > expr > x3::lit('}');
        const auto script_parser_def = assignments > tableau;

        BOOST_SPIRIT_DEFINE(
            tableau,
            script_parser
        );

        struct script_maker : public tessera_impl {
            tess::script make(tess::parser::script_spec ss) {
                auto [globals, tab_spec] = ss;
                auto [tab_params, tab_body] = tab_spec;
                return make_tess_obj<tess::script>(
                    globals,
                    patch_def(tab_params, tab_body)
                );
            }
        };
    }
}

std::variant<tess::script, tess::parser::exception> tess::parser::parse(const text_range& input)
{
    auto whole_script = tess::text_range(input);
    tess::parser::script_spec output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), script_parser, x3::space, output);
    } catch (tess::parser::exception e) {
        e.push_stack_item("script");
        if (!e.has_where())
            e.set_where(iter);
        return e;
    } catch (x3::expectation_failure<std::string::const_iterator> const& e) {
        return tess::parser::exception("script", e);
    } catch (std::exception e) {
        return tess::parser::exception("script", "unkown error");
    }
    if (success) {
        tess::parser::script_maker factory;
        return factory.make(output);
    } else {
        return tess::parser::exception("script", "unkown error");
    }
}
