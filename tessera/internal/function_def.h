#pragma once

#include "expression.h"
#include <string>
#include <vector>
#include <variant>
#include <memory>

namespace tess {

    class expr_value;
    class evaluation_context;
    class tile_def;
    class patch_def;

    class function_def : public expression {
        public:
            void compile(stack_machine::stack& stack) const override;
            std::string to_string() const override;
            const std::vector<std::string>& parameters() const;
            expr_ptr body() const;
            expr_ptr simplify() const override;
            void get_dependencies(std::unordered_set<std::string>& dependencies) const override;

            function_def(const std::vector<std::string>& params, expr_ptr body);

            friend class lambda;

        private:
            void compile_dependencies(const std::unordered_set<std::string>& dependent_vars, stack_machine::stack& stack) const;
            std::vector<std::string> get_variables() const;

            std::vector<std::string> parameters_;
            expr_ptr body_;
    };

}