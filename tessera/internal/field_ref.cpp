#include "field_ref.h"

tess::field_ref_impl::field_ref_impl(expr_value obj, std::string field) :
	obj_(obj),
	field_(field)
{
}

void tess::field_ref_impl::set(expr_value val)
{
	tess::insert_field(obj_, field_, val);
}
