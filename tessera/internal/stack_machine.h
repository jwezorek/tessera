#pragma once

#include "expr_value.h"
#include "tessera/error.h"
#include "evaluation_context.h"
#include "tessera_impl.h"
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

    namespace stack_machine
    {
        using context_stack = std::stack<evaluation_context>;

        struct variable {
        private:
            std::string name_;
        public:
            variable(std::string str = "") : name_(str) {}

            variable(int i) : name_(std::to_string(i)) {}

            std::string to_string() const {
                return  std::string("$(") + name_ + ")";
            };

            std::string name() const {
                return name_;
            }
        };

        class op;
        using op_ptr = std::shared_ptr<op>;

        class item : public std::variant<op_ptr, expr_value, error, variable>
        {
        public:
            std::string to_string() const;
        };

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

            std::string to_string() const;
            std::string to_formatted_string() const;
            std::vector<item> pop_all();
        private:
            std::vector<item> impl_;
        };

        class op {
            protected:
                int number_of_args_;
            public:
                op(int n) : number_of_args_(n) {}
                virtual void execute(stack& main_stack, stack& operand_stack, context_stack& contexts) = 0;
                virtual std::string to_string() const = 0;
        };

        class op_0 : public op {
        protected:
            virtual void execute(const std::vector<item>& operands, context_stack& contexts) const = 0;
        public:
            op_0(int n) : op(n) {}
            void execute(stack& main_stack, stack& operand_stack, context_stack& contexts);
        };

        class op_1 : public op{
        protected:
            virtual item execute(const std::vector<item>& operands, context_stack& contexts) const = 0;
        public:
            op_1(int n) : op(n) {}
            void execute(stack& main_stack, stack& operand_stack, context_stack& contexts);
        };

        class op_multi : public op {
        protected:
            virtual std::vector<item> execute(const std::vector<item>& operands, context_stack& contexts) const = 0;
        public:
            op_multi(int n) : op(n) {}
            void execute(stack& main_stack, stack& operand_stack, context_stack& contexts);
        };

        class machine
        {
        public:
            machine();
            expr_value run(execution_state& state);
        };
    };
};