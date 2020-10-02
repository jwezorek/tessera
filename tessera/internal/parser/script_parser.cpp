
#include "../script_impl.h"
#include "where_parser.h"
#include "script_parser.h"
#include "expr_parser.h"
#include "keywords.h"
#include "util.h"
#include "skipper.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <sstream>

namespace x3 = boost::spirit::x3;

using x3_expect_error = x3::expectation_failure<std::string::const_iterator>;

namespace tess {
    namespace parser {

        tess::error expectation_failure_to_tess_error(const x3_expect_error& e, const text_range& script) {
            return tess::error( "syntax error", script.get_line_number(e.where()) );
        }

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

        tess::script make_script(tess::parser::script_spec ss) {
            auto [globals, tab_spec] = ss;
            auto [tab_params, tab_body] = tab_spec;
            return tess::make_tess_obj<tess::script>(
                std::make_shared<tess::script::impl_type>(
                    globals,
                    std::make_shared<function_def>(tab_params, tab_body)
                )
            );
        }
    }
}

std::variant<tess::script, tess::error> tess::parser::parse(const text_range& input)
{
    auto whole_script = tess::text_range(input);
    tess::parser::script_spec output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), script_parser, skipper(), output);
    } catch (const x3_expect_error& e) {
        return expectation_failure_to_tess_error(e, whole_script);
    } catch ( ... ) {
        return tess::error("unkown error");
    }
    if (success) {
        return make_script(output);
    } else {
        return tess::error("unkown error");
    }
}
