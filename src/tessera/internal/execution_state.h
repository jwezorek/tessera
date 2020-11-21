#pragma once

#include <memory>
#include <map>
#include <vector>
#include <string>
#include "evaluation_context.h"
#include "stack_machine.h"

namespace tess {

    class execution_state
    {
        friend class evaluation_context;
    public:
        execution_state();
        evaluation_context create_eval_context();
        evaluation_context create_eval_context(const scope_frame& frame);
        class allocator& allocator();
        stack_machine::stack& main_stack();
        stack_machine::stack& operand_stack();
        stack_machine::context_stack& context_stack();

    private:
        class impl_type;
        std::shared_ptr<impl_type> impl_;
    };

}