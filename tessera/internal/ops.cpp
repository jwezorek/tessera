#include "ops.h"
#include "function_def.h"
#include "lambda.h"
#include "lambda_impl.h"
#include "allocator.h"
#include "variant_util.h"

tess::make_lambda::make_lambda() :
    op_1(2)
{
}

tess::stack_machine::item tess::make_lambda::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto& alloc = contexts.top().allocator();
    try {
        auto def_ptr = std::get<expr_ptr>(operands[0]);
        function_def def = *std::static_pointer_cast<function_def>(def_ptr);
        auto closure = std::get<scope_frame>(operands[1]);
        auto lambda = alloc.create<tess::lambda>(def, closure);
        return  make_expr_val_item(lambda) ;
    }  catch (tess::error e) {
        return e;
    } catch (...) {
        return tess::error("bad make_lamba op");
    }
}

/*---------------------------------------------------------------------------------------------*/

tess::get_var::get_var() : op_1(1)
{
}

tess::stack_machine::item tess::get_var::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto& ctxt = contexts.top();
    try {
        auto ident = std::get<stack_machine::identifier>(operands[0]);
        return ctxt.get(ident.name);
    } catch (tess::error e) {
        return e;
    } catch (...) {
        return tess::error("bad get_var op");
    }
}

/*---------------------------------------------------------------------------------------------*/

tess::make_scope_frame::make_scope_frame(int n) : op_1(2*n)
{
}

tess::stack_machine::item tess::make_scope_frame::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    int n = number_of_args_/2;
    std::vector<std::string> vars(n);
    std::vector<expr_value> vals(n);
    try {
        std::transform(operands.begin(), operands.begin() + n, vars.begin(),
            [&](const auto& item)->std::string {
                auto ident = std::get<stack_machine::identifier>(item);
                return ident.name;
            }
        );
        auto val_begin = operands.begin() + n;
        std::transform(val_begin, operands.end(), vals.begin(),
            [&](const auto& item)->tess::expr_value {
                return std::get<tess::expr_value>(item);
            }
        );
        return scope_frame(vars, vals);
    } catch (tess::error e) {
        return e;
    } catch (...) {
        return tess::error("bad get_var op");
    }
}

/*---------------------------------------------------------------------------------------------*/

tess::pop_eval_context::pop_eval_context() : stack_machine::op(0)
{
}

std::optional<tess::error> tess::pop_eval_context::execute(tess::stack_machine::stack& main_stack, stack_machine::stack& operand_stack, stack_machine::context_stack& contexts)
{
    auto& ctxt = contexts.top();
    contexts.push(ctxt.execution_state().create_eval_context());
    return std::nullopt;
}

/*---------------------------------------------------------------------------------------------*/

std::variant<std::vector<tess::stack_machine::item>, tess::error> tess::call_func::execute(const std::vector<tess::stack_machine::item>& operands, tess::stack_machine::context_stack& contexts) const
{
    try {
        lambda func = std::get<lambda>(std::get<expr_value>(operands[0]));
        std::vector<expr_value> args = get_vector<expr_value>(operands.begin() + 1, operands.end());

        if (func.parameters().size() != args.size())
            return tess::error("func call arg count mismatch.");

        scope_frame frame(func.parameters(), args);
        for (const auto& [var, val] : func.closure()) 
            frame.set(var, val);
        contexts.top().push_scope(frame);

        stack_machine::stack func_def;
        func.body()->compile(func_def);
        int sz = func_def.count();

        return func_def.pop(sz);

    }  catch (tess::error e) {
        return e;
    }
    catch (...) {
        return tess::error("bad function call op");
    }

    return std::vector<tess::stack_machine::item>();
}

tess::call_func::call_func(int num_args) : op_multi(num_args+1)
{
}

/*---------------------------------------------------------------------------------------------*/

tess::push_eval_context::push_eval_context() : stack_machine::op(0)
{
}

std::optional<tess::error> tess::push_eval_context::execute(stack_machine::stack& main_stack, stack_machine::stack& operand_stack, stack_machine::context_stack& contexts)
{
    contexts.pop();
    return std::nullopt;
}

tess::neg_op::neg_op() : stack_machine::op_1(1)
{
}

tess::stack_machine::item tess::neg_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    tess::number val = std::get<tess::number>(std::get<expr_value>(operands[0]));
    return stack_machine::item(expr_value{ -val });
}

tess::add_op::add_op(int args) : stack_machine::op_1(args)
{
}

tess::stack_machine::item tess::add_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto values = get_vector<expr_value>(operands.begin(), operands.end());
    auto nums = get_vector<tess::number>(values.begin(), values.end());
    tess::number sum = 0;
    for (const auto& num : nums)
        sum += num;
    return stack_machine::item( expr_value{sum} );
}
