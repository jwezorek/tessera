#include "allocator.h"
#include "expr_value.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda_impl.h"

tess::allocator::allocator( int sz) : id_counter_(1)
{
    tile_pool_.reserve(sz);
    patch_pool_.reserve(sz);
    edge_pool_.reserve(sz);
    vertex_pool_.reserve(sz);
    cluster_pool_.reserve(sz);
    lambda_pool_.reserve(sz);
}



