#include "lambda.h"
#include "function_def.h"
#include "value.h"
#include "execution_state.h"
#include "variant_util.h"
#include "lambda_impl.h"
#include <variant>

const std::vector<std::string>& tess::lambda::parameters() const
{
    return impl_->parameters;
}
const tess::scope_frame& tess::lambda::closure() const
{
    return impl_->closure;
}

tess::detail::lambda_impl::lambda_impl(obj_id id, const std::vector<std::string>& params, const std::vector<stack_machine::item>& bod, const std::vector<std::string>& deps) :
    tessera_impl(id), parameters(params), body(bod), dependencies(deps)
{
}

void tess::detail::lambda_impl::insert_field(const std::string& var, const value_& val)
{
    closure.set(var, val);
}

tess::value_ tess::detail::lambda_impl::get_field(allocator& allocator, const std::string& field) const
{
    auto maybe_value = closure.get(field);
    if (maybe_value.has_value())
        return { maybe_value.value() };
    else
        throw tess::error("referenced unknown lambda closure item: " + field);
}

void tess::detail::lambda_impl::get_references(std::unordered_set<obj_id>& alloc_set) const
{
    auto key = this->get_id();
    if (alloc_set.find(key) != alloc_set.end())
        return;
    alloc_set.insert(key);

    for (const auto& [var, val] : closure)
        tess::get_references(val, alloc_set);
}

void tess::detail::lambda_impl::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, lambda_ptr mutable_clone) const
{
    mutable_clone->parameters = parameters;
    mutable_clone->dependencies = dependencies;
    mutable_clone->body = body;

    for (const auto& [var, val] : closure) {
        mutable_clone->closure.set(var, tess::clone_value(allocator, orginal_to_clone, val));
    }
}

std::vector<std::string> tess::detail::lambda_impl::unfulfilled_dependencies() const
{
    std::vector<std::string> depends;
    std::copy_if(dependencies.begin(), dependencies.end(), std::back_inserter(depends),
        [&](std::string var) {
            return !closure.has(var);
        }
    );
    return depends;
}


