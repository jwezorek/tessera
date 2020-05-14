#pragma once

#include "expr_value.h"
#include <vector>
#include <string>
#include <memory>
#include <variant>
#include <optional>
#include <algorithm>
#include <stack>

namespace tess {

    class evaluation_context;
    class execution_state;

    class stack_machine
    {
    public:

        using context_stack = std::stack<evaluation_context>;

        class op;
        using op_ptr = std::shared_ptr<op>;
        class item : public std::variant<op_ptr, expr_value, std::vector<item>> {

        };

        class stack {
        public:
            item pop();
            void push(const item& item);

            template <typename Iter>
            void push(Iter beg, Iter end) {
                std::copy(beg, end, std::back_inserter(impl_));
            }
            void push(const std::vector<item>& item);
            std::vector<item> pop(int n);
            bool empty() const;
            int count() const;

        private:
            std::vector<item> impl_;
        };

        class op {
            protected:
                int number_of_args_;
                virtual std::variant<std::vector<item>, error> execute(stack& main_stack, const std::vector<item>& operands, context_stack& contexts) const = 0;
            public:
                op(int n) : number_of_args_(n) {}
                std::optional<error> execute(stack& main_stack, stack& operand_stack, context_stack& contexts);
        };

        stack_machine();
        expr_value run(execution_state& state);
    };
};