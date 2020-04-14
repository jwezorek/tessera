#include "script_impl.h"
#include <algorithm>

tess::script_impl::script_impl(const std::vector<std::tuple<std::string, expr_ptr>>& globals, std::vector<std::string>& params, expr_ptr tableau) :
    parameters_(params), tableau_(tableau)
{
    using value_pair = decltype(globals_)::value_type;
    std::transform(globals.begin(), globals.end(), std::inserter(globals_, globals_.end()),
        [](const auto& var_val)->value_pair {
            const auto& [var, val] = var_val;
            return { var, val };
        }
    );
}
