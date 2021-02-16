#include "tessera/tile.h"
#include "tile_impl.h"
#include "variant_util.h"
#include "lambda_impl.h"
#include "tile_patch_impl.h"

/*--------------------------------------------------------------------------------*/

std::vector<tess::vertex> tess::tile::vertices() const
{
	auto sz = impl_->end_vertices() - impl_->begin_vertices();
	std::vector<tess::vertex> wrapped_vertices(sz);
	std::transform(impl_->begin_vertices(), impl_->end_vertices(), wrapped_vertices.begin(),
		[]( auto& v) -> tess::vertex { 
			auto ptr = to_root_ptr(v);
			return tess::make_tess_obj<tess::vertex>(ptr.get()); 
		}
	);
	return wrapped_vertices;
}

std::vector<tess::edge> tess::tile::edges() const
{
	auto sz = impl_->end_edges() - impl_->begin_edges();
	std::vector<tess::edge> wrapped_edges(sz);
	std::transform( impl_->begin_edges(), impl_->end_edges(), wrapped_edges.begin(),
		[]( auto& v) -> tess::edge { 
			auto ptr = to_root_ptr(v);
			return tess::make_tess_obj<tess::edge>(ptr.get()); 
		}
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
	return tess::make_tess_obj<tess::edge>(impl_->out_edge().get());
}

tess::edge tess::vertex::in_edge() const
{
	return tess::make_tess_obj<tess::edge>(impl_->in_edge().get());
}

/*--------------------------------------------------------------------------------*/

tess::vertex tess::edge::u() const
{
	return tess::make_tess_obj<vertex>(impl_->u().get());
}

tess::vertex tess::edge::v() const
{
	return tess::make_tess_obj<vertex>(impl_->v().get());
}

tess::property_value tess::edge::get_property_variant(const std::string& prop) const
{
    return property_value();
}

