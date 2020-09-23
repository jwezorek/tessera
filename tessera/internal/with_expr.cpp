#include "with_expr.h"
#include "ops.h"
#include "expression.h"
#include "object_expr.h"
#include "parser/keywords.h"

namespace {
    void compile_field_def(tess::stack_machine::stack& stack, tess::field_definitions::field_def_pair fd) {
        const auto& [fields, val] = fd;
        stack.push(std::make_shared<tess::set_field_op>(static_cast<int>(fields.size())));
        for (const auto& field : fields)
            field->compile(stack);
        val->compile(stack);
    }
}

tess::with_expr::with_expr(const field_definitions& field_defs, expr_ptr body) :
    field_defs_(field_defs),
    body_(body)
{
}

tess::with_expr::with_expr(with_expr_params params) :
    field_defs_(std::get<1>(params)),
    body_(std::get<0>(params))
{
}

void tess::with_expr::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<pop_frame_op>());
    stack.push(std::make_shared<get_var>());
    stack.push(tess::stack_machine::variable("this"));
    field_defs_.compile(stack);
    stack.push(std::make_shared<tess::assign_op>(1));
    stack.push(tess::stack_machine::variable("this"));
    body_->compile(stack);
    stack.push(std::make_shared<push_frame_op>());
}

std::string tess::with_expr::to_string() const
{
    return "( with " + field_defs_.to_string() + " " + body_->to_string() + " )";
}

tess::expr_ptr tess::with_expr::simplify() const
{
    return std::make_shared<with_expr>(field_defs_.simplify(), body_->simplify());
}

void tess::with_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
}

tess::field_definitions::field_definitions(const std::vector<field_def_pair>& defs_with_verbs) :
    impl_(std::make_shared<std::vector<field_def_pair>>(defs_with_verbs))
{

}

tess::field_definitions::field_definitions(const std::vector<field_def>& defs_with_verbs) 
{
    using namespace parser;
    std::vector<field_def_pair> defs(defs_with_verbs.size());
    std::transform(defs_with_verbs.begin(), defs_with_verbs.end(), defs.begin(),
        [](const field_def& triple)->field_def_pair {
            auto const& [lhs, verb, rhs] = triple;
            if (verb == keyword(kw::on))
                return { lhs, std::make_shared<tess::on_expr>(std::make_shared<tess::var_expr>(keyword(kw::this_)), rhs) };
            else
                return {lhs, rhs};
        }
    );
    impl_ = std::make_shared<std::vector<field_def_pair>>(defs);
}

void tess::field_definitions::compile(stack_machine::stack& stack) const
{
    for (auto i = impl_->rbegin(); i != impl_->rend(); ++i) {
        const auto& fd = *i;
        compile_field_def(stack, fd);
    }
}

std::string tess::field_definitions::to_string() const
{
    std::stringstream contents;
    for (const auto& fields_val : *impl_) {
        const auto& [fields, val] = fields_val;
        if (fields.size() > 1) {
            contents << "(( ";
            for (const auto& field : fields)
                contents << field->to_string() << " ";
            contents << ") " << val->to_string() << ")";
        } else {
            contents << "( " << fields[0]->to_string() << " " << val->to_string() << " )";
        }
    }

    return "( define " + contents.str() + " )";
}

tess::field_definitions tess::field_definitions::simplify() const
{
    if (empty())
        return {};
    std::vector<field_def_pair> simplified(impl_->size());
    std::transform(impl_->begin(), impl_->end(), simplified.begin(),
        [](const field_def_pair& fd) ->field_def_pair {
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

void tess::field_definitions::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    if (empty())
        return;

    std::unordered_set<std::string> deps;
    for (const auto& [lhs, rhs] : *impl_) {
        for (const auto& v : lhs)
            v->get_dependencies(deps);
        rhs->get_dependencies(deps);
    }
    if (deps.find("this") != deps.end())
        deps.erase("this");
    std::copy(deps.begin(), deps.end(), std::inserter(dependencies, dependencies.end()));
}

bool tess::field_definitions::empty() const
{
    return !impl_ || impl_->empty();
}
