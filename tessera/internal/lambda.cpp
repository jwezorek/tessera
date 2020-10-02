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

std::vector<std::string> tess::lambda::dependencies() const
{
    return impl_->dependencies;
}

std::vector<std::string> tess::lambda::unfulfilled_dependencies() const
{
    std::vector<std::string> depends;
    const auto& closure = impl_->closure;
    std::copy_if(impl_->dependencies.begin(), impl_->dependencies.end(), std::back_inserter(depends),
        [&closure](std::string var) {
            return !closure.has(var);
        }
    );
    return depends;
}

tess::lambda::impl_type::impl_type(obj_id id, const std::vector<std::string>& params, const std::vector<stack_machine::item>& bod, const std::vector<std::string>& deps) :
    tessera_impl(id), parameters(params), body(bod), dependencies(deps)
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
        throw tess::error("referenced unknown lambda closure item: " + field);
}

void tess::lambda::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
    auto key = this->get_id();
    if (alloc_set.find(key) != alloc_set.end())
        return;
    alloc_set.insert(key);

    for (const auto& [var, val] : closure)
        val.get_all_referenced_allocations(alloc_set);
}

void tess::lambda::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, lambda::impl_type* clone) const
{
    clone->parameters = parameters;
    clone->dependencies = dependencies;
    clone->body = body;

    for (const auto& [var, val] : closure) {
        clone->closure.set(var, val.clone(allocator, orginal_to_clone));
    }
}

bool tess::operator==(lambda l, lambda r)
{
    return tess::get_impl(l) == tess::get_impl(r);
}
