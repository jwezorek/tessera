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
        auto make_mutable(Args&&... args) {
            auto ptr = impl_.make<typename std::remove_const<typename T::value_type>::type>(*this, std::forward<Args>(args)...);
            ptr->initialize(ptr);
            return ptr;
        }

        template<typename T, typename... Args>
        auto make_const(Args&&... args) {
            using const_type = typename std::add_const<typename T::value_type>::type;
            return gcpp::deferred_ptr<const_type>(
                make_mutable<T>(std::forward<Args>(args)...)
            );
        }

    };

};