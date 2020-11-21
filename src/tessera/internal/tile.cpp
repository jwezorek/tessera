#include "..\include\tessera\tile.h"
#include "tile_impl.h"
#include "variant_util.h"

/*--------------------------------------------------------------------------------*/

std::vector<tess::vertex> tess::tile::vertices() const
{
	const auto& vertices = impl_->vertices();
	std::vector<tess::vertex> wrapped_vertices(vertices.size());
	std::transform(vertices.begin(), vertices.end(), wrapped_vertices.begin(),
		[](auto v) -> tess::vertex { return tess::make_tess_obj<tess::vertex>(v); }
	);
	return wrapped_vertices;
}

std::vector<tess::edge> tess::tile::edges() const
{
	const auto& edges = impl_->edges();
	std::vector<tess::edge> wrapped_edges(edges.size());
	std::transform(edges.begin(), edges.end(), wrapped_edges.begin(),
		[](auto v) -> tess::edge { return tess::make_tess_obj<tess::edge>(v); }
	);
	return wrapped_edges;
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

tess::vertex tess::edge::u() const
{
	return tess::make_tess_obj<vertex>(impl_->u());
}

tess::vertex tess::edge::v() const
{
	return tess::make_tess_obj<vertex>(impl_->v());
}

tess::property_value tess::edge::get_property_variant(const std::string& prop) const
{
    return property_value();
}

