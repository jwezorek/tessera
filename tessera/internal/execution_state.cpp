#include "execution_state.h"
#include "tessera/error.h"
#include "expr_value.h"
#include "allocator.h"
#include "stack_machine.h"
#include <algorithm>
#include <unordered_set>
#include <iostream>

/*--------------------------------------------------------------------------------------------------------------------------------*/

class tess::execution_state::impl_type {
public:
    tess::allocator allocator_;
    tess::stack_machine stack_machine;

    impl_type(execution_state& state) : stack_machine(state)
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

tess::evaluation_context tess::execution_state::create_eval_context()
{
    return evaluation_context(
        *this
    );
}

tess::evaluation_context tess::execution_state::create_eval_context(const lex_scope::frame& frame)
{
    auto ctxt = create_eval_context();
    ctxt.push_scope(frame);
    return ctxt;
}



