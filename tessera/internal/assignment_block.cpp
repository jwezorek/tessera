#include "assignment_block.h"
#include "lambda.h"

namespace {

	void add_self_reference(tess::eval_context& original_ctxt, const std::string& var, tess::expr_value val)
	{
		if (std::holds_alternative<tess::lambda>(val)) {
			auto& lambda = std::get<tess::lambda>(val);
			lambda.add_to_closure(var, tess::expr_value{ lambda.get_ref() });
		}
	}

}

tess::assignment_block::assignment_block(const std::vector<var_assignment>& assignments) :
	impl_(std::make_shared<std::vector<var_assignment>>( assignments))
{
}

tess::scope_frame tess::assignment_block::eval(eval_context& original_ctxt)
{
	eval_context ctxt = original_ctxt;
	ctxt.push_scope();

	for (const auto [var, expr] : *impl_) {
		auto val = expr->eval(ctxt);
		add_self_reference(ctxt, var, val);
		ctxt.peek().set(var, val);
	}

	return ctxt.pop_scope();
}
