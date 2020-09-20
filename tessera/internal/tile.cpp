#include "..\include\tessera\tile.h"
#include "tile_impl.h"
#include "variant_util.h"

/*--------------------------------------------------------------------------------*/

const std::vector<tess::vertex>& tess::tile::vertices() const
{
	return impl_->vertices();
}

const std::vector<tess::edge>& tess::tile::edges() const
{
	return impl_->edges();
}

tess::property_value tess::tile::get_property_variant(const std::string& prop) const
{
	auto val = impl_->get_field(prop);
	if (std::holds_alternative<nil_val>(val)) {
		return nil_property();
	} else {
		return std::visit(
			overloaded{
				[](std::string str) { return property_value(str); },
				[](tess::number n) { return property_value(static_cast<double>(n)); },
				[](bool b) { return property_value(b); },
				[](auto) { return property_value(nil_property()); }
			},
			val
		);
	}
}

/*--------------------------------------------------------------------------------*/

std::tuple<double, double> tess::vertex::pos() const
{
	return impl_->to_floats();
}

tess::property_value tess::vertex::get_property_variant(const std::string& prop) const
{
    return property_value();
}

tess::edge tess::vertex::out_edge() const
{
	return tess::make_tess_obj<tess::edge>(impl_->out_edge());
}

tess::edge tess::vertex::in_edge() const
{
	return tess::make_tess_obj<tess::edge>(impl_->in_edge());
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

tess::property_value tess::edge::get_property_variant(const std::string& prop) const
{
    return property_value();
}
