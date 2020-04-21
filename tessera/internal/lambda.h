#pragma once

#include <string>
#include <variant>
#include <vector>
#include <memory>

namespace tess{
    
    class scope_frame;
    class expr_value;
    class function_def;
    class expression;

    class lambda {
        private:
            class impl_type;
            using lambda_impl_ptr = std::shared_ptr<impl_type>;
            std::variant<lambda_impl_ptr, impl_type*> impl_;

            lambda(impl_type& ref);
            impl_type& get_impl() const;

        public:
            lambda(std::shared_ptr<expression> func, const scope_frame& closure);
            const std::vector<std::string>& parameters() const;
            expr_value call(const std::vector<expr_value>& expr_value) const;
            lambda get_ref() const;
            void add_to_closure(const std::string& var, const expr_value& val);
    };

}