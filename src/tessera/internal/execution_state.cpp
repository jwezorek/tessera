#include "execution_state.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda_impl.h"
#include "tessera/error.h"
#include "value.h"
#include "allocator.h"
#include "stack_machine.h"
#include <algorithm>
#include <unordered_set>
#include <iostream>

/*--------------------------------------------------------------------------------------------------------------------------------*/

class tess::execution_state::impl_type {
public:
    tess::allocator allocator_;
    tess::stack_machine::stack main_stack_;
    tess::stack_machine::stack operand_stack_;
    tess::stack_machine::context_stack context_stack_;

    impl_type(execution_state& state) 
    {}
};

/*--------------------------------------------------------------------------------------------------------------------------------*/

tess::execution_state::execution_state() :
    impl_(std::make_shared<impl_type>(*this))
{
}

tess::allocator& tess::execution_state::allocator()
{
    return impl_->allocator_;
}


tess::stack_machine::stack& tess::execution_state::main_stack()
{
    return impl_->main_stack_;
}

tess::stack_machine::stack& tess::execution_state::operand_stack()
{
    return impl_->operand_stack_;
}

tess::stack_machine::context_stack& tess::execution_state::context_stack()
{
    return impl_->context_stack_;
}

tess::evaluation_context tess::execution_state::create_eval_context()
{
    return evaluation_context(
        *this
    );
}

tess::evaluation_context tess::execution_state::create_eval_context(const scope_frame& frame)
{
    auto ctxt = create_eval_context();
    ctxt.push_scope(frame);
    return ctxt;
}



