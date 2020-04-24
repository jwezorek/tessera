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

tess::scope_frame tess::assignment_block::eval(eval_context& original_ctxt) const
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

tess::assignment_block tess::assignment_block::simplify() const
{
	std::vector<var_assignment> simplified(impl_->size());
	std::transform(impl_->begin(), impl_->end(), simplified.begin(),
		[](const auto& var_val)->var_assignment {
			auto [var, val] = var_val;
			return { var, val->simplify() };
		}
	);
	return assignment_block(simplified);
}

std::vector<std::string> tess::assignment_block::get_variables() const
{
	std::vector<std::string> variables(impl_->size());
	std::transform(impl_->begin(), impl_->end(), variables.begin(),
		[](const auto& var_val) -> std::string {
			return std::get<0>(var_val);
		}
	);
	return variables;
}

bool tess::assignment_block::empty() const
{
	return !impl_ || impl_->empty();
}

tess::where_expr::where_expr(const assignment_block& assignments, expr_ptr body) :
	assignments_(assignments), body_(body)
{
}

tess::expr_value tess::where_expr::eval(eval_context& ctxt) const
{
	auto frame = assignments_.eval(ctxt);
	auto frame_copy = frame;
	scope sc(ctxt, std::move(frame));
	auto result = body_->eval(ctxt);

	if (result.is_valid() && result.is_object_like()) {
		for (const auto& [var, val] : frame_copy.variables())
			result.insert_field(var, val);
	}

	return result;
}

tess::expr_ptr tess::where_expr::simplify() const
{
	return std::make_shared<where_expr>(
		assignments_.simplify(),
		body_->simplify()
	);
}

void tess::where_expr::get_dependencies(std::vector<std::string>& dependencies) const
{
	// return the dependencies of the body expression that are not
	// satisfied by the assignments made by this "where" expression.

	auto variables = assignments_.get_variables();
	std::unordered_set<std::string> var_set;
	std::copy(variables.begin(), variables.end(), std::inserter(var_set, var_set.end()));
	
	std::vector<std::string> new_dependencies;
	body_->get_dependencies(new_dependencies);

	for (const auto& new_dependency : new_dependencies)
		if (var_set.find(new_dependency) == var_set.end())
			dependencies.push_back(new_dependency);
}
