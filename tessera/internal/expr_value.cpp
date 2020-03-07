#include "expr_value.h"

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
	return expr_value();
}

tess::expr_value tess::expr_value::get_field(const std::string& field, int ary_item_index) const
{
	return expr_value();
}