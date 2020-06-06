#include "lambda.h"
#include "function_def.h"
#include "expr_value.h"
#include "execution_state.h"
#include "variant_util.h"
#include "lambda_impl.h"
#include <variant>

const std::vector<std::string>& tess::lambda::parameters() const
{
    return impl_->parameters;
}

void tess::lambda::insert_field(const std::string& var, const expr_value& val)
{
    impl_->insert_field(var, val);
}

const tess::scope_frame& tess::lambda::closure() const
{
    return impl_->closure;
}

std::vector<tess::stack_machine::item> tess::lambda::body() const
{
    return impl_->body;
}


tess::lambda::impl_type::impl_type(const std::vector<std::string>& param, const std::vector<stack_machine::item>& bod, const scope_frame& c) :
    parameters(param), body(bod), closure(c)
{
}

void tess::lambda::impl_type::insert_field(const std::string& var, const expr_value& val)
{
    closure.set(var, val);
}

tess::expr_value tess::lambda::impl_type::get_field(allocator& allocator, const std::string& field) const
{
    auto maybe_value = closure.get(field);
    if (maybe_value.has_value())
        return { maybe_value.value() };
    else
        return { tess::error("referenced unknown lambda closure item") };
}

void tess::lambda::impl_type::get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const
{
    auto ptr = to_void_star(this);
    if (alloc_set.find(ptr) != alloc_set.end())
        return;
    alloc_set.insert(ptr);

    for (const auto& [var, val] : closure)
        val.get_all_referenced_allocations(alloc_set);
}
