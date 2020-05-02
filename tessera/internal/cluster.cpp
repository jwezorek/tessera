#include "cluster.h"
#include "tile_patch_impl.h"
#include "expr_value.h"

const std::vector<tess::expr_value>& tess::cluster::items() const
{
    return impl_->values();
}

int tess::cluster::count() const
{
    return impl_->get_ary_count();
}
