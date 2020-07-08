#include "ops.h"
#include "function_def.h"
#include "lambda.h"
#include "lambda_impl.h"
#include "allocator.h"
#include "variant_util.h"
#include "evaluation_context.h"
#include "lay_expr.h"
#include "tile_impl.h"
#include "cluster.h"
#include "tile_patch_impl.h"
#include "tessera/tile_patch.h"
#include <sstream>

namespace {

    tess::scope_frame get_closure(const std::vector<tess::stack_machine::item>& operands)
    {
        tess::scope_frame frame;
        int i = 0;
        while (i < operands.size()) {
            auto var = std::get<tess::stack_machine::variable>(operands[i]);
            auto val = std::get<tess::expr_value>(operands[i + 1]);
            frame.set(var.name(), val);
            i += 2;
        }
        return frame;
    }
}

tess::make_lambda::make_lambda(const std::vector<std::string>& parameters, const std::vector<stack_machine::item>& body, const std::vector<std::string>& deps) :
    op_1(0),
    parameters_(parameters),
    body_(body),
    dependencies_(deps)
{
}

tess::stack_machine::item tess::make_lambda::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto& ctxt = contexts.top();
    auto& alloc = contexts.top().allocator();
    try {
        auto lambda = alloc.create<tess::lambda>(parameters_, body_, dependencies_);

        for (auto dependency : dependencies_)
            if (ctxt.has(dependency))
                lambda.insert_field(dependency, ctxt.get(dependency));

        return  make_expr_val_item(lambda) ;
    }  catch (tess::error e) {
        return { e };
    } catch (...) {
        return { tess::error("bad make_lamba op") };
    }
}

std::string tess::make_lambda::to_string() const
{
    std::stringstream ss;
    ss << "<lambda-ify (deps " << std::to_string(number_of_args_/2) << ") ( ";
    for (auto str : parameters_)
        ss << str << " ";
    ss << ")> { ";
    auto body = body_;
    for (const auto& it : body) {
        std::visit(
            overloaded{
                [&](stack_machine::op_ptr op) {
                    ss << op->to_string();
                },
                [&](const auto& val) {
                    ss << val.to_string();
                }
            },
            it
        );
        ss << "; ";
    }
    ss << "}";
    return ss.str();
}

/*---------------------------------------------------------------------------------------------*/

tess::get_var::get_var(bool eval) : op_multi(1), eval_parameterless_funcs_(eval)
{
}

std::variant<std::vector<tess::stack_machine::item>, tess::error> tess::get_var::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto& ctxt = contexts.top();
    try {
        auto ident = std::get<stack_machine::variable>(operands[0]);
        auto value = ctxt.get(ident.identifier());

        if (eval_parameterless_funcs_ && std::holds_alternative<lambda>(value)) {
            auto lambda_val = std::get<lambda>(value);
            if (lambda_val.parameters().empty()) {
                return std::vector<tess::stack_machine::item>{
                    { std::make_shared<push_eval_context>() },
                    { value },
                    { std::make_shared<call_func>(0) },
                    { std::make_shared<pop_eval_context>() }
                };
            }
        }

        return std::vector<tess::stack_machine::item>{ {value} };

    } catch (tess::error e) {
        return { e };
    } catch (...) {
        return { tess::error("bad get_var op") };
    }
}

/*---------------------------------------------------------------------------------------------*/

/*
tess::make_scope_frame::make_scope_frame(int n) : op_1(2*n)
{
}

tess::stack_machine::item tess::make_scope_frame::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    int n = number_of_args_/2;
    std::vector<std::string> vars(n);
    std::vector<expr_value> vals(n);
    try {
        for (int i = 0; i < n; i += 2) {
            vars[i] = std::get<stack_machine::identifier>(operands[i]).name;
            vals[i] = std::get<expr_value>(operands[i + 1]);
        }
        return { scope_frame(vars, vals) };
    } catch (tess::error e) {
        return { e };
    } catch (...) {
        return { tess::error("bad get_var op") };
    }
}
*/

/*---------------------------------------------------------------------------------------------*/

tess::pop_eval_context::pop_eval_context() : op_0(0)
{
}

std::optional<tess::error> tess::pop_eval_context::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    contexts.pop();
    return std::nullopt;
}

/*---------------------------------------------------------------------------------------------*/

std::variant<std::vector<tess::stack_machine::item>, tess::error> tess::call_func::execute(const std::vector<tess::stack_machine::item>& operands, tess::stack_machine::context_stack& contexts) const
{
    try {
        lambda func = std::get<lambda>(std::get<expr_value>(operands[0]));
        std::vector<expr_value> args = get_vector<expr_value>(operands.begin() + 1, operands.end());

        if (func.parameters().size() != args.size())
            return tess::error("func call arg count mismatch.");

        scope_frame frame(func.parameters(), args);
        for (const auto& [var, val] : func.closure()) 
            frame.set(var, val);
        contexts.top().push_scope(frame);

        return func.body();

    }  catch (tess::error e) {
        return e;
    } catch (...) {
        return tess::error("bad function call op");
    }

    return std::vector<tess::stack_machine::item>();
}

tess::call_func::call_func(int num_args) : op_multi(num_args+1)
{
}

/*---------------------------------------------------------------------------------------------*/

tess::push_eval_context::push_eval_context() : stack_machine::op_0(0)
{
}

std::optional<tess::error> tess::push_eval_context::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const 
{
    auto& ctxt = contexts.top();
    contexts.push(ctxt.execution_state().create_eval_context());
    return std::nullopt;
}

/*---------------------------------------------------------------------------------------------*/

tess::pop_frame_op::pop_frame_op() : tess::stack_machine::op_0(0)
{
}

std::optional<tess::error> tess::pop_frame_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    if (contexts.empty())
        return tess::error("context stack underflow");

    if (contexts.top().empty())
        return tess::error("eval context underflow");

    auto scope = contexts.top().pop_scope();
    return std::nullopt;
}

/*---------------------------------------------------------------------------------------------*/

tess::push_frame_op::push_frame_op() : tess::stack_machine::op_0(0)
{
}

std::optional<tess::error> tess::push_frame_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    if (contexts.empty())
        return tess::error("context stack underflow");
    contexts.top().push_scope(scope_frame());
    return std::nullopt;
}

/*---------------------------------------------------------------------------------------------*/

tess::assign_op::assign_op(int num_vars) : stack_machine::op_0(num_vars+1)
{
}

std::optional<tess::error> tess::assign_op::execute(const std::vector<tess::stack_machine::item>& operands, tess::stack_machine::context_stack& contexts) const
{
    int num_vars = static_cast<int>(operands.size() - 1);
    auto value = std::get<expr_value>(operands.back());
    std::vector<stack_machine::variable> vars(num_vars);
    std::transform(operands.begin(), operands.begin() + num_vars, vars.begin(),
        [](const auto& item) { return std::get<stack_machine::variable>(item); }
    );

    auto& current_scope = contexts.top().peek();
    if (num_vars == 1) {
        current_scope.set(vars[0].identifier(), value);
    } else {
        int i = 0;
        for (const auto& var : vars) 
            current_scope.set(var.identifier(), value.get_ary_item(i++));
    }

    return std::nullopt;
}

tess::one_param_op::one_param_op(std::function<expr_value(tess::allocator&, const expr_value&)> func, std::string name) :
    stack_machine::op_1(1), func_(func), name_(name)
{
}

tess::stack_machine::item tess::one_param_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    return {
        func_(contexts.top().allocator(), std::get<expr_value>(operands[0]))
    };
}

tess::get_field_op::get_field_op(const std::string& field, bool get_ref) : 
    stack_machine::op_1(1), field_(field), get_ref_(get_ref)
{
}

tess::stack_machine::item tess::get_field_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    expr_value val = std::get<expr_value>(operands[0]);
    if (!get_ref_) {
        return { val.get_field(contexts.top().allocator(), field_) };
    }
    else {
        return { expr_value{field_ref(val, field_)} };
    }
}

std::string tess::get_field_op::to_string() const
{
    if (!get_ref_)
        return std::string("<get_field ") + field_ + ">";
    else
        return std::string("<get_field_ref ") + field_ + ">";
}

tess::lay_op::lay_op(int num_mappings) : stack_machine::op_1(2*num_mappings)
{
}

tess::stack_machine::item tess::lay_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{ 
    auto& ctxt = contexts.top();
    auto layees = ctxt.pop_scope();
    auto result = apply_mapping(operands);
    return  {
        expr_value {
            tess::flatten(ctxt.allocator(), layees.values())
        }
    };

}

std::string tess::lay_op::to_string() const
{
    return std::string("<lay " + std::to_string(number_of_args_/2) + ">");
}

std::optional<tess::error> tess::lay_op::apply_mapping(const std::vector<stack_machine::item>& operands) const
{
    auto values = get_vector<expr_value>(operands.begin(), operands.end());
    std::vector<std::tuple<edge::impl_type*, edge::impl_type*>> edge_to_edge;

    for (int i = 0; i < values.size(); i += 2) {
        auto edge1 = get_impl(std::get<edge>(values[i]));
        auto edge2 = get_impl(std::get<edge>(values[i+1]));
        edge_to_edge.push_back({ edge1, edge2} );
    }

    return tess::apply_mapping(edge_to_edge);
}

tess::val_func_op::val_func_op(int n, std::function<expr_value(tess::allocator& a, const std::vector<expr_value> & v)> func, std::string name) :
    stack_machine::op_1(n), name_(name), func_(func)
{
}

tess::stack_machine::item tess::val_func_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    std::vector<expr_value> args = get_vector<expr_value>(operands.begin(), operands.end());
    return { func_( contexts.top().allocator(), args ) };
}

tess::if_op::if_op(const std::vector<stack_machine::item>& if_clause, const std::vector<stack_machine::item>& else_clause) :
    stack_machine::op_multi(1), if_(if_clause), else_(else_clause)
{
}

std::variant<std::vector<tess::stack_machine::item>, tess::error> tess::if_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    bool cond = std::get<bool>(std::get<expr_value>(operands[0]));
    return (cond) ?
        if_ :
        else_;
}

std::string tess::if_op::to_string() const
{
    std::stringstream ss;
    ss << "<if> {";
    for (const auto& it : if_) {
        std::visit(
            overloaded{
                [&](stack_machine::op_ptr op) {
                    ss << op->to_string();
                },
                [&](const auto& val) {
                    ss << val.to_string();
                }
            },
            it
        );
        ss << "; ";
    }
    ss << "} {";
    for (const auto& it : else_) {
        std::visit(
            overloaded{
                [&](stack_machine::op_ptr op) {
                    ss << op->to_string();
                },
                [&](const auto& val) {
                    ss << val.to_string();
                }
            },
            it
        );
        ss << "; ";
    }
    ss << "}";
    return ss.str();
}

tess::get_ary_item_op::get_ary_item_op() : stack_machine::op_1(2)
{
}

tess::stack_machine::item tess::get_ary_item_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto ary = std::get<expr_value>(operands[0]);
    auto index = std::get<tess::number>(std::get<expr_value>(operands[1]));
    return { ary.get_ary_item(tess::to_int(index)) };
}


std::vector<tess::stack_machine::item> tess::iterate_op::start_next_item(int index, tess::cluster& ary) const
{
    std::vector<stack_machine::item> output{
        { expr_value{number(index)} },
        { expr_value{ary} },
        { std::make_shared<get_ary_item_op>() },
        { stack_machine::variable(index_var_) },
        { std::make_shared<assign_op>(1) }
    };
    std::copy(body_.begin(), body_.end(), std::back_inserter(output));
    return output;
}

tess::iterate_op::iterate_op(std::string index_var, int index_val, const std::vector<stack_machine::item>& body) :
    stack_machine::op_multi(3),
    index_var_(index_var),
    index_val_(index_val),
    body_(body)
{
}

std::variant<std::vector<tess::stack_machine::item>, tess::error> tess::iterate_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto& alloc = contexts.top().allocator();
    auto src = std::get<tess::cluster>(std::get<expr_value>( operands[0] ));
    auto* dst = (index_val_ > -1) ? get_impl(std::get<tess::cluster>(std::get<expr_value>(operands[1]))) : nullptr;
    auto curr_item = std::get<expr_value>( operands[2] );
    
    auto n = src.count();

    // we could clone the cluster::impl_type here but that would make iteration n^2
    // so we will just remove const-ness so we can reuse the same cluster implementation.
    // The other option would be to make operands non-const but right now
    // this is the only place where operands const-ness is an issue.

    if (dst) 
        const_cast<cluster::impl_type*>(dst)->push_value(curr_item);
    auto i = index_val_ + 1;
    
    if (i >= n)
        return std::vector<tess::stack_machine::item>{ operands[1] };

    auto next_iteration = start_next_item( i, src );

    stack_machine::item new_dst;
    if (!dst)
        new_dst = { expr_value{ alloc.create<cluster>(std::vector<expr_value>{}) } };
    else
        new_dst = operands[1];

    next_iteration.push_back( new_dst );
    next_iteration.push_back( operands[0] );
    next_iteration.push_back({ std::make_shared<iterate_op>(index_var_, i, body_) });

    return next_iteration;
}


std::string tess::iterate_op::to_string() const
{
    std::stringstream ss;
    ss << "<iterate " << index_var_ << " " << std::to_string(index_val_) << " {";
    for (auto it : body_)
        ss << it.to_string() << ";";
    ss << "}>";
    return ss.str();
}

tess::set_dependencies_op::set_dependencies_op() : stack_machine::op_0(0)
{
}

std::optional<tess::error> tess::set_dependencies_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto& ctxt = contexts.top();
    auto& frame = ctxt.peek();
    for (auto& val : frame.values()) {
        if (std::holds_alternative<lambda>(val)) {
            std::vector<std::string> vars;
            auto& func = std::get<tess::lambda>(val);
            for (const auto& var : func.unfulfilled_dependencies())   
                func.insert_field(var, ctxt.get(var));
        }
    }
    return std::nullopt;
}

tess::set_field_op::set_field_op(int num_fields) : stack_machine::op_0(num_fields + 1)
{
}

std::optional<tess::error> tess::set_field_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    int num_fields = static_cast<int>(operands.size() - 1);
    auto value = std::get<expr_value>(operands.back());
    std::vector<tess::field_ref> field_refs(num_fields);
    std::transform(operands.begin(), operands.begin() + num_fields, field_refs.begin(),
        [](const auto& item) { 
            return std::get<field_ref>(std::get<expr_value>(item));
        }
    );

    if (num_fields == 1) {
        field_refs[0].set(value);
    } else {
        int i = 0;
        for (auto& field_ref : field_refs) {
            field_ref.set(value.get_ary_item(i++));
        }
    }

    return std::nullopt;
}
