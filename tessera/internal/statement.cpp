#include "statement.h"

tess::lay_statement::lay_statement(const lay_params& params) :
    tiles_(params.tiles),
    such_that_clauses_( params.such_that_clauses )
{
}

tess::lay_statement::lay_statement(const std::vector<obj_ref_ptr>& tiles) :
    tiles_(tiles)
{
}

void tess::lay_statement::execute(exec_ctxt&) const
{
}

tess::if_statement::if_statement(const if_params& params) :
    condition_(params.condition),
    then_clause_(params.then_clause),
    else_clause_(params.else_clause)
{
}

void tess::if_statement::execute(exec_ctxt&) const
{
}
