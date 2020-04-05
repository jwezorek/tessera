#pragma once 

#include "tessera_impl.h"
#include "tessera/tile.h"
#include "tessera/tile_patch.h"
#include <vector>

namespace tess {

    class tile_patch::impl_type : public tessera_impl
    {
    private:
        std::vector<tess::tile> tiles_;
    public:
		impl_type(const std::vector<tess::tile>& tiles);
        const std::vector<tess::tile>& tiles() const;
    };
}