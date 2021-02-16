#include "field_ref.h"
#include "lambda_impl.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"

tess::field_ref_impl::field_ref_impl(value_ obj, std::string field) :
	obj_(obj),
	field_(field)
{
}

void tess::field_ref_impl::set(value_ val)
{
	tess::insert_field(obj_, field_, val);
}
