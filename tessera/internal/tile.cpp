#include "..\include\tessera\tile.h"
#include "tile_impl.h"

/*--------------------------------------------------------------------------------*/

const std::vector<tess::vertex>& tess::tile::vertices() const
{
	return impl_->vertices();
}

const std::vector<tess::edge>& tess::tile::edges() const
{
	return impl_->edges();
}

/*--------------------------------------------------------------------------------*/

std::tuple<double, double> tess::vertex::pos() const
{
	return impl_->to_floats();
}

/*--------------------------------------------------------------------------------*/


const tess::vertex& tess::edge::u() const
{
	return impl_->u();
}

const tess::vertex& tess::edge::v() const
{
	return impl_->v();
}
