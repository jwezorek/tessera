#include "allocator.h"
#include "value.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda_impl.h"

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
    }

}

tess::allocator::allocator( int sz) : id_counter_(1), allocations_since_collection_(0)
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



