#include "statement.h"
#include "script_impl.h"
#include "tile_patch_impl.h"

tess::lay_statement::lay_statement(const lay_params& params) :
    tiles_(params.tiles),
    such_that_clauses_( params.such_that_clauses )
{
}

tess::lay_statement::lay_statement(const std::vector<obj_ref_ptr>& tiles) :
    tiles_(tiles)
{
}

tess::expr_value tess::lay_statement::execute( tess::execution_ctxt& ctxt ) const
{
    const auto& script = ctxt.script();
    auto tile_def = script.get_tile_prototype("triangle");
    auto val = tile_def->eval(ctxt);
    auto patch = make_tess_obj<tess::tile_patch> (
        std::make_shared<tess::tile_patch_impl>(
            std::vector<tess::tile>{ std::get<tess::tile>(val) }
        )
    );
    return { patch };
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

    if (!std::holds_alternative<bool>(condition_val))
		return tess::expr_value{ error("if condition must evaluate to a boolean") };

    if (std::get<bool>(condition_val))
        return then_clause_->execute(ctxt);
    else
        return else_clause_->execute(ctxt);
}

tess::let_statement::let_statement(const std::tuple<std::string, expr_ptr> params) :
	lhs_( std::get<0>(params) ),
	rhs_( std::get<1>(params) )
{
}

tess::expr_value tess::let_statement::execute(execution_ctxt&) const
{
	return tess::expr_value{ nil_val() };
}

std::string tess::let_statement::lhs() const
{
	return lhs_;
}

tess::expr_ptr tess::let_statement::rhs() const
{
	return rhs_;
}