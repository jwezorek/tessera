#pragma once

#include "util.h"
#include "../where_expr.h"
#include "../expression.h"
#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>
#include <optional>

namespace x3 = boost::spirit::x3;

namespace tess {

	void test_trailing_where();

	namespace parser {
		struct assignment_block_ : public tess_parser<assignment_block_, assignment_block> {
			std::tuple<assignment_block, std::string::const_iterator> parse_aux(const text_range& input) const;
		};

		struct trailing_where_ : public tess_opt_parser<trailing_where_, assignment_block> {
			std::tuple<std::optional<assignment_block>, std::string::const_iterator> parse_aux(const text_range& input) const;
		};
	}
}