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
                std::map<std::string, field_value> closure_;
                std::vector<std::string> parameters_;
                std::vector<std::string> dependencies_;
                std::vector<stack_machine::item> body_;
            public:
                lambda_impl(gc_heap& a) : id_(0) {};
                lambda_impl(gc_heap& a, const std::vector<std::string>& param, const std::vector<stack_machine::item>& bod, const std::vector<std::string>& deps);
                void initialize( lambda_root_ptr p) {}

                void insert_field(const std::string& var, const value_& val);
                value_ get_field(gc_heap& allocator, const std::string& field) const;
                void set_id(unsigned int id);
                void clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, lambda_raw_ptr clone) const;
                std::vector<std::string> unfulfilled_dependencies() const;
                std::string serialize(serialization_state& state) const;

                using const_closure_iter = std::map<std::string, field_value>::const_iterator;

                const_closure_iter begin_closure() const;
                const_closure_iter end_closure() const;
                
                const std::vector<std::string>& parameters() const;
                const std::vector<std::string>& dependencies() const;
                const std::vector<stack_machine::item>& body() const;
        };
    }
}