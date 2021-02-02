#pragma once

#include "util.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {

		tess::expr_ptr parse_expression(const std::string& str);

		struct expression_ : tess_expr<expression_> {
			std::tuple<tess::expr_ptr, std::string::const_iterator> parse_aux(const text_range& input) const;
		};
    }
}
