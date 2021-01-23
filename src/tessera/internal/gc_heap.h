#pragma once

#include "deferred_allocator.h"
#include "deferred_heap.h"

namespace tess {

    class gc_heap {
    private:
        gcpp::deferred_heap impl_;

    public:

        gc_heap();

        template<typename T, typename... Args>
        auto create_mutable(Args&&... args) {
            auto ptr = impl_.make<typename std::remove_const<typename T::value_type>::type>(*this, std::forward<Args>(args)...);
            ptr->initialize(ptr);
            return ptr;
        }

        template<typename T, typename... Args>
        auto create_const(Args&&... args) {
            using base_type = typename std::remove_const<typename T::value_type>::type;
            auto ptr = impl_.make<base_type>(*this, std::forward<Args>(args)...);
            ptr->initialize(ptr);
            return gcpp::deferred_ptr<const base_type>(ptr);
        }

    };

};