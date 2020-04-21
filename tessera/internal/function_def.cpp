#include "function_def.h"
#include "expr_value.h"
#include "eval_context.h"
#include <sstream>
#include <variant>
#include <unordered_set>

tess::expr_value tess::function_def::eval(eval_context& ctxt) const
{
    std::vector<std::string> dependent_vars;
    get_dependencies(dependent_vars);

    std::vector<std::tuple<std::string, expr_value>> closure;
    for (const auto& var : dependent_vars) {
        if (ctxt.contains(var)) {
            closure.push_back({ var, ctxt.get(var) });
        }
    }

    return {
        lambda(this->simplify(), scope_frame(closure))
    };
}

const std::vector<std::string>& tess::function_def::parameters() const
{
    return std::visit(
        [](const auto& impl)->const std::vector<std::string>& {
            return impl->parameters();
        },
        impl_
    );
}

const std::variant<std::shared_ptr<tess::tile_def>, std::shared_ptr<tess::patch_def>>& tess::function_def::impl() const
{
    return impl_;
}

tess::expr_ptr tess::function_def::simplify() const
{
    return std::visit(
        [](const auto& def)->std::shared_ptr<function_def> {
            return std::make_shared<function_def>(
                def->simplify()
            );
        },
        impl_
    );
}

void tess::function_def::get_dependencies(std::vector<std::string>& dependencies) const
{
    const auto& params = parameters();
    std::unordered_set<std::string> param_set(params.begin(), params.end());

    auto all_vars = get_variables();
    for (const auto& var : all_vars) {
        if (param_set.find(var) == param_set.end())
            dependencies.push_back(var);
    }
}

tess::function_def::function_def(const tile_def& tile_definition) :
    impl_(std::make_shared<tile_def>(tile_definition))
{
}

tess::function_def::function_def(const patch_def& patch_definition) :
    impl_(std::make_shared<patch_def>(patch_definition))
{
}

std::vector<std::string> tess::function_def::get_variables() const
{
    return std::visit(
        [](const auto& impl)->std::vector<std::string> {
            return impl->get_variables();
        },
        impl_
    );
}


