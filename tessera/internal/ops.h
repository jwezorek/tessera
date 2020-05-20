#pragma once

#include "stack_machine.h"
#include "expr_value.h"

namespace tess {

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
        return stack_machine::item(expr_value{ val });
    }

    class make_lambda : public stack_machine::op_1 {
    public:
        make_lambda();
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    };

    class get_var : public stack_machine::op_1 {
    public:
        get_var();
    protected:
       stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    };

    class make_scope_frame : public stack_machine::op_1 {
    public:
        make_scope_frame(int n);
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    };

    class pop_eval_context : public stack_machine::op {
    public:
        pop_eval_context();
        std::optional<error> execute(stack_machine::stack& main_stack, stack_machine::stack& operand_stack, stack_machine::context_stack& contexts) override;
    };

    class call_func : public stack_machine::op_multi {
    protected:
        std::variant<std::vector<stack_machine::item>, tess::error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    public:
        call_func(int num_args);
    };

    class push_eval_context : public stack_machine::op {
    public:
        push_eval_context();
        std::optional<error> execute(stack_machine::stack& main_stack, stack_machine::stack& operand_stack, stack_machine::context_stack& contexts) override;
    };

    class neg_op : public stack_machine::op_1 {
    public:
        neg_op();
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    };

    class add_op : public stack_machine::op_1 {
    public:
        add_op(int args);
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    };

    class insert_fields_op : public stack_machine::op_1 {
    public:
        insert_fields_op();
    protected:
        stack_machine::item execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    };
    
    class pop_frame_op : public stack_machine::op {
    public:
        pop_frame_op();
        std::optional<error> execute(stack_machine::stack& main_stack, stack_machine::stack& operand_stack, stack_machine::context_stack& contexts) override;
    };

    class push_frame_op : public stack_machine::op {
    public:
        push_frame_op();
        std::optional<error> execute(stack_machine::stack& main_stack, stack_machine::stack& operand_stack, stack_machine::context_stack& contexts) override;
    };

    class dup_op : public stack_machine::op_multi {
    public:
        dup_op();
    protected:
        std::variant<std::vector<stack_machine::item>, tess::error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    };

    class assign_op : public stack_machine::op_multi {
    public:
        assign_op(int num_vars);
    protected:
        std::variant<std::vector<stack_machine::item>, tess::error> execute(const std::vector<stack_machine::item>& operands, stack_machine::context_stack& contexts) const override;
    };


};