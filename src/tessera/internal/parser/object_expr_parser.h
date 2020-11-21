#pragma once

#include "util.h"
#include "../expression.h"
#include "../object_expr.h"
#include "../text_range.h"
#include <boost/spirit/home/x3.hpp>
#include <tuple>
#include <string>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {

		struct object_expr_ : public tess_expr<object_expr_> {
			std::tuple<tess::expr_ptr, std::string::const_iterator> parse_aux(const text_range& input) const;
		};

	}
}