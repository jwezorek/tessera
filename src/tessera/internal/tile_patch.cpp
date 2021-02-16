#include "tessera/tile_patch.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda_impl.h"

std::vector<tess::tile> tess::tile_patch::tiles() const
{
	auto sz = count();
	std::vector<tess::tile> wrapped_tiles( sz );
	std::transform(impl_->begin_tiles(), impl_->end_tiles(), wrapped_tiles.begin(),
		[](auto& v) -> tess::tile { 
			return tess::make_tess_obj<tess::tile>(to_root_ptr(v).get()); 
		}
	);
	return wrapped_tiles;
}

int tess::tile_patch::count() const 
{
	return static_cast<int>(impl_->end_tiles() - impl_->begin_tiles());
}

tess::property_value tess::tile_patch::get_property_variant(const std::string& prop) const
{
    return property_value();
}

