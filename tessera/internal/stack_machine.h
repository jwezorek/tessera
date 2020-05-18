#pragma once

#include "expr_value.h"
#include "evaluation_context.h"
#include <vector>
#include <string>
#include <memory>
#include <variant>
#include <optional>
#include <algorithm>
#include <stack>

namespace tess {

    class expression;
    class execution_state;
    class error;

    class stack_machine
    {
    public:

        using context_stack = std::stack<evaluation_context>;

        struct identifier {
            std::string name;
            identifier(std::string str) : name(str) {}
        };

        class op;
        using op_ptr = std::shared_ptr<op>;
        using item = std::variant<op_ptr, expr_value, error, identifier, lex_scope::frame, std::shared_ptr<expression>>;

        class stack {
        public:
            item pop();
            void push(const item& item);

            template <typename T>
            void push(const T& val) {
                push( item{ val } );
            }

            template <typename Iter>
            void push(Iter beg, Iter end) {
                std::copy(beg, end, std::back_inserter(impl_));
            }
            void push(const std::vector<item>& item);
            void compile_and_push(const std::vector<std::shared_ptr<expression>>& exprs);

            std::vector<item> pop(int n);
            bool empty() const;
            int count() const;

        private:
            std::vector<item> impl_;
        };

        class op {
            protected:
                int number_of_args_;
            public:
                op(int n) : number_of_args_(n) {}
                virtual std::optional<error> execute(stack& main_stack, stack& operand_stack, context_stack& contexts) = 0;
        };

        class op_1 : public op{
        protected:
            virtual item execute(const std::vector<item>& operands, context_stack& contexts) const = 0;
        public:
            op_1(int n) : op(n) {}
            std::optional<error> execute(stack& main_stack, stack& operand_stack, context_stack& contexts);
        };

        class op_multi : public op {
        protected:
            virtual std::variant<std::vector<item>, tess::error> execute(const std::vector<item>& operands, context_stack& contexts) const = 0;
        public:
            op_multi(int n) : op(n) {}
            std::optional<error> execute(stack& main_stack, stack& operand_stack, context_stack& contexts);
        };

        stack_machine();
        expr_value run(execution_state& state);
    };
};