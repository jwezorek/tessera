#include "../include/tessera/script.h"
#include "script_impl.h"
#include "function_def.h"
#include "lambda.h"
#include "parser/script_parser.h"
#include "parser/expr_parser.h"
#include "allocator.h"

namespace {

	std::vector<tess::expr_value> evaluate_arguments(const std::vector<std::string>& args) {
		std::vector<tess::expr_value> output(args.size());
		std::transform(args.begin(), args.end(), output.begin(),
			[](const auto& str)->tess::expr_value {
				auto expr = tess::parser::parse_expression(str);
				if (!expr)
					return { tess::error(std::string("syntax error in argument: ") + str) };
				tess::eval_context ctxt;
				return expr->eval(ctxt);
			}
		);
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

	tess::scope_frame eval_global_declarations(const tess::assignment_block& declarations) {
		tess::eval_context ctxt;
		return declarations.eval(ctxt);
	}

	tess::expr_value execute_script(const std::vector<tess::expr_value>& args, tess::eval_context& ctxt, const tess::expr_ptr& tableau) {
		auto maybe_lambda = tableau->eval(ctxt);

		if (!std::holds_alternative<tess::lambda>(maybe_lambda))
			return (std::holds_alternative<tess::error>(maybe_lambda)) ? 
				maybe_lambda : tess::expr_value{ tess::error("unknown error") };

		const auto& lambda = std::get<tess::lambda>(maybe_lambda);
		return lambda.call(args);
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

tess::result tess::script::execute(const std::vector<std::string>& arg_strings) const
{
	allocator test;
	test.test();
	std::vector<expr_value> args = evaluate_arguments(arg_strings);
	eval_context ctxt;
	scope global_scope(ctxt, eval_global_declarations(impl_->globals()));
	
	auto output = execute_script(args, ctxt, impl_->tableau());

	return extract_tiles(output);

}
