#include "config.h"
#include "parser.h"
#include "expr.h"
#include "keywords.h"
#include "../expression.h"
#include "tessera/error.h"
#include "tile_parser.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {

		auto get() {
			auto p = expr_parser();
			return p;
		}
    }

	int get_line_number(const std::string& input, std::string::const_iterator iter) {
		int line_number = 1;
		for (auto i = input.cbegin(); i <= iter; ++i)
			if (*i == '\n')
				++line_number;
		return line_number;
	}

	tess::error make_generic_error(const std::string& input, std::string::const_iterator iter) {
		
		return { "unknown parsing error.", get_line_number(input,iter) };
	}

	tess::error exeception_to_err(const std::string& input, std::string::const_iterator iter, std::exception e) {
		return {
			e.what(),
			get_line_number(input,iter)
		};
	}

	tess::error make_err(const std::string& input, std::string::const_iterator iter, std::string msg) {
		return {
			msg,
			get_line_number(input,iter)
		};
	}
}

std::variant<tess::tessera_script, tess::error> tess::parser::parse(const std::string& input)
{
	//tess::tessera_script script;

	auto iter = input.begin();
	auto end = input.end();

	bool result = false;
	
	try {
		result = x3::phrase_parse(iter, end,
			tess::parser::get(),
			SKIPPER_DEF
		);
	}
	catch (x3::expectation_failure<std::string::const_iterator> e) {
		return make_err(input, iter, std::string("expectation failure"));
	} catch (std::exception e) {
		return exeception_to_err(input, iter, e);
	}

	if (!result || iter != input.end()) {
		return make_generic_error(input, iter);
	} else {
		return tessera_script();
	}
}
