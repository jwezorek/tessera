#include "script_impl.h"

void tess::tessera_script::script_impl::insert_tile_def(const std::string& name, std::vector<std::string> params, const text_range& source_code)
{
	tiles_.emplace(
		std::piecewise_construct,
		std::make_tuple(name),
		std::make_tuple(name, params, source_code)
	);
}

void tess::tessera_script::script_impl::insert_patch_def(const std::string& name, std::vector<std::string> params, const text_range& source_code)
{
	patches_.emplace(
		std::piecewise_construct,
		std::make_tuple(name),
		std::make_tuple(name, params, source_code)
	);
}

void tess::tessera_script::script_impl::insert_tableau_def(std::vector<std::string> params, const text_range& source_code)
{
	tableau_ = tableau_def(params, source_code);
}

void tess::tessera_script::script_impl::insert_globals(execution_ctxt& ctxt, global_vars global_defs)
{
	// TODO: make do topological sort etc. such that global definitions can reference each other...

	for (const auto& [var, val_expr] : global_defs) {
		auto value = val_expr->eval(ctxt);
		if (std::holds_alternative<error>(value))
			throw error(std::string("invalid global definition: " + var + ", " + std::get<error>(value).msg()));
		globals_[var] = value;
	}
}

void tess::tessera_script::script_impl::build_tiles(execution_ctxt& ctxt)
{
	for (auto& [dummy, tile] : tiles_) {
		auto maybe_tile = tile.eval(ctxt);
		if (std::holds_alternative<tess::tile>(maybe_tile))
			tile.set_value(std::get<tess::tile>(maybe_tile));
	}
}

std::optional<std::variant<tess::tile_def, tess::tile_patch_def>> tess::tessera_script::script_impl::get_functional(const std::string& func) const
{
	auto tile = tiles_.find(func);
	if (tile != tiles_.end())
		return tile->second;

	auto patch = patches_.find(func);
	if (patch != patches_.end())
		return patch->second;

	return std::nullopt;
}

std::optional<tess::expr_value> tess::tessera_script::script_impl::get_global(const std::string& var) const
{
	auto i = globals_.find(var);
	if (i != globals_.end())
		return i->second;
	else
		return std::nullopt;
}
