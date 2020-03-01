#include "execution_ctxt.h"

tess::expr_value tess::execution_ctxt::call(const std::string& func, const std::vector<tess::expr_value>& args) const
{
	return expr_value();
}

tess::expr_value tess::execution_ctxt::eval(const std::string& var) const
{
	return expr_value();
}

tess::expr_value tess::execution_ctxt::get_placeholder(int i) const
{
	return expr_value();
}
