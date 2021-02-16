#include "gc_heap.h"
#include "lambda_impl.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"

tess::gc_heap::gc_heap() : allocations_since_collection_(0)
{
    
}
