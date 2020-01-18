#include "config.h"
#include "parser.h"
#include "../expression.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include "expr.h"
//#include "expr_def.h"

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {
		auto get() {
			auto test = x3::string("foo:") >> expr_parser();
			return test;
		}
    }
}

std::variant<tess::tessera_script, tess::parse_error> tess::parser::parse(const std::string& input)
{
    std::tuple<std::string, std::shared_ptr<tess::expression>> expr;
    auto result = x3::phrase_parse(input.begin(), input.end(), tess::parser::get(), tess::parser::skipper, expr);

    return tess::tessera_script(std::get<1>(expr));
}
