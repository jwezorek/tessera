#include "ops.h"
#include "function_def.h"
#include "lambda.h"
#include "lambda_impl.h"
#include "allocator.h"
#include "variant_util.h"
#include "evaluation_context.h"
#include "lay_expr.h"
#include "tile_impl.h"
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

tess::make_lambda::make_lambda(const std::vector<std::string>& parameters, const std::vector<stack_machine::item>& body, int num_dependencies) : 
    op_1(num_dependencies*2),
    parameters_(parameters),
    body_(body)
{
}

tess::stack_machine::item tess::make_lambda::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto& alloc = contexts.top().allocator();
    try {
        auto closure = get_closure(operands);
        auto lambda = alloc.create<tess::lambda>(parameters_, body_, closure);
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
    ss << "<lambda-ify ( ";
    for (auto str : parameters_)
        ss << str << " ";
    ss << ") { ";
    auto body = body_;
    std::reverse(body.begin(), body.end());
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
        ss << " ";
    }
    ss << "} >";
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

        if (ident.name() == "triangle") {
            std::cout << "tri\n";
        }

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
    }
    catch (...) {
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

tess::neg_op::neg_op() : stack_machine::op_1(1)
{
}

tess::stack_machine::item tess::neg_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    tess::number val = std::get<tess::number>(std::get<expr_value>(operands[0]));
    return { expr_value{ -val } };
}

/*---------------------------------------------------------------------------------------------*/

tess::add_op::add_op(int args) : stack_machine::op_1(args)
{
}

tess::stack_machine::item tess::add_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    auto values = get_vector<expr_value>(operands.begin(), operands.end());
    auto nums = get_vector<tess::number>(values.begin(), values.end());
    tess::number sum = 0;
    for (const auto& num : nums)
        sum += num;
    return { expr_value{sum} };
}

/*---------------------------------------------------------------------------------------------*/

tess::pop_and_insert_fields_op::pop_and_insert_fields_op() : stack_machine::op_1(1)
{
}

tess::stack_machine::item tess::pop_and_insert_fields_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    if (contexts.empty())
        return { tess::error("context stack underflow") };
    auto scope = contexts.top().pop_scope();

    auto obj = std::get<expr_value>(operands[0]);
    for (const auto& [var, val] : scope)
        obj.insert_field(var, val);

    return { obj };
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

tess::dup_op::dup_op() : stack_machine::op_multi(1)
{
}

std::variant<std::vector<tess::stack_machine::item>, tess::error> tess::dup_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    return std::vector<tess::stack_machine::item> {
        operands[0],
        operands[0]
    };
}

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
        //TODO: insert self-reference if val is a lambda
        current_scope.set(vars[0].identifier(), value);
    } else {
        //TODO: insert self-reference if val is a lambda
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

tess::get_field_op::get_field_op(const std::string& field) : 
    stack_machine::op_1(1), field_(field)
{
}

tess::stack_machine::item tess::get_field_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
   expr_value val = std::get<expr_value>(operands[0]);
   return { val.get_field(contexts.top().allocator(), field_) };
}

std::string tess::get_field_op::to_string() const
{
    return std::string("<get_field ") + field_ + ">";
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
        flatten_tiles_and_patches(ctxt.allocator(), layees.values())
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

tess::expr_value tess::lay_op::flatten_tiles_and_patches( tess::allocator& allocator, const std::vector<expr_value>& pieces) const
{
    return{
        allocator.create<tile_patch>(
            tess::flatten_tiles_and_patches(pieces)
        )
    };
}

tess::val_func_op::val_func_op(int n, std::function<expr_value(const std::vector<expr_value> & v)> func, std::string name) :
    stack_machine::op_1(n), name_(name), func_(func)
{
}

tess::stack_machine::item tess::val_func_op::execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const
{
    std::vector<expr_value> args = get_vector<expr_value>(operands.begin(), operands.end());
    return { func_(args) };
}
