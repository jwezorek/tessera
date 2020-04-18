#include "lambda.h"
#include "function_def.h"
#include "expr_value.h"
#include "eval_context.h"

class tess::lambda::impl_type  {
    function_def func;
    scope_frame closure;
};

tess::lambda::lambda(const function_def& func, const scope_frame& closure)
{
}

tess::lambda::lambda(const lambda* ref)
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
