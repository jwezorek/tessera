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

tess::expr_value tess::lay_statement::execute(tess::execution_ctxt&) const
{
	return tess::expr_value{ nil_val() };
}

tess::if_statement::if_statement(const if_params& params) :
    condition_(params.condition),
    then_clause_(params.then_clause),
    else_clause_(params.else_clause)
{
}

tess::expr_value tess::if_statement::execute(tess::execution_ctxt& ctxt) const
{
    auto condition_val = condition_->eval(ctxt);
    if (std::holds_alternative<error>(condition_val))
        return condition_val;

    if (!std::holds_alternative<bool_val>(condition_val))
		return tess::expr_value{ error("if condition must evaluate to a boolean") };

    if (std::get<bool_val>(condition_val).value())
        return then_clause_->execute(ctxt);
    else
        return else_clause_->execute(ctxt);
}
