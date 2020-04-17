#include "function_parser.h"
#include "../function_def.h"
#include "expr_parser.h"
#include "tile_parser.h"
#include "util.h"
#include "keywords.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {

		auto make_patch = [&](auto& ctx) { 
			auto [params, body] = _attr(ctx);
			_val(ctx) = std::make_shared<tess::function_def>(params, body); 
		};

		x3::rule<class patch_, std::tuple<std::vector<std::string>, expr_ptr>> const patch_aux = "patch_aux";
		x3::rule<class patch_, expr_ptr> const patch = "patch";

		const auto expr = expression_();
		const auto identifier = indentifier_str_();
		const auto tile = tile_def_();
		const auto parameters = -(x3::lit('(') >> (identifier % x3::lit(',')) >> x3::lit(')'));
		const auto patch_aux_def = kw_lit<kw::patch>() >> parameters >> x3::lit('{') >> expr >> x3::lit('}');
		const auto patch_def = patch_aux[make_patch];
		const auto function = patch | tile;

		BOOST_SPIRIT_DEFINE(
			patch_aux,
			patch
		);

	}
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::function_def_::parse_aux(const text_range& input) const
{
	tess::expr_ptr output;
	auto iter = input.begin();
	bool success = false;

	try {
		success = x3::phrase_parse(iter, input.end(), tess::parser::function, x3::space, output);
	}
	catch (...) {
	}

	if (success)
		return { output, iter };
	else
		return { tess::expr_ptr(), iter };
}
