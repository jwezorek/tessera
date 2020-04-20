#include "script_impl.h"
#include <algorithm>

tess::script::impl_type::impl_type(const assignment_block& globals, const patch_def& tableau) :
    globals_(globals), tableau_(tableau)
{

}

const std::vector<std::string>& tess::script::impl_type::parameters() const
{
    return tableau_.parameters();
}
