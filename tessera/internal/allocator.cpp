#include "allocator.h"
#include "expr_value.h"

tess::allocator::allocator( int sz) 
{
    tile_pool_.reserve(sz);
    patch_pool_.reserve(sz);
    edge_pool_.reserve(sz);
    vertex_pool_.reserve(sz);
    cluster_pool_.reserve(sz);
    lambda_pool_.reserve(sz);
}



