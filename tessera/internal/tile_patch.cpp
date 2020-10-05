#include "..\include\tessera\tile_patch.h"
#include "tile_patch_impl.h"

std::vector<tess::tile> tess::tile_patch::tiles() const
{
	const auto& tiles = impl_->tiles();
	std::vector<tess::tile> wrapped_tiles(tiles.size());
	std::transform(tiles.begin(), tiles.end(), wrapped_tiles.begin(),
		[](auto v) -> tess::tile { return tess::make_tess_obj<tess::tile>(v); }
	);
	return wrapped_tiles;
}

int tess::tile_patch::count() const 
{
	return static_cast<int>( impl_->tiles().size() );
}

tess::property_value tess::tile_patch::get_property_variant(const std::string& prop) const
{
    return property_value();
}

bool tess::operator==(tile_patch l, tile_patch r)
{
	return get_impl(l) == get_impl(r);
}
