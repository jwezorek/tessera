#include "config.h"
#include "parser.h"
#include "expr.h"
#include "../expression.h"
#include "tessera/error.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {
		auto get() {
			auto test = x3::string("foo") > x3::lit(':') >> expr_parser();
			return test;
		}
    }

	tess::error make_generic_error(const std::string& input, std::string::const_iterator iter) {
		int line_number = 1;
		for (auto i = input.cbegin(); i <= iter; ++i)
			if (*i == '\n')
				++line_number;
		return { "unknown parsing error.", line_number };
	}

}

std::variant<tess::tessera_script, tess::error> tess::parser::parse(const std::string& input)
{
    std::tuple<std::string, std::shared_ptr<tess::expression>> expr;

	auto iter = input.begin();
	auto end = input.end();

    auto result = x3::phrase_parse(iter, end,
			tess::parser::get(),
		SKIPPER_DEF, 
		expr
	);

	if (!result || iter != input.end()) {
		return make_generic_error(input, iter);
	} else {
		return tess::tessera_script(std::get<1>(expr));
	}
}
