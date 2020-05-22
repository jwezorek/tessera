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
    class execution_state;
    class scope_frame;

    class lambda {
            friend class tessera_impl;
        public: 

            const std::vector<std::string>& parameters() const;
            expr_value call(execution_state& state, const std::vector<expr_value>& expr_value) const;
            void insert_field(const std::string& var, const expr_value& val);
            const scope_frame& closure() const;
            std::shared_ptr<expression> body();
            //std::vector<stack_machine::item> get_body() const;
            class impl_type;

        private:

            impl_type* impl_;
    };

}