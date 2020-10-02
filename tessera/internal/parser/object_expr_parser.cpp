#include "object_expr_parser.h"
#include "../expression.h"
#include "expr_parser.h"
#include "util.h"
#include "../variant_util.h"
#include "keywords.h"
#include "skipper.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace x3 = boost::spirit::x3;

namespace tess {
	namespace parser {
		using params_t = std::vector<expr_ptr>;
		using head_t = std::variant<std::string, int>;
		using op_t = std::variant< params_t, expr_ptr, std::string>;
		struct obj_list_t {
			head_t head;
			std::vector<op_t> tail;
		};
	}
}

BOOST_FUSION_ADAPT_STRUCT(tess::parser::obj_list_t,
	head, tail
)

namespace tess {
	namespace parser {

		x3::rule<class obj_list_, obj_list_t> const obj_list = "obj_list";
		x3::rule<class head_, head_t> const head = "head";
		x3::rule<class head_var_, std::string> const head_var = "head_var";
		x3::rule<class op_, op_t> const op = "op";
		x3::rule<class args_, std::vector<expr_ptr>> const args = "args";

		const auto expr = expression_();
		const auto identifier = indentifier_str_();
		const auto field = as<std::string>[identifier | kw_<kw::edges>() | kw_<kw::on>()];
		const auto placeholder = x3::lit('$') > x3::int32;
		const auto ary_item = x3::lit('[') >> expr >> x3::lit(']');
		const auto field_item = x3::lit('.') > field;

		const auto head_var_def = kw_<kw::this_>() | identifier;
		const auto head_def = head_var | placeholder;
		const auto empty_args_list = x3::lit('(') >> x3::lit(')');
		const auto args_list = x3::lit('(') >> (expr% x3::lit(',')) >> x3::lit(')');
		const auto args_def = args_list | empty_args_list;
		const auto op_def = args | ary_item | field_item;
		const auto obj_list_def = head >> *op;

		BOOST_SPIRIT_DEFINE(
			head,
			head_var,
			args,
			op,
			obj_list
		);

		template<typename T, typename...Args>
		expr_ptr make_(Args&&... args) {
			return std::static_pointer_cast<expression>(
				std::make_shared<T>( std::forward<Args>(args)... )
			);
		}

		expr_ptr expression_from_head(const head_t& head) {
			return std::visit(
				overloaded{
					[](int ph) { return make_<placeholder_expr>(ph); },
					[](const std::string& var) {return make_<var_expr>(var);  }
				},
				head
			);
		}

		bool is_field_expr( expr_ptr e) { 
			return dynamic_cast<obj_field_expr*>(e.get()) != nullptr;
		}

		expr_ptr make_method_call(expr_ptr self, std::string method, const params_t& func_params)
		{
			// this is a method call so we need to add a "this" argument.
			std::vector<expr_ptr> method_params(func_params.size() + 1);
			if (method == "on" && func_params.size() == 1) {
				return std::make_shared<on_expr>(self, func_params[0]);
			};
			throw tess::error("bad method call");
		}

		expr_ptr func_call_or_method_call_expr(expr_ptr e, const params_t& func_params) {
			if (! is_field_expr(e))
				return make_<func_call_expr>(e, func_params);

			auto field_expr = std::static_pointer_cast<obj_field_expr>(e);
			auto this_ptr = field_expr->get_object();
			auto method = field_expr->get_field();
			return make_method_call(this_ptr, method, func_params);
		}

		expr_ptr expression_from_op(expr_ptr e, const op_t& op) {
			return std::visit(
				overloaded{
					[&e](const params_t& func_params) { return func_call_or_method_call_expr(e, func_params); },
					[&e](expr_ptr ary_index) { return make_<array_item_expr>(e, ary_index); },
					[&e](const std::string& field) { return make_<obj_field_expr>(e, field, false);  }
				},
				op
			);
		}

		expr_ptr unpack_object_list(const obj_list_t& list) {
			
			expr_ptr current = expression_from_head(list.head);
			for (const auto& op : list.tail) {
				current = expression_from_op(current, op);
			}
			return current;
		}
	}
}

std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::object_expr_::parse_aux(const tess::text_range& input) const
{
	tess::parser::obj_list_t output;
	auto iter = input.begin();
	bool success = x3::phrase_parse(iter, input.end(), tess::parser::obj_list, skipper(), output);
	if (success)
		return { unpack_object_list(output) , iter };
	else
		return { tess::expr_ptr(), iter };
}

