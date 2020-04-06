#include "tile_impl.h"
#include "tile_patch_impl.h"

tess::tile_patch::impl_type::impl_type(const std::vector<tess::tile>& tiles) :
    tiles_(tiles)
{
}

const std::vector<tess::tile>& tess::tile_patch::impl_type::tiles() const
{
    return tiles_;
}

tess::expr_value tess::tile_patch::impl_type::get_field(const std::string& field) const
{
	return expr_value();
}

void tess::tile_patch::impl_type::apply(const matrix& mat)
{
	for (auto& tile : tiles_)
		get_impl(tile)->apply(mat);
}
