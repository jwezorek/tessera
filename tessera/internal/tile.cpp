#include "..\include\tessera\tile.h"
#include "tile_impl.h"

/*--------------------------------------------------------------------------------*/
std::string tess::tile::name() const
{
	return impl_->name();
}

const std::vector<tess::vertex>& tess::tile::vertices() const
{
	return impl_->vertices();
}

const std::vector<tess::edge>& tess::tile::edges() const
{
	return impl_->edges();
}

/*--------------------------------------------------------------------------------*/

std::string tess::vertex::name() const
{
	return impl_->name();
}

std::string tess::vertex::vertex_class() const
{
	return impl_->vertex_class();
}

std::tuple<double, double> tess::vertex::pos() const
{
	return impl_->to_floats();
}

/*--------------------------------------------------------------------------------*/

std::string tess::edge::name() const
{
	return impl_->name();
}

std::string tess::edge::edge_class() const
{
	return impl_->edge_class();
}

const tess::vertex& tess::edge::u() const
{
	return impl_->u();
}

const tess::vertex& tess::edge::v() const
{
	return impl_->v();
}
