#include "function_def.h"
#include "tile_def.h"
#include "expr_value.h"
#include "ops.h"
#include "execution_state.h"
#include <sstream>
#include <variant>
#include <unordered_set>
#include "allocator.h"

tess::expr_value tess::function_def::eval(evaluation_context& ctxt) const
{
    std::unordered_set<std::string> dependent_vars;
    get_dependencies(dependent_vars);

    std::vector<std::tuple<std::string, expr_value>> closure;
    for (const auto& var : dependent_vars) {
        if (ctxt.contains(var)) {
            closure.push_back({ var, ctxt.get(var) });
        }
    }

    return {
        ctxt.allocator().create<lambda>(*this, scope_frame(closure))
    };
}

void tess::function_def::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<make_lambda>());
    stack.push(this->simplify());
    compile_dependencies(stack);
}

std::string tess::function_def::to_string() const
{
    std::string parameters = std::accumulate(
        std::next(parameters_.begin()),
        parameters_.end(),
        parameters_[0],
        [](std::string a, std::string b) {
            return a + " , " + b;
        }
    );

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

void tess::function_def::compile_dependencies(stack_machine::stack& stack) const
{
    std::unordered_set<std::string> dependent_vars;
    get_dependencies(dependent_vars);

    int n = static_cast<int>(dependent_vars.size());
    std::vector<stack_machine::item> operands;
    operands.reserve( n * 3 );

    for (const auto& var : dependent_vars) {
        operands.push_back(stack_machine::identifier(var));
        operands.push_back(stack_machine::item(std::make_shared<get_var>()));
        operands.push_back(stack_machine::identifier(var));
    }

    stack.push(std::make_shared<make_scope_frame>(n));
    stack.push(operands);
}

std::vector<std::string> tess::function_def::get_variables() const
{
    std::unordered_set<std::string> vars;
    body_->get_dependencies(vars);
    return std::vector<std::string>(vars.begin(),vars.end());
}


