#include "lambda.h"

tess::lambda::lambda(std::shared_ptr<function_def> func, const scope_frame& closure)
{
}

tess::expr_value tess::lambda::call(const std::vector<expr_value>& expr_value) const
{
    return {};
}

tess::lambda tess::lambda::get_ref() const
{
    return lambda(this);
}
