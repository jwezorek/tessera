#include "function_def.h"
#include "value.h"
#include "ops.h"
#include "execution_state.h"
#include "gc_heap.h"
#include <variant>
#include <unordered_set>
#include <numeric>

void tess::function_def::compile(stack_machine::stack& stack) const
{
    stack_machine::stack body;
    body_->compile(body);
    std::unordered_set<std::string> deps;
    get_dependencies(deps);
    stack.push(std::make_shared<make_lambda>(parameters_, body.pop_all(), std::vector<std::string>(deps.begin(), deps.end())));
}

std::string tess::function_def::to_string() const
{
    std::string parameters;
    if (!parameters_.empty()) {
        parameters = std::accumulate(
            std::next(parameters_.begin()),
            parameters_.end(),
            parameters_[0],
            [](std::string a, std::string b) {
                return a + " " + b;
            }
        );
    }
    return std::string("( lambda (") + parameters + ")  " + body_->to_string() + " )";
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

void tess::function_def::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    const auto& params = parameters();
    std::unordered_set<std::string> param_set(params.begin(), params.end());

    auto all_vars = get_variables();
    for (const auto& var : all_vars) {
        if (param_set.find(var) == param_set.end())
            dependencies.insert(var);
    }
}

tess::function_def::function_def(const std::vector<std::string>& params, expr_ptr body) :
    parameters_(params), body_(body)
{
}

std::vector<std::string> tess::function_def::get_variables() const
{
    std::unordered_set<std::string> vars;
    body_->get_dependencies(vars);
    return std::vector<std::string>(vars.begin(),vars.end());
}


