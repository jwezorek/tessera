#include "cluster.h"
#include "tile_patch_impl.h"
#include "value.h"

const std::vector<tess::value_>& tess::cluster::items() const
{
    return impl_->values();
}

int tess::cluster::count() const
{
    return impl_->get_ary_count();
}

