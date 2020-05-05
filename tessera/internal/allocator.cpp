#include "allocator.h"
#include "expr_value.h"
#include "execution_state.h"

constexpr int k_gc_freq = 10000;

void tess::allocator::collect_garbage()
{
    if (count++ > k_gc_freq) {
        count = 0;
        state_.collect_garbage();
    }
}

tess::allocator::allocator(execution_state& state, int sz) :
    state_(state), count(0)
{
    tile_pool_.reserve(sz);
    patch_pool_.reserve(sz);
    edge_pool_.reserve(sz);
    vertex_pool_.reserve(sz);
    cluster_pool_.reserve(sz);
    lambda_pool_.reserve(sz);
}


