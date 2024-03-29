#pragma once

#include "util.h"
#include "../with_expr.h"
#include "../expression.h"
#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>
#include <optional>

namespace x3 = boost::spirit::x3;

namespace tess {

	namespace parser {

		struct trailing_with_ : public tess_parser<trailing_with_, field_definitions> {
			std::tuple<field_definitions, std::string::const_iterator> parse_aux(const text_range& input) const;
		};

		struct with_expr_ : public tess_expr<with_expr_> {
			std::tuple<tess::expr_ptr, std::string::const_iterator> parse_aux(const text_range& input) const;
		};
	}
}