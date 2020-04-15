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
        const auto assignment_stmt = assigment_stmt_();
        const auto expr = expression_();
        const auto identifier = indentifier_str_();

        using tableau_specifier = std::tuple<std::vector<std::string>, expr_ptr>;
        using script_spec = std::tuple<std::vector<var_assignment>, tableau_specifier>;

        x3::rule<class tableau_, tableau_specifier> const tableau = "tableau";
        x3::rule<class script_, script_spec> const script = "script";

        auto const parameters = -(x3::lit('(') >> (identifier % x3::lit(',')) >> x3::lit(')'));
        const auto tableau_def = kw_lit<kw::tableau>() >> parameters > x3::lit('{') > expr > x3::lit('}');
        const auto script_def = *assignment_stmt > tableau;

        BOOST_SPIRIT_DEFINE(
            tableau,
            script
        );
    }
}

std::variant<tess::script_specifier, tess::parser::exception> tess::parser::parse(const text_range& input)
{
    auto whole_script = tess::text_range(input);
    tess::parser::script_spec output;
    auto iter = input.begin();
    bool success = false;

    try {
        success = x3::phrase_parse(iter, input.end(), tess::parser::script, x3::space, output);
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
        auto [globals, tableau] = output;
        auto [params, body] = tableau;
        return { script_specifier(globals, params, body) };
    } else {
        return tess::parser::exception("script", "unknown syntax error", iter);
    }
}
