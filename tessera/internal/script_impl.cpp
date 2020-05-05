#include "script_impl.h"
#include <algorithm>

tess::script::impl_type::impl_type(const assignment_block& globals, const tess::expr_ptr& tableau) :
    globals_(globals.simplify()), tableau_(tableau->simplify())
{
}

const std::vector<std::string>& tess::script::impl_type::parameters() const
{
    auto fn_ptr = std::dynamic_pointer_cast<function_def>(tableau_);
    if (!fn_ptr)
        throw tess::error("unknown error");
    return fn_ptr->parameters();
}


const tess::assignment_block& tess::script::impl_type::globals() const {
    return globals_;
}

tess::expr_ptr tess::script::impl_type::tableau() const {
    return tableau_;
}

tess::execution_state& tess::script::impl_type::state()
{
    return state_;
}

