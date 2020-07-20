#include "tile_impl.h"
#include "cluster.h"
#include "variant_util.h"
#include "parser/keywords.h"
#include "allocator.h"
#include <algorithm>

namespace {

	tess::cluster edges_as_cluster(tess::allocator& allocator,  const std::vector<tess::edge>& edges) {
		std::vector<tess::expr_value> cluster_contents(edges.size());
		std::transform(edges.begin(), edges.end(), cluster_contents.begin(),
			[](const tess::edge& e)->tess::expr_value { return { e }; }
		);
		return allocator.create<tess::cluster>( cluster_contents ); 
	}

	struct impl_cloner : public tess::tessera_impl
	{
		template<typename T>
		typename T::impl_type* clone(tess::allocator& allocator, std::unordered_map<tess::obj_id, void*>& orginal_to_clone, typename T::impl_type* impl)
		{
			tess::expr_value wrapper = { make_tess_obj<T>(impl) };
			auto wrapper_clone = std::get<T>(wrapper.clone(allocator, orginal_to_clone));
			return  get_impl(wrapper_clone);
		}
	};
}

std::vector<std::tuple<tess::number, tess::number>> get_regular_poly_vert_loc(int n) {
	std::vector<std::tuple<tess::number, tess::number>> points(n);
	return points;
}

tess::tile::impl_type::impl_type(obj_id id, tess::allocator* allocator, const std::vector<std::tuple<tess::number, tess::number>>& vertex_locations) :
	tessera_impl(id),
	parent_(nullptr)
{
	auto n = static_cast<int>(vertex_locations.size());
	vertices_.resize(n);
	edges_.resize(n);

	for (int i = 0; i < n; ++i) {
		vertices_[i] = allocator->create<tess::vertex>(this, i, vertex_locations[i]);
		edges_[i] = allocator->create<tess::edge>(this, i, i, (i + 1) % n);
	}
}

const std::vector<tess::vertex>& tess::tile::impl_type::vertices() const
{
    return vertices_;
}

std::vector<tess::vertex>& tess::tile::impl_type::vertices()
{
	const auto* ct = this;
	return const_cast<std::vector<tess::vertex>&>(ct->vertices());
}

const std::vector<tess::edge>& tess::tile::impl_type::edges() const
{
    return edges_;
}

std::vector<tess::edge>& tess::tile::impl_type::edges()
{
	const auto* ct = this;
	return const_cast<std::vector<tess::edge>&>(ct->edges());
}

void tess::tile::impl_type::set( std::vector<tess::vertex>&& vertices, std::vector<tess::edge>&& edges )
{
	vertices_ = std::move(vertices);
    edges_ = std::move(edges);
}

void tess::tile::impl_type::insert_field(const std::string& var, const expr_value& val)
{
	fields_[var] = val;
}

void tess::tile::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	for (const auto& edge : edges_) 
		 expr_value{edge}.get_all_referenced_allocations(alloc_set);
	for (const auto& vertex : vertices_) 
		expr_value{vertex}.get_all_referenced_allocations(alloc_set);
	for (const auto& [var, val] : fields_)
		val.get_all_referenced_allocations(alloc_set);
}

void tess::tile::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, tile::impl_type* clone) const
{
	for (const auto& v : vertices_) {
		clone->vertices_.push_back(std::get<vertex>(expr_value{ v }.clone(allocator, orginal_to_clone)));
	}
	for (const auto& e : edges_) {
		clone->edges_.push_back(std::get<edge>(expr_value{ e }.clone(allocator, orginal_to_clone)));
	}

	if (parent_ != nullptr) {
		impl_cloner cloner;
		clone->parent_ = cloner.clone<tile_patch>(allocator, orginal_to_clone, parent_);
	} else {
		clone->parent_ = nullptr;
	}

	for (const auto& [var, val] : fields_) {
		clone->fields_[var] = val.clone(allocator, orginal_to_clone);
	}
}

bool tess::tile::impl_type::is_detached() const
{
	return parent_ == nullptr;
}

tess::tile tess::tile::impl_type::clone_detached(tess::allocator& a) const
{
	auto this_tile = tess::make_tess_obj<tess::tile>(this);
	if (is_detached())
		return tess::clone(a, this_tile);
	
	// clone this tile such that its parent is only shallow copied.
	auto tile_value = expr_value{ this_tile };
	std::unordered_map<obj_id, void*> original_to_clone;
	auto this_patch_key =  parent_->get_id();
	original_to_clone[this_patch_key] = parent_;
	expr_value clone_expr_value = tile_value.clone(a, original_to_clone);

	//now return the clone with the parent detached.
	auto clone_tile = std::get<tess::tile>(clone_expr_value);
	get_impl(clone_tile)->set_parent(nullptr);
	return clone_tile;
}

std::string tess::tile::impl_type::debug() const
{
	std::stringstream ss;
	ss << "{ ";
	for (const auto& e : edges_) {
		ss << get_impl(e)->debug() << " ";
	}
	ss << "}";
	return ss.str();
}

tess::expr_value tess::tile::impl_type::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);
	return { nil_val() };
}

tess::expr_value tess::tile::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	if (field == parser::keyword(parser::kw::edge) || field == "edges") {
		return { edges_as_cluster(allocator, edges_) };
	}

	auto val = get_field(field);
	if (!std::holds_alternative< nil_val>(val))
		return val;
	else
		return { error(std::string("refrenced undefined tile edge, vertex, or field: ") + field ) };
}

void tess::tile::impl_type::apply(const matrix& mat)
{
	for (auto& vertex : vertices_) {
		get_impl(vertex)->apply(mat);
	}
}

tess::tile tess::tile::impl_type::flip(allocator& a) const
{
	tess::tile flippee;
	if (is_detached()) {
		auto this_tile = tess::make_tess_obj<tess::tile>(this);
		flippee = tess::clone(a, this_tile);
	} else {
		flippee = clone_detached(a);
	}
	get_impl(flippee)->flip();
	return flippee;
}

void tess::tile::impl_type::flip()
{
	apply(flip_matrix());
	for (auto& e : edges_) {
		get_impl(e)->flip();
	}
}

void tess::tile::impl_type::set_parent(tess::tile_patch::impl_type* parent) {
	if (parent != nullptr) {
		parent_ = parent;
	} else {
		for (auto& v : vertices_) 
			get_impl(v)->set_location(v.pos());
		parent_ = nullptr;
	}
}

bool tess::tile::impl_type::has_parent() const {
	return parent_ != nullptr;
}

tess::tile_patch::impl_type* tess::tile::impl_type::parent() const {
	return parent_;
}


/*--------------------------------------------------------------------------------*/

tess::edge::impl_type::impl_type( obj_id id, tile::impl_type* parent, int index, int u, int v) :
	tessera_impl(id),
	parent_(parent),
	index_(index),
	u_(u),
	v_(v)
{}

const tess::vertex& tess::edge::impl_type::u() const
{
    return parent_->vertices().at(u_);
}

const tess::vertex& tess::edge::impl_type::v() const
{
    return parent_->vertices().at(v_);
}

tess::expr_value tess::edge::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	return {};
}

tess::tile::impl_type* tess::edge::impl_type::parent() const
{
	return parent_;
}

void  tess::edge::impl_type::flip()
{
	std::swap(u_, v_);
}

tess::edge_indices tess::edge::impl_type::get_edge_location_indices() const {
	return edge_indices(
		get_impl(u())->location_index(),
		get_impl(v())->location_index()
	);
}


std::string tess::edge::impl_type::debug() const
{
	std::stringstream ss;
	ss << "[ " << u_ << ":" << get_impl(u())->debug()
		<< " -> " << v_ << ":" << get_impl(v())->debug() << " ]";
	return ss.str();
}

void tess::edge::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	parent_->get_all_referenced_allocations(alloc_set);
}

void  tess::edge::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, edge::impl_type* clone) const
{
	clone->index_ = index_;
	clone->u_ = u_;
	clone->v_ = v_;
	impl_cloner cloner;
	clone->parent_ = cloner.clone<tile>(allocator, orginal_to_clone, parent_);
}

/*--------------------------------------------------------------------------------*/

tess::vertex::impl_type::impl_type(obj_id id, tile::impl_type* parent, int n, std::tuple<number, number> loc) :
	tessera_impl(id),
	parent_(parent), index_(n), location_(loc)
{
}

std::tuple<double, double> tess::vertex::impl_type::to_floats() const
{
	auto [x, y] = pos();
    return {
        static_cast<double>(x),
		static_cast<double>(y),
    };
}

tess::point tess::vertex::impl_type::pos() const
{
	return std::visit(
		overloaded{
			[&](int vert_index)->point { 
				auto* patch = this->grandparent();
				if (patch == nullptr)
					throw error("corrupt tile patch");
				return patch->get_vertex_location(vert_index);
			},
			[&](point pt)->point {
				return pt;
			},
		},
		location_
	);
}

tess::expr_value tess::vertex::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	return {};
}

void tess::vertex::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	parent_->get_all_referenced_allocations(alloc_set);
}

void tess::vertex::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, vertex::impl_type* clone) const
{
	clone->index_ = index_;
	clone->location_ = location_;
	impl_cloner cloner;
	clone->parent_ = cloner.clone<tile>(allocator, orginal_to_clone, parent_);
};

tess::tile_patch::impl_type* tess::vertex::impl_type::grandparent() const
{
	if (!parent_)
		return nullptr;
	return parent_->parent();
}

void tess::vertex::impl_type::set_location(int n)
{
	location_ = n;
}

void tess::vertex::impl_type::set_location(point pt)
{
	location_ = pt;
}

int tess::vertex::impl_type::location_index() const
{
	return std::get<int>(location_);
}

std::string  tess::vertex::impl_type::debug() const
{
	std::stringstream ss;
	auto [x, y] = pos();
	ss << "{" << index_ << " (" << x << " , " << y << " )} ";
	return ss.str();
}



void tess::vertex::impl_type::apply(const tess::matrix& mat) {

	if (std::holds_alternative<point>(location_)) {
		location_ = apply_matrix(mat, std::get<point>(location_));
	} else {
		throw error("vertex::impl_type::apply called on patch vertex");
	}
}

tess::tile::impl_type* tess::vertex::impl_type::parent() const
{
	return parent_;
}


