#pragma once

#include "function_def.h"
#include "execution_state.h"
#include "stack_machine.h"

namespace tess {
    class lambda::impl_type {
        public:
            scope_frame closure;
            std::vector<std::string> parameters;
            std::vector<stack_machine::item> body;

            impl_type(const std::vector<std::string>& param, const std::vector<stack_machine::item>& bod, const scope_frame& c);
            void insert_field(const std::string& var, const expr_value& val);
            expr_value get_field(allocator& allocator, const std::string& field) const; 
            void get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const;
    };
}