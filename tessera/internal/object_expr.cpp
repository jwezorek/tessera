#include "object_expr.h"

tess::var_expr::var_expr(const std::string& var) : 
    var_(var)
{
}

tess::expr_value tess::var_expr::eval(execution_ctxt& ctx) const
{
    return expr_value();
}

/*--------------------------------------------------------------------------------*/

tess::placeholder_expr::placeholder_expr(int placeholder) :
    placeholder_(placeholder)
{
}

tess::expr_value tess::placeholder_expr::eval(execution_ctxt& ctx) const
{
    return expr_value();
}

/*--------------------------------------------------------------------------------*/

tess::array_item_expr::array_item_expr(expr_ptr ary, expr_ptr index) :
    ary_(ary),
    index_(index)
{
}

tess::expr_value tess::array_item_expr::eval(execution_ctxt& ctx) const
{
    return expr_value();
}

/*--------------------------------------------------------------------------------*/

tess::func_call_expr::func_call_expr(expr_ptr func, const std::vector<expr_ptr>& args) :
    func_(func),
    args_(args)
{
}

tess::expr_value tess::func_call_expr::eval(execution_ctxt& ctx) const
{
    return expr_value();
}

/*--------------------------------------------------------------------------------*/

tess::obj_field_expr::obj_field_expr(expr_ptr obj, std::string field) :
    obj_(obj),
    field_(field)
{
}

tess::expr_value tess::obj_field_expr::eval(execution_ctxt& ctx) const
{
    return expr_value();
}
