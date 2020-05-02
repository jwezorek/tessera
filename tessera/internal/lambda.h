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
            friend class allocator;
        public: 

            const std::vector<std::string>& parameters() const;
            expr_value call(const std::vector<expr_value>& expr_value) const;
            lambda get_ref() const;
            void add_to_closure(const std::string& var, const expr_value& val);
            class impl_type;

        private:

            impl_type* impl_;
    };

}