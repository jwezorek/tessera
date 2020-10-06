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
#include "tessera_impl.h"

namespace tess {

    class allocator {
        template<typename T>
        using impl_pool = std::vector<std::unique_ptr<typename std::remove_pointer<T>::type>>;
    private:
        impl_pool<tile::impl_type*> tile_pool_;
        impl_pool<tile_patch::impl_type*> patch_pool_;
        impl_pool<edge::impl_type*> edge_pool_;
        impl_pool<vertex::impl_type*> vertex_pool_;
        impl_pool<lambda::impl_type*> lambda_pool_;
        impl_pool<cluster::impl_type*> cluster_pool_;

        template<typename T>
        impl_pool<T>& get_pool() {
            if constexpr (std::is_same<T, tile::impl_type*>::value)
                return tile_pool_;
            else if constexpr (std::is_same<T, tile_patch::impl_type*>::value)
                return patch_pool_;
            else if constexpr (std::is_same<T, edge::impl_type*>::value)
                return edge_pool_;
            else if constexpr (std::is_same<T, vertex::impl_type*>::value)
                return vertex_pool_;
            else if constexpr (std::is_same<T, lambda::impl_type*>::value)
                return lambda_pool_;
            else if constexpr (std::is_same<T, cluster::impl_type*>::value)
                return cluster_pool_;
        };

        obj_id id_counter_;

    public:
        allocator( int sz = 1024);

        /*
        template<typename T, typename... Args>
        typename T::impl_type* create_impl(Args&&... args) {
            auto& imp_pool = get_pool<T>();
            imp_pool.emplace_back(std::make_unique<typename T::impl_type>(id_counter_++, std::forward<Args>(args)...));
            return imp_pool.back().get();
        }
        */

        template<typename T, typename... Args>
        T create_implementation(Args&&... args) {
            auto& imp_pool = get_pool<T>();
            imp_pool.emplace_back(std::make_unique<std::remove_pointer<T>::type>(id_counter_++, std::forward<Args>(args)...));
            return imp_pool.back().get();
        }

        /*
        template<typename T, typename... Args>
        T create(Args&&... args) {
            return make_tess_obj<T>(
                create_impl<T>(std::forward<Args>(args)...)
            );
        }
        */

    };

};