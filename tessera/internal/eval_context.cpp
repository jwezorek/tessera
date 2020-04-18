#include "eval_context.h"

tess::eval_context::eval_context()
{
}

tess::eval_context::eval_context(const scope_frame& frame)
{
	scope_stack_.push_back(frame);
}

tess::expr_value tess::eval_context::get(const std::string& var) const
{
	for (auto i = scope_stack_.rbegin(); i != scope_stack_.rend(); ++i) {
		auto maybe_value = i->get(var);
		if (maybe_value.has_value())
			return maybe_value.value();
	}
	return tess::expr_value{ error("Unknown variable: " + var) };
}

tess::expr_value tess::eval_context::get(int ph) const
{
	for (auto i = scope_stack_.rbegin(); i != scope_stack_.rend(); ++i) {
		auto maybe_value = i->get(ph);
		if (maybe_value.has_value())
			return maybe_value.value();
	}
	return tess::expr_value{ error("Unknown placeholder: $" + ph) };
}

tess::scope_frame& tess::eval_context::peek()
{
	return scope_stack_.back();
}

void tess::eval_context::push_scope()
{
	push_scope(scope_frame({}, {}));
}

void tess::eval_context::push_scope(scope_frame&& scope)
{
	scope_stack_.push_back(std::move(scope));
}

tess::scope_frame tess::eval_context::pop_scope()
{
	auto frame = scope_stack_.back();
	scope_stack_.pop_back();
	return frame;
}

/*----------------------------------------------------------------------------------------------------------------------*/

tess::scope_frame::scope_frame(const std::vector<std::string>& param, const std::vector<expr_value>& args)
{
	for (int i = 0; i < param.size(); ++i)
		variables_[param[i]] = args[i];
}

tess::scope_frame::scope_frame(const std::vector<expr_value>& args)
{
	for (int i = 0; i < args.size(); ++i)
		placeholders_[i+1] = args[i];
}

std::optional<tess::expr_value> tess::scope_frame::get(int ph) const
{
	auto i = placeholders_.find(ph);
	if (i != placeholders_.end())
		return i->second;
	else
		return std::nullopt;
}

std::optional<tess::expr_value> tess::scope_frame::get(std::string str) const
{
	auto i = variables_.find(str);
	if (i != variables_.end())
		return i->second;
	else
		return std::nullopt;
}

void tess::scope_frame::set(const std::string& var, expr_value val) {
	variables_[var] = val;
}

void tess::scope_frame::set(int i, expr_value val) {
	placeholders_[i] = val;
}

void tess::scope_frame::set(const::std::vector<std::string>& vars, const::std::vector<expr_value>& vals) {

	if (vars.size() != vals.size())
		throw tess::error("argument/parameter mismatch");
	int i = 0;
	for (const auto& var : vars)
		set(var, vals[i++]);
}


/*----------------------------------------------------------------------------------------------------------------------*/

tess::scope::scope(eval_context& ctxt, scope_frame&& ls) :
	ctxt_(ctxt)
{
	ctxt_.push_scope(std::move(ls));
}

tess::scope::~scope()
{
	ctxt_.pop_scope();
}
