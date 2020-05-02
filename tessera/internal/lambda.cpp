#include "lambda.h"
#include "function_def.h"
#include "tile_def.h"
#include "expr_value.h"
#include "eval_context.h"
#include "variant_util.h"
#include "lambda_impl.h"
#include <variant>

const std::vector<std::string>& tess::lambda::parameters() const
{
    return impl_->func.parameters();
}

tess::expr_value tess::lambda::call(const std::vector<expr_value>& args) const
{
    scope_frame frame = impl_->closure;
    const auto& parameters = impl_->func.parameters();
    auto body = impl_->func.body();

    frame.set(parameters, args);
    eval_context ctxt(frame);

    return body->eval(ctxt);
}

tess::lambda tess::lambda::get_ref() const
{
    lambda ref;
    ref.impl_ = impl_;
    return ref;
}

void tess::lambda::add_to_closure(const std::string& var, const expr_value& val)
{
    impl_->closure.set(var, val);
}

tess::lambda::impl_type::impl_type(const function_def& f, const scope_frame& c) :
    func(f), closure(c)
{
}
