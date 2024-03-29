#pragma once

#include "stack_machine.h"
#include "value.h"
#include "tile_patch_impl.h"
#include "gc_heap.h"

namespace tess {


    template <typename T>
    T get_from_item(const stack_machine::item& item) {
        if (std::holds_alternative<value_>(item)) {
            auto ev = std::get<value_>(item);
            return std::get<T>(ev);
        } else {
            return std::get<T>(item);
        }
    }

    template <typename T>
    stack_machine::item make_expr_val_item(const T& val) {
        return stack_machine::item{ tess::make_value(val) };
    }

    class make_lambda : public stack_machine::op_1 {
    public:
        make_lambda(const std::vector<std::string>& parameters, const std::vector<stack_machine::item>& body, const std::vector<std::string>& deps);
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override;
        std::vector<std::string> parameters_;
        std::vector<std::string> dependencies_;
        std::vector<stack_machine::item> body_;
    };

    class get_var : public stack_machine::op_multi {
    public:
        get_var(bool eval_parameterless_funcs = true);
    protected:
       std::vector<stack_machine::item> execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
       std::string to_string() const override { return "<get>"; }
       bool eval_parameterless_funcs_;
    };

    class pop_eval_context : public stack_machine::op_0 {
    public:
        pop_eval_context();
        void execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return "<pop_context>"; }
    };

    class call_func : public stack_machine::op_multi {
    protected:
        std::vector<stack_machine::item> execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
    public:
        call_func(int num_args);
        std::string to_string() const override { return "<apply " + std::to_string(number_of_args_ - 1) + ">"; }
    };

    class push_eval_context : public stack_machine::op_0 {
    public:
        push_eval_context();
        void execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return "<push_context>"; }
    };
    
    class pop_frame_op : public stack_machine::op_0 {
    public:
        pop_frame_op();
        void execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return "<pop_frame>"; }
    };

    class push_frame_op : public stack_machine::op_0 {
    public:
        push_frame_op();
        void execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return "<push_frame>"; }
    };

    class assign_op : public stack_machine::op_0 {
    public:
        assign_op(int num_vars);
    protected:
        void execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return "<assign " + std::to_string(number_of_args_-1) + ">"; }
    };

    class one_param_op : public stack_machine::op_1 {
    public:
        one_param_op(std::function<value_(gc_heap& a, const value_ & v)> func, std::string name);
    protected:
        std::string name_;
        std::function<value_(gc_heap & a, const value_ & v)> func_;
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return name_; }

    };

    class val_func_op : public stack_machine::op_1 {
    public:
        val_func_op(int n, std::function<value_(gc_heap& a, const std::vector<value_>& v)> func, std::string name);
    protected:
        std::string name_;
        std::function<value_(gc_heap& a, const std::vector<value_> & v)> func_;
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return name_; }

    };

    class get_field_op : public stack_machine::op_1 {
    public:
        get_field_op(const std::string& field, bool get_ref);
    protected:
        std::string field_;
        bool get_ref_;

        stack_machine::item execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override;
    };

    class set_field_op: public stack_machine::op_0{
    public:
        set_field_op(int fields);
    protected:
        void execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return "<set_field " + std::to_string(number_of_args_ - 1) + ">"; }
    };

    class lay_op : public stack_machine::op_1 {
    public:
        lay_op(int num_mappings);
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override;

        std::optional<tess::error> apply_mapping(const std::vector<stack_machine::item>& operands) const;
    };

    class if_op : public stack_machine::op_multi {
    public:
        if_op(const std::vector<stack_machine::item>& if_clause, const std::vector<stack_machine::item>& else_clause);
    protected:
        std::vector<stack_machine::item> execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override;

        std::vector<stack_machine::item> if_;
        std::vector<stack_machine::item> else_;
    };

    class get_ary_item_op : public stack_machine::op_1 {
    public:
        get_ary_item_op();
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return "<get_ary_item>"; }
    };

    class iterate_op : public stack_machine::op_multi {
    protected:
        std::vector<stack_machine::item> execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;

        std::string index_var_;
        int index_val_;
        std::vector<stack_machine::item> body_;

        std::vector<stack_machine::item> start_next_item(int index, tess::const_cluster_root_ptr ary) const;

    public:
        iterate_op(std::string index_var, int index_val, const std::vector<stack_machine::item>& body);
        std::string to_string() const override;
    };

    class set_dependencies_op : public stack_machine::op_0 {
    public:
        set_dependencies_op();
    protected:
        void execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return "<set dependencies>"; }
    };

    class memoize_func_call_op : public stack_machine::op_1 {
    public:
        memoize_func_call_op(std::string func_call_key);
    protected:
        std::string key_;

        stack_machine::item execute(const std::vector<stack_machine::item>& operands, tess::context_stack& contexts) const override;
        std::string to_string() const override { return "<get_ary_item " + key_ + ">"; }
    };
}