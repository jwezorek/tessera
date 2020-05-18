#pragma once

#include "function_def.h"
#include "execution_state.h"

namespace tess {
    class lambda::impl_type {
        public:
            function_def func;
            scope_frame closure;

            impl_type(const function_def& f, const scope_frame& c);
            void insert_field(const std::string& var, const expr_value& val);
            expr_value get_field(allocator& allocator, const std::string& field) const; 
            void get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const;
    };
}