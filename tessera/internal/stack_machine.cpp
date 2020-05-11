
#include "stack_machine.h"
#include "tessera/error.h"

tess::stack_machine::stack_machine(execution_state& state) :
    parent_(state)
{
}

std::optional<tess::error> tess::stack_machine::op::execute(stack& main_stack, stack& operand_stack, context_stack& contexts)
{
    return std::optional<error>();
}
