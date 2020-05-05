#include "lambda.h"
#include "function_def.h"
#include "tile_def.h"
#include "expr_value.h"
#include "execution_state.h"
#include "variant_util.h"
#include "lambda_impl.h"
#include <variant>

const std::vector<std::string>& tess::lambda::parameters() const
{
    return impl_->func.parameters();
}

tess::expr_value tess::lambda::call(execution_state& state, const std::vector<expr_value>& args) const
{
    lex_scope::frame frame = impl_->closure;
    const auto& parameters = impl_->func.parameters();
    auto body = impl_->func.body();

    frame.set(parameters, args);

    auto ctxt = state.create_eval_context();
    lex_scope scope(ctxt, frame);
    return body->eval(ctxt);
}

void tess::lambda::add_to_closure(const std::string& var, const expr_value& val)
{
    impl_->closure.set(var, val);
}

tess::lambda::impl_type::impl_type(const function_def& f, const lex_scope::frame& c) :
    func(f), closure(c)
{
}
