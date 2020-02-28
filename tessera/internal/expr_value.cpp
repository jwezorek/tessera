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
