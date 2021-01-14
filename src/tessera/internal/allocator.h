#pragma once

#include <vector>
#include <memory>
#include <tuple>
#include "value.h"
#include "tessera_impl.h"

namespace tess {

    class allocator {

        template <typename T>
        using base_type = typename std::remove_const<typename std::remove_pointer<T>::type>::type;

        template<typename T>
        using impl_pool = std::vector<std::unique_ptr<base_type<T>>>;

    private:
        impl_pool<const_tile_ptr> tile_pool_;
        impl_pool<const_patch_ptr> patch_pool_;
        impl_pool<const_edge_ptr> edge_pool_;
        impl_pool<const_vertex_ptr> vertex_pool_;
        impl_pool<const_lambda_ptr> lambda_pool_;
        impl_pool<const_cluster_ptr> cluster_pool_;
        int allocations_since_collection_;
        const int k_collection_freq_ = 1024;

        template<typename T>
        impl_pool<T>& get_pool() {
            if constexpr (std::is_same<T, const_tile_ptr>::value)
                return tile_pool_;
            else if constexpr (std::is_same<T, const_patch_ptr>::value)
                return patch_pool_;
            else if constexpr (std::is_same<T, const_edge_ptr>::value)
                return edge_pool_;
            else if constexpr (std::is_same<T, const_vertex_ptr>::value)
                return vertex_pool_;
            else if constexpr (std::is_same<T, const_lambda_ptr>::value)
                return lambda_pool_;
            else if constexpr (std::is_same<T, const_cluster_ptr>::value)
                return cluster_pool_;
        };

        obj_id id_counter_;

    public:
        allocator( int sz = 1024);

        template<typename T, typename... Args>
        auto create(Args&&... args) {
            auto& imp_pool = get_pool<T>();
            imp_pool.emplace_back(std::make_unique<base_type<T>>(id_counter_++, std::forward<Args>(args)...));
            ++allocations_since_collection_;
            return imp_pool.back().get();
        }

        void debug() const;
        size_t size() const;
        bool should_collect() const;
        void collect(const std::unordered_set<tess::obj_id>& live_objects);
    };

};