#include "allocator.h"
#include "value.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda_impl.h"
#include <iostream>

namespace {

    template<typename T>
    void erase_unused_allocations( std::vector<std::unique_ptr<T>>& pool, const std::unordered_set<tess::obj_id>& live_objects) {
        pool.erase(
            std::remove_if(
                pool.begin(),  pool.end(),
                [&live_objects](const std::unique_ptr<T>& allocation) {
                    auto id = allocation->get_id();
                    return live_objects.find(id) == live_objects.end();
                }
            ),
            pool.end()
        );
        pool.shrink_to_fit();
    }

    template<typename T>
    void generate_debug_output( const std::string& title, const std::vector<std::unique_ptr<T>>& pool) {
        std::cout << title << "\n";
        for (const auto& obj : pool) {
            std::cout << "    " << obj->get_id() << "\n";
        }
    }

}

tess::allocator::allocator( int sz) : allocations_since_collection_(0)
{
    tile_pool_.reserve(sz);
    patch_pool_.reserve(sz);
    edge_pool_.reserve(sz);
    vertex_pool_.reserve(sz);
    cluster_pool_.reserve(sz);
    lambda_pool_.reserve(sz);
}

void tess::allocator::collect(const std::unordered_set<tess::obj_id> &live_objects) {

    erase_unused_allocations( tile_pool_, live_objects);
    erase_unused_allocations( patch_pool_, live_objects);
    erase_unused_allocations( edge_pool_, live_objects);
    erase_unused_allocations( vertex_pool_, live_objects);
    erase_unused_allocations( cluster_pool_, live_objects);
    erase_unused_allocations( lambda_pool_, live_objects);

    allocations_since_collection_ = 0;
}

bool tess::allocator::should_collect() const {
    return allocations_since_collection_ > k_collection_freq_;
}

void tess::allocator::debug() const {
    std::cout << "------ allocation pools ---------\n";
    generate_debug_output( "tiles", tile_pool_);
    generate_debug_output( "patches", patch_pool_);
    generate_debug_output( "edges", edge_pool_);
    generate_debug_output( "vertices", vertex_pool_);
    generate_debug_output( "clusters", cluster_pool_);
    generate_debug_output( "lambdas", lambda_pool_);
    std::cout << "---------------------------------\n";
}

size_t tess::allocator::size() const {
    return tile_pool_.size() +
        patch_pool_.size() +
        edge_pool_.size() +
        vertex_pool_.size() +
        cluster_pool_.size() +
        lambda_pool_.size();
}



