#include "function_def.h"
#include "tile_def.h"
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
        lambda(*this, scope_frame(closure))
    };
}

const std::vector<std::string>& tess::function_def::parameters() const
{
    return parameters_;
}

tess::expr_ptr tess::function_def::body() const
{
    return body_;
}

tess::expr_ptr tess::function_def::simplify() const
{
    return std::make_shared<function_def>(
        parameters_,
        body_->simplify()
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

tess::function_def::function_def(const std::vector<std::string>& params, expr_ptr body) :
    parameters_(params), body_(body)
{
}

std::vector<std::string> tess::function_def::get_variables() const
{
    std::vector<std::string> vars;
    body_->get_dependencies(vars);
    return vars;
}


