#include "statement.h"

tess::lay_statement::lay_statement(const lay_params& params) :
    layees_(params.layees),
    such_that_clauses_( params.such_that_clauses )
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
