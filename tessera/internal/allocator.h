#pragma once

#include <vector>
#include <memory>
#include <tuple>
#include "value.h"

namespace tess {

    class allocator {
        template<typename T>
        using impl_pool = std::vector<std::unique_ptr<typename std::remove_pointer<T>::type>>;
    private:
        impl_pool<tile_ptr> tile_pool_;
        impl_pool<patch_ptr> patch_pool_;
        impl_pool<edge_ptr> edge_pool_;
        impl_pool<vertex_ptr> vertex_pool_;
        impl_pool<lambda_ptr> lambda_pool_;
        impl_pool<cluster_ptr> cluster_pool_;

        template<typename T>
        impl_pool<T>& get_pool() {
            if constexpr (std::is_same<T, tile_ptr>::value)
                return tile_pool_;
            else if constexpr (std::is_same<T, patch_ptr>::value)
                return patch_pool_;
            else if constexpr (std::is_same<T, edge_ptr>::value)
                return edge_pool_;
            else if constexpr (std::is_same<T, vertex_ptr>::value)
                return vertex_pool_;
            else if constexpr (std::is_same<T, lambda_ptr>::value)
                return lambda_pool_;
            else if constexpr (std::is_same<T, cluster_ptr>::value)
                return cluster_pool_;
        };

        obj_id id_counter_;

    public:
        allocator( int sz = 1024);

        template<typename T, typename... Args>
        T create(Args&&... args) {
            auto& imp_pool = get_pool<T>();
            imp_pool.emplace_back(std::make_unique<std::remove_pointer<T>::type>(id_counter_++, std::forward<Args>(args)...));
            return imp_pool.back().get();
        }

    };

};