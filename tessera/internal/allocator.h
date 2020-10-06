#pragma once

#include <vector>
#include <memory>
#include <tuple>
#include "expr_value.h"
#include "../include/tessera/tile.h"
#include "../include/tessera/tile_patch.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda.h"
#include "lambda_impl.h"
#include "cluster.h"
#include "tessera_impl.h"

namespace tess {

    class allocator {
        template<typename T>
        using impl_pool = std::vector<std::unique_ptr<typename std::remove_pointer<T>::type>>;
    private:
        impl_pool<tile_handle> tile_pool_;
        impl_pool<patch_handle> patch_pool_;
        impl_pool<edge_handle> edge_pool_;
        impl_pool<vertex_handle> vertex_pool_;
        impl_pool<lambda_handle> lambda_pool_;
        impl_pool<cluster_handle> cluster_pool_;

        template<typename T>
        impl_pool<T>& get_pool() {
            if constexpr (std::is_same<T, tile_handle>::value)
                return tile_pool_;
            else if constexpr (std::is_same<T, patch_handle>::value)
                return patch_pool_;
            else if constexpr (std::is_same<T, edge_handle>::value)
                return edge_pool_;
            else if constexpr (std::is_same<T, vertex_handle>::value)
                return vertex_pool_;
            else if constexpr (std::is_same<T, lambda_handle>::value)
                return lambda_pool_;
            else if constexpr (std::is_same<T, cluster_handle>::value)
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