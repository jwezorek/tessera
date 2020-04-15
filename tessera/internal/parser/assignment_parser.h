#pragma once

#include "util.h"
#include "../expression.h"
#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {

	using var_assignment = std::tuple<std::string, tess::expr_ptr>;

	namespace parser {
		struct assigment_stmt_ : public tess_parser<assigment_stmt_, var_assignment> {
			std::tuple<var_assignment, std::string::const_iterator> parse_aux(const text_range& input) const;
		};
	}
}