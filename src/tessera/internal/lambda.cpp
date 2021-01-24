#include "lambda.h"
#include "function_def.h"
#include "value.h"
#include "execution_state.h"
#include "variant_util.h"
#include "lambda_impl.h"
#include <variant>

namespace {

    unsigned int g_id = 0;

}

tess::detail::lambda_impl::lambda_impl( gc_heap& a, const std::vector<std::string>& params, const std::vector<stack_machine::item>& bod, const std::vector<std::string>& deps) :
    parameters(params), body(bod), dependencies(deps), id_(++g_id)
{
}

void tess::detail::lambda_impl::insert_field(const std::string& var, const value_& val)
{
    closure.set(var, val);
}

tess::value_ tess::detail::lambda_impl::get_field(gc_heap& allocator, const std::string& field) const
{
    auto maybe_value = closure.get(field);
    if (maybe_value.has_value())
        return { maybe_value.value() };
    else
        throw tess::error("referenced unknown lambda closure item: " + field);
}

void tess::detail::lambda_impl::set_id(unsigned int id)
{
    id_ = id;
}

void tess::detail::lambda_impl::clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, mutable_object_value>& orginal_to_clone, lambda_ptr mutable_clone) const
{
    mutable_clone->set_id(id_);
    mutable_clone->parameters = parameters;
    mutable_clone->dependencies = dependencies;
    mutable_clone->body = body;

    for (const auto& [var, val] : closure) {
        mutable_clone->closure.set(var, tess::clone_value(allocator, orginal_to_clone, val)); //clone fields
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


