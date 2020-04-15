#include "function.h"

tess::expr_value tess::function::eval(execution_ctxt&) const
{
    return { shared_from_this() };
}

const std::vector<std::string>& tess::function::parameters() const
{
    return parameters_;
}

tess::expr_value tess::function::call(std::vector<expr_value> args, execution_ctxt& ctxt) const
{
    //TODO
    return expr_value();
}

tess::function::function(const std::vector<std::string>& params, const tile_def& tile_definition) :
    parameters_(params),
    body_(tile_definition)
{
}

tess::function::function(const std::vector<std::string>& params, const expr_ptr& body) :
    parameters_(params),
    body_(body)
{
}
