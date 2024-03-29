#pragma once

#include <memory>
#include <vector>
#include <string>
#include "evaluation_context.h"
#include "stack_machine.h"
#include "tessera_impl.h"
#include <unordered_set>
#include "gc_heap.h"

namespace tess {

    class execution_state
    {
        friend class evaluation_context;
    public:
        execution_state();
        evaluation_context create_eval_context();
        evaluation_context create_eval_context(const scope_frame& frame);
        tess::gc_heap& allocator();
        stack_machine::stack& main_stack();
        stack_machine::stack& operand_stack();
        tess::context_stack& context_stack();
        //std::unordered_set<tess::obj_id> get_references() const;
        //void debug() const;
    private:
        class impl_type;
        std::shared_ptr<impl_type> impl_;
    };

}