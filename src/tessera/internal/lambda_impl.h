#pragma once

#include "function_def.h"
#include "execution_state.h"
#include "stack_machine.h"
#include "tessera_impl.h"
#include "gc_heap.h"

namespace tess {
    namespace detail {
        class lambda_impl : public tessera_impl {
            private:
                unsigned int id_;
            public:
                scope_frame closure;
                std::vector<std::string> parameters;
                std::vector<std::string> dependencies;
                std::vector<stack_machine::item> body;

                lambda_impl(gc_heap& a) : id_(0) {};
                lambda_impl(gc_heap& a, const std::vector<std::string>& param, const std::vector<stack_machine::item>& bod, const std::vector<std::string>& deps);
                void initialize( lambda_ptr p) {}

                void insert_field(const std::string& var, const value_& val);
                value_ get_field(gc_heap& allocator, const std::string& field) const;
                void set_id(unsigned int id);
                void clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, lambda_ptr clone) const;
                std::vector<std::string> unfulfilled_dependencies() const;
                std::string serialize(serialization_state& state) const;
        };
    }
}