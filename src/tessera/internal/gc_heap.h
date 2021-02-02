#pragma once

#include "deferred_allocator.h"
#include "deferred_heap.h"

namespace tess {

    class gc_heap {
    private:
        gcpp::deferred_heap impl_;
        int allocations_since_collection_;
    public:

        gc_heap();

        template<typename T, typename... Args>
        auto make_mutable(Args&&... args) {
            if (allocations_since_collection_++ > 10000) {
                impl_.collect();
                allocations_since_collection_ = 0;
            }
            auto ptr = impl_.make<typename std::remove_const<typename T::value_type>::type>(*this, std::forward<Args>(args)...);
            ptr->initialize(ptr);
            return ptr;
        }

        template<typename T, typename... Args>
        auto make_const(Args&&... args) {
            return gcpp::deferred_ptr<typename std::add_const<typename T::value_type>::type>(
                make_mutable<T>(std::forward<Args>(args)...)
            );
        }

    };

};