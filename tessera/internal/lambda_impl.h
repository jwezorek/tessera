#pragma once

#include "function_def.h"
#include "execution_state.h"
#include "stack_machine.h"

namespace tess {
    class lambda::impl_type {
        public:
            scope_frame closure;
            std::vector<std::string> parameters;
            std::vector<std::string> dependencies;
            std::vector<stack_machine::item> body;

            impl_type() {};
            impl_type(const std::vector<std::string>& param, const std::vector<stack_machine::item>& bod, const std::vector<std::string>& deps);
            void insert_field(const std::string& var, const expr_value& val);
            expr_value get_field(allocator& allocator, const std::string& field) const; 
            void get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const; 
            void clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, lambda::impl_type* clone) const;
    };
}