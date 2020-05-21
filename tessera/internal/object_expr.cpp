#include "object_expr.h"
#include "execution_state.h"
#include "ops.h"

std::optional<int> tess::eval_integer_expr(const tess::expr_ptr& expr, tess::evaluation_context& ctxt)
{
    auto int_val = expr->eval(ctxt);
    if (!std::holds_alternative<tess::number>(int_val))
        return std::nullopt;
    return tess::to_int(std::get<tess::number>(int_val));
}

tess::var_expr::var_expr(const std::string& var) : 
    var_(var)
{
}

tess::expr_value tess::var_expr::eval(evaluation_context& ctx) const
{
    auto value = ctx.get(var_);

    // syntactic sugar: when evaluating a lambda with no parameters call the lambda
    // even if it does not have a trailing "()".
    // TODO: make it so it is possible to refer to a parameter less lambda by prepending
    // its name with "fn" as in "fn square_tile" 

    if (std::holds_alternative<tess::lambda>(value)) {
        auto lambda = std::get<tess::lambda>(value);
        if (lambda.parameters().size() == 0)
            return value.call(ctx.execution_state(),{});
    }

    return value;
}

void tess::var_expr::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<get_var>());
    stack.push(stack_machine::identifier(var_));
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

tess::expr_value tess::placeholder_expr::eval(evaluation_context& ctx) const
{
    return ctx.get(placeholder_);
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

tess::expr_value tess::array_item_expr::eval(evaluation_context& ctx) const
{
    auto maybe_index = eval_integer_expr(index_, ctx);
    if (!maybe_index.has_value())
        return { tess::error("array index evaluated to non-number") };

    return ary_->eval(ctx).get_ary_item(maybe_index.value());
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
    func_->compile(stack);
    stack.compile_and_push(args_);
    stack.push(std::make_shared<push_eval_context>());
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
    return "( call " + func_->to_string() + " " + args + " )";
}

tess::expr_value tess::func_call_expr::eval(evaluation_context& ctx) const
{
    std::vector<expr_value> args(args_.size());
    std::transform(args_.begin(), args_.end(), args.begin(), 
        [&ctx](auto expr) {return expr->eval(ctx);} );
    return func_->eval(ctx).call(ctx.execution_state(), args);
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

tess::obj_field_expr::obj_field_expr(expr_ptr obj, std::string field) :
    obj_(obj),
    field_(field)
{
}

tess::expr_value tess::obj_field_expr::eval(evaluation_context& ctx) const
{
    return obj_->eval(ctx).get_field(ctx.allocator(), field_);
}

void tess::obj_field_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    obj_->get_dependencies(dependencies);
}

tess::expr_ptr tess::obj_field_expr::simplify() const
{
    return std::make_shared< obj_field_expr>(
        obj_->simplify(),
        field_
    );
}
