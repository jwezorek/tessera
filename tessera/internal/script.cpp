#include "tessera/script.h"
#include "tessera/tile_patch.h"
#include "tile_patch_impl.h"
#include "script_impl.h"
#include "function_def.h"
#include "lambda.h"
#include "where_expr.h"
#include "parser/script_parser.h"
#include "parser/expr_parser.h"
#include "allocator.h"
#include "execution_state.h"
#include "object_expr.h"

namespace {

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


	tess::result extract_tiles(const tess::expr_value& output) {

		if (std::holds_alternative<tess::tile_ptr>(output))
			return std::vector<tess::tile>{ tess::make_tess_obj<tess::tile>(std::get<tess::tile_ptr>(output)) };
		if (!std::holds_alternative<tess::patch_ptr>(output))
			return { tess::error("tableau does not evaulate to a tile patch.") };

		const auto& tile_impls = std::get<tess::patch_ptr>(output)->tiles();
		std::vector<tess::tile> tiles( tile_impls.size() );
		std::transform(tile_impls.begin(), tile_impls.end(), tiles.begin(),
			[](tess::tile_ptr i) {
				return tess::make_tess_obj<tess::tile>(i);
			}
		);

		return { tiles };
	}
}

std::variant<tess::script, tess::error> tess::script::parse(const std::string& script)
{
	text_range source_code{ script };
	return tess::parser::parse(source_code);
}

const std::vector<std::string>& tess::script::parameters() const
{
	return impl_->parameters();
}

tess::result tess::script::execute(const std::vector<std::string>& arg_strings) const
{
	try {
		auto maybe_args = parse_arguments(arg_strings);
		if (std::holds_alternative<tess::error>(maybe_args))
			return std::get<tess::error>(maybe_args);
		auto args = std::get<std::vector<expr_ptr>>(maybe_args);

		expr_ptr eval_script_expr = std::make_shared<where_expr>(
			impl_->globals(),
			std::make_shared<func_call_expr>(impl_->tableau(), args)
			);

		auto& state = impl_->state();
		std::string expr_str = eval_script_expr->to_string();
		eval_script_expr->compile(state.main_stack());
		std::string stack_str = state.main_stack().to_formatted_string();
		stack_machine::machine sm;
		auto output = sm.run(state);

		return extract_tiles(output);
	} catch (tess::error e) {
		return e;
	} catch(...) {
		return tess::error("Unknown error");
	}
}

tess::script::script(std::shared_ptr<impl_type> impl) : impl_(impl)
{
}


