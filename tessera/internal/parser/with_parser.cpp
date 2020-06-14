#include "../variant_util.h"
#include "../expression.h"
#include "../object_expr.h"
#include "with_parser.h"
#include "keywords.h"
#include "expr_parser.h"
#include "util.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {
		using op_t = std::variant< expr_ptr, std::string>;
		struct obj_ref_list_t {
			std::string head;
			std::vector<op_t> tail;
		};
	}
}

BOOST_FUSION_ADAPT_STRUCT(tess::parser::obj_ref_list_t,
	head, tail
)

namespace tess {
	namespace parser {

		x3::rule<class obj_list_, obj_ref_list_t> const obj_list = "obj_list";
		x3::rule<class op_, op_t> const op = "op";
		x3::rule<class lhs_, expr_ptr> const lhs = "lhs";
		x3::rule<class trailing_with__, tess::field_definitions> trailing_with = "trailing_with";

		expr_ptr unpack_obj_list(const obj_ref_list_t& ol);
		auto make_lhs = [&](auto& ctx) { _val(ctx) = unpack_obj_list(_attr(ctx)); };

		const auto expr = expression_();
		const auto identifier = indentifier_str_();
		const auto field = as<std::string>[identifier | kw_<kw::edge>()];

		const auto placeholder = x3::lit('$') > x3::int32;
		const auto ary_item = x3::lit('[') >> expr >> x3::lit(']');
		const auto field_item = x3::lit('.') > field;
		const auto op_def = ary_item | field_item;
		const auto obj_list_def = identifier >> *op;
		const auto lhs_def = obj_list[make_lhs];
		

		auto const field_defs = x3::lit("foo");
		auto const trailing_with_def = kw_lit<kw::where>() > x3::lit('{') > field_defs > x3::lit('}');

		BOOST_SPIRIT_DEFINE(
			trailing_with,
			op,
			obj_list,
			lhs
		)
		
	}
}

tess::expr_ptr unpack_obj_list_aux(tess::expr_ptr e, std::vector<tess::parser::op_t>& list)
{
	auto top = list.back();
	list.pop_back();
	if (std::holds_alternative<std::string>(top)) {
		std::string field = std::get<std::string>(top);
		e = std::make_shared<tess::obj_field_expr>(e, field, list.empty());
	} else {
		auto index_expr = std::get<tess::expr_ptr>(top);
		e = std::make_shared<tess::array_item_expr>(e, index_expr);
	}
	return (list.empty()) ?
		e :
		unpack_obj_list_aux(e, list);
}

tess::expr_ptr tess::parser::unpack_obj_list(const obj_ref_list_t& ol) {

	std::vector<op_t> pieces;
	const auto& head = ol.head;
	if (head != "this")
		pieces.push_back(head);
	std::copy(ol.tail.begin(), ol.tail.end(), std::back_inserter(pieces));
	std::reverse(pieces.begin(), pieces.end());
	return unpack_obj_list_aux(
		std::make_shared<tess::var_expr>("this"),
		pieces
	);
}

void tess::parser::test_with_parser()
{
	std::string test_inp = "foo.foo[a].bar";
	std::string::const_iterator iter = test_inp.begin();
	std::string::const_iterator end_iter = test_inp.end();
	tess::expr_ptr output;
	bool success = x3::phrase_parse(iter, end_iter, tess::parser::lhs, x3::space, output);
	output = output->simplify();
	auto str = output->to_string();
	int aaa;
	aaa = 5;
}

std::tuple<tess::field_definitions, std::string::const_iterator> tess::parser::trailing_with_::parse_aux(const text_range& input) const
{
	field_definitions output;
	std::string::const_iterator iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::trailing_with, x3::space, output);

	if (success)
		return { field_definitions(output), iter };
	else
		return { field_definitions(), iter };
}