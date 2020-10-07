#pragma once

#include "expr_value.h"
#include <string>
#include <variant>
#include <vector>
#include <memory>
#include "stack_machine.h"

namespace tess{
    
    class scope_frame;
    class function_def;
    class expression;
    class execution_state;
    class scope_frame;

    namespace detail {
        class lambda_impl;
    }

    class lambda {
            friend class tessera_impl;
        public: 

            const std::vector<std::string>& parameters() const;
            void insert_field(const std::string& var, const expr_value& val);
            const scope_frame& closure() const;
            std::vector<stack_machine::item> body() const;
            std::vector<std::string> dependencies() const;
            using impl_type = detail::lambda_impl;

        private:

            impl_type* impl_;
    };

}