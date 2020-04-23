#include "script_impl.h"
#include <algorithm>

tess::script::impl_type::impl_type(const assignment_block& globals, const patch_def& tableau) :
    globals_(globals.simplify()), tableau_(tableau.simplify())
{

}

const std::vector<std::string>& tess::script::impl_type::parameters() const
{
    return tableau_.parameters();
}


tess::assignment_block tess::script::impl_type::globals() const {
    return globals_;
}

tess::patch_def tess::script::impl_type::tableau() const {
    return tableau_;
}

