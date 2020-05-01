#pragma once

#include <vector>
#include <memory>
#include <tuple>
#include "../include/tessera/tile.h"
#include "../include/tessera/tile_patch.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda.h"
#include "lambda_impl.h"
#include "cluster.h"

namespace tess {

    class impl_pool {
        template<typename T>
        using pool_t = std::vector<std::unique_ptr<typename T::impl_type>>;
    private:
        pool_t<tile> tile_pool_;
        pool_t<tile_patch> patch_pool_;
        pool_t<edge> edge_pool_;
        pool_t<vertex> vertex_pool_;
        pool_t<lambda> lambda_pool_;
        pool_t<cluster> cluster_pool_;

        template<typename T>
        pool_t<T>& get_pool() {
            if constexpr (std::is_same<T, tile>::value)
                return tile_pool_;
            else if constexpr (std::is_same<T, tile_patch>::value)
                return patch_pool_;
            else if constexpr (std::is_same<T, edge>::value)
                return edge_pool_;
            else if constexpr (std::is_same<T, vertex>::value)
                return vertex_pool_;
            else if constexpr (std::is_same<T, lambda>::value)
                return lambda_pool_;
            else if constexpr (std::is_same<T, cluster>::value)
                return cluster_pool_;
        }

    public:
        impl_pool(int sz = 1024);

        template<typename T, typename... Args>
        typename T::impl_type* create(Args&&... args) {
            auto& imp_pool = get_pool<T>();
            imp_pool.emplace_back(std::make_unique<typename T::impl_type>(std::forward<Args>(args)...));
            return imp_pool.back().get();
        }

        //void test();
    };

};