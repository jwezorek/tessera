#pragma once

#include <string>
#include <variant>
#include <vector>
#include <memory>

namespace tess{
    
    class scope_frame;
    class expr_value;
    class function_def;

    class lambda {

        public:
            lambda(const function_def& func, const scope_frame& closure);
            expr_value call(const std::vector<expr_value>& expr_value) const;
            lambda get_ref() const;

        private:
            lambda(const lambda* ref);

            class impl_type;
            using lambda_impl_ptr = std::shared_ptr<impl_type>;

            std::variant<lambda_impl_ptr, const lambda*> impl_;
            
    };

}