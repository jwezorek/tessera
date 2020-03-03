#include "expr_value.h"

tess::nil_val::nil_val()
{
}

bool tess::expr_value::is_object() const
{
	return std::holds_alternative<tile_val>(*this) ||
		std::holds_alternative<patch_val>(*this) ||
		std::holds_alternative<vertex_val>(*this) ||
		std::holds_alternative<edge_val>(*this);
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