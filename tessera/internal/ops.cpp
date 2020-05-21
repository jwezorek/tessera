#include "ops.h"
#include "function_def.h"
#include "lambda.h"
#include "lambda_impl.h"
#include "allocator.h"
#include "variant_util.h"
#include "evaluation_context.h"

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
        for (int i = 0; i < n; i += 2) {
            vars.push_back(std::get<stack_machine::identifier>(operands[i]).name);
            vals.push_back(std::get<expr_value>(operands[i + 1]));
        }
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
    contexts.pop();
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
    auto& ctxt = contexts.top();
    contexts.push(ctxt.execution_state().create_eval_context());
    return std::nullopt;
}

/*---------------------------------------------------------------------------------------------*/

tess::neg_op::neg_op() : stack_machine::op_1(1)
{
}

tess::stack_machine::item tess::neg_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    tess::number val = std::get<tess::number>(std::get<expr_value>(operands[0]));
    return stack_machine::item(expr_value{ -val });
}

/*---------------------------------------------------------------------------------------------*/

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

/*---------------------------------------------------------------------------------------------*/

tess::insert_fields_op::insert_fields_op() : op_1(2)
{
}

tess::stack_machine::item tess::insert_fields_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto obj = std::get<expr_value>(operands[0]);
    auto frame = std::get<tess::scope_frame>(operands[1]);

    for (const auto& [var, val] : frame)
        obj.insert_field(var, val);

    return obj;
}

/*---------------------------------------------------------------------------------------------*/

tess::pop_frame_op::pop_frame_op() : tess::stack_machine::op(0)
{
}

std::optional<tess::error> tess::pop_frame_op::execute(stack_machine::stack& main_stack, stack_machine::stack& operand_stack, stack_machine::context_stack& contexts)
{
    if (contexts.empty())
        return tess::error("context stack underflow");

    if (contexts.top().empty())
        return tess::error("eval context underflow");

    contexts.top().pop_scope();
    return std::nullopt;
}

/*---------------------------------------------------------------------------------------------*/

tess::push_frame_op::push_frame_op() : tess::stack_machine::op(1)
{
}

std::optional<tess::error> tess::push_frame_op::execute(stack_machine::stack& main_stack, stack_machine::stack& operand_stack, stack_machine::context_stack& contexts)
{
    if (contexts.empty())
        return tess::error("context stack underflow");
    auto frame = std::get<scope_frame>(operand_stack.pop());
    contexts.top().push_scope(frame);
    return std::nullopt;
}

/*---------------------------------------------------------------------------------------------*/

tess::dup_op::dup_op() : stack_machine::op_multi(1)
{
}

std::variant<std::vector<tess::stack_machine::item>, tess::error> tess::dup_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    return std::vector<tess::stack_machine::item> {
        operands[0],
        operands[0]
    };
}

tess::assign_op::assign_op(int num_vars) : stack_machine::op_multi(num_vars+1)
{
}

std::variant<std::vector<tess::stack_machine::item>, tess::error> tess::assign_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    int num_vars = static_cast<int>(operands.size() - 1);
    auto value = std::get<expr_value>(operands.back());
    std::vector<stack_machine::identifier> vars(num_vars);
    std::transform(operands.begin(), operands.begin() + num_vars, vars.begin(),
        [](const auto& item) { return std::get<stack_machine::identifier>(item); }
    );

    if (num_vars == 1) {
        //TODO: insert self-reference if val is a lambda
        std::vector<tess::stack_machine::item> output = { vars[0], value };
        return output;
    } else {
        //TODO: insert self-reference if val is a lambda
        std::vector<tess::stack_machine::item> output(num_vars);
        int i = 0;
        for (const auto& var : vars) {
            output.push_back(var);
            output.push_back(value.get_ary_item(i++));
        }
        return output;
    }
}