#include "with_expr.h"

tess::with_expr::with_expr(const std::vector<std::tuple<expr_ptr, expr_ptr>>& field_defs, expr_ptr body) :
    field_defs_(field_defs),
    body_(body)
{
}

void tess::with_expr::compile(stack_machine::stack& stack) const
{
}

std::string tess::with_expr::to_string() const
{
    return std::string();
}

tess::expr_ptr tess::with_expr::simplify() const
{
    return expr_ptr();
}

void tess::with_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
}
