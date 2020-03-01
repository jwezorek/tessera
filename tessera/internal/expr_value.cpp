#include "expr_value.h"

tess::nil_val::nil_val()
{
}

tess::bool_val::bool_val(bool v) :
    val_(v)
{
}

bool tess::bool_val::value() const
{
    return val_;
}

tess::number_val::number_val(int num) :
	val_(num)
{
}

tess::number_val::number_val(const SymEngine::Expression& expr)
{
	val_ = expr;
}

SymEngine::Expression tess::number_val::value() const
{
	return val_;
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