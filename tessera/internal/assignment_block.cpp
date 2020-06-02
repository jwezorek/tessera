#include "assignment_block.h"
#include "lambda.h"
#include "ops.h"
#include <sstream>

namespace {

	void add_self_reference(tess::evaluation_context& original_ctxt, const std::string& var, tess::expr_value val)
	{
		if (std::holds_alternative<tess::lambda>(val)) {
			auto& lambda = std::get<tess::lambda>(val);
			lambda.insert_field(var, tess::expr_value{lambda});
		}
	}

	int num_vars_in_assignment(tess::var_assignment va) {
		return static_cast<int>(std::get<0>(va).size() );
	}

	void compile_multi_assignment(tess::stack_machine::stack& stack, tess::var_assignment va) {
		const auto& [vars, val] = va;
		stack.push(std::make_shared<tess::assign_op>(static_cast<int>(vars.size())));

		std::vector<tess::stack_machine::item> identifiers(vars.size());
		std::transform(vars.begin(), vars.end(), identifiers.begin(),
			[](const auto& var)->tess::stack_machine::item {
				return { tess::stack_machine::identifier( var ) };
			}
		);

		stack.push(identifiers);
		val->compile(stack);
	}
	
	void compile_assignment(tess::stack_machine::stack& stack, tess::var_assignment va) {
		const auto& [vars, val] = va;
		stack.push(std::make_shared<tess::assign_op>(1));
		stack.push(tess::stack_machine::identifier(vars[0]));
		val->compile(stack);
	}
}

tess::assignment_block::assignment_block(const std::vector<var_assignment>& assignments) :
	impl_(std::make_shared<std::vector<var_assignment>>( assignments))
{
}

void set_one_var(tess::evaluation_context& ctxt, const std::string& var, tess::expr_value val)
{
	add_self_reference(ctxt, var, val);
	ctxt.peek().set(var, val);
}

tess::scope_frame tess::assignment_block::eval(evaluation_context& ctxt) const
{
	ctxt.push_scope();

	for (const auto [vars, expr] : *impl_) {
		auto val = expr->eval(ctxt);
		if (vars.size() == 1) {
			set_one_var(ctxt, vars[0], val);
		} else {
			if (vars.size() != val.get_ary_count())
				throw tess::error("multi-assignment count mismatch");
			for (int i = 0; i < val.get_ary_count(); i++)
				set_one_var(ctxt, vars[i], val.get_ary_item(i));
		}
	}

	return ctxt.pop_scope();
}

void tess::assignment_block::compile(stack_machine::stack& stack) const
{
	for (auto i = impl_->rbegin(); i != impl_->rend(); ++i) {
		const auto& assignment = *i;
		int num_vars = num_vars_in_assignment(assignment);
		if (num_vars > 1)
			compile_multi_assignment(stack, assignment);
		else
			compile_assignment(stack, assignment);
	}
}

std::string tess::assignment_block::to_string() const
{
	std::stringstream contents;
	for (const auto& vars_val : *impl_) {
		const auto& [vars, val] = vars_val;
		contents << "(( ";
		for (const auto& var : vars)
			contents << var << " ";
		contents << ") " << val->to_string() << ")";
	}

	return "( let (" + contents.str() + ") )";
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
	std::vector<std::string> variables;
	for(const auto& var_assgn : *impl_)
		for (const auto& var : std::get<0>(var_assgn))
			variables.push_back(var);
	return variables;
}

std::vector<tess::expr_ptr> tess::assignment_block::get_values() const
{
	std::vector<expr_ptr> values(impl_->size());
	std::transform(impl_->begin(), impl_->end(), values.begin(),
		[](const auto& var_val) -> expr_ptr {
			return std::get<1>(var_val);
		}
	);
	return values;
}

bool tess::assignment_block::empty() const
{
	return !impl_ || impl_->empty();
}

int tess::assignment_block::num_vars() const
{
	// TODO: use std::accumulate
	int count = 0;
	for (const auto& [vars, val] : *impl_) 
		count += static_cast<int>(vars.size());
	return count;
}

tess::where_expr::where_expr(const assignment_block& assignments, expr_ptr body) :
	assignments_(assignments), body_(body)
{
}

tess::expr_value tess::where_expr::eval(evaluation_context& ctxt) const
{
	auto frame = assignments_.eval(ctxt);
	lex_scope sc(ctxt, frame);

	auto result = body_->eval(ctxt);

	if (result.is_valid() && result.is_object_like()) {
		for (const auto& [var, val] : frame)
			result.insert_field(var, val);
	}

	return result;
}
/*
void tess::where_expr::compile(stack_machine::stack& stack) const
{
	stack.push(std::make_shared<insert_fields_op>());
	stack.push(std::make_shared<pop_frame_op>());
	body_->compile(stack);
	stack.push(std::make_shared<push_frame_op>());
	stack.push(std::make_shared<dup_op>());
	assignments_.compile(stack);
}
*/

void tess::where_expr::compile(stack_machine::stack& stack) const
{
	stack.push(std::make_shared<pop_and_insert_fields_op>());
	body_->compile(stack);
	assignments_.compile(stack);
	stack.push(std::make_shared<push_frame_op>());
}

std::string tess::where_expr::to_string() const
{
	return "( where " + assignments_.to_string() + " " + body_->to_string() + " )";
}

tess::expr_ptr tess::where_expr::simplify() const
{
	return std::make_shared<where_expr>(
		assignments_.simplify(),
		body_->simplify()
	);
}

void tess::where_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
	// return the dependencies of the body expression and rhs expressions of the where
	// assignemnets that are not satisfied by the assignments made by this where
	// expression itself.

	auto variables = assignments_.get_variables();
	std::unordered_set<std::string> var_set;
	std::copy(variables.begin(), variables.end(), std::inserter(var_set, var_set.end()));
	
	std::unordered_set<std::string> new_dependencies;
	body_->get_dependencies(new_dependencies);
	for (const auto& val : assignments_.get_values())
		val->get_dependencies(new_dependencies);

	for (const auto& new_dependency : new_dependencies)
		if (var_set.find(new_dependency) == var_set.end())
			dependencies.insert(new_dependency);
}
