#include "variant_util.h"
#include "expr_value.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"

tess::nil_val::nil_val()
{
}

bool tess::expr_value::is_object_like() const
{
	// by "object-like" we mean epression values that may have fields.
	return std::holds_alternative<tile>(*this) ||
		std::holds_alternative<tile_patch>(*this) ||
		std::holds_alternative<vertex>(*this) ||
		std::holds_alternative<edge>(*this) ||
		std::holds_alternative<cluster>(*this);
}

bool tess::expr_value::is_array_like() const
{
	// by "array-like" we mean epression values that may be dereferenced via the [] operator.
	return  std::holds_alternative<tile_patch>(*this) ||
		std::holds_alternative<cluster>(*this);
}

bool tess::expr_value::is_valid() const
{
    return ! is_error();
}

bool tess::expr_value::is_error() const
{
	return std::holds_alternative<tess::error>(*this);
}

tess::expr_value tess::expr_value::get_ary_item(int index) const
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like())
		return { error("attempted reference to a sub-tile of a value that is not a tile patch.") };
	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile_patch, cluster> ary_variant = variant_cast(value);

	return std::visit(
		[&](auto&& obj)->expr_value { return get_impl(obj)->get_ary_item(index); },
		ary_variant
	);
}

int tess::expr_value::get_ary_count() const
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like())
		return -1;
	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile_patch, cluster> ary_variant = variant_cast(value);

	return std::visit(
		[&](auto&& obj)->int { return get_impl(obj)->get_ary_count(); },
		ary_variant
	);
}

tess::expr_value tess::expr_value::get_field(const std::string& field) const
{
	if (!is_object_like()) {
		return { error("attempted reference to field of a non-object.") };
	}
	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile, tile_patch, vertex, edge, cluster> obj_variant = variant_cast(value);
	
	return std::visit(
		[&](auto&& obj)->expr_value { return get_impl(obj)->get_field(field); },
		obj_variant
	);
}

tess::expr_value tess::expr_value::call(const std::vector<expr_value>& args) const
{
	if (!std::holds_alternative<lambda>(*this))
		return { error("attempted to call a value that is not functionlike") };
	const auto& lambda = std::get<tess::lambda>(*this);
	return lambda.call(args);
}

void tess::expr_value::insert_field(const std::string& var, expr_value val) const
{
	if (!is_object_like())
		return;
	std::variant<tile, tile_patch, vertex, edge, cluster> obj_variant = variant_cast(static_cast<expr_val_var>(*this));
	std::visit(
		[&](auto&& obj) { get_impl(obj)->insert_field(var,val); },
		obj_variant
	);
}

