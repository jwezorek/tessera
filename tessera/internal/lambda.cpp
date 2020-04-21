#include "lambda.h"
#include "function_def.h"
#include "expr_value.h"
#include "eval_context.h"
#include "util.h"
#include <variant>

class tess::lambda::impl_type  {
    public:
        std::shared_ptr<function_def> func;
        scope_frame closure;

        impl_type(const std::shared_ptr<function_def>& f, const scope_frame& c) :
            func(f), closure(c)
        {}
};

tess::lambda::lambda(expr_ptr func, const scope_frame& closure) :
    impl_(std::make_shared<impl_type>(std::static_pointer_cast<function_def>(func), closure))
{
}

const std::vector<std::string>& tess::lambda::parameters() const
{
    return get_impl().func->parameters();
}

tess::lambda::lambda(impl_type& ref) :
    impl_(&ref)
{
}

tess::lambda::impl_type& tess::lambda::get_impl() const
{
    return std::visit(
        [](auto ptr)->impl_type& { return *ptr; },
        impl_
    );
}

tess::expr_value tess::lambda::call(const std::vector<expr_value>& args) const
{
    auto& impl = get_impl();

    scope_frame frame = impl.closure;
    const auto& parameters = impl.func->parameters();
    const auto& func = impl.func->impl();

    frame.set(parameters, args);
    eval_context ctxt(frame);

    return std::visit(
        [&ctxt](const auto& fn)->expr_value {
            return fn->call(ctxt);
        },
        func
    );
}

tess::lambda tess::lambda::get_ref() const
{
    return lambda( get_impl() );
}

void tess::lambda::add_to_closure(const std::string& var, const expr_value& val)
{
    auto& impl = get_impl();
    impl.closure.set(var, val);
}
