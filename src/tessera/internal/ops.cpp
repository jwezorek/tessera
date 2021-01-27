#include "ops.h"
#include "function_def.h"
#include "lambda.h"
#include "lambda_impl.h"
#include "gc_heap.h"
#include "variant_util.h"
#include "evaluation_context.h"
#include "lay_expr.h"
#include "tile_impl.h"
#include "cluster.h"
#include "field_ref.h"
#include "tile_patch_impl.h"
#include "tessera/tile_patch.h"
#include <sstream>

namespace {

    using edge_parent_type = std::variant<tess::tile_ptr, tess::patch_ptr>;

    edge_parent_type parent_of_edge( tess::edge::impl_type& e) {
        auto tile = e.parent();
        if (!tile->has_parent())
            return tile;
        else
            return tile->parent();
    }

    void apply_edge_matrix(const edge_parent_type& ep, const tess::matrix& mat) {
        return std::visit([&mat](auto ptr) { ptr->apply(mat); }, ep);
    }

    tess::matrix edge_to_edge_matrix(const tess::edge::impl_type& e1, const tess::edge::impl_type& e2)
    {
        auto u1 = e1.u();
        auto v1 = e1.v();
        auto u2 = e2.u();
        auto v2 = e2.v();

        return tess::line_seg_to_line_seg({ u1->pos() , v1->pos() }, { v2->pos() , u2->pos() });
    }

    tess::obj_id get_key(edge_parent_type obj)
    {
        return std::visit([](auto p)->tess::obj_id {return p->get_id(); }, obj);
    }

    bool is_untouched(edge_parent_type obj, const std::unordered_set<tess::obj_id>& moved)
    {
        return moved.find(get_key(obj)) == moved.end();
    }

    void touch(edge_parent_type obj, std::unordered_set<tess::obj_id >& moved)
    {
        moved.insert(get_key(obj));
    }

    using edge_mapping = std::tuple<tess::const_edge_ptr, tess::const_edge_ptr>;

    std::optional<tess::error> apply_edge_mapping(const edge_mapping& mapping, std::unordered_set<tess::obj_id>& moved)
    {
        auto [ptr_edge1, ptr_edge2] = mapping;
        auto edge1 = *ptr_edge1;
        auto edge2 = *ptr_edge2;
        auto parent_1 = parent_of_edge(edge1);
        auto parent_2 = parent_of_edge(edge2);

        if (parent_1 == parent_2)
            return tess::error("edge mapping internal to a single tile");

        bool both_are_touched = !is_untouched(parent_1, moved) && !is_untouched(parent_2, moved);
        bool both_are_untouched = is_untouched(parent_1, moved) && is_untouched(parent_2, moved);
        bool just_parent2_is_untouched = !is_untouched(parent_1, moved) && is_untouched(parent_2, moved);

        if (both_are_touched)
            return tess::error("TODO: this should verify the two edges lie on each other and error out if not");

        if (both_are_untouched || just_parent2_is_untouched) {
            //move tile2
            apply_edge_matrix(parent_2, edge_to_edge_matrix(edge2, edge1));
            touch(parent_2, moved);
        }
        else {
            //move tile1
            apply_edge_matrix(parent_1, edge_to_edge_matrix(edge1, edge2));
            touch(parent_1, moved);
        }

        return std::nullopt;
    }
}

std::optional<tess::error> apply_mapping(const std::vector<edge_mapping>& mappings)
{
    std::unordered_set<tess::obj_id> moved;
    for (const auto& mapping : mappings) {
        auto result = apply_edge_mapping(mapping, moved);
        if (result.has_value()) {
            return { result.value() };
        }
    }
    return std::nullopt;
}

tess::make_lambda::make_lambda(const std::vector<std::string>& parameters, const std::vector<stack_machine::item>& body, const std::vector<std::string>& deps) :
    op_1(0),
    parameters_(parameters),
    body_(body),
    dependencies_(deps)
{
}

tess::stack_machine::item tess::make_lambda::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    auto& ctxt = contexts.top();
    auto& alloc = contexts.top().allocator();
    try {
        auto lambda = alloc.make_mutable<tess::const_lambda_ptr>(parameters_, body_, dependencies_);

        for (auto dependency : dependencies_)
            if (ctxt.has(dependency))
                lambda->insert_field(dependency, ctxt.get(dependency));

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
        it.visit(
            overloaded{
                [&](stack_machine::op_ptr op) {
                    ss << op->to_string();
                },
                [&](value_ v) {
                    ss << tess::to_string(v);
                },
                [&](const auto& val) {
                    ss << val.to_string();
                }
            }
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

std::vector<tess::stack_machine::item> tess::get_var::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    auto& ctxt = contexts.top();
    auto ident = std::get<stack_machine::variable>(operands[0]);
    auto value = ctxt.get(ident.name());

    if (eval_parameterless_funcs_ && std::holds_alternative<const_lambda_ptr>(value)) {
        auto lambda_val = std::get<const_lambda_ptr>(value);
        if (lambda_val->parameters.empty()) {
            return std::vector<tess::stack_machine::item>{
                { std::make_shared<push_eval_context>() },
                { value },
                { std::make_shared<call_func>(0) },
                { std::make_shared<pop_eval_context>() }
            };
        }
    }

    return std::vector<tess::stack_machine::item>{ {value} };
}

/*---------------------------------------------------------------------------------------------*/

tess::pop_eval_context::pop_eval_context() : op_0(0)
{
}

void tess::pop_eval_context::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    contexts.pop();
}

/*---------------------------------------------------------------------------------------------*/

std::string serialize_func_call(const tess::const_lambda_ptr& func, const std::vector<tess::value_>& args) {
    std::stringstream ss;
    ss << tess::serialize( {func} );
    for (const auto& v : args) {
        auto v_str = tess::serialize(v);
        if (v_str.empty())
            return {};
        ss << " " << v_str;
    }
    return ss.str();
}

std::vector<tess::stack_machine::item> tess::call_func::execute(const std::vector<tess::stack_machine::item>& operands, tess::context_stack& contexts) const
{
    //TODO: make a function that tests a stackitem for an expr_val containing type and throws if not there
    if (!std::holds_alternative<value_>(operands[0]) || !std::holds_alternative<const_lambda_ptr>(std::get<value_>(operands[0])))
        throw tess::error("Attempted to evaluate non-lambda");

    const_lambda_ptr func = std::get<const_lambda_ptr>(std::get<value_>(operands[0]));
    std::vector<value_> args = get_vector<value_>(operands.begin() + 1, operands.end());

    auto key = serialize_func_call(func, args);
    auto& memo_tbl = contexts.memos();

    //std::cout << key << "\n";
    //std::cout << memo_tbl.size() << "\n";

    if ((key.empty()) || (!memo_tbl.contains(key))) {

        if (func->parameters.size() != args.size())
            throw tess::error("func call arg count mismatch.");

            scope_frame frame(func->parameters, args);
            for (const auto& [var, val] : func->closure)
                frame.set(var, val);
            contexts.top().push_scope(frame);

            auto func_body = func->body;
            func_body.push_back(
                { std::make_shared<memoize_func_call_op>(key) }
        );

        return func_body;
    } else {
        std::cout << "memoization hit\n";
        auto memo_val = memo_tbl.get(key);
        auto& heap = contexts.top().allocator();
        return std::vector<tess::stack_machine::item>{ {tess::clone_value(heap, memo_val)} };
    }
}

tess::call_func::call_func(int num_args) : op_multi(num_args+1)
{
}

/*---------------------------------------------------------------------------------------------*/

tess::push_eval_context::push_eval_context() : stack_machine::op_0(0)
{
}

void tess::push_eval_context::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    auto& ctxt = contexts.top();
    contexts.push(ctxt.execution_state().create_eval_context());
}

/*---------------------------------------------------------------------------------------------*/

tess::pop_frame_op::pop_frame_op() : tess::stack_machine::op_0(0)
{
}

void tess::pop_frame_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    if (contexts.empty())
        throw tess::error("context stack underflow");

    if (contexts.top().empty())
        throw tess::error("eval context underflow");

    auto scope = contexts.top().pop_scope();
}

/*---------------------------------------------------------------------------------------------*/

tess::push_frame_op::push_frame_op() : tess::stack_machine::op_0(0)
{
}

void tess::push_frame_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    if (contexts.empty())
        throw tess::error("context stack underflow");
    contexts.top().push_scope(scope_frame());
}

/*---------------------------------------------------------------------------------------------*/

tess::assign_op::assign_op(int num_vars) : stack_machine::op_0(num_vars+1)
{
}

void tess::assign_op::execute(const std::vector<tess::stack_machine::item>& operands, tess::context_stack& contexts) const
{
    int num_vars = static_cast<int>(operands.size() - 1);
    auto value = std::get<value_>(operands.back());
    std::vector<stack_machine::variable> vars(num_vars);
    std::transform(operands.begin(), operands.begin() + num_vars, vars.begin(),
        [](const auto& item) { return std::get<stack_machine::variable>(item); }
    );

    auto& current_scope = contexts.top().peek();
    if (num_vars == 1) {
        current_scope.set(vars[0].name(), value);
    } else {
        int i = 0;
        for (const auto& var : vars) 
            current_scope.set(var.name(), tess::get_ary_item(value, i++));
    }
}

tess::one_param_op::one_param_op(std::function<value_(tess::gc_heap&, const value_&)> func, std::string name) :
    stack_machine::op_1(1), func_(func), name_(name)
{
}

tess::stack_machine::item tess::one_param_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    return {
        func_(contexts.top().allocator(), std::get<value_>(operands[0]))
    };
}

tess::get_field_op::get_field_op(const std::string& field, bool get_ref) : 
    stack_machine::op_1(1), field_(field), get_ref_(get_ref)
{
}

tess::stack_machine::item tess::get_field_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    value_ val = std::get<value_>(operands[0]);
    if (!get_ref_) {
        return { tess::get_field(val, contexts.top().allocator(), field_) };
    }
    else {
        return { value_{ std::make_shared<field_ref_impl>(val, field_)} };
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

std::vector<tess::value_> get_layees(const tess::evaluation_context& ctxt) {
    std::vector<tess::value_> values;
    for (int i = 1; ctxt.has(std::to_string(i)); ++i) {
        values.push_back(ctxt.get(std::to_string(i)));
    }
    return values;
}

tess::stack_machine::item tess::lay_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{ 
    auto& ctxt = contexts.top();
    auto layees = get_layees(ctxt);
    auto result = apply_mapping(operands);
    return  {
        tess::make_value(
            tess::flatten(ctxt.allocator(), layees, true)
        )
    };
}

std::string tess::lay_op::to_string() const
{
    return std::string("<lay " + std::to_string(number_of_args_/2) + ">");
}

std::optional<tess::error> tess::lay_op::apply_mapping(const std::vector<stack_machine::item>& operands) const
{
    auto values = get_vector<value_>(operands.begin(), operands.end());
    std::vector<std::tuple<const_edge_ptr, const_edge_ptr>> edge_to_edge;

    for (const auto& e : values) {
        if (!std::holds_alternative<tess::const_edge_ptr>(e))
            throw tess::error("mapping argument in a lay or join expression does not evaluate to an edge.");
    }

    for (int i = 0; i < values.size(); i += 2) {
        auto edge1 = std::get<const_edge_ptr>(values[i]);
        auto edge2 = std::get<const_edge_ptr>(values[i+1]);
        edge_to_edge.push_back({ edge1, edge2} );
    }

    return ::apply_mapping(edge_to_edge);
}

tess::val_func_op::val_func_op(int n, std::function<value_(tess::gc_heap& a, const std::vector<value_> & v)> func, std::string name) :
    stack_machine::op_1(n), name_(name), func_(func)
{
}

tess::stack_machine::item tess::val_func_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    std::vector<value_> args = get_vector<value_>(operands.begin(), operands.end());
    return { func_( contexts.top().allocator(), args ) };
}

tess::if_op::if_op(const std::vector<stack_machine::item>& if_clause, const std::vector<stack_machine::item>& else_clause) :
    stack_machine::op_multi(1), if_(if_clause), else_(else_clause)
{
}

std::vector<tess::stack_machine::item> tess::if_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    bool cond = std::get<bool>(std::get<value_>(operands[0]));
    return (cond) ?
        if_ :
        else_;
}

std::string tess::if_op::to_string() const
{
    std::stringstream ss;
    ss << "<if> {";
    for (const auto& it : if_) {
        it.visit(
            overloaded{
                [&](stack_machine::op_ptr op) {
                    ss << op->to_string();
                },

                [&](const value_& val) {
                    ss << tess::to_string(val);
                },

                [&](const auto& val) {
                    ss << val.to_string();
                }
            }
        );
        ss << "; ";
    }
    ss << "} {";
    for (const auto& it : else_) {
        it.visit(
            overloaded{
                [&](stack_machine::op_ptr op) {
                    ss << op->to_string();
                },

                [&](const value_& val) {
                    ss << tess::to_string(val);
                },

                [&](const auto& val) {
                    ss << val.to_string();
                }
            }
        );
        ss << "; ";
    }
    ss << "}";
    return ss.str();
}

tess::get_ary_item_op::get_ary_item_op() : stack_machine::op_1(2)
{
}

tess::stack_machine::item tess::get_ary_item_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    auto ary = std::get<value_>(operands[0]);
    auto index = std::get<tess::number>(std::get<value_>(operands[1]));
    return { tess::get_ary_item(ary, tess::to_int(index)) };
}


std::vector<tess::stack_machine::item> tess::iterate_op::start_next_item(int index, tess::const_cluster_ptr ary) const
{
    std::vector<stack_machine::item> output{
        { value_{number(index)} },
        { value_{ary} },
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

std::vector<tess::stack_machine::item> tess::iterate_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    auto& alloc = contexts.top().allocator();
    auto src = std::get<tess::const_cluster_ptr>(std::get<value_>( operands[0] ));
    auto dst = (index_val_ > -1) ? get_mutable<tess::const_cluster_ptr>(std::get<value_>(operands[1])) : nullptr;
    auto curr_item = std::get<value_>( operands[2] );
    
    auto n = src->get_ary_count();

    // we could clone the cluster::impl_type here but that would make iteration n^2
    // so we will just remove const-ness so we can reuse the same cluster implementation.
    // The other option would be to make operands non-const but right now
    // this is the only place where operands const-ness is an issue.

    if (dst) 
        dst->push_value(curr_item);
    auto i = index_val_ + 1;
    
    if (i >= n)
        return std::vector<tess::stack_machine::item>{ operands[1] };

    auto next_iteration = start_next_item( i, src );

    stack_machine::item new_dst;
    if (!dst)
        new_dst = { value_{ alloc.make_const<const_cluster_ptr>( std::vector<value_>{}) } };
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

void tess::set_dependencies_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    auto& ctxt = contexts.top();
    auto& frame = ctxt.peek();
    for (auto& val : frame.values()) {
        if (std::holds_alternative<const_lambda_ptr>(val)) {
            std::vector<std::string> vars;
            auto func = get_mutable<tess::const_lambda_ptr>(val);
            for (const auto& var : func->unfulfilled_dependencies())   
                func->insert_field(var, ctxt.get(var));
        }
    }
}

tess::set_field_op::set_field_op(int num_fields) : stack_machine::op_0(num_fields + 1)
{
}

void tess::set_field_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    int num_fields = static_cast<int>(operands.size() - 1);
    auto value = std::get<value_>(operands.back());
    std::vector<tess::field_ref_ptr> field_refs(num_fields);
    std::transform(operands.begin(), operands.begin() + num_fields, field_refs.begin(),
        [](const auto& item) { 
            return std::get<field_ref_ptr>(std::get<value_>(item));
        }
    );

    if (num_fields == 1) {
        field_refs[0]->set(value);
    } else {
        int i = 0;
        for (auto& field_ref : field_refs) {
            field_ref->set(tess::get_ary_item(value, i++));
        }
    }
}

tess::memoize_func_call_op::memoize_func_call_op(std::string func_call_key) : stack_machine::op_1(1),
    key_(func_call_key)
{
}

tess::stack_machine::item tess::memoize_func_call_op::execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const
{
    auto val = std::get<value_>(operands[0]);
    if (!key_.empty()) {
        contexts.memos().insert(key_, val);
    }
    return { val };
}
