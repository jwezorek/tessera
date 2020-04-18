#include "lambda.h"
#include "function_def.h"
#include "expr_value.h"
#include "eval_context.h"
#include "util.h"
#include <variant>

class tess::lambda::impl_type  {
    public:
        function_def func;
        scope_frame closure;

        impl_type(const function_def f, const scope_frame& c) :
            func(f), closure(c)
        {}
};

tess::lambda::lambda(const function_def& func, const scope_frame& closure) :
    impl_(std::make_shared<impl_type>(func, closure))
{
}

tess::lambda::lambda(const lambda* ref) :
    impl_(ref)
{
}

tess::lambda::impl_type* tess::lambda::get_impl() const
{
    if (std::holds_alternative<tess::lambda::lambda_impl_ptr>(impl_))
        return std::get< tess::lambda::lambda_impl_ptr>(impl_).get();
    else
        return std::get<const lambda*>(impl_)->get_impl();
}

tess::expr_value tess::lambda::call(const std::vector<expr_value>& args) const
{
    auto* impl = get_impl();

    scope_frame frame = impl->closure;
    const auto& parameters = impl->func.parameters();
    const auto& body = impl->func.body();

    frame.set(parameters, args);
    eval_context ctxt(frame);

    return std::visit(
        [&ctxt](const auto& fn) {
            return fn->eval(ctxt);
        },
        body
    );
}

tess::lambda tess::lambda::get_ref() const
{
    return lambda(this);
}

void tess::lambda::add_to_closure(const std::string& var, const expr_value& val)
{
    auto* impl = get_impl();
    impl->closure.set(var, val);
}
