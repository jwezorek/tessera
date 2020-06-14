#include "with_expr.h"

tess::with_expr::with_expr(const field_definitions& field_defs, expr_ptr body) :
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

tess::field_definitions::field_definitions(const std::vector<field_def>& assignments) :
    impl_(std::make_shared<std::vector<field_def>>(assignments))
{
}

void tess::field_definitions::compile(stack_machine::stack& stack) const
{
}

std::string tess::field_definitions::to_string() const
{
    return std::string();
}

tess::field_definitions tess::field_definitions::simplify() const
{
    std::vector<field_def> simplified(impl_->size());
    std::transform(impl_->begin(), impl_->end(), simplified.begin(),
        [](const field_def& fd) ->field_def {
            const auto& [lhs, rhs] = fd;
            std::vector<expr_ptr> simplified_lhs(lhs.size());
            std::transform(lhs.begin(), lhs.end(), simplified_lhs.begin(),
                [](auto ex) {
                    return ex->simplify();
                }
            );
            return { simplified_lhs, rhs->simplify() };
        }
    );
    return tess::field_definitions(simplified);
}

std::vector<std::string> tess::field_definitions::get_variables() const
{
    return {};
}

std::vector<tess::expr_ptr> tess::field_definitions::get_values() const
{
    return std::vector<expr_ptr>();
}

bool tess::field_definitions::empty() const
{
    return false;
}
