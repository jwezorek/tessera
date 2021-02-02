#include "cluster_expr.h"
#include "object_expr.h"
#include "value.h"
#include "cluster.h"
#include "gc_heap.h"
#include "execution_state.h"
#include "ops.h"

tess::cluster_expr::cluster_expr(const std::vector<expr_ptr>& exprs) :
    exprs_(exprs)
{
}

void tess::cluster_expr::compile(stack_machine::stack& stack) const
{
    int n = static_cast<int>(exprs_.size());
    stack.push(
        std::make_shared<val_func_op>(
            n,
            [](gc_heap& a, const std::vector<value_>& values)->value_ {
                return value_(a.make_const<const_cluster_ptr>(values));
            },
            "<make_cluster " + std::to_string(n) + ">"
        )
    );
    for (auto e : exprs_)
        e->compile(stack);
}

tess::expr_ptr tess::cluster_expr::simplify() const
{
    std::vector<expr_ptr> simplified(exprs_.size());
    std::transform(exprs_.begin(), exprs_.end(), simplified.begin(),
         [](expr_ptr e) {
             return e->simplify();
         }
    );
    return std::make_shared<tess::cluster_expr>(simplified);
}

std::string tess::cluster_expr::to_string() const
{
    std::stringstream ss;
    for (const auto& e : exprs_)
        ss << e->to_string() << " ";
    return "( cluster " + ss.str() + ")";
}

void tess::cluster_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    for (const auto& e : exprs_)
        e->get_dependencies(dependencies);
}

/*---------------------------------------------------------------------------------------------------------*/

tess::num_range_expr::num_range_expr(expr_ptr from, expr_ptr to) :
    from_(from), to_(to)
{
}

tess::num_range_expr::num_range_expr(const std::tuple<expr_ptr, expr_ptr>& tup) :
    num_range_expr(std::get<0>(tup), std::get<1>(tup))
{
}

void tess::num_range_expr::compile(stack_machine::stack& stack) const
{
    stack.push(
        std::make_shared<val_func_op>(
            2,
            [](gc_heap& a, const std::vector<value_>& values)->value_ {
                int from = to_int(std::get<number>( values[0] ));
                int to = to_int(std::get<number>( values[1] ));
                int n = (to >= from) ? to - from + 1 : 0;

                std::vector<value_> range;
                if (n == 0)
                    return value_(a.make_const<const_cluster_ptr>(range));
                range.reserve(n);

                for (int i = from; i <= to; i++)
                    range.push_back(value_( tess::number(i) ));

                return value_(a.make_const<const_cluster_ptr>(range) );
            },
            "<make_range>"
        )
    );
    from_->compile(stack);
    to_->compile(stack);
}

tess::expr_ptr tess::num_range_expr::simplify() const
{
    return std::make_shared<num_range_expr>(from_->simplify(), to_->simplify());
}

std::string tess::num_range_expr::to_string() const
{
    return "( range " + from_->to_string() + " " + to_->to_string() + " )";
}

/*---------------------------------------------------------------------------------------------------------*/

void tess::num_range_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    from_->get_dependencies(dependencies);
    to_->get_dependencies(dependencies);
}

tess::cluster_comprehension_expr::cluster_comprehension_expr(expr_ptr ex, const std::string& var, expr_ptr range_expr) :
    item_expr_(ex), var_(var), range_expr_(range_expr)
{
}

tess::cluster_comprehension_expr::cluster_comprehension_expr(std::tuple<expr_ptr, std::string, expr_ptr> tup) :
    cluster_comprehension_expr(std::get<0>(tup), std::get<1>(tup), std::get<2>(tup))
{
}

std::string tess::cluster_comprehension_expr::to_string() const
{
    return "( cluster_comprehension " + 
        var_ + 
        " " + 
        item_expr_->to_string() + 
        " " + 
        range_expr_->to_string() + 
        " )";
}

void tess::cluster_comprehension_expr::compile(stack_machine::stack& stack) const
{
    stack_machine::stack body;
    item_expr_->compile(body);
    stack.push(std::make_shared<pop_frame_op>());
    stack.push(std::make_shared<iterate_op>(var_, -1, body.pop_all()));
    range_expr_->compile(stack);
    stack.push(value_());
    stack.push(value_());
    stack.push(std::make_shared<push_frame_op>());
}

tess::expr_ptr tess::cluster_comprehension_expr::simplify() const
{
    return std::make_shared<cluster_comprehension_expr>(
        item_expr_->simplify(),
        var_,
        range_expr_->simplify()
    );
}

void tess::cluster_comprehension_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    std::unordered_set<std::string> item_expr_dependencies;
    item_expr_->get_dependencies(item_expr_dependencies);
    for (const auto& var : item_expr_dependencies)
        if (var != var_)
            dependencies.insert(var);
    range_expr_->get_dependencies(dependencies);
}

tess::map_expr::map_expr(expr_ptr lambda, expr_ptr cluster) :
    lambda_(lambda), cluster_(cluster)
{
}

std::string tess::map_expr::to_string() const
{
    return std::string();
}

void tess::map_expr::compile(stack_machine::stack& stack) const
{
    //TODO
}

tess::expr_ptr tess::map_expr::simplify() const
{
    return std::make_shared<map_expr>(lambda_->simplify(), cluster_->simplify());
}

void tess::map_expr::get_dependencies(std::unordered_set<std::string>& dependencies) const
{
    lambda_->get_dependencies(dependencies);
    cluster_->get_dependencies(dependencies);
}
