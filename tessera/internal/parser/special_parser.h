#pragma once

#include "util.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {

		struct special_expr_ : public tess_expr<special_expr_> {
			std::tuple<tess::expr_ptr, std::string::const_iterator> parse_aux(const text_range& input) const;
		};

	}
}