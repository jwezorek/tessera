#include "../include/tessera/script.h"
#include "script_impl.h"
#include "function_def.h"
#include "lambda.h"
#include "assignment_block.h"
#include "parser/script_parser.h"
#include "parser/expr_parser.h"
#include "allocator.h"
#include "execution_state.h"
#include "object_expr.h"

namespace {

	std::vector<tess::expr_value> evaluate_arguments(tess::execution_state& state, const std::vector<std::string>& args) {
		std::vector<tess::expr_value> output(args.size());
		std::transform(args.begin(), args.end(), output.begin(),
			[&state](const auto& str)->tess::expr_value {
				auto expr = tess::parser::parse_expression(str);
				if (!expr)
					return { tess::error(std::string("syntax error in argument: ") + str) };
				auto ctxt = state.create_eval_context();
				return expr->eval(ctxt);
			}
		);
		return output;
	}

	std::variant<std::vector<tess::expr_ptr>, tess::error> parse_arguments(const std::vector<std::string>& args) {
		std::vector<tess::expr_ptr> output(args.size());
		try {
			std::transform(args.begin(), args.end(), output.begin(),
				[](const auto& str)->tess::expr_ptr {
					auto expr = tess::parser::parse_expression(str);
					if (!expr)
						throw  tess::error(std::string("syntax error in argument: ") + str);
					return expr->simplify();
				}
			);
		} catch (tess::error e) {
			return e;
		}
		return output;
	}

	tess::error make_error(tess::text_range script, tess::parser::exception e)
	{
		int line_number = (e.has_where()) ?
			tess::text_range(script.begin(), e.where()).get_line_count() :
			-1;
		return tess::error(
			e.to_string(),
			line_number
		);
	}

	tess::expr_value execute_script(const std::vector<tess::expr_value>& args, tess::execution_state& state, const tess::expr_ptr& tableau) {
		auto ctxt = state.create_eval_context();
		auto maybe_lambda = tableau->eval(ctxt);

		if (!std::holds_alternative<tess::lambda>(maybe_lambda))
			return (std::holds_alternative<tess::error>(maybe_lambda)) ? 
				maybe_lambda : tess::expr_value{ tess::error("unknown error") };

		const auto& lambda = std::get<tess::lambda>(maybe_lambda);
		return lambda.call(state, args);
	}

	tess::result extract_tiles(const tess::expr_value& output) {
		if (std::holds_alternative<tess::error>(output))
			return { std::get<tess::error>(output) };
		if (!std::holds_alternative<tess::tile_patch>(output))
			return { tess::error("tableau does not evaulate to a tile patch.") };

		const auto& tiles = std::get<tess::tile_patch>(output).tiles();
		return { tiles };
	}
}

std::variant<tess::script, tess::error> tess::script::interpret(const std::string& script)
{
    text_range source_code{ script };
    auto result = tess::parser::parse(source_code);
	
	if (std::holds_alternative<tess::script>(result)) {
		return std::get<tess::script>(result);
	} else {
		return make_error(source_code, std::get<tess::parser::exception>(result));
	}
}

const std::vector<std::string>& tess::script::parameters() const
{
	return impl_->parameters();
}


tess::result tess::script::exec(const std::vector<std::string>& arg_strings) const
{
	auto& state = impl_->state();

	std::vector<expr_value> args = evaluate_arguments(state, arg_strings);
	expr_ptr script_expr = std::make_shared<where_expr>(impl_->globals(), impl_->tableau());
	auto output = execute_script(args, state, script_expr);

	return extract_tiles(output);

}

tess::result tess::script::execute(const std::vector<std::string>& arg_strings) const
{
	auto maybe_args = parse_arguments(arg_strings);
	if (std::holds_alternative<tess::error>(maybe_args))
		return std::get<tess::error>(maybe_args);
	auto args = std::get<std::vector<expr_ptr>>(maybe_args);

	expr_ptr script_expr = std::make_shared<where_expr>(impl_->globals(), impl_->tableau());
	expr_ptr eval_script_expr = std::make_shared<func_call_expr>(script_expr, args);

	auto& state = impl_->state();
	std::string expr_str = eval_script_expr->to_string();
	eval_script_expr->compile(state.main_stack());
	std::string stack_str = state.main_stack().to_string();
	stack_machine sm;
	auto output = sm.run(state);

	return extract_tiles(output);

}


