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

SymEngine::Expression tess::number_val::value() const
{
	return val_;
}

bool tess::is_object(const expr_value& val)
{
	return std::holds_alternative<tile_val>(val) ||
		std::holds_alternative<patch_val>(val) ||
		std::holds_alternative<vertex_val>(val) ||
		std::holds_alternative<edge_val>(val);
}
