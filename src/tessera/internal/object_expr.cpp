#include "object_expr.h"
#include "execution_state.h"
#include "ops.h"
#include <numeric>

tess::var_expr::var_expr(const std::string& var) : 
    var_(var)
{
}

void tess::var_expr::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<get_var>());
    stack.push(stack_machine::variable(var_));
}

std::string tess::var_expr::to_string() const
{
    return var_;
}

void tess::var_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    dependencies.insert(var_);
}

tess::expr_ptr tess::var_expr::simplify() const
{
    return std::make_shared<var_expr>(var_);
}

/*--------------------------------------------------------------------------------*/

tess::placeholder_expr::placeholder_expr(int placeholder) :
    placeholder_(placeholder)
{
}


void tess::placeholder_expr::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<get_var>());
    stack.push(stack_machine::variable(placeholder_));
}

void tess::placeholder_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
}

tess::expr_ptr tess::placeholder_expr::simplify() const
{
    return std::make_shared<placeholder_expr>(placeholder_);
}

/*--------------------------------------------------------------------------------*/

tess::array_item_expr::array_item_expr(expr_ptr ary, expr_ptr index) :
    ary_(ary),
    index_(index)
{
}

std::string tess::array_item_expr::to_string() const
{
    return "( get_ary_item " + ary_->to_string() + " " + index_->to_string() + " )";
}

void tess::array_item_expr::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<get_ary_item_op>());
    ary_->compile(stack);
    index_->compile(stack);
}

void tess::array_item_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    ary_->get_dependencies(dependencies);
    index_->get_dependencies(dependencies);
}

tess::expr_ptr tess::array_item_expr::simplify() const
{
    return std::make_shared< array_item_expr>(
        ary_->simplify(),
        index_->simplify()
    );
}

/*--------------------------------------------------------------------------------*/

tess::func_call_expr::func_call_expr(expr_ptr func, const std::vector<expr_ptr>& args) :
    func_(func),
    args_(args)
{
}

void tess::func_call_expr::compile(stack_machine::stack& stack) const
{
    int n = static_cast<int>(args_.size());
    stack.push(std::make_shared<pop_eval_context>());
    stack.push(std::make_shared<call_func>(n));
    stack.push(std::make_shared<push_eval_context>());
    func_->compile(stack);
    stack.compile_and_push(args_);
}

std::string tess::func_call_expr::to_string() const
{
    std::vector<std::string> arg_strs(args_.size());
    std::transform(args_.begin(), args_.end(), arg_strs.begin(),
        [](const auto& e) {return e->to_string(); }
    );
    std::string args = std::accumulate(
        std::next(arg_strs.begin()),
        arg_strs.end(),
        arg_strs[0],
        [](std::string a, std::string b) {
            return a + " " + b;
        }
    );
    return "( apply " + func_->to_string() + " " + args + " )";
}

void tess::func_call_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    func_->get_dependencies(dependencies);
    for (const auto& arg : args_)
        arg->get_dependencies(dependencies);
}

tess::expr_ptr tess::func_call_expr::simplify() const
{
    std::vector<expr_ptr> simplified(args_.size());
    std::transform(args_.begin(), args_.end(), simplified.begin(),
        [](const auto& e) {return e->simplify(); }
    );
    return std::make_shared<func_call_expr>(
        func_->simplify(),
        simplified
    );
}

/*--------------------------------------------------------------------------------*/

tess::obj_field_expr::obj_field_expr(expr_ptr obj, std::string field, bool is_ref) :
    obj_(obj),
    field_(field),
    is_ref_(is_ref)
{
}

std::string tess::obj_field_expr::to_string() const
{
    if (!is_ref_)
        return "( get_field " + obj_->to_string() + " " + field_ + " )";
    else
        return "( get_field_ref " + obj_->to_string() + " " + field_ + " )";
}

tess::expr_ptr tess::obj_field_expr::get_object() const
{
    return obj_;
}

std::string tess::obj_field_expr::get_field() const
{
    return field_;
}

void tess::obj_field_expr::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<get_field_op>(field_, is_ref_));
    obj_->compile(stack);
}

void tess::obj_field_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    obj_->get_dependencies(dependencies);
}

tess::expr_ptr tess::obj_field_expr::simplify() const
{
    return std::make_shared< obj_field_expr>(
        obj_->simplify(),
        field_,
        is_ref_
    );
}
