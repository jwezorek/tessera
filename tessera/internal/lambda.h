#pragma once

#include "value.h"
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
            const scope_frame& closure() const;
            using impl_type = detail::lambda_impl;

        private:

            const impl_type* impl_;
    };

}