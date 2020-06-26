#include "..\include\tessera\tile_patch.h"
#include "tile_patch_impl.h"

std::string tess::tile_patch::name() const
{
	return std::string();
}

const std::vector<tess::tile>& tess::tile_patch::tiles() const
{
	return impl_->tiles();
}

int tess::tile_patch::count() const 
{
	return static_cast<int>( impl_->tiles().size() );
}

tess::property_value tess::tile_patch::get_property_variant(const std::string& prop) const
{
    return property_value();
}
