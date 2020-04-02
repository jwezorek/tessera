#include "tile_patch_impl.h"

tess::tile_patch_impl::tile_patch_impl(const std::vector<tess::tile>& tiles) :
    tiles_(tiles)
{
}

const std::vector<tess::tile>& tess::tile_patch_impl::tiles() const
{
    return tiles_;
}
