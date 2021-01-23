#pragma once

#include "deferred_allocator.h"
#include "deferred_heap.h"

namespace tess {

    using gc_heap = gcpp::deferred_heap;

    /*
    namespace detail {
        template <typename T>
        using base_type = typename std::remove_const<typename std::remove_pointer<T>::type>::type;

    }
    */

    template<typename T, typename... Args>
    auto create_mutable(gc_heap& a, Args&&... args) {
        auto ptr = a.make<typename std::remove_const<typename T::value_type>::type>(a, std::forward<Args>(args)...);
        ptr->initialize( ptr );
        return ptr;
    }

    template<typename T, typename... Args>
    auto create_const(gc_heap& a, Args&&... args) {
        using base_type = typename std::remove_const<typename T::value_type>::type;
        auto ptr = a.make<base_type>(a, std::forward<Args>(args)...);
        ptr->initialize( ptr );
        return gcpp::deferred_ptr<const base_type>(ptr);
    }

};