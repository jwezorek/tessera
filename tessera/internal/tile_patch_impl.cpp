#include "tile_impl.h"
#include "tile_patch_impl.h"

tess::tile_patch::impl_type::impl_type(const std::vector<tess::tile>& tiles) :
    tiles_(tiles)
{
	for (auto& tile : tiles) {
		get_impl(tile)->set_parent(this);
	}
}

const std::vector<tess::tile>& tess::tile_patch::impl_type::tiles() const
{
    return tiles_;
}

tess::expr_value tess::tile_patch::impl_type::get_field(const std::string& field) const
{
	return expr_value();
}

tess::expr_value tess::tile_patch::impl_type::get_ary_item(int i) const
{
	return { tiles_.at(i) };
}

void tess::tile_patch::impl_type::apply(const matrix& mat)
{
	for (auto& tile : tiles_)
		get_impl(tile)->apply(mat);
}

bool tess::tile_patch::impl_type::is_untouched() const 
{
	return get_impl(tiles_.front())->is_untouched();
}

/*---------------------------------------------------------------------------------------------*/

tess::cluster::impl_type::impl_type(const std::vector<expr_value>& values) :
	values_(values)
{}


tess::expr_value tess::cluster::impl_type::get_field(const std::string& field) const
{
	return expr_value();
}

tess::expr_value tess::cluster::impl_type::get_ary_item(int i) const
{
	return values_.at(i);
}