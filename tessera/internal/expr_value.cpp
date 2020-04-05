#include "util.h"
#include "expr_value.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"

tess::nil_val::nil_val()
{
}

bool tess::expr_value::is_object() const
{
	return std::holds_alternative<tile>(*this) ||
		std::holds_alternative<tile_patch>(*this) ||
		std::holds_alternative<vertex>(*this) ||
		std::holds_alternative<edge>(*this);
}

tess::expr_value tess::expr_value::get_ary_item(int index) const
{
	return expr_value();
}

tess::expr_value tess::expr_value::get_field(const std::string& field) const
{
	if (!is_object()) {
		return { error("attempted reference to field of a non-object.") };
	}
	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile, tile_patch, vertex, edge> obj_variant = variant_cast(value);
	
	return std::visit(
		[&](auto&& obj)->expr_value { return get_impl(obj)->get_field(field); },
		obj_variant
	);
}

tess::expr_value tess::expr_value::get_field(const std::string& field, int ary_item_index) const
{
	auto obj = get_ary_item(ary_item_index);
	if (!obj.is_object())
		return { error("attempted to reference a non-array like object like an array.") };
	return obj.get_field(field);
}