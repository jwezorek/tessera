#pragma once 

#include "tessera_impl.h"
#include "tessera/tile.h"
#include <vector>

namespace tess {

    class tile_patch_impl : public tessera_impl
    {
    private:
        std::vector<tess::tile> tiles_;
    public:
        tile_patch_impl(const std::vector<tess::tile>& tiles);
        const std::vector<tess::tile>& tiles() const;
    };
}