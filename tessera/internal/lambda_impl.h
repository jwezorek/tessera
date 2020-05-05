#pragma once

#include "function_def.h"
#include "execution_state.h"

namespace tess {
    class lambda::impl_type {
        public:
            function_def func;
            lex_scope::frame closure;

            impl_type(const function_def& f, const lex_scope::frame& c);
            void insert_field(const std::string& var, const expr_value& val);
            expr_value get_field(allocator& allocator, const std::string& field) const;
    };
}