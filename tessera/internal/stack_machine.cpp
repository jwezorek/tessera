#include "stack_machine.h"
#include "tessera/error.h"
#include "variant_util.h"
#include "evaluation_context.h"
#include "execution_state.h"

std::optional<tess::error> tess::stack_machine::op::execute(tess::stack_machine::stack& main_stack, tess::stack_machine::stack& operand_stack, tess::stack_machine::context_stack& contexts)
{
    if (number_of_args_ > operand_stack.count())
        return tess::error("operand stack underflow.");
    auto operands = operand_stack.pop(number_of_args_);
    auto op_result = execute(main_stack, operands, contexts);
    if (std::holds_alternative<tess::error>(op_result))
        return std::get<tess::error>(op_result);
    auto& items = std::get<std::vector<stack_machine::item>>(op_result);
    main_stack.push(items);
    return std::nullopt;
}

void tess::stack_machine::stack::push(const tess::stack_machine::item& item)
{
    impl_.push_back(item);
}

tess::stack_machine::item tess::stack_machine::stack::pop()
{
    auto val = impl_.back();
    impl_.pop_back();
    return val;
}

void tess::stack_machine::stack::push(const std::vector<item>& items)
{
    push(items.begin(), items.end());
}

std::vector<tess::stack_machine::item> tess::stack_machine::stack::pop(int n)
{
    std::vector<tess::stack_machine::item> output(n);
    std::copy(impl_.rbegin(), impl_.rbegin() + n, output.begin());
    impl_.resize(impl_.size() - n);
    return output;
}


bool tess::stack_machine::stack::empty() const
{
    return impl_.empty();
}

int tess::stack_machine::stack::count() const
{
    return static_cast<int>(impl_.size());
}

/*------------------------------------------------------------------------------*/

tess::stack_machine::stack_machine() 
{
}

tess::expr_value tess::stack_machine::run(execution_state& state)
{
    auto& contexts = state.context_stack();
    auto& main_stack = state.main_stack();
    auto& operands = state.operand_stack();
    expr_value output;

    try {

        while (!main_stack.empty()) {
            auto stack_item = main_stack.pop();
            std::visit(
                overloaded{
                    [&](op_ptr op) {
                        auto maybe_error = op->execute(main_stack, operands, contexts);
                        if (maybe_error.has_value())
                            throw maybe_error.value();
                    },
                    [&](auto val) {
                        operands.push(stack_machine::item{ val });
                    }
                },
                stack_item
            );
        }
        output = std::get<expr_value>(operands.pop());

    } catch (tess::error error) {
        return { error };
    } catch (...) {
        return { tess::error("Unknown error.") };
    }
    return output;
}