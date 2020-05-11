#pragma once

#include <memory>
#include <map>
#include <vector>
#include <string>
#include "evaluation_context.h"

namespace tess {

    class stack_machine;

    class execution_state
    {
        friend class evaluation_context;
    public:
        execution_state();
        evaluation_context create_eval_context();
        evaluation_context create_eval_context(const lex_scope::frame& frame);
        allocator& allocator();
        stack_machine& stack_machine();
    private:
        class impl_type;
        std::shared_ptr<impl_type> impl_;
    };

}