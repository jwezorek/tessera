#pragma once

#include "function_def.h"
#include "execution_state.h"
#include "stack_machine.h"
#include "tessera_impl.h"

namespace tess {
    namespace detail {
        class lambda_impl : public tessera_impl {
            public:
                scope_frame closure;
                std::vector<std::string> parameters;
                std::vector<std::string> dependencies;
                std::vector<stack_machine::item> body;

                lambda_impl(obj_id id) : tessera_impl(id) {};
                lambda_impl(obj_id id, const std::vector<std::string>& param, const std::vector<stack_machine::item>& bod, const std::vector<std::string>& deps);
                void insert_field(const std::string& var, const expr_value& val);
                expr_value get_field(allocator& allocator, const std::string& field) const;
                void get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, lambda_ptr clone) const;
                std::vector<std::string> unfulfilled_dependencies() const;
        };
    }
}