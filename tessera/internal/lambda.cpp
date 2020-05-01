#include "lambda.h"
#include "function_def.h"
#include "tile_def.h"
#include "expr_value.h"
#include "eval_context.h"
#include "variant_util.h"
#include "lambda_impl.h"
#include <variant>

tess::lambda::lambda(const function_def& func, const scope_frame& closure) :
    impl_(std::make_shared<impl_type>(func, closure))
{
}

const std::vector<std::string>& tess::lambda::parameters() const
{
    return get_impl().func.parameters();
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
    const auto& parameters = impl.func.parameters();
    auto body = impl.func.body();

    frame.set(parameters, args);
    eval_context ctxt(frame);

    return body->eval(ctxt);
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

tess::lambda::impl_type::impl_type(const function_def& f, const scope_frame& c) :
    func(f), closure(c)
{
}
