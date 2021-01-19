#pragma once

#include "function_def.h"
#include "execution_state.h"
#include "stack_machine.h"
#include "tessera_impl.h"
#include "allocator.h"

namespace tess {
    namespace detail {
        class lambda_impl : public tessera_impl {
            public:
                scope_frame closure;
                std::vector<std::string> parameters;
                std::vector<std::string> dependencies;
                std::vector<stack_machine::item> body;

                lambda_impl(allocator& a)  {};
                lambda_impl(allocator& a, const std::vector<std::string>& param, const std::vector<stack_machine::item>& bod, const std::vector<std::string>& deps);
                void initialize( lambda_ptr p) {}

                void insert_field(const std::string& var, const value_& val);
                value_ get_field(allocator& allocator, const std::string& field) const;
                //void get_references(std::unordered_set<obj_id>& alloc_set) const;
                void clone_to(tess::allocator& allocator, std::unordered_map<obj_id, mutable_object_value>& orginal_to_clone, lambda_ptr clone) const;
                std::vector<std::string> unfulfilled_dependencies() const;
        };
    }
}