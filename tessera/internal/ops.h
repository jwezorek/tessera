#pragma once

#include "stack_machine.h"
#include "expr_value.h"

namespace tess {

    class allocator;

    template <typename T>
    T get_from_item(const stack_machine::item& item) {
        if (std::holds_alternative<expr_value>(item)) {
            auto ev = std::get<expr_value>(item);
            return std::get<T>(ev);
        } else {
            return std::get<T>(item);
        }
    }

    template <typename T>
    stack_machine::item make_expr_val_item(const T& val) {
        return stack_machine::item{ expr_value{ val } };
    }

    class make_lambda : public stack_machine::op_1 {
    public:
        make_lambda(const std::vector<std::string>& parameters, const std::vector<stack_machine::item>& body, int num_dependencies);
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override;
        std::vector<std::string> parameters_;
        std::vector<stack_machine::item> body_;
    };

    class get_var : public stack_machine::op_multi {
    public:
        get_var();
    protected:
       std::variant<std::vector<stack_machine::item>, tess::error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
       std::string to_string() const override { return "<get>"; }
    };

    class pop_eval_context : public stack_machine::op_0 {
    public:
        pop_eval_context();
        std::optional<error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return "<pop_context>"; }
    };

    class call_func : public stack_machine::op_multi {
    protected:
        std::variant<std::vector<stack_machine::item>, tess::error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    public:
        call_func(int num_args);
        std::string to_string() const override { return "<apply " + std::to_string(number_of_args_ - 1) + ">"; }
    };

    class push_eval_context : public stack_machine::op_0 {
    public:
        push_eval_context();
        std::optional<error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return "<push_context>"; }
    };

    class neg_op : public stack_machine::op_1 {
    public:
        neg_op();
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return "<neg>"; }
    };

    class add_op : public stack_machine::op_1 {
    public:
        add_op(int args);
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return "<add " + std::to_string(number_of_args_) + ">"; }
    };

    class pop_and_insert_fields_op : public stack_machine::op_1 {
    public:
        pop_and_insert_fields_op();
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return "<pop_and_insert_fields>"; }
    };
    
    class pop_frame_op : public stack_machine::op_0 {
    public:
        pop_frame_op();
        std::optional<error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return "<pop_frame>"; }
    };

    class push_frame_op : public stack_machine::op_0 {
    public:
        push_frame_op();
        std::optional<error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return "<push_frame>"; }
    };

    class dup_op : public stack_machine::op_multi {
    public:
        dup_op();
    protected:
        std::variant<std::vector<stack_machine::item>, tess::error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return "<dup>"; }
    };

    class assign_op : public stack_machine::op_0 {
    public:
        assign_op(int num_vars);
    protected:
        std::optional<error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return "<assign " + std::to_string(number_of_args_-1) + ">"; }
    };

    class one_param_op : public stack_machine::op_1 {
    public:
        one_param_op(std::function<expr_value(allocator& a, const expr_value & v)> func, std::string name);
    protected:
        std::string name_;
        std::function<expr_value(allocator & a, const expr_value & v)> func_;
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override { return name_; }

    };

    class get_field_op : public stack_machine::op_1 {
    public:
        get_field_op(const std::string& field);
    protected:
        std::string field_;

        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override;
    };

    class lay_op : public stack_machine::op_1 {
    public:
        lay_op(int num_mappings);
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
        std::string to_string() const override;
    };

}