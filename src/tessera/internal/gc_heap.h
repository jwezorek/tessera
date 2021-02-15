#pragma once

#include "graph_ptr.h"

namespace tess {

    namespace detail {
        class vertex_impl;
        class edge_impl;
        class cluster_impl;
        class tile_impl;
        class patch_impl;
        class lambda_impl;
    }

    using graph_pool = gp::graph_pool<detail::vertex_impl, detail::edge_impl, detail::cluster_impl, detail::tile_impl, detail::patch_impl, detail::lambda_impl>;

    template <typename T>
    using graph_ptr = graph_pool::graph_ptr<T>;

    template <typename T>
    using graph_root_ptr = graph_pool::graph_root_ptr<T>;

    template <typename T>
    using enable_self_ptr = graph_pool::enable_self_graph_ptr<T>;

    class gc_heap {
    private:
        graph_pool impl_;
        int allocations_since_collection_;
    public:

        gc_heap();

        template<typename T, typename... Args>
        auto make_mutable(Args&&... args) {
            if (allocations_since_collection_++ > 10000) {
                impl_.collect();
                allocations_since_collection_ = 0;
            }

            auto ptr = impl_.make_root<typename std::remove_const<typename T::value_type>::type>();
            ptr->initialize(*this, std::forward<Args>(args)...);

            return ptr;
        }

        template<typename T, typename... Args>
        auto make_blank(Args&&... args) {
            if (allocations_since_collection_++ > 10000) {
                impl_.collect();
                allocations_since_collection_ = 0;
            }
            auto ptr = impl_.make_root<typename std::remove_const<typename T::value_type>::type>();
            return ptr;
        }

        template<typename T, typename... Args>
        auto make_const(Args&&... args) {
            using V = typename T::value_type;
            auto ptr = make_mutable<T>(std::forward<Args>(args)...);
            return graph_pool::const_pointer_cast<typename std::add_const<V>::type>(ptr);
        }

        template<typename V, typename U, typename... Args>
        auto make_graph_ptr(const graph_ptr<U>& u, Args&&... args) {
            auto root = make_mutable<V>(std::forward<Args>(args)...);
            using obj_type = typename decltype(root)::value_type;
            return graph_ptr<obj_type>(u, root);
        }

    };

};