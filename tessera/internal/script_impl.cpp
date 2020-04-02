#include "script_impl.h"
#include "./parser/keywords.h"

void tess::script_impl::insert_tile_def(const std::string& name, std::vector<std::string> params, const text_range& source_code)
{
	tiles_[name] = std::make_shared<tile_def>(name, params, source_code);
}

void tess::script_impl::insert_patch_def(const std::string& name, std::vector<std::string> params, const text_range& source_code)
{
	patches_.emplace(
		std::piecewise_construct,
		std::make_tuple(name),
		std::make_tuple(name, params, source_code)
	);
}

void tess::script_impl::insert_tableau_def(std::vector<std::string> params, const text_range& source_code)
{
	insert_patch_def(parser::keyword(parser::kw::tableau), params, source_code);
}

void tess::script_impl::insert_globals(execution_ctxt& ctxt, global_vars global_defs)
{
	// TODO: make do topological sort etc. such that global definitions can reference each other...

	for (const auto& [var, val_expr] : global_defs) {
		auto value = val_expr->eval(ctxt);
		if (std::holds_alternative<error>(value))
			throw error(std::string("invalid global definition: " + var + ", " + std::get<error>(value).msg()));
		globals_[var] = value;
	}
}

std::optional<std::variant<tess::tile_def, tess::tile_patch_def>> tess::script_impl::get_functional(const std::string& func) const
{
	auto tile = tiles_.find(func);
	if (tile != tiles_.end())
		return *(tile->second);

	auto patch = patches_.find(func);
	if (patch != patches_.end())
		return patch->second;

	return std::nullopt;
}

std::shared_ptr<const tess::tile_def> tess::script_impl::get_tile_prototype(const std::string& name) const
{
	return tiles_.at(name);
}

const tess::tile_patch_def& tess::script_impl::tableau() const
{
	return patches_.at(parser::keyword(parser::kw::tableau));
}

std::vector<tess::tile> tess::script_impl::execute(execution_ctxt& ctxt) const
{
	auto tile_def = tiles_.at("triangle");
	auto val = tile_def->eval(ctxt);
	return std::vector<tess::tile>{ std::get<tile>(val) };
}

std::optional<tess::expr_value> tess::script_impl::get_global(const std::string& var) const
{
	auto i = globals_.find(var);
	if (i != globals_.end())
		return i->second;
	else
		return std::nullopt;
}
