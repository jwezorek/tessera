#pragma once

#include <string>
#include <variant>
#include "execution_ctxt.h"
#include "expr_value.h"

namespace tess{

    class lambda {

        public:
            lambda(std::shared_ptr<function_def> func, const scope_frame& closure);
            expr_value call(const std::vector<expr_value>& expr_value) const;
            lambda get_ref() const;

        private:
            lambda(const lambda* ref);

            struct lambda_impl_t {
                std::shared_ptr<function_def> func;
                scope_frame closure;
            };
            using lambda_impl_ptr = std::shared_ptr<lambda_impl_t>;

            std::variant<lambda_impl_ptr, const lambda*> impl_;
            
    };

}