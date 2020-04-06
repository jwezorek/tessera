#include "execution_ctxt.h"
#include "script_impl.h"

tess::execution_ctxt::execution_ctxt(const tessera_script& script) :
	script_(script)
{
}

bool tess::execution_ctxt::is_functional(const std::string& func) const
{
	auto maybe_functional = script_.impl_->get_functional(func);
	return maybe_functional.has_value();
}

tess::expr_value tess::execution_ctxt::call(const std::string& func, const std::vector<tess::expr_value>& args) const
{
	auto maybe_functional = script_.impl_->get_functional(func);
	if ( !maybe_functional.has_value() )
		return expr_value{ error("unknown tile or patch: " + func) };
	
	auto visitor = [&](const auto& func)->expr_value {
		auto params = func.params();
		if (params.size() != args.size())
			return expr_value{ "invalid tile/patch call" };

		auto ctxt = get_global_scope();
		tess::scope scope(ctxt, scope_frame(params, args) );
		auto output = func.eval(ctxt);

		return output;
	};

	return std::visit(visitor, maybe_functional.value());
}

tess::expr_value tess::execution_ctxt::eval(const std::string& var) const
{
	for (auto i = scope_stack_.rbegin(); i != scope_stack_.rend(); ++i) {
		auto maybe_value = i->get(var);
		if (maybe_value.has_value())
			return maybe_value.value();
	}
	auto maybe_global = script_.impl_->get_global(var);
	if (maybe_global.has_value())
		return maybe_global.value();

	// it might be a tile or patch called with an empty argument list.
	if (is_functional(var))
		return call(var, {});
	else
		return tess::expr_value{ error("Unknown variable: " + var) };
}

tess::expr_value tess::execution_ctxt::get_placeholder(int ph) const
{
	for (auto i = scope_stack_.rbegin(); i != scope_stack_.rend(); ++i) {
		auto maybe_value = i->get(ph);
		if (maybe_value.has_value())
			return maybe_value.value();
	}
	return tess::expr_value{ error("Unknown placeholder: $" + ph) };
}

void tess::execution_ctxt::push_scope(scope_frame&& scope)
{
	scope_stack_.push_back(std::move(scope));
}

void tess::execution_ctxt::pop_scope()
{
	scope_stack_.pop_back();
}

tess::execution_ctxt tess::execution_ctxt::get_global_scope() const
{
	return execution_ctxt(script_);
}

const tess::script_impl& tess::execution_ctxt::script() const
{
	return *(script_.impl_);
}

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

tess::scope::scope(execution_ctxt& ctxt, scope_frame&& ls) :
	ctxt_(ctxt)
{
	ctxt_.push_scope(std::move(ls));
}

tess::scope::~scope()
{
	ctxt_.pop_scope();
}
