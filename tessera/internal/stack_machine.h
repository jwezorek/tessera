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

            std::vector<item> pop(int n);
            std::vector<item> pop_until(std::function<bool(const item&)> pred);

        private:
            std::vector<item> impl_;
        };

        class op {
            private:
                virtual std::variant<std::vector<item>, error> execute(stack& main_stack, const std::vector<item>& operands, context_stack& contexts) const = 0;
                virtual int number_of_operands() const = 0;
            public:
                std::optional<error> execute(stack& main_stack, stack& operand_stack, context_stack& contexts);
        };

        stack_machine(execution_state& parent);
    private:
        tess::execution_state& parent_;
    };
};